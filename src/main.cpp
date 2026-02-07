/* Z8 (Zane V8)
 * A high-performance, competitive JavaScript engine.
 * Copyright (C) 2026 Zane V8 Authors
 * Copyright (C) 2026 Sao Tin Developer Team
 */

// Standard headers
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

// V8 headers
#include "v8.h"
#include <v8-isolate.h>

#include "config.h"
#include "libplatform/libplatform.h"

// Environment interface
#include "module/console.h"
#include "module/timer.h"

// Interface for the node.js module
#include "module/node/fs/fs.h"
#include "module/node/os/os.h"
#include "module/node/path/path.h"
#include "module/node/util/util.h"
#include "task_queue.h"
#include "thread_pool.h"

// Windows headers
#define NOMINMAX
#ifdef _WIN32
#include <windows.h>
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
#endif

// Z8 Namespace
namespace z8 {

class Runtime {
  public:
    static void Initialize(const char* exec_path) {
        // Clean high-performance V8 flags
        const char* flags = "--stack-size=2048 "
                            "--max-semi-space-size=128 "
                            "--no-optimize-for-size "
                            "--turbo-fast-api-calls";
        v8::V8::SetFlagsFromString(flags);

        v8::V8::InitializeICUDefaultLocation(exec_path);

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

        // Increase memory limits for competitive benchmarking
        // Deno uses ~128MB semi-space, which is ~256MB young generation
        create_params.constraints.set_max_old_generation_size_in_bytes(4096ULL * 1024 * 1024);
        create_params.constraints.set_max_young_generation_size_in_bytes(256ULL * 1024 * 1024);

        isolate_ = v8::Isolate::New(create_params);

        v8::Isolate::Scope isolate_scope(isolate_);
        v8::HandleScope handle_scope(isolate_);

        v8::Local<v8::ObjectTemplate> global_template = v8::ObjectTemplate::New(isolate_);
        v8::Local<v8::Context> context = v8::Context::New(isolate_, nullptr, global_template);
        context_.Reset(isolate_, context);

        // Force override the 'console' object because V8 might have a default empty one
        v8::Context::Scope context_scope(context);
        v8::Local<v8::Object> global = context->Global();
        v8::Local<v8::Object> console =
            z8::module::Console::createTemplate(isolate_)->NewInstance(context).ToLocalChecked();

        global->Set(context, v8::String::NewFromUtf8(isolate_, "console").ToLocalChecked(), console).Check();

        // Initialize Timer module
        z8::module::Timer::initialize(isolate_, context);
    }

    ~Runtime() {
        context_.Reset();
        isolate_->Dispose();
    }

