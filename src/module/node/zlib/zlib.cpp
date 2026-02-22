#include "zlib.h"
#include "../buffer/buffer.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <string>
#include "../../../../deps/zlib/zlib.h"
#include "../../../../deps/brotli/c/include/brotli/encode.h"
#include "../../../../deps/brotli/c/include/brotli/decode.h"
#include "../../../../deps/zstd/lib/zstd.h"
#include "task_queue.h"
#include "thread_pool.h"

namespace z8 {
namespace module {

struct ZlibAsyncCtx {
    std::vector<uint8_t> m_input;
    std::vector<uint8_t> m_output;
    int32_t m_level = Z_DEFAULT_COMPRESSION;
    int32_t m_window_bits = 15;
    int32_t m_mem_level = 8;
    int32_t m_strategy = Z_DEFAULT_STRATEGY;
    int32_t m_chunk_size = 16384;
    std::vector<uint8_t> m_dictionary;
    bool m_is_deflate;
    bool m_is_brotli = false;
    bool m_is_error = false;
    std::string m_error_msg;

    // Brotli specific
    int32_t m_brotli_quality = BROTLI_DEFAULT_QUALITY;
    int32_t m_brotli_window = BROTLI_DEFAULT_WINDOW;
    int32_t m_brotli_mode = BROTLI_DEFAULT_MODE;

    // Zstd specific
    bool m_is_zstd = false;
    int32_t m_zstd_level = 3; // ZSTD_CLEVEL_DEFAULT
};

static void parseZlibOptions(v8::Isolate* p_isolate, v8::Local<v8::Context> context, v8::Local<v8::Value> options_val,
                             int32_t& level, int32_t& window_bits, int32_t& mem_level, int32_t& strategy,
                             int32_t& chunk_size, std::vector<uint8_t>& dictionary) {
    if (options_val.IsEmpty() || !options_val->IsObject()) {
        return;
    }
    v8::Local<v8::Object> options = options_val.As<v8::Object>();
    v8::Local<v8::Value> val;
    if (options->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "level")).ToLocal(&val) && val->IsNumber()) {
        level = val->Int32Value(context).FromMaybe(level);
    }
    if (options->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "windowBits")).ToLocal(&val) && val->IsNumber()) {
        window_bits = val->Int32Value(context).FromMaybe(window_bits);
    }
    if (options->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "memLevel")).ToLocal(&val) && val->IsNumber()) {
        mem_level = val->Int32Value(context).FromMaybe(mem_level);
    }
    if (options->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "strategy")).ToLocal(&val) && val->IsNumber()) {
        strategy = val->Int32Value(context).FromMaybe(strategy);
    }
    if (options->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "chunkSize")).ToLocal(&val) && val->IsNumber()) {
        chunk_size = val->Int32Value(context).FromMaybe(chunk_size);
    }
    if (options->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "dictionary")).ToLocal(&val)) {
        if (val->IsUint8Array()) {
            v8::Local<v8::Uint8Array> dict = val.As<v8::Uint8Array>();
            dictionary.assign(static_cast<uint8_t*>(dict->Buffer()->GetBackingStore()->Data()) + dict->ByteOffset(),
                              static_cast<uint8_t*>(dict->Buffer()->GetBackingStore()->Data()) + dict->ByteOffset() + dict->ByteLength());
        }
    }
}

static void parseBrotliOptions(v8::Isolate* p_isolate, v8::Local<v8::Context> context, v8::Local<v8::Value> options_val,
                               int32_t& quality, int32_t& window, int32_t& mode) {
    if (options_val.IsEmpty() || !options_val->IsObject()) {
        return;
    }
    v8::Local<v8::Object> options = options_val.As<v8::Object>();
    v8::Local<v8::Value> val;
    if (options->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "quality")).ToLocal(&val) && val->IsNumber()) {
        quality = val->Int32Value(context).FromMaybe(quality);
    }
    if (options->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "window")).ToLocal(&val) && val->IsNumber()) {
        window = val->Int32Value(context).FromMaybe(window);
    }
    if (options->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "mode")).ToLocal(&val) && val->IsNumber()) {
        mode = val->Int32Value(context).FromMaybe(mode);
    }
}

static void parseZstdOptions(v8::Isolate* p_isolate, v8::Local<v8::Context> context, v8::Local<v8::Value> options_val,
                             int32_t& level) {
    if (options_val.IsEmpty() || !options_val->IsObject()) {
        return;
    }
    v8::Local<v8::Object> options = options_val.As<v8::Object>();
    v8::Local<v8::Value> val;
    if (options->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "level")).ToLocal(&val) && val->IsNumber()) {
        level = val->Int32Value(context).FromMaybe(level);
    }
}

static bool getInput(const v8::FunctionCallbackInfo<v8::Value>& args, uint8_t** p_data, size_t* length, std::vector<uint8_t>& storage) {
    v8::Isolate* p_isolate = args.GetIsolate();
    if (args.Length() < 1) {
        p_isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(p_isolate, "Argument required")));
        return false;
    }

    if (args[0]->IsString()) {
        v8::String::Utf8Value str(p_isolate, args[0]);
        storage.assign(reinterpret_cast<uint8_t*>(*str), reinterpret_cast<uint8_t*>(*str) + str.length());
        *p_data = storage.data();
        *length = storage.size();
        return true;
    }

    if (!args[0]->IsUint8Array()) {
        p_isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(p_isolate, "Argument must be a Uint8Array or string")));
        return false;
    }

    v8::Local<v8::Uint8Array> input = args[0].As<v8::Uint8Array>();
    v8::Local<v8::ArrayBuffer> ab = input->Buffer();
    *p_data = static_cast<uint8_t*>(ab->GetBackingStore()->Data()) + input->ByteOffset();
    *length = input->ByteLength();
    return true;
}

static void returnBuffer(const v8::FunctionCallbackInfo<v8::Value>& args, const std::vector<uint8_t>& out_buffer) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Uint8Array> ui = z8::module::Buffer::createBuffer(p_isolate, out_buffer.size());
    memcpy(ui->Buffer()->GetBackingStore()->Data(), out_buffer.data(), out_buffer.size());
    args.GetReturnValue().Set(ui);
}

