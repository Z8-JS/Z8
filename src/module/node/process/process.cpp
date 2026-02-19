#include "process.h"
#include "config.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace z8 {
namespace module {

namespace fs = std::filesystem;

static auto start_time = std::chrono::steady_clock::now();
std::vector<std::string> Process::m_argv;

void Process::setArgv(int32_t argc, char* argv[]) {
    m_argv.clear();
    for (int32_t i = 0; i < argc; ++i) {
        m_argv.push_back(argv[i]);
    }
}

v8::Local<v8::ObjectTemplate> Process::createTemplate(v8::Isolate* p_isolate) {
    v8::Local<v8::ObjectTemplate> tmpl = v8::ObjectTemplate::New(p_isolate);

    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "cwd"), v8::FunctionTemplate::New(p_isolate, cwd));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "chdir"), v8::FunctionTemplate::New(p_isolate, chdir));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "exit"), v8::FunctionTemplate::New(p_isolate, exit));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "uptime"), v8::FunctionTemplate::New(p_isolate, uptime));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "nextTick"), v8::FunctionTemplate::New(p_isolate, nextTick));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "memoryUsage"), v8::FunctionTemplate::New(p_isolate, memoryUsage));

    return tmpl;
}

v8::Local<v8::Object> Process::createObject(v8::Isolate* p_isolate, v8::Local<v8::Context> context) {
    v8::Local<v8::ObjectTemplate> tmpl = createTemplate(p_isolate);
    v8::Local<v8::Object> obj = tmpl->NewInstance(context).ToLocalChecked();

    // Set process.env
    obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "env"), createEnvObject(p_isolate, context)).Check();

    // Set process.argv
    v8::Local<v8::Array> argv_arr = v8::Array::New(p_isolate, (int) m_argv.size());
    for (size_t i = 0; i < m_argv.size(); ++i) {
        argv_arr->Set(context, (uint32_t) i, v8::String::NewFromUtf8(p_isolate, m_argv[i].c_str()).ToLocalChecked())
            .Check();
    }
    obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "argv"), argv_arr).Check();

    // Set process.pid
#ifdef _WIN32
    uint32_t pid = GetCurrentProcessId();
#else
    uint32_t pid = getpid();
#endif
    obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "pid"), v8::Number::New(p_isolate, pid)).Check();

    // Set process.stdout
    v8::Local<v8::Object> stdout_obj = v8::Object::New(p_isolate);
    stdout_obj
        ->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "write"), v8::FunctionTemplate::New(p_isolate, stdoutWrite)->GetFunction(context).ToLocalChecked())
        .Check();
    obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "stdout"), stdout_obj).Check();

    // Set process.stderr
    v8::Local<v8::Object> stderr_obj = v8::Object::New(p_isolate);
    stderr_obj
        ->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "write"), v8::FunctionTemplate::New(p_isolate, stderrWrite)->GetFunction(context).ToLocalChecked())
        .Check();
    obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "stderr"), stderr_obj).Check();

    // Set process.platform
#ifdef _WIN32
    obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "platform"), v8::String::NewFromUtf8Literal(p_isolate, "win32"))
        .Check();
#elif __APPLE__
    obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "platform"), v8::String::NewFromUtf8Literal(p_isolate, "darwin"))
        .Check();
#else
    obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "platform"), v8::String::NewFromUtf8Literal(p_isolate, "linux"))
        .Check();
#endif

    // Set process.version
    obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "version"), v8::String::NewFromUtf8Literal(p_isolate, "v" Z8_APP_VERSION))
        .Check();

    // Set process.versions
    v8::Local<v8::Object> versions_obj = v8::Object::New(p_isolate);
    versions_obj
        ->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "z8"), v8::String::NewFromUtf8Literal(p_isolate, Z8_APP_VERSION))
        .Check();
    versions_obj
        ->Set(context,
              v8::String::NewFromUtf8Literal(p_isolate, "v8"),
              v8::String::NewFromUtf8(p_isolate, v8::V8::GetVersion()).ToLocalChecked())
        .Check();
    obj->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "versions"), versions_obj).Check();

    return obj;
}

void Process::cwd(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    try {
        std::string path = fs::current_path().string();
        args.GetReturnValue().Set(v8::String::NewFromUtf8(p_isolate, path.c_str()).ToLocalChecked());
    } catch (...) {
        args.GetReturnValue().Set(v8::String::NewFromUtf8Literal(p_isolate, "."));
    }
}

void Process::chdir(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    if (args.Length() < 1 || !args[0]->IsString()) {
        p_isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8Literal(p_isolate, "Directory must be a string")));
        return;
    }
    v8::String::Utf8Value path(p_isolate, args[0]);
    try {
        fs::current_path(*path);
    } catch (const std::exception& e) {
        p_isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(p_isolate, e.what()).ToLocalChecked()));
    }
}

void Process::exit(const v8::FunctionCallbackInfo<v8::Value>& args) {
    int32_t code = 0;
    if (args.Length() > 0 && args[0]->IsInt32()) {
        code = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
    }
    std::exit(code);
}