    static v8::MaybeLocal<v8::Module> ResolveModuleCallback(v8::Local<v8::Context> context,
                                                            v8::Local<v8::String> specifier,
                                                            v8::Local<v8::FixedArray> import_assertions,
                                                            v8::Local<v8::Module> referrer) {
        v8::Isolate* isolate = v8::Isolate::GetCurrent();
        v8::String::Utf8Value specifier_utf8(isolate, specifier);
        std::string specifier_str(*specifier_utf8);

        if (specifier_str == "node:fs") {
            v8::Local<v8::ObjectTemplate> fs_template = z8::module::FS::createTemplate(isolate);

            // To get property names, we must create an instance first.
            v8::Local<v8::Object> fs_instance;
            if (!fs_template->NewInstance(context).ToLocal(&fs_instance)) {
                return v8::MaybeLocal<v8::Module>();
            }

            v8::Local<v8::Array> prop_names;
            if (!fs_instance->GetPropertyNames(context).ToLocal(&prop_names)) {
                return v8::MaybeLocal<v8::Module>();
            }

            std::vector<v8::Local<v8::String>> export_names;
            export_names.push_back(v8::String::NewFromUtf8Literal(isolate, "default"));

            for (uint32_t i = 0; i < prop_names->Length(); ++i) {
                v8::Local<v8::Value> name_val = prop_names->Get(context, i).ToLocalChecked();
                export_names.push_back(name_val.As<v8::String>());
            }

            auto module = v8::Module::CreateSyntheticModule(
                isolate,
                v8::String::NewFromUtf8Literal(isolate, "node:fs"),
                v8::MemorySpan<const v8::Local<v8::String>>(export_names.data(), export_names.size()),
                [](v8::Local<v8::Context> context, v8::Local<v8::Module> module) -> v8::MaybeLocal<v8::Value> {
                    v8::Isolate* isolate = v8::Isolate::GetCurrent();
                    v8::Local<v8::ObjectTemplate> fs_template = z8::module::FS::createTemplate(isolate);
                    v8::Local<v8::Object> fs_obj = fs_template->NewInstance(context).ToLocalChecked();
                    module
                        ->SetSyntheticModuleExport(isolate, v8::String::NewFromUtf8Literal(isolate, "default"), fs_obj)
                        .Check();
                    v8::Local<v8::Array> prop_names;
                    if (fs_obj->GetPropertyNames(context).ToLocal(&prop_names)) {
                        for (uint32_t i = 0; i < prop_names->Length(); ++i) {
                            v8::Local<v8::Value> name_val = prop_names->Get(context, i).ToLocalChecked();
                            v8::Local<v8::Value> prop_val = fs_obj->Get(context, name_val).ToLocalChecked();
                            module->SetSyntheticModuleExport(isolate, name_val.As<v8::String>(), prop_val).Check();
                        }
                    }
                    return v8::Undefined(isolate);
                });
            return module;
        }

        if (specifier_str == "node:path") {
            v8::Local<v8::ObjectTemplate> path_template = z8::module::Path::createTemplate(isolate);
            v8::Local<v8::Object> path_instance = path_template->NewInstance(context).ToLocalChecked();
            v8::Local<v8::Array> prop_names = path_instance->GetPropertyNames(context).ToLocalChecked();

            std::vector<v8::Local<v8::String>> export_names;
            export_names.push_back(v8::String::NewFromUtf8Literal(isolate, "default"));
            for (uint32_t i = 0; i < prop_names->Length(); ++i) {
                export_names.push_back(prop_names->Get(context, i).ToLocalChecked().As<v8::String>());
            }

            auto module = v8::Module::CreateSyntheticModule(
                isolate,
                v8::String::NewFromUtf8Literal(isolate, "node:path"),
                v8::MemorySpan<const v8::Local<v8::String>>(export_names.data(), export_names.size()),
                [](v8::Local<v8::Context> context, v8::Local<v8::Module> module) -> v8::MaybeLocal<v8::Value> {
                    v8::Isolate* isolate = v8::Isolate::GetCurrent();
                    v8::Local<v8::ObjectTemplate> path_template = z8::module::Path::createTemplate(isolate);
                    v8::Local<v8::Object> path_obj = path_template->NewInstance(context).ToLocalChecked();
                    module
                        ->SetSyntheticModuleExport(
                            isolate, v8::String::NewFromUtf8Literal(isolate, "default"), path_obj)
                        .Check();
                    v8::Local<v8::Array> prop_names = path_obj->GetPropertyNames(context).ToLocalChecked();
                    for (uint32_t i = 0; i < prop_names->Length(); ++i) {
                        v8::Local<v8::String> name = prop_names->Get(context, i).ToLocalChecked().As<v8::String>();
                        module->SetSyntheticModuleExport(isolate, name, path_obj->Get(context, name).ToLocalChecked())
                            .Check();
                    }
                    return v8::Undefined(isolate);
                });
            return module;
        }

        if (specifier_str == "node:os") {
            v8::Local<v8::ObjectTemplate> os_template = z8::module::OS::createTemplate(isolate);
            v8::Local<v8::Object> os_instance = os_template->NewInstance(context).ToLocalChecked();
            v8::Local<v8::Array> prop_names = os_instance->GetPropertyNames(context).ToLocalChecked();

            std::vector<v8::Local<v8::String>> export_names;
            export_names.push_back(v8::String::NewFromUtf8Literal(isolate, "default"));
            for (uint32_t i = 0; i < prop_names->Length(); ++i) {
                export_names.push_back(prop_names->Get(context, i).ToLocalChecked().As<v8::String>());
            }

            auto module = v8::Module::CreateSyntheticModule(
                isolate,
                v8::String::NewFromUtf8Literal(isolate, "node:os"),
                v8::MemorySpan<const v8::Local<v8::String>>(export_names.data(), export_names.size()),
                [](v8::Local<v8::Context> context, v8::Local<v8::Module> module) -> v8::MaybeLocal<v8::Value> {
                    v8::Isolate* isolate = v8::Isolate::GetCurrent();
                    v8::Local<v8::ObjectTemplate> os_template = z8::module::OS::createTemplate(isolate);
                    v8::Local<v8::Object> os_obj = os_template->NewInstance(context).ToLocalChecked();
                    module
                        ->SetSyntheticModuleExport(isolate, v8::String::NewFromUtf8Literal(isolate, "default"), os_obj)
                        .Check();
                    v8::Local<v8::Array> prop_names = os_obj->GetPropertyNames(context).ToLocalChecked();
                    for (uint32_t i = 0; i < prop_names->Length(); ++i) {
                        v8::Local<v8::String> name = prop_names->Get(context, i).ToLocalChecked().As<v8::String>();
                        module->SetSyntheticModuleExport(isolate, name, os_obj->Get(context, name).ToLocalChecked())
                            .Check();
                    }
                    return v8::Undefined(isolate);
                });
            return module;
        }

        if (specifier_str == "node:fs/promises") {
            v8::Local<v8::ObjectTemplate> fs_template = z8::module::FS::createPromisesTemplate(isolate);
            v8::Local<v8::Object> fs_instance = fs_template->NewInstance(context).ToLocalChecked();
            v8::Local<v8::Array> prop_names = fs_instance->GetPropertyNames(context).ToLocalChecked();

            std::vector<v8::Local<v8::String>> export_names;
            for (uint32_t i = 0; i < prop_names->Length(); ++i) {
                export_names.push_back(prop_names->Get(context, i).ToLocalChecked().As<v8::String>());
            }

            auto module = v8::Module::CreateSyntheticModule(
                isolate,
                v8::String::NewFromUtf8Literal(isolate, "node:fs/promises"),
                v8::MemorySpan<const v8::Local<v8::String>>(export_names.data(), export_names.size()),
                [](v8::Local<v8::Context> context, v8::Local<v8::Module> module) -> v8::MaybeLocal<v8::Value> {
                    v8::Isolate* isolate = v8::Isolate::GetCurrent();
                    v8::Local<v8::ObjectTemplate> fs_template = z8::module::FS::createPromisesTemplate(isolate);
                    v8::Local<v8::Object> fs_obj = fs_template->NewInstance(context).ToLocalChecked();
                    v8::Local<v8::Array> prop_names = fs_obj->GetPropertyNames(context).ToLocalChecked();
                    for (uint32_t i = 0; i < prop_names->Length(); ++i) {
                        v8::Local<v8::String> name = prop_names->Get(context, i).ToLocalChecked().As<v8::String>();
                        module->SetSyntheticModuleExport(isolate, name, fs_obj->Get(context, name).ToLocalChecked())
                            .Check();
                    }
                    return v8::Undefined(isolate);
                });
            return module;
        }

        if (specifier_str == "node:util") {
            v8::Local<v8::ObjectTemplate> util_template = z8::module::Util::createTemplate(isolate);
            v8::Local<v8::Object> util_instance = util_template->NewInstance(context).ToLocalChecked();
            v8::Local<v8::Array> prop_names = util_instance->GetPropertyNames(context).ToLocalChecked();

            std::vector<v8::Local<v8::String>> export_names;
            export_names.push_back(v8::String::NewFromUtf8Literal(isolate, "default"));
            for (uint32_t i = 0; i < prop_names->Length(); ++i) {
                export_names.push_back(prop_names->Get(context, i).ToLocalChecked().As<v8::String>());
            }

            auto module = v8::Module::CreateSyntheticModule(
                isolate,
                v8::String::NewFromUtf8Literal(isolate, "node:util"),
                v8::MemorySpan<const v8::Local<v8::String>>(export_names.data(), export_names.size()),
                [](v8::Local<v8::Context> context, v8::Local<v8::Module> module) -> v8::MaybeLocal<v8::Value> {
                    v8::Isolate* isolate = v8::Isolate::GetCurrent();
                    v8::Local<v8::ObjectTemplate> util_template = z8::module::Util::createTemplate(isolate);
                    v8::Local<v8::Object> util_obj = util_template->NewInstance(context).ToLocalChecked();
                    module
                        ->SetSyntheticModuleExport(
                            isolate, v8::String::NewFromUtf8Literal(isolate, "default"), util_obj)
                        .Check();
                    v8::Local<v8::Array> prop_names = util_obj->GetPropertyNames(context).ToLocalChecked();
                    for (uint32_t i = 0; i < prop_names->Length(); ++i) {
                        v8::Local<v8::String> name = prop_names->Get(context, i).ToLocalChecked().As<v8::String>();
                        module->SetSyntheticModuleExport(isolate, name, util_obj->Get(context, name).ToLocalChecked())
                            .Check();
                    }
                    return v8::Undefined(isolate);
                });
            return module;
        }

        // Handle relative imports (very basic for now)
        // In a real implementation, we'd read the file and compile it as a module
        isolate->ThrowException(
            v8::String::NewFromUtf8(isolate, ("Module not found: " + specifier_str).c_str()).ToLocalChecked());
        return v8::MaybeLocal<v8::Module>();
    }