v8::Local<v8::ObjectTemplate> Zlib::createTemplate(v8::Isolate* p_isolate) {
    v8::Local<v8::ObjectTemplate> tmpl = v8::ObjectTemplate::New(p_isolate);

    v8::Local<v8::ObjectTemplate> constants = v8::ObjectTemplate::New(p_isolate);
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_NO_COMPRESSION"), v8::Number::New(p_isolate, (double)Z_NO_COMPRESSION));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_BEST_SPEED"), v8::Number::New(p_isolate, (double)Z_BEST_SPEED));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_BEST_COMPRESSION"), v8::Number::New(p_isolate, (double)Z_BEST_COMPRESSION));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_DEFAULT_COMPRESSION"), v8::Number::New(p_isolate, (double)Z_DEFAULT_COMPRESSION));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_FILTERED"), v8::Number::New(p_isolate, (double)Z_FILTERED));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_HUFFMAN_ONLY"), v8::Number::New(p_isolate, (double)Z_HUFFMAN_ONLY));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_RLE"), v8::Number::New(p_isolate, (double)Z_RLE));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_FIXED"), v8::Number::New(p_isolate, (double)Z_FIXED));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_DEFAULT_STRATEGY"), v8::Number::New(p_isolate, (double)Z_DEFAULT_STRATEGY));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_NO_FLUSH"), v8::Number::New(p_isolate, (double)Z_NO_FLUSH));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_PARTIAL_FLUSH"), v8::Number::New(p_isolate, (double)Z_PARTIAL_FLUSH));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_SYNC_FLUSH"), v8::Number::New(p_isolate, (double)Z_SYNC_FLUSH));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_FULL_FLUSH"), v8::Number::New(p_isolate, (double)Z_FULL_FLUSH));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_FINISH"), v8::Number::New(p_isolate, (double)Z_FINISH));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_BLOCK"), v8::Number::New(p_isolate, (double)Z_BLOCK));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_TREES"), v8::Number::New(p_isolate, (double)Z_TREES));

    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_MIN_LEVEL"), v8::Number::New(p_isolate, (double)-1));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_MAX_LEVEL"), v8::Number::New(p_isolate, (double)9));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_MIN_MEMLEVEL"), v8::Number::New(p_isolate, (double)1));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_MAX_MEMLEVEL"), v8::Number::New(p_isolate, (double)9));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_MIN_WINDOWBITS"), v8::Number::New(p_isolate, (double)8));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_MAX_WINDOWBITS"), v8::Number::New(p_isolate, (double)15));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_MIN_CHUNK"), v8::Number::New(p_isolate, (double)64));

    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_OK"), v8::Number::New(p_isolate, (double)Z_OK));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_STREAM_END"), v8::Number::New(p_isolate, (double)Z_STREAM_END));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_NEED_DICT"), v8::Number::New(p_isolate, (double)Z_NEED_DICT));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_ERRNO"), v8::Number::New(p_isolate, (double)Z_ERRNO));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_STREAM_ERROR"), v8::Number::New(p_isolate, (double)Z_STREAM_ERROR));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_DATA_ERROR"), v8::Number::New(p_isolate, (double)Z_DATA_ERROR));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_MEM_ERROR"), v8::Number::New(p_isolate, (double)Z_MEM_ERROR));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_BUF_ERROR"), v8::Number::New(p_isolate, (double)Z_BUF_ERROR));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_VERSION_ERROR"), v8::Number::New(p_isolate, (double)Z_VERSION_ERROR));

    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "BROTLI_PARAM_MODE"), v8::Number::New(p_isolate, (double)BROTLI_PARAM_MODE));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "BROTLI_PARAM_QUALITY"), v8::Number::New(p_isolate, (double)BROTLI_PARAM_QUALITY));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "BROTLI_PARAM_LGWIN"), v8::Number::New(p_isolate, (double)BROTLI_PARAM_LGWIN));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "BROTLI_PARAM_LGBLOCK"), v8::Number::New(p_isolate, (double)BROTLI_PARAM_LGBLOCK));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "BROTLI_PARAM_DISABLE_LITERAL_CONTEXT_MODELING"), v8::Number::New(p_isolate, (double)BROTLI_PARAM_DISABLE_LITERAL_CONTEXT_MODELING));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "BROTLI_PARAM_SIZE_HINT"), v8::Number::New(p_isolate, (double)BROTLI_PARAM_SIZE_HINT));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "BROTLI_PARAM_LARGE_WINDOW"), v8::Number::New(p_isolate, (double)BROTLI_PARAM_LARGE_WINDOW));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "BROTLI_PARAM_NPOSTFIX"), v8::Number::New(p_isolate, (double)BROTLI_PARAM_NPOSTFIX));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "BROTLI_PARAM_NDIRECT"), v8::Number::New(p_isolate, (double)BROTLI_PARAM_NDIRECT));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "BROTLI_PARAM_STREAM_OFFSET"), v8::Number::New(p_isolate, (double)BROTLI_PARAM_STREAM_OFFSET));

    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "BROTLI_MODE_GENERIC"), v8::Number::New(p_isolate, (double)BROTLI_MODE_GENERIC));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "BROTLI_MODE_TEXT"), v8::Number::New(p_isolate, (double)BROTLI_MODE_TEXT));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "BROTLI_MODE_FONT"), v8::Number::New(p_isolate, (double)BROTLI_MODE_FONT));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "BROTLI_DEFAULT_QUALITY"), v8::Number::New(p_isolate, (double)BROTLI_DEFAULT_QUALITY));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "BROTLI_DEFAULT_WINDOW"), v8::Number::New(p_isolate, (double)BROTLI_DEFAULT_WINDOW));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "BROTLI_MIN_QUALITY"), v8::Number::New(p_isolate, (double)BROTLI_MIN_QUALITY));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "BROTLI_MAX_QUALITY"), v8::Number::New(p_isolate, (double)BROTLI_MAX_QUALITY));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "BROTLI_MIN_WINDOW_BITS"), v8::Number::New(p_isolate, (double)BROTLI_MIN_WINDOW_BITS));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "BROTLI_MAX_WINDOW_BITS"), v8::Number::New(p_isolate, (double)BROTLI_MAX_WINDOW_BITS));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "BROTLI_LARGE_MAX_WINDOW_BITS"), v8::Number::New(p_isolate, (double)BROTLI_LARGE_MAX_WINDOW_BITS));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "BROTLI_MIN_INPUT_BLOCK_BITS"), v8::Number::New(p_isolate, (double)BROTLI_MIN_INPUT_BLOCK_BITS));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "BROTLI_MAX_INPUT_BLOCK_BITS"), v8::Number::New(p_isolate, (double)BROTLI_MAX_INPUT_BLOCK_BITS));

    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "BROTLI_DECODER_PARAM_DISABLE_RING_BUFFER_REALLOCATION"), v8::Number::New(p_isolate, (double)BROTLI_DECODER_PARAM_DISABLE_RING_BUFFER_REALLOCATION));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "BROTLI_DECODER_PARAM_LARGE_WINDOW"), v8::Number::New(p_isolate, (double)BROTLI_DECODER_PARAM_LARGE_WINDOW));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "BROTLI_DECODER_RESULT_ERROR"), v8::Number::New(p_isolate, (double)BROTLI_DECODER_RESULT_ERROR));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "BROTLI_DECODER_RESULT_SUCCESS"), v8::Number::New(p_isolate, (double)BROTLI_DECODER_RESULT_SUCCESS));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT"), v8::Number::New(p_isolate, (double)BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT"), v8::Number::New(p_isolate, (double)BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT));

    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "ZSTD_CLEVEL_DEFAULT"), v8::Number::New(p_isolate, (double)ZSTD_CLEVEL_DEFAULT));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "ZSTD_CLEVEL_MIN"), v8::Number::New(p_isolate, (double)-22));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "ZSTD_CLEVEL_MAX"), v8::Number::New(p_isolate, (double)22));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "ZSTD_e_continue"), v8::Number::New(p_isolate, (double)ZSTD_e_continue));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "ZSTD_e_flush"), v8::Number::New(p_isolate, (double)ZSTD_e_flush));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "ZSTD_e_end"), v8::Number::New(p_isolate, (double)ZSTD_e_end));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "ZSTD_c_compressionLevel"), v8::Number::New(p_isolate, (double)ZSTD_c_compressionLevel));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "ZSTD_c_checksumFlag"), v8::Number::New(p_isolate, (double)ZSTD_c_checksumFlag));
    constants->Set(v8::String::NewFromUtf8Literal(p_isolate, "ZSTD_c_dictIDFlag"), v8::Number::New(p_isolate, (double)ZSTD_c_dictIDFlag));

    v8::Local<v8::ObjectTemplate> codes = v8::ObjectTemplate::New(p_isolate);
    codes->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_OK"), v8::Number::New(p_isolate, (double)Z_OK));
    codes->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_STREAM_END"), v8::Number::New(p_isolate, (double)Z_STREAM_END));
    codes->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_NEED_DICT"), v8::Number::New(p_isolate, (double)Z_NEED_DICT));
    codes->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_ERRNO"), v8::Number::New(p_isolate, (double)Z_ERRNO));
    codes->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_STREAM_ERROR"), v8::Number::New(p_isolate, (double)Z_STREAM_ERROR));
    codes->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_DATA_ERROR"), v8::Number::New(p_isolate, (double)Z_DATA_ERROR));
    codes->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_MEM_ERROR"), v8::Number::New(p_isolate, (double)Z_MEM_ERROR));
    codes->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_BUF_ERROR"), v8::Number::New(p_isolate, (double)Z_BUF_ERROR));
    codes->Set(v8::String::NewFromUtf8Literal(p_isolate, "Z_VERSION_ERROR"), v8::Number::New(p_isolate, (double)Z_VERSION_ERROR));

    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "codes"), codes);
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "constants"), constants);

    // Node.js also exposes constants on the root object
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    v8::Local<v8::Object> const_obj = constants->NewInstance(context).ToLocalChecked();
    v8::Local<v8::Array> const_names = const_obj->GetPropertyNames(context).ToLocalChecked();
    for (uint32_t i = 0; i < const_names->Length(); i++) {
        v8::Local<v8::Value> key = const_names->Get(context, i).ToLocalChecked();
        v8::Local<v8::Value> val = const_obj->Get(context, key).ToLocalChecked();
        tmpl->Set(key.As<v8::Name>(), val);
    }

    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "crc32"), v8::FunctionTemplate::New(p_isolate, crc32));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "adler32"), v8::FunctionTemplate::New(p_isolate, adler32));

    v8::Local<v8::FunctionTemplate> ft_zlib_base = v8::FunctionTemplate::New(p_isolate);
    ft_zlib_base->SetClassName(v8::String::NewFromUtf8Literal(p_isolate, "Zlib"));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "Zlib"), ft_zlib_base);

    // Register classes as constructors (aliases to create functions for now)
    v8::Local<v8::FunctionTemplate> ft_gzip = v8::FunctionTemplate::New(p_isolate, createGzip);
    ft_gzip->Inherit(ft_zlib_base);
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "Gzip"), ft_gzip);
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "createGzip"), ft_gzip);

    v8::Local<v8::FunctionTemplate> ft_gunzip = v8::FunctionTemplate::New(p_isolate, createGunzip);
    ft_gunzip->Inherit(ft_zlib_base);
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "Gunzip"), ft_gunzip);
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "createGunzip"), ft_gunzip);

    v8::Local<v8::FunctionTemplate> ft_deflate = v8::FunctionTemplate::New(p_isolate, createDeflate);
    ft_deflate->Inherit(ft_zlib_base);
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "Deflate"), ft_deflate);
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "createDeflate"), ft_deflate);

    v8::Local<v8::FunctionTemplate> ft_inflate = v8::FunctionTemplate::New(p_isolate, createInflate);
    ft_inflate->Inherit(ft_zlib_base);
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "Inflate"), ft_inflate);
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "createInflate"), ft_inflate);

    v8::Local<v8::FunctionTemplate> ft_deflate_raw = v8::FunctionTemplate::New(p_isolate, createDeflateRaw);
    ft_deflate_raw->Inherit(ft_zlib_base);
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "DeflateRaw"), ft_deflate_raw);
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "createDeflateRaw"), ft_deflate_raw);

    v8::Local<v8::FunctionTemplate> ft_inflate_raw = v8::FunctionTemplate::New(p_isolate, createInflateRaw);
    ft_inflate_raw->Inherit(ft_zlib_base);
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "InflateRaw"), ft_inflate_raw);
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "createInflateRaw"), ft_inflate_raw);

    v8::Local<v8::FunctionTemplate> ft_unzip = v8::FunctionTemplate::New(p_isolate, createUnzip);
    ft_unzip->Inherit(ft_zlib_base);
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "Unzip"), ft_unzip);
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "createUnzip"), ft_unzip);

    v8::Local<v8::FunctionTemplate> ft_br_comp = v8::FunctionTemplate::New(p_isolate, createBrotliCompress);
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "BrotliCompress"), ft_br_comp);
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "createBrotliCompress"), ft_br_comp);

    v8::Local<v8::FunctionTemplate> ft_br_decomp = v8::FunctionTemplate::New(p_isolate, createBrotliDecompress);
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "BrotliDecompress"), ft_br_decomp);
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "createBrotliDecompress"), ft_br_decomp);

    v8::Local<v8::FunctionTemplate> ft_zstd_comp = v8::FunctionTemplate::New(p_isolate, createZstdCompress);
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "ZstdCompress"), ft_zstd_comp);
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "createZstdCompress"), ft_zstd_comp);

    v8::Local<v8::FunctionTemplate> ft_zstd_decomp = v8::FunctionTemplate::New(p_isolate, createZstdDecompress);
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "ZstdDecompress"), ft_zstd_decomp);
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "createZstdDecompress"), ft_zstd_decomp);

    // Sync methods
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "deflateSync"), v8::FunctionTemplate::New(p_isolate, deflateSync));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "inflateSync"), v8::FunctionTemplate::New(p_isolate, inflateSync));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "deflateRawSync"), v8::FunctionTemplate::New(p_isolate, deflateRawSync));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "inflateRawSync"), v8::FunctionTemplate::New(p_isolate, inflateRawSync));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "gzipSync"), v8::FunctionTemplate::New(p_isolate, gzipSync));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "gunzipSync"), v8::FunctionTemplate::New(p_isolate, gunzipSync));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "unzipSync"), v8::FunctionTemplate::New(p_isolate, unzipSync));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "brotliCompressSync"), v8::FunctionTemplate::New(p_isolate, brotliCompressSync));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "brotliDecompressSync"), v8::FunctionTemplate::New(p_isolate, brotliDecompressSync));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "zstdCompressSync"), v8::FunctionTemplate::New(p_isolate, zstdCompressSync));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "zstdDecompressSync"), v8::FunctionTemplate::New(p_isolate, zstdDecompressSync));

    // Async methods
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "deflate"), v8::FunctionTemplate::New(p_isolate, deflate));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "inflate"), v8::FunctionTemplate::New(p_isolate, inflate));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "deflateRaw"), v8::FunctionTemplate::New(p_isolate, deflateRaw));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "inflateRaw"), v8::FunctionTemplate::New(p_isolate, inflateRaw));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "gzip"), v8::FunctionTemplate::New(p_isolate, gzip));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "gunzip"), v8::FunctionTemplate::New(p_isolate, gunzip));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "unzip"), v8::FunctionTemplate::New(p_isolate, unzip));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "brotliCompress"), v8::FunctionTemplate::New(p_isolate, brotliCompress));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "brotliDecompress"), v8::FunctionTemplate::New(p_isolate, brotliDecompress));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "zstdCompress"), v8::FunctionTemplate::New(p_isolate, zstdCompress));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "zstdDecompress"), v8::FunctionTemplate::New(p_isolate, zstdDecompress));

    // Promises
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "promises"), createPromisesTemplate(p_isolate));

    return tmpl;
}

