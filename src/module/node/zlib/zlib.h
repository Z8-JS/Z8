#ifndef Z8_MODULE_ZLIB_H
#define Z8_MODULE_ZLIB_H

#include "v8.h"

namespace z8 {
namespace module {

class Zlib {
  public:
    static v8::Local<v8::ObjectTemplate> createTemplate(v8::Isolate* p_isolate);

    static void deflateSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void inflateSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void deflateRawSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void inflateRawSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void gzipSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void gunzipSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void unzipSync(const v8::FunctionCallbackInfo<v8::Value>& args);

    static void deflate(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void inflate(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void deflateRaw(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void inflateRaw(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void gzip(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void gunzip(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void unzip(const v8::FunctionCallbackInfo<v8::Value>& args);

    static void brotliCompressSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void brotliDecompressSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void brotliCompress(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void brotliDecompress(const v8::FunctionCallbackInfo<v8::Value>& args);

    static void crc32(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void adler32(const v8::FunctionCallbackInfo<v8::Value>& args);

    static void createGzip(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void createGunzip(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void createDeflate(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void createInflate(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void createDeflateRaw(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void createInflateRaw(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void createUnzip(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void createBrotliCompress(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void createBrotliDecompress(const v8::FunctionCallbackInfo<v8::Value>& args);
};

} // namespace module
} // namespace z8

#endif // Z8_MODULE_ZLIB_H