void Process::uptime(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto now = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);
    args.GetReturnValue().Set(v8::Number::New(args.GetIsolate(), (double) diff.count()));
}

void Process::nextTick(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    if (args.Length() > 0 && args[0]->IsFunction()) {
        p_isolate->EnqueueMicrotask(args[0].As<v8::Function>());
    }
}

void Process::memoryUsage(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();

    v8::HeapStatistics heap_stats;
    p_isolate->GetHeapStatistics(&heap_stats);

    v8::Local<v8::Object> res = v8::Object::New(p_isolate);
    res->Set(context,
             v8::String::NewFromUtf8Literal(p_isolate, "rss"),
             v8::BigInt::NewFromUnsigned(p_isolate, heap_stats.heap_size_limit())) // Rough estimate for RSS
        .Check();
    res->Set(context,
             v8::String::NewFromUtf8Literal(p_isolate, "heapTotal"),
             v8::BigInt::NewFromUnsigned(p_isolate, heap_stats.total_heap_size()))
        .Check();
    res->Set(context,
             v8::String::NewFromUtf8Literal(p_isolate, "heapUsed"),
             v8::BigInt::NewFromUnsigned(p_isolate, heap_stats.used_heap_size()))
        .Check();
    res->Set(context,
             v8::String::NewFromUtf8Literal(p_isolate, "external"),
             v8::BigInt::NewFromUnsigned(p_isolate, heap_stats.external_memory()))
        .Check();

    args.GetReturnValue().Set(res);
}

void Process::stdoutWrite(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    if (args.Length() > 0) {
        v8::String::Utf8Value str(p_isolate, args[0]);
        if (*str) {
            fwrite(*str, 1, str.length(), stdout);
        }
    }
    args.GetReturnValue().Set(v8::Boolean::New(p_isolate, true));
}

void Process::stderrWrite(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    if (args.Length() > 0) {
        v8::String::Utf8Value str(p_isolate, args[0]);
        if (*str) {
            fwrite(*str, 1, str.length(), stderr);
        }
    }
    args.GetReturnValue().Set(v8::Boolean::New(p_isolate, true));
}

v8::Local<v8::Object> Process::createEnvObject(v8::Isolate* p_isolate, v8::Local<v8::Context> context) {
    v8::Local<v8::Object> env_obj = v8::Object::New(p_isolate);

    // 1. Load system environment variables
#ifdef _WIN32
    auto set_env_from_system = [&](const char* name) {
        char* buf = nullptr;
        size_t sz = 0;
        if (_dupenv_s(&buf, &sz, name) == 0 && buf != nullptr) {
            env_obj
                ->Set(context,
                      v8::String::NewFromUtf8(p_isolate, name).ToLocalChecked(),
                      v8::String::NewFromUtf8(p_isolate, buf).ToLocalChecked())
                .Check();
            free(buf);
        }
    };

    // Common Windows env vars
    const char* common_vars[] = {"PATH", "USERPROFILE", "HOMEDRIVE", "HOMEPATH", "TEMP", "TMP", "USERNAME", "COMPUTERNAME"};
    for (const char* var : common_vars) {
        set_env_from_system(var);
    }
#else
    extern char** environ;
    for (char** env = environ; *env != nullptr; ++env) {
        std::string s(*env);
        size_t pos = s.find('=');
        if (pos != std::string::npos) {
            std::string key = s.substr(0, pos);
            std::string val = s.substr(pos + 1);
            env_obj
                ->Set(context,
                      v8::String::NewFromUtf8(p_isolate, key.c_str()).ToLocalChecked(),
                      v8::String::NewFromUtf8(p_isolate, val.c_str()).ToLocalChecked())
                .Check();
        }
    }
#endif

    // 2. Load .env file (dotenv functionality)
    std::map<std::string, std::string> dotenv = loadDotEnv();
    for (auto const& [key, val] : dotenv) {
        env_obj
            ->Set(context,
                  v8::String::NewFromUtf8(p_isolate, key.c_str()).ToLocalChecked(),
                  v8::String::NewFromUtf8(p_isolate, val.c_str()).ToLocalChecked())
            .Check();
    }

    return env_obj;
}

std::map<std::string, std::string> Process::loadDotEnv() {
    std::map<std::string, std::string> res;
    std::ifstream file(".env");
    if (!file.is_open()) {
        return res;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Remove leading/trailing whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty() || line[0] == '#') {
            continue;
        }

        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string val = line.substr(pos + 1);

            // Basic cleanup for key and value
            key.erase(key.find_last_not_of(" \t") + 1);
            val.erase(0, val.find_first_not_of(" \t"));

            // Remove quotes if present
            if (val.size() >= 2 && ((val.front() == '"' && val.back() == '"') || (val.front() == '\'' && val.back() == '\''))) {
                val = val.substr(1, val.size() - 2);
            }

            res[key] = val;
        }
    }

    return res;
}

} // namespace module
} // namespace z8
