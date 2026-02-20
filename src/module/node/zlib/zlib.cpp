#include "zlib.h"
#include <vector>
#include <cstring>
#include <string>
#include "../../../../deps/zlib/zlib.h"
#include "../../../../deps/brotli/c/include/brotli/encode.h"
#include "../../../../deps/brotli/c/include/brotli/decode.h"
#include "task_queue.h"
#include "thread_pool.h"

namespace z8 {
namespace module {

struct ZlibAsyncCtx {
    std::vector<uint8_t> m_input;
    std::vector<uint8_t> m_output;
    int32_t m_window_bits;
    bool m_is_deflate;
    bool m_is_brotli = false;
    bool m_is_error = false;
    std::string m_error_msg;
};

static bool getInput(const v8::FunctionCallbackInfo<v8::Value>& args, uint8_t** p_data, size_t* length) {
    v8::Isolate* p_isolate = args.GetIsolate();
    if (args.Length() < 1 || !args[0]->IsUint8Array()) {
        p_isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(p_isolate, "Argument must be a Uint8Array")));
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
    v8::Local<v8::ArrayBuffer> result_ab = v8::ArrayBuffer::New(p_isolate, out_buffer.size());
    memcpy(result_ab->GetBackingStore()->Data(), out_buffer.data(), out_buffer.size());
    args.GetReturnValue().Set(v8::Uint8Array::New(result_ab, 0, out_buffer.size()));
}

v8::Local<v8::ObjectTemplate> Zlib::createTemplate(v8::Isolate* p_isolate) {
    v8::Local<v8::ObjectTemplate> tmpl = v8::ObjectTemplate::New(p_isolate);

    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "deflateSync"),
              v8::FunctionTemplate::New(p_isolate, deflateSync));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "inflateSync"),
              v8::FunctionTemplate::New(p_isolate, inflateSync));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "deflateRawSync"),
              v8::FunctionTemplate::New(p_isolate, deflateRawSync));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "inflateRawSync"),
              v8::FunctionTemplate::New(p_isolate, inflateRawSync));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "gzipSync"),
              v8::FunctionTemplate::New(p_isolate, gzipSync));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "gunzipSync"),
              v8::FunctionTemplate::New(p_isolate, gunzipSync));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "unzipSync"),
              v8::FunctionTemplate::New(p_isolate, unzipSync));

    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "deflate"),
              v8::FunctionTemplate::New(p_isolate, deflate));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "inflate"),
              v8::FunctionTemplate::New(p_isolate, inflate));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "deflateRaw"),
              v8::FunctionTemplate::New(p_isolate, deflateRaw));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "inflateRaw"),
              v8::FunctionTemplate::New(p_isolate, inflateRaw));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "gzip"),
              v8::FunctionTemplate::New(p_isolate, gzip));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "gunzip"),
              v8::FunctionTemplate::New(p_isolate, gunzip));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "unzip"),
              v8::FunctionTemplate::New(p_isolate, unzip));

    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "brotliCompressSync"),
              v8::FunctionTemplate::New(p_isolate, brotliCompressSync));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "brotliDecompressSync"),
              v8::FunctionTemplate::New(p_isolate, brotliDecompressSync));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "brotliCompress"),
              v8::FunctionTemplate::New(p_isolate, brotliCompress));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "brotliDecompress"),
              v8::FunctionTemplate::New(p_isolate, brotliDecompress));

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

    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "constants"), constants);

    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "crc32"),
              v8::FunctionTemplate::New(p_isolate, crc32));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "adler32"),
              v8::FunctionTemplate::New(p_isolate, adler32));

    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "createGzip"),
              v8::FunctionTemplate::New(p_isolate, createGzip));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "createGunzip"),
              v8::FunctionTemplate::New(p_isolate, createGunzip));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "createDeflate"),
              v8::FunctionTemplate::New(p_isolate, createDeflate));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "createInflate"),
              v8::FunctionTemplate::New(p_isolate, createInflate));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "createDeflateRaw"),
              v8::FunctionTemplate::New(p_isolate, createDeflateRaw));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "createInflateRaw"),
              v8::FunctionTemplate::New(p_isolate, createInflateRaw));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "createUnzip"),
              v8::FunctionTemplate::New(p_isolate, createUnzip));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "createBrotliCompress"),
              v8::FunctionTemplate::New(p_isolate, createBrotliCompress));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "createBrotliDecompress"),
              v8::FunctionTemplate::New(p_isolate, createBrotliDecompress));
    return tmpl;
}