static void doDeflate(const v8::FunctionCallbackInfo<v8::Value>& args, int32_t default_window_bits) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    uint8_t* p_data;
    size_t length;
    std::vector<uint8_t> storage;
    if (!getInput(args, &p_data, &length, storage)) return;

    int32_t level = Z_DEFAULT_COMPRESSION;
    int32_t window_bits = default_window_bits;
    int32_t mem_level = 8;
    int32_t strategy = Z_DEFAULT_STRATEGY;
    int32_t chunk_size = 16384;
    std::vector<uint8_t> dictionary;

    if (args.Length() >= 2) {
        parseZlibOptions(p_isolate, context, args[1], level, window_bits, mem_level, strategy, chunk_size, dictionary);
    }

    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    if (deflateInit2(&strm, level, Z_DEFLATED, window_bits, mem_level, strategy) != Z_OK) {
        p_isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8Literal(p_isolate, "deflateInit failed")));
        return;
    }

    if (!dictionary.empty()) {
        deflateSetDictionary(&strm, (const Bytef*)dictionary.data(), (uInt)dictionary.size());
    }

    strm.next_in = (Bytef*)p_data;
    strm.avail_in = (uInt)length;
    std::vector<uint8_t> out_buffer;
    out_buffer.resize(chunk_size);
    int32_t ret;
    size_t total_out = 0;
    do {
        if (total_out + chunk_size > out_buffer.size()) {
            out_buffer.resize(out_buffer.size() + chunk_size);
        }
        strm.next_out = out_buffer.data() + total_out;
        strm.avail_out = (uInt)chunk_size;

        ret = deflate(&strm, Z_FINISH);
        total_out += chunk_size - strm.avail_out;
    } while (ret == Z_OK);

    deflateEnd(&strm);

    if (ret != Z_STREAM_END) {
        p_isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8Literal(p_isolate, "deflate failed")));
        return;
    }

    out_buffer.resize(total_out);
    returnBuffer(args, out_buffer);
}

static void doInflate(const v8::FunctionCallbackInfo<v8::Value>& args, int32_t default_window_bits) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    uint8_t* p_data;
    size_t length;
    std::vector<uint8_t> storage;
    if (!getInput(args, &p_data, &length, storage)) return;

    int32_t level = Z_DEFAULT_COMPRESSION;
    int32_t window_bits = default_window_bits;
    int32_t mem_level = 8;
    int32_t strategy = Z_DEFAULT_STRATEGY;
    int32_t chunk_size = 16384;
    std::vector<uint8_t> dictionary;

    if (args.Length() >= 2) {
        parseZlibOptions(p_isolate, context, args[1], level, window_bits, mem_level, strategy, chunk_size, dictionary);
    }

    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.next_in = (Bytef*)p_data;
    strm.avail_in = (uInt)length;

    if (inflateInit2(&strm, window_bits) != Z_OK) {
        p_isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8Literal(p_isolate, "inflateInit failed")));
        return;
    }
    std::vector<uint8_t> out_buffer;
    out_buffer.resize(chunk_size);
    int32_t ret;
    size_t total_out = 0;
    do {
        if (total_out + chunk_size > out_buffer.size()) {
            out_buffer.resize(out_buffer.size() + chunk_size);
        }
        strm.next_out = out_buffer.data() + total_out;
        strm.avail_out = (uInt)chunk_size;

        ret = inflate(&strm, Z_NO_FLUSH);
        total_out += chunk_size - strm.avail_out;
        
        if (ret == Z_NEED_DICT) {
            if (!dictionary.empty()) {
                ret = inflateSetDictionary(&strm, (const Bytef*)dictionary.data(), (uInt)dictionary.size());
                if (ret == Z_OK) continue;
            }
        }

        if (ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
            inflateEnd(&strm);
            p_isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8Literal(p_isolate, "inflate failed")));
            return;
        }
    } while (ret != Z_STREAM_END);

    inflateEnd(&strm);

    out_buffer.resize(total_out);
    returnBuffer(args, out_buffer);
}