    bool Run(const std::string& source, const std::string& filename) {
        v8::Isolate* isolate = isolate_;
        v8::Isolate::Scope isolate_scope(isolate);
        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Context> context = context_.Get(isolate);
        v8::Context::Scope context_scope(context);

        v8::TryCatch try_catch(isolate);

        v8::Local<v8::String> v8_source = v8::String::NewFromUtf8(isolate, source.c_str()).ToLocalChecked();
        v8::Local<v8::String> v8_filename = v8::String::NewFromUtf8(isolate, filename.c_str()).ToLocalChecked();

        // Modules are faster as V8 applies more aggressive optimizations to them
        v8::ScriptOrigin origin(v8_filename, 0, 0, false, -1, v8::Local<v8::Value>(), false, false, true);
        v8::ScriptCompiler::Source script_source(v8_source, origin);
        v8::Local<v8::Module> module;

        if (!v8::ScriptCompiler::CompileModule(isolate, &script_source).ToLocal(&module)) {
            ReportException(isolate, &try_catch);
            return false;
        }

        if (!module->InstantiateModule(context, ResolveModuleCallback).FromMaybe(false)) {
            ReportException(isolate, &try_catch);
            return false;
        }

        v8::MaybeLocal<v8::Value> result = module->Evaluate(context);

        if (try_catch.HasCaught()) {
            ReportException(isolate, &try_catch);
            return false;
        }

        // Only check Promise if we actually got a result and it looks like a Promise
        v8::Local<v8::Value> result_val;
        if (result.ToLocal(&result_val) && result_val->IsPromise()) {
            v8::Local<v8::Promise> promise = result_val.As<v8::Promise>();
            if (promise->State() == v8::Promise::kRejected) {
                isolate->ThrowException(promise->Result());
                ReportException(isolate, &try_catch);
                return false;
            }
        }

        // Event Loop
        bool keep_running = true;
        while (keep_running) {
            // 1. Process Tasks from Thread Pool
            while (!z8::TaskQueue::getInstance().isEmpty()) {
                z8::Task* task = z8::TaskQueue::getInstance().dequeue();
                if (task) {
                    v8::TryCatch task_try_catch(isolate);
                    task->runner(isolate, context, task);
                    delete task;

                    // Resume JS execution
                    isolate->PerformMicrotaskCheckpoint();

                    if (task_try_catch.HasCaught()) {
                        ReportException(isolate, &task_try_catch);
                        return false;
                    }
                }
            }

            // 2. Process Timers
            if (z8::module::Timer::hasActiveTimers()) {
                v8::TryCatch loop_try_catch(isolate);
                z8::module::Timer::tick(isolate, context);
                isolate->PerformMicrotaskCheckpoint();
                if (loop_try_catch.HasCaught()) {
                    ReportException(isolate, &loop_try_catch);
                    return false;
                }
            }

            // 3. Final termination check
            if (!z8::module::Timer::hasActiveTimers() && z8::TaskQueue::getInstance().isEmpty() &&
                !z8::ThreadPool::getInstance().hasPendingTasks()) {
                // One last check for microtasks that might have been queued
                isolate->PerformMicrotaskCheckpoint();

                if (z8::TaskQueue::getInstance().isEmpty() && !z8::ThreadPool::getInstance().hasPendingTasks()) {
                    keep_running = false;
                }
            }

            // 4. Wait for work (Instant Wakeup)
            if (keep_running && z8::TaskQueue::getInstance().isEmpty()) {
                std::chrono::milliseconds delay = z8::module::Timer::getNextDelay();
                std::chrono::milliseconds timeout(50); // Balanced polling
                if (delay.count() >= 0) {
                    timeout = std::chrono::milliseconds(std::min((long long) delay.count(), 50LL));
                }
                z8::TaskQueue::getInstance().wait(timeout);
            }
        }

        return true;
    }