static void doDeflate(const v8::FunctionCallbackInfo<v8::Value>& args, int32_t window_bits) {
    v8::Isolate* p_isolate = args.GetIsolate();
    uint8_t* p_data;
    size_t length;
    if (!getInput(args, &p_data, &length)) return;
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    if (deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, window_bits, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        p_isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8Literal(p_isolate, "deflateInit failed")));
        return;
    }

    strm.next_in = (Bytef*)p_data;
    strm.avail_in = (uInt)length;
    std::vector<uint8_t> out_buffer;
    const size_t chunk_size = 16384;
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

static void doInflate(const v8::FunctionCallbackInfo<v8::Value>& args, int32_t window_bits) {
    v8::Isolate* p_isolate = args.GetIsolate();
    uint8_t* p_data;
    size_t length;
    if (!getInput(args, &p_data, &length)) return;
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
    const size_t chunk_size = 16384;
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

static void doZlibAsync(const v8::FunctionCallbackInfo<v8::Value>& args, int32_t window_bits, bool is_deflate, bool is_brotli = false) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::HandleScope handle_scope(p_isolate);
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    uint8_t* p_data;
    size_t length;
    if (!getInput(args, &p_data, &length)) return;
    v8::Local<v8::Function> callback;
    if (args.Length() >= 2 && args[args.Length() - 1]->IsFunction()) {
        callback = args[args.Length() - 1].As<v8::Function>();
    } else {
        p_isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(p_isolate, "Callback must be provided")));
        return;
    }

    ZlibAsyncCtx* p_ctx = new ZlibAsyncCtx();
    p_ctx->m_input.assign(p_data, p_data + length);
    p_ctx->m_window_bits = window_bits;
    p_ctx->m_is_deflate = is_deflate;
    p_ctx->m_is_brotli = is_brotli;

    Task* p_task = new Task();
    p_task->m_is_promise = false;
    p_task->m_callback.Reset(p_isolate, callback);
    p_task->p_data = p_ctx;
    p_task->m_runner = [](v8::Isolate* p_isolate, v8::Local<v8::Context> context, Task* task) {
        ZlibAsyncCtx* p_ctx = static_cast<ZlibAsyncCtx*>(task->p_data);
        v8::Local<v8::Value> argv[2];
        if (p_ctx->m_is_error) {
            argv[0] = v8::Exception::Error(v8::String::NewFromUtf8(p_isolate, p_ctx->m_error_msg.c_str()).ToLocalChecked());
            argv[1] = v8::Null(p_isolate);
        } else {
            v8::Local<v8::ArrayBuffer> ab = v8::ArrayBuffer::New(p_isolate, p_ctx->m_output.size());
            memcpy(ab->GetBackingStore()->Data(), p_ctx->m_output.data(), p_ctx->m_output.size());
            argv[0] = v8::Null(p_isolate);
            argv[1] = v8::Uint8Array::New(ab, 0, p_ctx->m_output.size());
        }
        (void)task->m_callback.Get(p_isolate)->Call(context, context->Global(), 2, argv);
        delete p_ctx;
    };

    ThreadPool::getInstance().enqueue([p_task]() {
        ZlibAsyncCtx* p_ctx = static_cast<ZlibAsyncCtx*>(p_task->p_data);
        
        if (p_ctx->m_is_brotli) {
            if (p_ctx->m_is_deflate) { // Brotli Compress
                size_t out_len = BrotliEncoderMaxCompressedSize(p_ctx->m_input.size());
                p_ctx->m_output.resize(out_len);
                if (BrotliEncoderCompress(BROTLI_DEFAULT_QUALITY, BROTLI_DEFAULT_WINDOW, BROTLI_DEFAULT_MODE,
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
        } else { // Standard Zlib
            z_stream strm;
            strm.zalloc = Z_NULL;
            strm.zfree = Z_NULL;
            strm.opaque = Z_NULL;

            int32_t ret;
            if (p_ctx->m_is_deflate) {
                ret = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, p_ctx->m_window_bits, 8, Z_DEFAULT_STRATEGY);
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

            const size_t chunk_size = 16384;
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

                if (ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
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

void Zlib::deflate(const v8::FunctionCallbackInfo<v8::Value>& args) {
    doZlibAsync(args, 15, true);
}

void Zlib::inflate(const v8::FunctionCallbackInfo<v8::Value>& args) {
    doZlibAsync(args, 15, false);
}

void Zlib::deflateRaw(const v8::FunctionCallbackInfo<v8::Value>& args) {
    doZlibAsync(args, -15, true);
}

void Zlib::inflateRaw(const v8::FunctionCallbackInfo<v8::Value>& args) {
    doZlibAsync(args, -15, false);
}

void Zlib::gzip(const v8::FunctionCallbackInfo<v8::Value>& args) {
    doZlibAsync(args, 15 + 16, true);
}

void Zlib::gunzip(const v8::FunctionCallbackInfo<v8::Value>& args) {
    doZlibAsync(args, 15 + 16, false);
}

void Zlib::unzip(const v8::FunctionCallbackInfo<v8::Value>& args) {
    doZlibAsync(args, 15 + 32, false);
}

void Zlib::brotliCompressSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    uint8_t* p_data;
    size_t length;
    if (!getInput(args, &p_data, &length)) return;

    size_t out_len = BrotliEncoderMaxCompressedSize(length);
    std::vector<uint8_t> out_buffer(out_len);

    if (BrotliEncoderCompress(BROTLI_DEFAULT_QUALITY, BROTLI_DEFAULT_WINDOW, BROTLI_DEFAULT_MODE,
                              length, p_data, &out_len, out_buffer.data())) {
        out_buffer.resize(out_len);
        returnBuffer(args, out_buffer);
    } else {
        p_isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8Literal(p_isolate, "Brotli compression failed")));
    }
}

void Zlib::brotliDecompressSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    uint8_t* p_data;
    size_t length;
    if (!getInput(args, &p_data, &length)) return;

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

void Zlib::brotliCompress(const v8::FunctionCallbackInfo<v8::Value>& args) {
    doZlibAsync(args, 0, true, true);
}

void Zlib::brotliDecompress(const v8::FunctionCallbackInfo<v8::Value>& args) {
    doZlibAsync(args, 0, false, true);
}

void Zlib::crc32(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    uint8_t* p_data;
    size_t length;
    if (!getInput(args, &p_data, &length)) return;

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
    if (!getInput(args, &p_data, &length)) return;

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
    const size_t chunk_size = 16384;
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

static void createZlibStream(const v8::FunctionCallbackInfo<v8::Value>& args, int32_t window_bits, bool is_deflate) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();

    ZlibStreamObject* p_stream = new ZlibStreamObject();
    p_stream->m_is_deflate = is_deflate;
    
    int32_t ret;
    if (is_deflate) {
        ret = deflateInit2(&p_stream->m_strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, window_bits, 8, Z_DEFAULT_STRATEGY);
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

    args.GetReturnValue().Set(js_obj);
}

void Zlib::createGzip(const v8::FunctionCallbackInfo<v8::Value>& args) { createZlibStream(args, 15 + 16, true); }
void Zlib::createGunzip(const v8::FunctionCallbackInfo<v8::Value>& args) { createZlibStream(args, 15 + 16, false); }
void Zlib::createDeflate(const v8::FunctionCallbackInfo<v8::Value>& args) { createZlibStream(args, 15, true); }
void Zlib::createInflate(const v8::FunctionCallbackInfo<v8::Value>& args) { createZlibStream(args, 15, false); }
void Zlib::createDeflateRaw(const v8::FunctionCallbackInfo<v8::Value>& args) { createZlibStream(args, -15, true); }
void Zlib::createInflateRaw(const v8::FunctionCallbackInfo<v8::Value>& args) { createZlibStream(args, -15, false); }
void Zlib::createUnzip(const v8::FunctionCallbackInfo<v8::Value>& args) { createZlibStream(args, 15 + 32, false); }

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
};

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
    BrotliStreamObject* p_stream = new BrotliStreamObject(true);
    
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
    args.GetReturnValue().Set(js_obj);
}

void Zlib::createBrotliDecompress(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
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
    args.GetReturnValue().Set(js_obj);
}

} // namespace module
} // namespace z8
