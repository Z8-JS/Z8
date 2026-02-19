#ifndef Z8_MODULE_PROCESS_H
#define Z8_MODULE_PROCESS_H

#include "v8.h"
#include <map>
#include <string>

namespace z8 {
namespace module {

class Process {
  public:
    static v8::Local<v8::ObjectTemplate> createTemplate(v8::Isolate* p_isolate);

    // Initializer to be called when creating the global 'process' object
    static v8::Local<v8::Object> createObject(v8::Isolate* p_isolate, v8::Local<v8::Context> context);

    // Setup global state (call once)
    static void setArgv(int32_t argc, char* argv[]);

  private:
    static void cwd(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void chdir(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void exit(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void uptime(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void nextTick(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void memoryUsage(const v8::FunctionCallbackInfo<v8::Value>& args);

    // Stream write helpers
    static void stdoutWrite(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void stderrWrite(const v8::FunctionCallbackInfo<v8::Value>& args);
    
    // Internal helper to load environment variables and .env file
    static v8::Local<v8::Object> createEnvObject(v8::Isolate* p_isolate, v8::Local<v8::Context> context);
    static std::map<std::string, std::string> loadDotEnv();

    static std::vector<std::string> m_argv;
};

} // namespace module
} // namespace z8

#endif // Z8_MODULE_PROCESS_H