    void RunREPL() {
        v8::Isolate* isolate = isolate_;
        v8::Isolate::Scope isolate_scope(isolate);
        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Context> context = context_.Get(isolate);
        v8::Context::Scope context_scope(context);

        std::cout << "Welcome to Zane V8 (Z8) v" << Z8_VERSION << std::endl;
        std::cout << "Type 'exit' or '.exit' to quit." << std::endl;

        std::string line;
        while (true) {
            std::cout << "> " << std::flush;
            if (!std::getline(std::cin, line))
                break;
            if (line == "exit" || line == ".exit")
                break;
            if (line.empty())
                continue;

            v8::TryCatch try_catch(isolate);
            v8::Local<v8::String> source = v8::String::NewFromUtf8(isolate, line.c_str()).ToLocalChecked();
            v8::Local<v8::Script> script;
            if (!v8::Script::Compile(context, source).ToLocal(&script)) {
                ReportException(isolate, &try_catch);
                continue;
            }

            v8::Local<v8::Value> result;
            if (!script->Run(context).ToLocal(&result)) {
                ReportException(isolate, &try_catch);
                continue;
            }

            if (!result->IsUndefined()) {
                v8::String::Utf8Value str(isolate, result);
                std::cout << *str << std::endl;
            }
        }
    }

