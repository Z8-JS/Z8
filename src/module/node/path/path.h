#ifndef Z8_MODULE_PATH_H
#define Z8_MODULE_PATH_H

#include "v8.h"

namespace z8 {
namespace module {

class Path {
  public:
    static v8::Local<v8::ObjectTemplate> createTemplate(v8::Isolate* p_isolate);

    static void resolve(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void join(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void normalize(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void isAbsolute(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void relative(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void dirname(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void basename(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void extname(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void parse(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void format(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void toNamespacedPath(const v8::FunctionCallbackInfo<v8::Value>& args);
};

} // namespace module
} // namespace z8

#endif // Z8_MODULE_PATH_H
