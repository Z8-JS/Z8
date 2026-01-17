#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>

#include "v8.h"
#include "libplatform/libplatform.h"

// Linker pragmas for Windows
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "rpcrt4.lib")
#pragma comment(lib, "ntdll.lib")

// Z8 Namespace
namespace z8 {

class Runtime {
public:
    static void Initialize(const char* exec_path) {
        v8::V8::InitializeICUDefaultLocation(exec_path);
        v8::V8::InitializeExternalStartupData(exec_path);
        
        static std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
        v8::V8::InitializePlatform(platform.get());
        v8::V8::Initialize();
    }

    static void Shutdown() {
        v8::V8::Dispose();
        v8::V8::DisposePlatform();
    }

    Runtime() {
        v8::Isolate::CreateParams create_params;
        create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
        isolate_ = v8::Isolate::New(create_params);
        
        v8::Isolate::Scope isolate_scope(isolate_);
        v8::HandleScope handle_scope(isolate_);

        auto global = v8::ObjectTemplate::New(isolate_);
        
        // Let's add the 'console' object
        auto console = v8::ObjectTemplate::New(isolate_);
        console->Set(isolate_, "log", v8::FunctionTemplate::New(isolate_, ConsoleLog));
        global->Set(isolate_, "console", console);

        auto context = v8::Context::New(isolate_, nullptr, global);
        context_.Reset(isolate_, context);
    }

    ~Runtime() {
        context_.Reset();
        isolate_->Dispose();
    }

    void Run(const std::string& source, const std::string& filename) {
        v8::Isolate::Scope isolate_scope(isolate_);
        v8::HandleScope handle_scope(isolate_);
        v8::Local<v8::Context> context = context_.Get(isolate_);
        v8::Context::Scope context_scope(context);

        v8::Local<v8::String> v8_source = v8::String::NewFromUtf8(isolate_, source.c_str()).ToLocalChecked();
        v8::Local<v8::String> v8_filename = v8::String::NewFromUtf8(isolate_, filename.c_str()).ToLocalChecked();
        
        v8::ScriptOrigin origin(isolate_, v8_filename);
        v8::Local<v8::Script> script;
        if (v8::Script::Compile(context, v8_source, &origin).ToLocal(&script)) {
            v8::MaybeLocal<v8::Value> result = script->Run(context);
        }
    }

private:
    static void ConsoleLog(const v8::FunctionCallbackInfo<v8::Value>& args) {
        v8::Isolate* isolate = args.GetIsolate();
        v8::HandleScope handle_scope(isolate);
        
        for (int i = 0; i < args.Length(); i++) {
            v8::String::Utf8Value utf8(isolate, args[i]);
            std::cout << *utf8 << (i < args.Length() - 1 ? " " : "");
        }
        std::cout << std::endl;
    }

    v8::Isolate* isolate_;
    v8::Global<v8::Context> context_;
};

} // namespace z8

int main(int argc, char* argv[]) {
    std::cout << "≡ƒÜÇ Zane V8 (Z8) starting..." << std::endl;

    z8::Runtime::Initialize(argv[0]);

    {
        z8::Runtime rt;
        std::cout << "≡ƒô£ Running hello.js..." << std::endl;
        rt.Run("console.log('Hello from Z8!', 'Status:', 'Pure C++ Perfection');", "hello.js");
    }

    z8::Runtime::Shutdown();
    std::cout << "Γœà Z8 finished execution." << std::endl;

    return 0;
}