static void doZlibAsync(const v8::FunctionCallbackInfo<v8::Value>& args, int32_t window_bits, bool is_deflate, bool is_promise = false, bool is_brotli = false, bool is_zstd = false) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::HandleScope handle_scope(p_isolate);
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    uint8_t* p_data;
    size_t length;
    std::vector<uint8_t> storage;
    if (!getInput(args, &p_data, &length, storage)) return;
    
    v8::Local<v8::Function> callback;
    v8::Local<v8::Promise::Resolver> resolver;

    if (is_promise) {
        if (!v8::Promise::Resolver::New(context).ToLocal(&resolver)) return;
        args.GetReturnValue().Set(resolver->GetPromise());
    } else {
        if (args.Length() >= 2 && args[args.Length() - 1]->IsFunction()) {
            callback = args[args.Length() - 1].As<v8::Function>();
        } else {
            p_isolate->ThrowException(v8::Exception::TypeError(
                v8::String::NewFromUtf8Literal(p_isolate, "Callback must be provided")));
            return;
        }
    }

    ZlibAsyncCtx* p_ctx = new ZlibAsyncCtx();
    p_ctx->m_input.assign(p_data, p_data + length);
    p_ctx->m_window_bits = window_bits;
    p_ctx->m_is_deflate = is_deflate;
    p_ctx->m_is_brotli = is_brotli;
    p_ctx->m_is_zstd = is_zstd;

    if (args.Length() >= 2 && !args[1]->IsFunction()) {
        if (is_brotli) {
            parseBrotliOptions(p_isolate, context, args[1], p_ctx->m_brotli_quality, p_ctx->m_brotli_window, p_ctx->m_brotli_mode);
        } else if (is_zstd) {
            parseZstdOptions(p_isolate, context, args[1], p_ctx->m_zstd_level);
        } else {
            parseZlibOptions(p_isolate, context, args[1], p_ctx->m_level, p_ctx->m_window_bits, p_ctx->m_mem_level, p_ctx->m_strategy, p_ctx->m_chunk_size, p_ctx->m_dictionary);
        }
    }

    Task* p_task = new Task();
    p_task->m_is_promise = is_promise;
    if (is_promise) {
        p_task->m_resolver.Reset(p_isolate, resolver);
    } else {
        p_task->m_callback.Reset(p_isolate, callback);
    }
    p_task->p_data = p_ctx;

    p_task->m_runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        ZlibAsyncCtx* p_ctx = static_cast<ZlibAsyncCtx*>(task->p_data);
        if (task->m_is_promise) {
            auto resolver = task->m_resolver.Get(isolate);
            if (p_ctx->m_is_error) {
                resolver->Reject(context, v8::Exception::Error(v8::String::NewFromUtf8(isolate, p_ctx->m_error_msg.c_str()).ToLocalChecked())).Check();
            } else {
                v8::Local<v8::Uint8Array> ui = z8::module::Buffer::createBuffer(isolate, p_ctx->m_output.size());
                memcpy(ui->Buffer()->GetBackingStore()->Data(), p_ctx->m_output.data(), p_ctx->m_output.size());
                resolver->Resolve(context, ui).Check();
            }
        } else {
            v8::Local<v8::Value> argv[2];
            if (p_ctx->m_is_error) {
                argv[0] = v8::Exception::Error(v8::String::NewFromUtf8(isolate, p_ctx->m_error_msg.c_str()).ToLocalChecked());
                argv[1] = v8::Null(isolate);
            } else {
                argv[0] = v8::Null(isolate);
                v8::Local<v8::Uint8Array> ui = z8::module::Buffer::createBuffer(isolate, p_ctx->m_output.size());
                memcpy(ui->Buffer()->GetBackingStore()->Data(), p_ctx->m_output.data(), p_ctx->m_output.size());
                argv[1] = ui;
            }
            (void)task->m_callback.Get(isolate)->Call(context, context->Global(), 2, argv);
        }
        delete p_ctx;
    };

    ThreadPool::getInstance().enqueue([p_task]() {
        ZlibAsyncCtx* p_ctx = static_cast<ZlibAsyncCtx*>(p_task->p_data);
        
        if (p_ctx->m_is_brotli) {
            if (p_ctx->m_is_deflate) { // Brotli Compress
                size_t out_len = BrotliEncoderMaxCompressedSize(p_ctx->m_input.size());
                p_ctx->m_output.resize(out_len);
                if (BrotliEncoderCompress(p_ctx->m_brotli_quality, p_ctx->m_brotli_window, (BrotliEncoderMode)p_ctx->m_brotli_mode,
                                          p_ctx->m_input.size(), p_ctx->m_input.data(), &out_len, p_ctx->m_output.data())) {
                    p_ctx->m_output.resize(out_len);
                } else {
                    p_ctx->m_is_error = true;
                    p_ctx->m_error_msg = "Brotli compression failed";
                }
            } else { // Brotli Decompress
                BrotliDecoderState* p_s = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
                if (!p_s) {
                    p_ctx->m_is_error = true;
                    p_ctx->m_error_msg = "Failed to create Brotli decoder";
                } else {
                    size_t available_in = p_ctx->m_input.size();
                    const uint8_t* p_next_in = p_ctx->m_input.data();
                    size_t total_out = 0;
                    const size_t chunk_size = 16384;
                    p_ctx->m_output.clear();

                    BrotliDecoderResult res = BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT;
                    while (res == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) {
                        p_ctx->m_output.resize(total_out + chunk_size);
                        size_t available_out = chunk_size;
                        uint8_t* p_next_out = p_ctx->m_output.data() + total_out;
                        res = BrotliDecoderDecompressStream(p_s, &available_in, &p_next_in, &available_out, &p_next_out, &total_out);
                    }

                    if (res != BROTLI_DECODER_RESULT_SUCCESS) {
                        p_ctx->m_is_error = true;
                        p_ctx->m_error_msg = "Brotli decompression failed";
                    } else {
                        p_ctx->m_output.resize(total_out);
                    }
                    BrotliDecoderDestroyInstance(p_s);
                }
            }
        } else if (p_ctx->m_is_zstd) {
            if (p_ctx->m_is_deflate) { // Zstd Compress
                size_t out_len = ZSTD_compressBound(p_ctx->m_input.size());
                p_ctx->m_output.resize(out_len);
                size_t res = ZSTD_compress(p_ctx->m_output.data(), out_len, p_ctx->m_input.data(), p_ctx->m_input.size(), p_ctx->m_zstd_level);
                if (ZSTD_isError(res)) {
                    p_ctx->m_is_error = true;
                    p_ctx->m_error_msg = ZSTD_getErrorName(res);
                } else {
                    p_ctx->m_output.resize(res);
                }
            } else { // Zstd Decompress
                uint64_t const decoded_size = ZSTD_getFrameContentSize(p_ctx->m_input.data(), p_ctx->m_input.size());
                if (decoded_size != ZSTD_CONTENTSIZE_ERROR && decoded_size != ZSTD_CONTENTSIZE_UNKNOWN) {
                    p_ctx->m_output.resize(decoded_size);
                    size_t res = ZSTD_decompress(p_ctx->m_output.data(), decoded_size, p_ctx->m_input.data(), p_ctx->m_input.size());
                    if (ZSTD_isError(res)) {
                        p_ctx->m_is_error = true;
                        p_ctx->m_error_msg = ZSTD_getErrorName(res);
                    }
                } else {
                    // Unknown size, use streaming decompress or large buffer
                    size_t const chunk_size = 128 * 1024;
                    p_ctx->m_output.resize(chunk_size);
                    ZSTD_DCtx* p_dctx = ZSTD_createDCtx();
                    ZSTD_inBuffer input = { p_ctx->m_input.data(), p_ctx->m_input.size(), 0 };
                    size_t total_out = 0;
                    while (input.pos < input.size) {
                        if (total_out + chunk_size > p_ctx->m_output.size()) {
                            p_ctx->m_output.resize(p_ctx->m_output.size() + chunk_size);
                        }
                        ZSTD_outBuffer output = { p_ctx->m_output.data(), p_ctx->m_output.size(), total_out };
                        size_t const res = ZSTD_decompressStream(p_dctx, &output, &input);
                        total_out = output.pos;
                        if (ZSTD_isError(res)) {
                            p_ctx->m_is_error = true;
                            p_ctx->m_error_msg = ZSTD_getErrorName(res);
                            break;
                        }
                        if (res == 0) break;
                    }
                    p_ctx->m_output.resize(total_out);
                    ZSTD_freeDCtx(p_dctx);
                }
            }
        } else { // Standard Zlib
            z_stream strm;
            strm.zalloc = Z_NULL;
            strm.zfree = Z_NULL;
            strm.opaque = Z_NULL;

            int32_t ret;
            if (p_ctx->m_is_deflate) {
                ret = deflateInit2(&strm, p_ctx->m_level, Z_DEFLATED, p_ctx->m_window_bits, p_ctx->m_mem_level, p_ctx->m_strategy);
                if (ret == Z_OK && !p_ctx->m_dictionary.empty()) {
                    deflateSetDictionary(&strm, (const Bytef*)p_ctx->m_dictionary.data(), (uInt)p_ctx->m_dictionary.size());
                }
            } else {
                ret = inflateInit2(&strm, p_ctx->m_window_bits);
            }

            if (ret != Z_OK) {
                p_ctx->m_is_error = true;
                p_ctx->m_error_msg = "Zlib initialization failed";
                TaskQueue::getInstance().enqueue(p_task);
                return;
            }

            strm.next_in = (Bytef*)p_ctx->m_input.data();
            strm.avail_in = (uInt)p_ctx->m_input.size();
            
            size_t chunk_size = p_ctx->m_chunk_size;
            p_ctx->m_output.resize(chunk_size);
            size_t total_out = 0;

            do {
                if (total_out + chunk_size > p_ctx->m_output.size()) {
                    p_ctx->m_output.resize(p_ctx->m_output.size() + chunk_size);
                }
                strm.next_out = p_ctx->m_output.data() + total_out;
                strm.avail_out = (uInt)chunk_size;

                if (p_ctx->m_is_deflate) {
                    ret = deflate(&strm, Z_FINISH);
                } else {
                    ret = inflate(&strm, Z_NO_FLUSH);
                }
                total_out += chunk_size - strm.avail_out;

                if (ret == Z_NEED_DICT) {
                    if (!p_ctx->m_dictionary.empty()) {
                        ret = inflateSetDictionary(&strm, (const Bytef*)p_ctx->m_dictionary.data(), (uInt)p_ctx->m_dictionary.size());
                        if (ret == Z_OK) continue;
                    }
                    break;
                }
                if (ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
                    break;
                }
            } while (ret != Z_STREAM_END && (p_ctx->m_is_deflate ? (ret == Z_OK) : true));

            if (p_ctx->m_is_deflate) deflateEnd(&strm);
            else inflateEnd(&strm);

            if (ret != Z_STREAM_END) {
                p_ctx->m_is_error = true;
                p_ctx->m_error_msg = "Zlib processing failed";
            } else {
                p_ctx->m_output.resize(total_out);
            }
        }

        TaskQueue::getInstance().enqueue(p_task);
    });
}

void Zlib::deflateSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    doDeflate(args, 15);
}

void Zlib::inflateSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    doInflate(args, 15);
}

void Zlib::deflateRawSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    doDeflate(args, -15);
}

void Zlib::inflateRawSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    doInflate(args, -15);
}

void Zlib::gzipSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    doDeflate(args, 15 + 16);
}

void Zlib::gunzipSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    doInflate(args, 15 + 16);
}

void Zlib::unzipSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    doInflate(args, 15 + 32);
}

void Zlib::deflate(const v8::FunctionCallbackInfo<v8::Value>& args) { doZlibAsync(args, 15, true, false, false, false); }
void Zlib::inflate(const v8::FunctionCallbackInfo<v8::Value>& args) { doZlibAsync(args, 15, false, false, false, false); }
void Zlib::deflateRaw(const v8::FunctionCallbackInfo<v8::Value>& args) { doZlibAsync(args, -15, true, false, false, false); }
void Zlib::inflateRaw(const v8::FunctionCallbackInfo<v8::Value>& args) { doZlibAsync(args, -15, false, false, false, false); }
void Zlib::gzip(const v8::FunctionCallbackInfo<v8::Value>& args) { doZlibAsync(args, 15 + 16, true, false, false, false); }
void Zlib::gunzip(const v8::FunctionCallbackInfo<v8::Value>& args) { doZlibAsync(args, 15 + 16, false, false, false, false); }
void Zlib::unzip(const v8::FunctionCallbackInfo<v8::Value>& args) { doZlibAsync(args, 15 + 32, false, false, false, false); }