  private:
    void ReportException(v8::Isolate* isolate, v8::TryCatch* try_catch) {
        fflush(stdout); // Rescue any buffered stdout before reporting error
        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Context> context = isolate->GetCurrentContext();
        v8::Local<v8::Message> message = try_catch->Message();

        if (message.IsEmpty()) {
            // V8 didn't provide a message, just print the exception
            v8::Local<v8::Value> exception = try_catch->Exception();
            if (!exception.IsEmpty()) {
                v8::String::Utf8Value exception_str(isolate, exception);
                std::cerr << "Uncaught Exception: " << (*exception_str ? *exception_str : "unknown") << std::endl;
            } else {
                std::cerr << "Uncaught Exception (empty exception object)" << std::endl;
            }
        } else {
            v8::String::Utf8Value exception(isolate, try_catch->Exception());
            v8::Local<v8::Value> resource_name = message->GetScriptResourceName();
            v8::String::Utf8Value filename(isolate, resource_name);

            int32_t linenum = message->GetLineNumber(context).FromMaybe(-1);
            const char* filename_str = *filename ? *filename : "unknown";
            const char* exception_str = *exception ? *exception : "unknown";

            std::cerr << filename_str << ":" << linenum << ": " << exception_str << std::endl;

            v8::MaybeLocal<v8::String> sourceline_maybe = message->GetSourceLine(context);
            if (!sourceline_maybe.IsEmpty()) {
                v8::String::Utf8Value sourceline(isolate, sourceline_maybe.ToLocalChecked());
                std::cerr << *sourceline << std::endl;

                int32_t start = message->GetStartColumn(context).FromMaybe(0);
                for (int32_t i = 0; i < start; i++)
                    std::cerr << " ";
                int32_t end = message->GetEndColumn(context).FromMaybe(0);
                for (int32_t i = start; i < end; i++)
                    std::cerr << "^";
                std::cerr << std::endl;
            }
        }

        v8::Local<v8::Value> stack_trace;
        if (try_catch->StackTrace(context).ToLocal(&stack_trace) && stack_trace->IsString() &&
            v8::Local<v8::String>::Cast(stack_trace)->Length() > 0) {
            v8::String::Utf8Value stack_trace_str(isolate, stack_trace);
            std::cerr << *stack_trace_str << std::endl;
        }

        fflush(stderr);
    }

