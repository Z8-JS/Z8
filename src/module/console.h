#ifndef Z8_CONSOLE_H
#define Z8_CONSOLE_H

#include "v8.h"

namespace z8 {
namespace module {

class Console {
public:
    static v8::Local<v8::ObjectTemplate> CreateTemplate(v8::Isolate* isolate);

private:
    static void Log(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Error(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Warn(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Info(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Assert(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Count(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void CountReset(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Dir(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Group(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void GroupCollapsed(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void GroupEnd(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Time(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void TimeLog(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void TimeEnd(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Trace(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Clear(const v8::FunctionCallbackInfo<v8::Value>& args);
    
    static void Write(const v8::FunctionCallbackInfo<v8::Value>& args, const char* prefix, bool is_error = false);
    static void AdaptiveFlush(FILE* out);

private:
    static int indentation_level;
};

} // namespace module
} // namespace z8

#endif // Z8_CONSOLE_H