void Zlib::deflatePromise(const v8::FunctionCallbackInfo<v8::Value>& args) { doZlibAsync(args, 15, true, true, false, false); }
void Zlib::inflatePromise(const v8::FunctionCallbackInfo<v8::Value>& args) { doZlibAsync(args, 15, false, true, false, false); }
void Zlib::deflateRawPromise(const v8::FunctionCallbackInfo<v8::Value>& args) { doZlibAsync(args, -15, true, true, false, false); }
void Zlib::inflateRawPromise(const v8::FunctionCallbackInfo<v8::Value>& args) { doZlibAsync(args, -15, false, true, false, false); }
void Zlib::gzipPromise(const v8::FunctionCallbackInfo<v8::Value>& args) { doZlibAsync(args, 15 + 16, true, true, false, false); }
void Zlib::gunzipPromise(const v8::FunctionCallbackInfo<v8::Value>& args) { doZlibAsync(args, 15 + 16, false, true, false, false); }
void Zlib::unzipPromise(const v8::FunctionCallbackInfo<v8::Value>& args) { doZlibAsync(args, 15 + 32, false, true, false, false); }

void Zlib::brotliCompress(const v8::FunctionCallbackInfo<v8::Value>& args) { doZlibAsync(args, 0, true, false, true, false); }
void Zlib::brotliDecompress(const v8::FunctionCallbackInfo<v8::Value>& args) { doZlibAsync(args, 0, false, false, true, false); }
void Zlib::brotliCompressPromise(const v8::FunctionCallbackInfo<v8::Value>& args) { doZlibAsync(args, 0, true, true, true, false); }
void Zlib::brotliDecompressPromise(const v8::FunctionCallbackInfo<v8::Value>& args) { doZlibAsync(args, 0, false, true, true, false); }

void Zlib::zstdCompress(const v8::FunctionCallbackInfo<v8::Value>& args) {
    doZlibAsync(args, 0, true, false, false, true);
}

void Zlib::zstdDecompress(const v8::FunctionCallbackInfo<v8::Value>& args) {
    doZlibAsync(args, 0, false, false, false, true);
}

void Zlib::zstdCompressPromise(const v8::FunctionCallbackInfo<v8::Value>& args) {
    doZlibAsync(args, 0, true, true, false, true);
}

void Zlib::zstdDecompressPromise(const v8::FunctionCallbackInfo<v8::Value>& args) {
    doZlibAsync(args, 0, false, true, false, true);
}

void Zlib::brotliCompressSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    uint8_t* p_data;
    size_t length;
    std::vector<uint8_t> storage;
    if (!getInput(args, &p_data, &length, storage)) return;

    int32_t quality = BROTLI_DEFAULT_QUALITY;
    int32_t window = BROTLI_DEFAULT_WINDOW;
    int32_t mode = BROTLI_DEFAULT_MODE;
    if (args.Length() >= 2) {
        parseBrotliOptions(p_isolate, context, args[1], quality, window, mode);
    }

    size_t out_len = BrotliEncoderMaxCompressedSize(length);
    std::vector<uint8_t> out_buffer(out_len);

    if (BrotliEncoderCompress(quality, window, (BrotliEncoderMode)mode,
                              length, p_data, &out_len, out_buffer.data())) {
        out_buffer.resize(out_len);
        returnBuffer(args, out_buffer);
    } else {
        p_isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8Literal(p_isolate, "Brotli compression failed")));
    }
}

void Zlib::brotliDecompressSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    uint8_t* p_data;
    size_t length;
    std::vector<uint8_t> storage;
    if (!getInput(args, &p_data, &length, storage)) return;

    // Brotli decompress options are less common in sync, but we could parse them
    BrotliDecoderState* p_s = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
    if (!p_s) {
        p_isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8Literal(p_isolate, "Failed to create Brotli decoder")));
        return;
    }

    size_t available_in = length;
    const uint8_t* p_next_in = p_data;
    size_t total_out = 0;
    const size_t chunk_size = 16384;
    std::vector<uint8_t> out_buffer;

    BrotliDecoderResult res = BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT;
    while (res == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) {
        out_buffer.resize(total_out + chunk_size);
        size_t available_out = chunk_size;
        uint8_t* p_next_out = out_buffer.data() + total_out;
        res = BrotliDecoderDecompressStream(p_s, &available_in, &p_next_in, &available_out, &p_next_out, &total_out);
    }

    if (res != BROTLI_DECODER_RESULT_SUCCESS) {
        const char* p_err_str = BrotliDecoderErrorString(BrotliDecoderGetErrorCode(p_s));
        BrotliDecoderDestroyInstance(p_s);
        p_isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(p_isolate, (std::string("Brotli decompression failed: ") + p_err_str).c_str()).ToLocalChecked()));
        return;
    }

    out_buffer.resize(total_out);
    BrotliDecoderDestroyInstance(p_s);
    returnBuffer(args, out_buffer);
}

void Zlib::zstdCompressSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    uint8_t* p_data;
    size_t length;
    std::vector<uint8_t> storage;
    if (!getInput(args, &p_data, &length, storage)) return;

    int32_t level = 3;
    if (args.Length() >= 2) {
        parseZstdOptions(p_isolate, context, args[1], level);
    }

    size_t out_len = ZSTD_compressBound(length);
    std::vector<uint8_t> out_buffer(out_len);
    size_t res = ZSTD_compress(out_buffer.data(), out_len, p_data, length, level);

    if (ZSTD_isError(res)) {
        p_isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(p_isolate, ZSTD_getErrorName(res)).ToLocalChecked()));
        return;
    }

    out_buffer.resize(res);
    returnBuffer(args, out_buffer);
}

void Zlib::zstdDecompressSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    uint8_t* p_data;
    size_t length;
    std::vector<uint8_t> storage;
    if (!getInput(args, &p_data, &length, storage)) return;

    uint64_t const decoded_size = ZSTD_getFrameContentSize(p_data, length);
    if (decoded_size != ZSTD_CONTENTSIZE_ERROR && decoded_size != ZSTD_CONTENTSIZE_UNKNOWN) {
        std::vector<uint8_t> out_buffer(decoded_size);
        size_t res = ZSTD_decompress(out_buffer.data(), decoded_size, p_data, length);
        if (ZSTD_isError(res)) {
            p_isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(p_isolate, ZSTD_getErrorName(res)).ToLocalChecked()));
            return;
        }
        returnBuffer(args, out_buffer);
    } else {
        // Fallback for unknown size
        size_t const chunk_size = 128 * 1024;
        std::vector<uint8_t> out_buffer;
        ZSTD_DCtx* p_dctx = ZSTD_createDCtx();
        ZSTD_inBuffer input = { p_data, length, 0 };
        size_t total_out = 0;
        while (input.pos < input.size) {
            out_buffer.resize(total_out + chunk_size);
            ZSTD_outBuffer output = { out_buffer.data(), out_buffer.size(), total_out };
            size_t const res = ZSTD_decompressStream(p_dctx, &output, &input);
            total_out = output.pos;
            if (ZSTD_isError(res)) {
                ZSTD_freeDCtx(p_dctx);
                p_isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(p_isolate, ZSTD_getErrorName(res)).ToLocalChecked()));
                return;
            }
            if (res == 0) break;
        }
        out_buffer.resize(total_out);
        ZSTD_freeDCtx(p_dctx);
        returnBuffer(args, out_buffer);
    }
}


void Zlib::crc32(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    uint8_t* p_data;
    size_t length;
    std::vector<uint8_t> storage;
    if (!getInput(args, &p_data, &length, storage)) return;

    uint32_t crc = 0;
    if (args.Length() >= 2 && args[1]->IsNumber()) {
        crc = args[1]->Uint32Value(p_isolate->GetCurrentContext()).FromMaybe(0);
    }

    uint32_t result = ::crc32(crc, (const Bytef*)p_data, (uInt)length);
    args.GetReturnValue().Set(v8::Integer::NewFromUnsigned(p_isolate, result));
}

void Zlib::adler32(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    uint8_t* p_data;
    size_t length;
    std::vector<uint8_t> storage;
    if (!getInput(args, &p_data, &length, storage)) return;

    uint32_t adler = 1;
    if (args.Length() >= 2 && args[1]->IsNumber()) {
        adler = args[1]->Uint32Value(p_isolate->GetCurrentContext()).FromMaybe(1);
    }

    uint32_t result = ::adler32(adler, (const Bytef*)p_data, (uInt)length);
    args.GetReturnValue().Set(v8::Integer::NewFromUnsigned(p_isolate, result));
}

struct ZlibStreamObject {
    z_stream m_strm;
    bool m_is_deflate;
    bool m_initialized = false;
    bool m_finished = false;
    int32_t m_chunk_size = 16384;
    std::vector<uint8_t> m_dictionary;
    int32_t m_window_bits;

    ZlibStreamObject() {
        memset(&m_strm, 0, sizeof(m_strm));
    }
    ~ZlibStreamObject() {
        if (m_initialized) {
            if (m_is_deflate) deflateEnd(&m_strm);
            else inflateEnd(&m_strm);
        }
    }
};

