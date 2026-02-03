#ifndef Z8_MODULE_UTIL_H
#define Z8_MODULE_UTIL_H

#include "v8.h"

namespace z8 {
namespace module {

class Util {
  public:
    static v8::Local<v8::ObjectTemplate> createTemplate(v8::Isolate* p_isolate);

    static void format(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void promisify(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void callbackify(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void inherits(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void inspect(const v8::FunctionCallbackInfo<v8::Value>& args);

    // util.types
    static v8::Local<v8::ObjectTemplate> createTypesTemplate(v8::Isolate* p_isolate);
    static void isDate(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void isRegExp(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void isPromise(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void isTypedArray(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void isUint8Array(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void isMap(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void isSet(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void isNativeError(const v8::FunctionCallbackInfo<v8::Value>& args);
};

} // namespace module
} // namespace z8

#endif // Z8_MODULE_UTIL_H