  public:
    v8::Isolate* isolate_;
    v8::Global<v8::Context> context_;
};

} // namespace z8

#include <filesystem>

namespace fs = std::filesystem;

// Validate and sanitize file path to prevent path traversal attacks
bool ValidatePath(const std::string& path, std::string& sanitized_path) {
    try {
        // Resolve to canonical absolute path
        fs::path canonical = fs::canonical(path);

        // Get current working directory
        fs::path cwd = fs::current_path();

        // Convert to strings for comparison
        std::string canonical_str = canonical.string();
        std::string cwd_str = cwd.string();

        // Check for common path traversal patterns in the original path
        if (path.find("..") != std::string::npos) {
            // Only allow if the canonical path is still within or below CWD
            // This allows legitimate use of .. that doesn't escape
            if (canonical_str.find(cwd_str) != 0) {
                return false;
            }
        }

        sanitized_path = canonical_str;
        return true;
    } catch (const fs::filesystem_error&) {
        // Path doesn't exist or other filesystem error
        return false;
    }
}

// Helper to read file
std::string ReadFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        return "";

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return content;
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    if (argc < 2) {
        z8::Runtime::Initialize(argv[0]);
        {
            z8::Runtime rt;
            rt.RunREPL();
        }
        z8::Runtime::Shutdown();
        return 0;
    }

    std::string filename;
    std::string source;

    if (std::string(argv[1]) == "-e" && argc > 2) {
        filename = "eval";
        source = argv[2];
    } else {
        std::string raw_filename = argv[1];

        // Validate and sanitize the path to prevent path traversal attacks
        if (!ValidatePath(raw_filename, filename)) {
            std::cerr << "✖ Error: Invalid or inaccessible file path: " << raw_filename << std::endl;
            return 1;
        }

        if (!fs::exists(filename)) {
            std::cerr << "✖ Error: File not found: " << filename << std::endl;
            return 1;
        }

        source = ReadFile(filename);
        if (source.empty()) {
            // Check if file is actually empty or read failed
            if (fs::file_size(filename) > 0) {
                std::cerr << "✖ Error: Could not read file: " << filename << std::endl;
                return 1;
            }
        }
    }

    z8::Runtime::Initialize(argv[0]);
    bool success = false;
    {
        z8::Runtime rt;
        success = rt.Run(source, filename);
    }

    z8::Runtime::Shutdown();
    return success ? 0 : 1;
}