static void streamProcess(const v8::FunctionCallbackInfo<v8::Value>& args, int32_t flush) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    v8::Local<v8::Object> self = args.This();
    
    v8::Local<v8::Data> internal_data = self->GetInternalField(0);
    if (internal_data.IsEmpty() || !internal_data.As<v8::Value>()->IsExternal()) return;
    ZlibStreamObject* p_obj = static_cast<ZlibStreamObject*>(internal_data.As<v8::External>()->Value());

    uint8_t* p_data = nullptr;
    size_t length = 0;
    if (args.Length() > 0 && args[0]->IsUint8Array()) {
        v8::Local<v8::Uint8Array> input = args[0].As<v8::Uint8Array>();
        p_data = static_cast<uint8_t*>(input->Buffer()->GetBackingStore()->Data()) + input->ByteOffset();
        length = input->ByteLength();
    }

    p_obj->m_strm.next_in = (Bytef*)p_data;
    p_obj->m_strm.avail_in = (uInt)length;
    std::vector<uint8_t> out_buffer;
    const size_t chunk_size = p_obj->m_chunk_size;
    out_buffer.resize(chunk_size);
    int32_t ret;
    size_t total_out = 0;
    do {
        if (total_out + chunk_size > out_buffer.size()) {
            out_buffer.resize(out_buffer.size() + chunk_size);
        }
        p_obj->m_strm.next_out = out_buffer.data() + total_out;
        p_obj->m_strm.avail_out = (uInt)chunk_size;

        if (p_obj->m_is_deflate) {
            ret = deflate(&p_obj->m_strm, flush);
        } else {
            ret = inflate(&p_obj->m_strm, flush);
            if (ret == Z_NEED_DICT && !p_obj->m_dictionary.empty()) {
                ret = inflateSetDictionary(&p_obj->m_strm, (const Bytef*)p_obj->m_dictionary.data(), (uInt)p_obj->m_dictionary.size());
                if (ret == Z_OK) {
                    ret = inflate(&p_obj->m_strm, flush);
                }
            }
        }
        total_out += chunk_size - p_obj->m_strm.avail_out;
    } while (p_obj->m_strm.avail_out == 0 && ret == Z_OK);

    if (ret != Z_OK && ret != Z_STREAM_END && ret != Z_BUF_ERROR) {
        p_isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8Literal(p_isolate, "Zlib stream error")));
        return;
    }

    if (ret == Z_STREAM_END) {
        p_obj->m_finished = true;
    }

    v8::Local<v8::ArrayBuffer> ab = v8::ArrayBuffer::New(p_isolate, total_out);
    memcpy(ab->GetBackingStore()->Data(), out_buffer.data(), total_out);
    args.GetReturnValue().Set(v8::Uint8Array::New(ab, 0, total_out));
}

static void streamWrite(const v8::FunctionCallbackInfo<v8::Value>& args) {
    streamProcess(args, Z_NO_FLUSH);
}

static void streamFlush(const v8::FunctionCallbackInfo<v8::Value>& args) {
    streamProcess(args, Z_SYNC_FLUSH);
}

static void streamEnd(const v8::FunctionCallbackInfo<v8::Value>& args) {
    streamProcess(args, Z_FINISH);
}

static void streamClose(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Local<v8::Object> self = args.This();
    v8::Local<v8::Data> internal_data = self->GetInternalField(0);
    if (internal_data.IsEmpty() || !internal_data.As<v8::Value>()->IsExternal()) return;
    ZlibStreamObject* p_obj = static_cast<ZlibStreamObject*>(internal_data.As<v8::External>()->Value());
    if (p_obj->m_initialized) {
        if (p_obj->m_is_deflate) {
            deflateEnd(&p_obj->m_strm);
        } else {
            inflateEnd(&p_obj->m_strm);
        }
        p_obj->m_initialized = false;
    }
}

static void streamReset(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Local<v8::Object> self = args.This();
    v8::Local<v8::Data> internal_data = self->GetInternalField(0);
    if (internal_data.IsEmpty() || !internal_data.As<v8::Value>()->IsExternal()) return;
    ZlibStreamObject* p_obj = static_cast<ZlibStreamObject*>(internal_data.As<v8::External>()->Value());
    if (p_obj->m_initialized) {
        if (p_obj->m_is_deflate) {
            deflateReset(&p_obj->m_strm);
        } else {
            inflateReset(&p_obj->m_strm);
        }
        p_obj->m_finished = false;
    }
}

static void streamParams(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    v8::Local<v8::Object> self = args.This();
    v8::Local<v8::Data> internal_data = self->GetInternalField(0);
    if (internal_data.IsEmpty() || !internal_data.As<v8::Value>()->IsExternal()) return;
    ZlibStreamObject* p_obj = static_cast<ZlibStreamObject*>(internal_data.As<v8::External>()->Value());
    
    if (p_obj->m_initialized && p_obj->m_is_deflate) {
        int32_t level = Z_DEFAULT_COMPRESSION;
        int32_t strategy = Z_DEFAULT_STRATEGY;
        if (args.Length() >= 1 && args[0]->IsNumber()) {
            level = args[0]->Int32Value(context).FromMaybe(level);
        }
        if (args.Length() >= 2 && args[1]->IsNumber()) {
            strategy = args[1]->Int32Value(context).FromMaybe(strategy);
        }
        deflateParams(&p_obj->m_strm, level, strategy);
    }
}

static void createZlibStream(const v8::FunctionCallbackInfo<v8::Value>& args, int32_t default_window_bits, bool is_deflate, v8::Local<v8::Value> options_val) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();

    int32_t level = Z_DEFAULT_COMPRESSION;
    int32_t window_bits = default_window_bits;
    int32_t mem_level = 8;
    int32_t strategy = Z_DEFAULT_STRATEGY;
    int32_t chunk_size = 16384;
    std::vector<uint8_t> dictionary;

    parseZlibOptions(p_isolate, context, options_val, level, window_bits, mem_level, strategy, chunk_size, dictionary);

    ZlibStreamObject* p_stream = new ZlibStreamObject();
    p_stream->m_is_deflate = is_deflate;
    p_stream->m_chunk_size = chunk_size;
    p_stream->m_dictionary = dictionary;
    p_stream->m_window_bits = window_bits;
    
    int32_t ret;
    if (is_deflate) {
        ret = deflateInit2(&p_stream->m_strm, level, Z_DEFLATED, window_bits, mem_level, strategy);
        if (ret == Z_OK && !p_stream->m_dictionary.empty()) {
            deflateSetDictionary(&p_stream->m_strm, (const Bytef*)p_stream->m_dictionary.data(), (uInt)p_stream->m_dictionary.size());
        }
    } else {
        ret = inflateInit2(&p_stream->m_strm, window_bits);
    }

    if (ret != Z_OK) {
        delete p_stream;
        p_isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8Literal(p_isolate, "Zlib init failed")));
        return;
    }
    p_stream->m_initialized = true;

    v8::Local<v8::ObjectTemplate> tmpl = v8::ObjectTemplate::New(p_isolate);
    tmpl->SetInternalFieldCount(1);
    v8::Local<v8::Object> js_obj = tmpl->NewInstance(context).ToLocalChecked();
    js_obj->SetInternalField(0, v8::External::New(p_isolate, p_stream));
    
    // Register Weak Callback for cleanup
    v8::Global<v8::Object> global_obj(p_isolate, js_obj);
    global_obj.SetWeak(p_stream, [](const v8::WeakCallbackInfo<ZlibStreamObject>& data) {
        delete data.GetParameter();
    }, v8::WeakCallbackType::kParameter);

    js_obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "write"), v8::FunctionTemplate::New(p_isolate, streamWrite)->GetFunction(context).ToLocalChecked()).Check();
    js_obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "flush"), v8::FunctionTemplate::New(p_isolate, streamFlush)->GetFunction(context).ToLocalChecked()).Check();
    js_obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "end"), v8::FunctionTemplate::New(p_isolate, streamEnd)->GetFunction(context).ToLocalChecked()).Check();
    js_obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "close"), v8::FunctionTemplate::New(p_isolate, streamClose)->GetFunction(context).ToLocalChecked()).Check();
    js_obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "reset"), v8::FunctionTemplate::New(p_isolate, streamReset)->GetFunction(context).ToLocalChecked()).Check();
    js_obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "params"), v8::FunctionTemplate::New(p_isolate, streamParams)->GetFunction(context).ToLocalChecked()).Check();

    args.GetReturnValue().Set(js_obj);
}

void Zlib::createGzip(const v8::FunctionCallbackInfo<v8::Value>& args) { createZlibStream(args, 15 + 16, true, args.Length() > 0 ? args[0] : v8::Local<v8::Value>()); }
void Zlib::createGunzip(const v8::FunctionCallbackInfo<v8::Value>& args) { createZlibStream(args, 15 + 16, false, args.Length() > 0 ? args[0] : v8::Local<v8::Value>()); }
void Zlib::createDeflate(const v8::FunctionCallbackInfo<v8::Value>& args) { createZlibStream(args, 15, true, args.Length() > 0 ? args[0] : v8::Local<v8::Value>()); }
void Zlib::createInflate(const v8::FunctionCallbackInfo<v8::Value>& args) { createZlibStream(args, 15, false, args.Length() > 0 ? args[0] : v8::Local<v8::Value>()); }
void Zlib::createDeflateRaw(const v8::FunctionCallbackInfo<v8::Value>& args) { createZlibStream(args, -15, true, args.Length() > 0 ? args[0] : v8::Local<v8::Value>()); }
void Zlib::createInflateRaw(const v8::FunctionCallbackInfo<v8::Value>& args) { createZlibStream(args, -15, false, args.Length() > 0 ? args[0] : v8::Local<v8::Value>()); }
void Zlib::createUnzip(const v8::FunctionCallbackInfo<v8::Value>& args) { createZlibStream(args, 15 + 32, false, args.Length() > 0 ? args[0] : v8::Local<v8::Value>()); }

struct BrotliStreamObject {
    BrotliEncoderState* p_enc = nullptr;
    BrotliDecoderState* p_dec = nullptr;
    bool m_is_encoder;
    bool m_finished = false;

    BrotliStreamObject(bool enc_mode) : m_is_encoder(enc_mode) {
        if (m_is_encoder) p_enc = BrotliEncoderCreateInstance(nullptr, nullptr, nullptr);
        else p_dec = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
    }
    ~BrotliStreamObject() {
        if (p_enc) BrotliEncoderDestroyInstance(p_enc);
        if (p_dec) BrotliDecoderDestroyInstance(p_dec);
    }

