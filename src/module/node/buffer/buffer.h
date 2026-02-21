#ifndef Z8_MODULE_BUFFER_H
#define Z8_MODULE_BUFFER_H

#include "v8.h"

namespace z8 {
namespace module {

class Buffer {
  public:
    static v8::Local<v8::FunctionTemplate> createTemplate(v8::Isolate* p_isolate);
    static void initialize(v8::Isolate* p_isolate, v8::Local<v8::Context> context);

    // Static methods
    static void alloc(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void allocUnsafe(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void from(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void concat(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void isBuffer(const v8::FunctionCallbackInfo<v8::Value>& args);

    // Instance methods
    static void toString(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void write(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void fill(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void copy(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void slice(const v8::FunctionCallbackInfo<v8::Value>& args);
    
    // Internal helpers
    static v8::Local<v8::Uint8Array> createBuffer(v8::Isolate* p_isolate, size_t length);
};

} // namespace module
} // namespace z8

#endif // Z8_MODULE_BUFFER_H