    void reset() {
        if (m_is_encoder) {
            if (p_enc) BrotliEncoderDestroyInstance(p_enc);
            p_enc = BrotliEncoderCreateInstance(nullptr, nullptr, nullptr);
        } else {
            if (p_dec) BrotliDecoderDestroyInstance(p_dec);
            p_dec = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
        }
        m_finished = false;
    }
};

static void brotliStreamClose(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Local<v8::Object> self = args.This();
    v8::Local<v8::Data> internal_data = self->GetInternalField(0);
    if (internal_data.IsEmpty() || !internal_data.As<v8::Value>()->IsExternal()) return;
    BrotliStreamObject* p_obj = static_cast<BrotliStreamObject*>(internal_data.As<v8::External>()->Value());
    if (p_obj->p_enc) {
        BrotliEncoderDestroyInstance(p_obj->p_enc);
        p_obj->p_enc = nullptr;
    }
    if (p_obj->p_dec) {
        BrotliDecoderDestroyInstance(p_obj->p_dec);
        p_obj->p_dec = nullptr;
    }
}

static void brotliStreamReset(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Local<v8::Object> self = args.This();
    v8::Local<v8::Data> internal_data = self->GetInternalField(0);
    if (internal_data.IsEmpty() || !internal_data.As<v8::Value>()->IsExternal()) return;
    BrotliStreamObject* p_obj = static_cast<BrotliStreamObject*>(internal_data.As<v8::External>()->Value());
    p_obj->reset();
}

static void brotliStreamWrite(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    v8::Local<v8::Object> self = args.This();
    
    v8::Local<v8::Data> internal_data = self->GetInternalField(0);
    if (internal_data.IsEmpty() || !internal_data.As<v8::Value>()->IsExternal()) return;
    BrotliStreamObject* p_obj = static_cast<BrotliStreamObject*>(internal_data.As<v8::External>()->Value());

    uint8_t* p_data = nullptr;
    size_t length = 0;
    if (args.Length() > 0 && args[0]->IsUint8Array()) {
        v8::Local<v8::Uint8Array> input = args[0].As<v8::Uint8Array>();
        p_data = static_cast<uint8_t*>(input->Buffer()->GetBackingStore()->Data()) + input->ByteOffset();
        length = input->ByteLength();
    }
    std::vector<uint8_t> out_buffer;
    const size_t chunk_size = 16384;
    size_t total_out = 0;

    if (p_obj->m_is_encoder) {
        size_t available_in = length;
        const uint8_t* p_next_in = p_data;
        while (available_in > 0 || BrotliEncoderHasMoreOutput(p_obj->p_enc)) {
            out_buffer.resize(total_out + chunk_size);
            size_t available_out = chunk_size;
            uint8_t* p_next_out = out_buffer.data() + total_out;
            if (!BrotliEncoderCompressStream(p_obj->p_enc, BROTLI_OPERATION_PROCESS, &available_in, &p_next_in, &available_out, &p_next_out, &total_out)) {
                p_isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8Literal(p_isolate, "Brotli encoding error")));
                return;
            }
        }
    } else {
        size_t available_in = length;
        const uint8_t* p_next_in = p_data;
        BrotliDecoderResult res = BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT;
        while (available_in > 0 || res == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) {
            out_buffer.resize(total_out + chunk_size);
            size_t available_out = chunk_size;
            uint8_t* p_next_out = out_buffer.data() + total_out;
            res = BrotliDecoderDecompressStream(p_obj->p_dec, &available_in, &p_next_in, &available_out, &p_next_out, &total_out);
            if (res == BROTLI_DECODER_RESULT_ERROR) {
                p_isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8Literal(p_isolate, "Brotli decoding error")));
                return;
            }
            if (res == BROTLI_DECODER_RESULT_SUCCESS) break;
        }
    }

    v8::Local<v8::ArrayBuffer> ab = v8::ArrayBuffer::New(p_isolate, total_out);
    memcpy(ab->GetBackingStore()->Data(), out_buffer.data(), total_out);
    args.GetReturnValue().Set(v8::Uint8Array::New(ab, 0, total_out));
}

static void brotliStreamEnd(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    v8::Local<v8::Object> self = args.This();
    
    v8::Local<v8::Data> internal_data = self->GetInternalField(0);
    if (internal_data.IsEmpty() || !internal_data.As<v8::Value>()->IsExternal()) return;
    BrotliStreamObject* p_obj = static_cast<BrotliStreamObject*>(internal_data.As<v8::External>()->Value());
    std::vector<uint8_t> out_buffer;
    const size_t chunk_size = 16384;
    size_t total_out = 0;

    if (p_obj->m_is_encoder) {
        size_t available_in = 0;
        const uint8_t* p_next_in = nullptr;
        while (!BrotliEncoderIsFinished(p_obj->p_enc)) {
            out_buffer.resize(total_out + chunk_size);
            size_t available_out = chunk_size;
            uint8_t* p_next_out = out_buffer.data() + total_out;
            if (!BrotliEncoderCompressStream(p_obj->p_enc, BROTLI_OPERATION_FINISH, &available_in, &p_next_in, &available_out, &p_next_out, &total_out)) {
                p_isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8Literal(p_isolate, "Brotli finish error")));
                return;
            }
        }
    }
    // Decompressor doesn't have a special "finish" beyond processing remaining input.

    v8::Local<v8::ArrayBuffer> ab = v8::ArrayBuffer::New(p_isolate, total_out);
    memcpy(ab->GetBackingStore()->Data(), out_buffer.data(), total_out);
    args.GetReturnValue().Set(v8::Uint8Array::New(ab, 0, total_out));
}

void Zlib::createBrotliCompress(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();

    int32_t quality = BROTLI_DEFAULT_QUALITY;
    int32_t window = BROTLI_DEFAULT_WINDOW;
    int32_t mode = BROTLI_DEFAULT_MODE;

    if (args.Length() > 0) {
        parseBrotliOptions(p_isolate, context, args[0], quality, window, mode);
    }

    BrotliStreamObject* p_stream = new BrotliStreamObject(true);
    if (p_stream->p_enc) {
        BrotliEncoderSetParameter(p_stream->p_enc, BROTLI_PARAM_QUALITY, quality);
        BrotliEncoderSetParameter(p_stream->p_enc, BROTLI_PARAM_LGWIN, window);
        BrotliEncoderSetParameter(p_stream->p_enc, BROTLI_PARAM_MODE, mode);
    }
    
    v8::Local<v8::ObjectTemplate> tmpl = v8::ObjectTemplate::New(p_isolate);
    tmpl->SetInternalFieldCount(1);
    v8::Local<v8::Object> js_obj = tmpl->NewInstance(context).ToLocalChecked();
    js_obj->SetInternalField(0, v8::External::New(p_isolate, p_stream));
    
    v8::Global<v8::Object> global_obj(p_isolate, js_obj);
    global_obj.SetWeak(p_stream, [](const v8::WeakCallbackInfo<BrotliStreamObject>& data) {
        delete data.GetParameter();
    }, v8::WeakCallbackType::kParameter);

    js_obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "write"), v8::FunctionTemplate::New(p_isolate, brotliStreamWrite)->GetFunction(context).ToLocalChecked()).Check();
    js_obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "end"), v8::FunctionTemplate::New(p_isolate, brotliStreamEnd)->GetFunction(context).ToLocalChecked()).Check();
    js_obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "close"), v8::FunctionTemplate::New(p_isolate, brotliStreamClose)->GetFunction(context).ToLocalChecked()).Check();
    js_obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "reset"), v8::FunctionTemplate::New(p_isolate, brotliStreamReset)->GetFunction(context).ToLocalChecked()).Check();
    args.GetReturnValue().Set(js_obj);
}

void Zlib::createBrotliDecompress(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    
    // Decompressor might also have options in future
    BrotliStreamObject* p_stream = new BrotliStreamObject(false);
    
    v8::Local<v8::ObjectTemplate> tmpl = v8::ObjectTemplate::New(p_isolate);
    tmpl->SetInternalFieldCount(1);
    v8::Local<v8::Object> js_obj = tmpl->NewInstance(context).ToLocalChecked();
    js_obj->SetInternalField(0, v8::External::New(p_isolate, p_stream));
    
    v8::Global<v8::Object> global_obj(p_isolate, js_obj);
    global_obj.SetWeak(p_stream, [](const v8::WeakCallbackInfo<BrotliStreamObject>& data) {
        delete data.GetParameter();
    }, v8::WeakCallbackType::kParameter);

    js_obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "write"), v8::FunctionTemplate::New(p_isolate, brotliStreamWrite)->GetFunction(context).ToLocalChecked()).Check();
    js_obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "end"), v8::FunctionTemplate::New(p_isolate, brotliStreamEnd)->GetFunction(context).ToLocalChecked()).Check();
    js_obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "close"), v8::FunctionTemplate::New(p_isolate, brotliStreamClose)->GetFunction(context).ToLocalChecked()).Check();
    js_obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "reset"), v8::FunctionTemplate::New(p_isolate, brotliStreamReset)->GetFunction(context).ToLocalChecked()).Check();
    args.GetReturnValue().Set(js_obj);
}

struct ZstdStreamObject {
    ZSTD_CCtx* p_cctx = nullptr;
    ZSTD_DCtx* p_dctx = nullptr;
    bool m_is_compressor;
    bool m_finished = false;

    ZstdStreamObject(bool compress_mode) : m_is_compressor(compress_mode) {
        if (m_is_compressor) p_cctx = ZSTD_createCCtx();
        else p_dctx = ZSTD_createDCtx();
    }
    ~ZstdStreamObject() {
        if (p_cctx) ZSTD_freeCCtx(p_cctx);
        if (p_dctx) ZSTD_freeDCtx(p_dctx);
    }
    
    void reset() {
        if (m_is_compressor) {
            if (p_cctx) ZSTD_CCtx_reset(p_cctx, ZSTD_reset_session_and_parameters);
        } else {
            if (p_dctx) ZSTD_DCtx_reset(p_dctx, ZSTD_reset_session_and_parameters);
        }
        m_finished = false;
    }
};

static void zstdStreamClose(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Local<v8::Object> self = args.This();
    v8::Local<v8::Data> internal_data = self->GetInternalField(0);
    if (internal_data.IsEmpty() || !internal_data.As<v8::Value>()->IsExternal()) return;
    ZstdStreamObject* p_obj = static_cast<ZstdStreamObject*>(internal_data.As<v8::External>()->Value());
    if (p_obj->p_cctx) {
        ZSTD_freeCCtx(p_obj->p_cctx);
        p_obj->p_cctx = nullptr;
    }
    if (p_obj->p_dctx) {
        ZSTD_freeDCtx(p_obj->p_dctx);
        p_obj->p_dctx = nullptr;
    }
}

static void zstdStreamReset(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Local<v8::Object> self = args.This();
    v8::Local<v8::Data> internal_data = self->GetInternalField(0);
    if (internal_data.IsEmpty() || !internal_data.As<v8::Value>()->IsExternal()) return;
    ZstdStreamObject* p_obj = static_cast<ZstdStreamObject*>(internal_data.As<v8::External>()->Value());
    p_obj->reset();
}

static void zstdStreamWrite(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Object> self = args.This();
    
    v8::Local<v8::Data> internal_data = self->GetInternalField(0);
    if (internal_data.IsEmpty() || !internal_data.As<v8::Value>()->IsExternal()) return;
    ZstdStreamObject* p_obj = static_cast<ZstdStreamObject*>(internal_data.As<v8::External>()->Value());

    uint8_t* p_data = nullptr;
    size_t length = 0;
    if (args.Length() > 0 && args[0]->IsUint8Array()) {
        v8::Local<v8::Uint8Array> input = args[0].As<v8::Uint8Array>();
        p_data = static_cast<uint8_t*>(input->Buffer()->GetBackingStore()->Data()) + input->ByteOffset();
        length = input->ByteLength();
    }

    std::vector<uint8_t> out_buffer;
    const size_t chunk_size = 128 * 1024;
    size_t total_out = 0;

    if (p_obj->m_is_compressor) {
        ZSTD_inBuffer input = { p_data, length, 0 };
        while (input.pos < input.size) {
            out_buffer.resize(total_out + chunk_size);
            ZSTD_outBuffer output = { out_buffer.data(), out_buffer.size(), total_out };
            size_t const res = ZSTD_compressStream2(p_obj->p_cctx, &output, &input, ZSTD_e_continue);
            total_out = output.pos;
            if (ZSTD_isError(res)) {
                p_isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(p_isolate, ZSTD_getErrorName(res)).ToLocalChecked()));
                return;
            }
        }
    } else {
        ZSTD_inBuffer input = { p_data, length, 0 };
        while (input.pos < input.size) {
            out_buffer.resize(total_out + chunk_size);
            ZSTD_outBuffer output = { out_buffer.data(), out_buffer.size(), total_out };
            size_t const res = ZSTD_decompressStream(p_obj->p_dctx, &output, &input);
            total_out = output.pos;
            if (ZSTD_isError(res)) {
                p_isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(p_isolate, ZSTD_getErrorName(res)).ToLocalChecked()));
                return;
            }
            if (res == 0) break;
        }
    }

    v8::Local<v8::ArrayBuffer> ab = v8::ArrayBuffer::New(p_isolate, total_out);
    memcpy(ab->GetBackingStore()->Data(), out_buffer.data(), total_out);
    args.GetReturnValue().Set(v8::Uint8Array::New(ab, 0, total_out));
}

static void zstdStreamEnd(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Object> self = args.This();
    
    v8::Local<v8::Data> internal_data = self->GetInternalField(0);
    if (internal_data.IsEmpty() || !internal_data.As<v8::Value>()->IsExternal()) return;
    ZstdStreamObject* p_obj = static_cast<ZstdStreamObject*>(internal_data.As<v8::External>()->Value());

    std::vector<uint8_t> out_buffer;
    const size_t chunk_size = 128 * 1024;
    size_t total_out = 0;

    if (p_obj->m_is_compressor) {
        ZSTD_inBuffer input = { nullptr, 0, 0 };
        size_t res;
        do {
            out_buffer.resize(total_out + chunk_size);
            ZSTD_outBuffer output = { out_buffer.data(), out_buffer.size(), total_out };
            res = ZSTD_compressStream2(p_obj->p_cctx, &output, &input, ZSTD_e_end);
            total_out = output.pos;
            if (ZSTD_isError(res)) {
                p_isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(p_isolate, ZSTD_getErrorName(res)).ToLocalChecked()));
                return;
            }
        } while (res > 0);
    }

    v8::Local<v8::ArrayBuffer> ab = v8::ArrayBuffer::New(p_isolate, total_out);
    memcpy(ab->GetBackingStore()->Data(), out_buffer.data(), total_out);
    args.GetReturnValue().Set(v8::Uint8Array::New(ab, 0, total_out));
}

void Zlib::createZstdCompress(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();

    int32_t level = 3;
    if (args.Length() > 0) {
        parseZstdOptions(p_isolate, context, args[0], level);
    }

    ZstdStreamObject* p_stream = new ZstdStreamObject(true);
    if (p_stream->p_cctx) {
        ZSTD_CCtx_setParameter(p_stream->p_cctx, ZSTD_c_compressionLevel, level);
    }
    
    v8::Local<v8::ObjectTemplate> tmpl = v8::ObjectTemplate::New(p_isolate);
    tmpl->SetInternalFieldCount(1);
    v8::Local<v8::Object> js_obj = tmpl->NewInstance(context).ToLocalChecked();
    js_obj->SetInternalField(0, v8::External::New(p_isolate, p_stream));
    
    v8::Global<v8::Object> global_obj(p_isolate, js_obj);
    global_obj.SetWeak(p_stream, [](const v8::WeakCallbackInfo<ZstdStreamObject>& data) {
        delete data.GetParameter();
    }, v8::WeakCallbackType::kParameter);

    js_obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "write"), v8::FunctionTemplate::New(p_isolate, zstdStreamWrite)->GetFunction(context).ToLocalChecked()).Check();
    js_obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "end"), v8::FunctionTemplate::New(p_isolate, zstdStreamEnd)->GetFunction(context).ToLocalChecked()).Check();
    js_obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "close"), v8::FunctionTemplate::New(p_isolate, zstdStreamClose)->GetFunction(context).ToLocalChecked()).Check();
    js_obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "reset"), v8::FunctionTemplate::New(p_isolate, zstdStreamReset)->GetFunction(context).ToLocalChecked()).Check();
    args.GetReturnValue().Set(js_obj);
}

void Zlib::createZstdDecompress(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();

    ZstdStreamObject* p_stream = new ZstdStreamObject(false);
    
    v8::Local<v8::ObjectTemplate> tmpl = v8::ObjectTemplate::New(p_isolate);
    tmpl->SetInternalFieldCount(1);
    v8::Local<v8::Object> js_obj = tmpl->NewInstance(context).ToLocalChecked();
    js_obj->SetInternalField(0, v8::External::New(p_isolate, p_stream));
    
    v8::Global<v8::Object> global_obj(p_isolate, js_obj);
    global_obj.SetWeak(p_stream, [](const v8::WeakCallbackInfo<ZstdStreamObject>& data) {
        delete data.GetParameter();
    }, v8::WeakCallbackType::kParameter);

    js_obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "write"), v8::FunctionTemplate::New(p_isolate, zstdStreamWrite)->GetFunction(context).ToLocalChecked()).Check();
    js_obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "end"), v8::FunctionTemplate::New(p_isolate, zstdStreamEnd)->GetFunction(context).ToLocalChecked()).Check();
    js_obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "close"), v8::FunctionTemplate::New(p_isolate, zstdStreamClose)->GetFunction(context).ToLocalChecked()).Check();
    js_obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "reset"), v8::FunctionTemplate::New(p_isolate, zstdStreamReset)->GetFunction(context).ToLocalChecked()).Check();
    args.GetReturnValue().Set(js_obj);
}

v8::Local<v8::ObjectTemplate> Zlib::createPromisesTemplate(v8::Isolate* p_isolate) {
    v8::Local<v8::ObjectTemplate> tmpl = v8::ObjectTemplate::New(p_isolate);
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "deflate"), v8::FunctionTemplate::New(p_isolate, deflatePromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "inflate"), v8::FunctionTemplate::New(p_isolate, inflatePromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "deflateRaw"), v8::FunctionTemplate::New(p_isolate, deflateRawPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "inflateRaw"), v8::FunctionTemplate::New(p_isolate, inflateRawPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "gzip"), v8::FunctionTemplate::New(p_isolate, gzipPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "gunzip"), v8::FunctionTemplate::New(p_isolate, gunzipPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "unzip"), v8::FunctionTemplate::New(p_isolate, unzipPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "brotliCompress"), v8::FunctionTemplate::New(p_isolate, brotliCompressPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "brotliDecompress"), v8::FunctionTemplate::New(p_isolate, brotliDecompressPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "zstdCompress"), v8::FunctionTemplate::New(p_isolate, zstdCompressPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "zstdDecompress"), v8::FunctionTemplate::New(p_isolate, zstdDecompressPromise));
    return tmpl;
}

} // namespace module
} // namespace z8
