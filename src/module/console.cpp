#include "console.h"
#include <string.h>
#ifdef _WIN32
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif
#include <chrono>
#include <iostream>
#include <map>
#include <signal.h>
#include <stdlib.h>
#include <string>

namespace z8 {
namespace module {

// Global exit/crash handler to rescue logs
static void FlushAll() {
    fflush(stdout);
    fflush(stderr);
}

static void HandleCrash(int32_t sig) {
    // Attempt one last flush before dying
    FlushAll();
    // Re-raise the signal to allow default OS behavior (core dump, etc.)
    signal(sig, SIG_DFL);
    raise(sig);
}

int32_t Console::m_indentation_level = 0;

bool ShouldUseColors(FILE* p_stream) {
    static std::map<FILE*, bool> cache;
    auto it = cache.find(p_stream);
    if (it != cache.end())
        return it->second;

    bool result = true;
    if (!isatty(fileno(p_stream)))
        result = false;
    else {
        const char* p_no_color = getenv("NO_COLOR");
        if (p_no_color != nullptr && strlen(p_no_color) > 0)
            result = false;
        else {
            const char* p_term = getenv("TERM");
            if (p_term != nullptr && strcmp(p_term, "dumb") == 0)
                result = false;
        }
    }

    cache[p_stream] = result;
    return result;
}

v8::Local<v8::ObjectTemplate> Console::createTemplate(v8::Isolate* p_isolate) {
    static bool buffered = []() {
        setvbuf(stdout, nullptr, _IOFBF, 64 * 1024);
        setvbuf(stderr, nullptr, _IOFBF, 64 * 1024);

        // Register normal exit handler
        std::atexit(FlushAll);

        // Register crash handlers
        signal(SIGSEGV, HandleCrash);
        signal(SIGABRT, HandleCrash);
        signal(SIGFPE, HandleCrash);
        signal(SIGILL, HandleCrash);
#ifdef _WIN32
        signal(SIGTERM, HandleCrash);
#endif
        return true;
    }();

    v8::Local<v8::ObjectTemplate> console = v8::ObjectTemplate::New(p_isolate);
    console->Set(v8::String::NewFromUtf8(p_isolate, "log").ToLocalChecked(), v8::FunctionTemplate::New(p_isolate, log));
    console->Set(v8::String::NewFromUtf8(p_isolate, "error").ToLocalChecked(),
                 v8::FunctionTemplate::New(p_isolate, error));
    console->Set(v8::String::NewFromUtf8(p_isolate, "warn").ToLocalChecked(),
                 v8::FunctionTemplate::New(p_isolate, warn));
    console->Set(v8::String::NewFromUtf8(p_isolate, "info").ToLocalChecked(),
                 v8::FunctionTemplate::New(p_isolate, info));
    console->Set(v8::String::NewFromUtf8(p_isolate, "assert").ToLocalChecked(),
                 v8::FunctionTemplate::New(p_isolate, assert_));
    console->Set(v8::String::NewFromUtf8(p_isolate, "count").ToLocalChecked(),
                 v8::FunctionTemplate::New(p_isolate, count));
    console->Set(v8::String::NewFromUtf8(p_isolate, "countReset").ToLocalChecked(),
                 v8::FunctionTemplate::New(p_isolate, countReset));
    console->Set(v8::String::NewFromUtf8(p_isolate, "dir").ToLocalChecked(), v8::FunctionTemplate::New(p_isolate, dir));
    console->Set(v8::String::NewFromUtf8(p_isolate, "dirxml").ToLocalChecked(),
                 v8::FunctionTemplate::New(p_isolate, log));
    console->Set(v8::String::NewFromUtf8(p_isolate, "group").ToLocalChecked(),
                 v8::FunctionTemplate::New(p_isolate, group));
    console->Set(v8::String::NewFromUtf8(p_isolate, "groupCollapsed").ToLocalChecked(),
                 v8::FunctionTemplate::New(p_isolate, groupCollapsed));
    console->Set(v8::String::NewFromUtf8(p_isolate, "groupEnd").ToLocalChecked(),
                 v8::FunctionTemplate::New(p_isolate, groupEnd));
    console->Set(v8::String::NewFromUtf8(p_isolate, "time").ToLocalChecked(),
                 v8::FunctionTemplate::New(p_isolate, time));
    console->Set(v8::String::NewFromUtf8(p_isolate, "timeLog").ToLocalChecked(),
                 v8::FunctionTemplate::New(p_isolate, timeLog));
    console->Set(v8::String::NewFromUtf8(p_isolate, "timeEnd").ToLocalChecked(),
                 v8::FunctionTemplate::New(p_isolate, timeEnd));
    console->Set(v8::String::NewFromUtf8(p_isolate, "trace").ToLocalChecked(),
                 v8::FunctionTemplate::New(p_isolate, trace));
    console->Set(v8::String::NewFromUtf8(p_isolate, "clear").ToLocalChecked(),
                 v8::FunctionTemplate::New(p_isolate, clear));
    return console;
}

void Console::log(const v8::FunctionCallbackInfo<v8::Value>& args) {
    write(args, nullptr);
}

void Console::error(const v8::FunctionCallbackInfo<v8::Value>& args) {
    write(args, "\x1b[31m", true);
}

void Console::warn(const v8::FunctionCallbackInfo<v8::Value>& args) {
    write(args, "\x1b[33m");
}

void Console::info(const v8::FunctionCallbackInfo<v8::Value>& args) {
    write(args, "\x1b[36m");
}

void Console::assert_(const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (args.Length() > 0 && args[0]->BooleanValue(args.GetIsolate())) {
        return;
    }

    v8::Isolate* p_isolate = args.GetIsolate();
    v8::HandleScope handle_scope(p_isolate);
    FILE* p_out = stderr;
    bool use_color = ShouldUseColors(p_out);

    if (use_color)
        fputs("\x1b[31m", p_out);
    fputs("Assertion failed:", p_out);

    int32_t len = args.Length();
    if (len > 1) {
        fputc(' ', p_out);
        for (int32_t i = 1; i < len; i++) {
            v8::String::Utf8Value utf8(p_isolate, args[i]);
            if (*utf8) {
                fwrite(*utf8, 1, utf8.length(), p_out);
            } else {
                fputs("undefined", p_out);
            }
            if (i < len - 1)
                fputc(' ', p_out);
        }
    } else {
        fputs(" console.assert", p_out);
    }

    if (use_color)
        fputs("\x1b[0m\n", p_out);
    else
        fputc('\n', p_out);
    fflush(p_out);
}

static std::map<std::string, int32_t> s_console_counts;

void Console::count(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    std::string label = "default";

    if (args.Length() > 0 && !args[0]->IsUndefined()) {
        v8::String::Utf8Value utf8(p_isolate, args[0]);
        if (*utf8)
            label = *utf8;
    }

    int32_t count = ++s_console_counts[label];
    FILE* p_out = stdout;

    // Apply indentation
    for (int32_t i = 0; i < m_indentation_level; i++)
        fputs("  ", p_out);

    std::string output = label + ": " + std::to_string(count) + "\n";
    fwrite(output.c_str(), 1, output.length(), p_out);
    adaptiveFlush(p_out);
}

void Console::countReset(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    std::string label = "default";

    if (args.Length() > 0 && !args[0]->IsUndefined()) {
        v8::String::Utf8Value utf8(p_isolate, args[0]);
        if (*utf8)
            label = *utf8;
    }

    if (s_console_counts.find(label) != s_console_counts.end()) {
        s_console_counts[label] = 0;
    } else {
        std::string msg = "Count for '" + label + "' does not exist\n";
        fwrite(msg.c_str(), 1, msg.length(), stderr);
        fflush(stderr);
    }
}

static std::string
Inspect(v8::Isolate* p_isolate, v8::Local<v8::Value> value, int32_t depth, int32_t current_depth, bool colors) {
    if (value->IsUndefined())
        return colors ? "\x1b[90mundefined\x1b[0m" : "undefined";
    if (value->IsNull())
        return colors ? "\x1b[90mnull\x1b[0m" : "null";
    if (value->IsBoolean()) {
        bool val = value->BooleanValue(p_isolate);
        return colors ? "\x1b[33m" + std::string(val ? "true" : "false") + "\x1b[0m" : (val ? "true" : "false");
    }
    if (value->IsNumber()) {
        v8::String::Utf8Value str(p_isolate, value);
        return colors ? "\x1b[33m" + std::string(*str) + "\x1b[0m" : *str;
    }
    if (value->IsString()) {
        v8::String::Utf8Value str(p_isolate, value);
        return colors ? "\x1b[32m\"" + std::string(*str) + "\"\x1b[0m" : "\"" + std::string(*str) + "\"";
    }

    if (value->IsObject()) {
        if (depth != -1 && current_depth >= depth)
            return colors ? "\x1b[38;5;242m[Object]\x1b[0m" : "[Object]";

        v8::Local<v8::Object> obj = value.As<v8::Object>();
        v8::Local<v8::Context> p_context = p_isolate->GetCurrentContext();

        // Special handling for Error objects (display stack or message)
        if (value->IsNativeError()) {
            v8::Local<v8::Value> stack;
            if (obj->Get(p_context, v8::String::NewFromUtf8Literal(p_isolate, "stack")).ToLocal(&stack) &&
                stack->IsString()) {
                v8::String::Utf8Value stack_str(p_isolate, stack);
                return colors ? "\x1b[31m" + std::string(*stack_str) + "\x1b[0m" : *stack_str;
            }
        }

        v8::Local<v8::Array> props;
        if (!obj->GetPropertyNames(p_context).ToLocal(&props))
            return "[Object]";

        std::string indent = "";
        for (int32_t i = 0; i < current_depth + 1; i++)
            indent += "  ";

        std::string result = "{\n";
        for (uint32_t i = 0; i < props->Length(); i++) {
            v8::Local<v8::Value> key;
            if (!props->Get(p_context, i).ToLocal(&key))
                continue;
            v8::Local<v8::Value> val;
            if (!obj->Get(p_context, key).ToLocal(&val))
                continue;

            v8::String::Utf8Value key_str(p_isolate, key);
            result += indent + (colors ? "\x1b[36m" + std::string(*key_str) + "\x1b[0m" : *key_str) + ": ";
            result += Inspect(p_isolate, val, depth, current_depth + 1, colors);
            if (i < props->Length() - 1)
                result += ",";
            result += "\n";
        }
        std::string closing_indent = "";
        for (int32_t i = 0; i < current_depth; i++)
            closing_indent += "  ";
        result += closing_indent + "}";
        return result;
    }

    return "[Unknown]";
}

void Console::dir(const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (args.Length() == 0)
        return;
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::HandleScope handle_scope(p_isolate);
    v8::Local<v8::Context> p_context = p_isolate->GetCurrentContext();

    int32_t depth = 2;
    bool colors = ShouldUseColors(stdout);

    if (args.Length() > 1 && args[1]->IsObject()) {
        v8::Local<v8::Object> options = args[1].As<v8::Object>();
        v8::Local<v8::Value> depth_val;
        if (options->Get(p_context, v8::String::NewFromUtf8(p_isolate, "depth").ToLocalChecked()).ToLocal(&depth_val)) {
            if (depth_val->IsNull())
                depth = -1;
            else if (depth_val->IsNumber())
                depth = static_cast<int32_t>(depth_val->NumberValue(p_context).FromMaybe(2.0));
        }
        v8::Local<v8::Value> colors_val;
        if (options->Get(p_context, v8::String::NewFromUtf8(p_isolate, "colors").ToLocalChecked())
                .ToLocal(&colors_val)) {
            if (colors_val->IsBoolean())
                colors = colors_val->BooleanValue(p_isolate);
        }
    }

    std::string result = Inspect(p_isolate, args[0], depth, 0, colors) + "\n";
    FILE* p_out = stdout;

    // Apply indentation to each line of the result
    std::string indent = "";
    for (int32_t i = 0; i < m_indentation_level; i++)
        indent += "  ";

    // Prepend indent to the first line
    fwrite(indent.c_str(), 1, indent.length(), p_out);

    // For subsequent lines, we should probably handle it within result replacement or just simple replacement
    size_t pos = 0;
    while ((pos = result.find('\n', pos)) != std::string::npos) {
        if (pos + 1 < result.length()) {
            result.insert(pos + 1, indent);
            pos += indent.length() + 1;
        } else {
            break;
        }
    }

    fwrite(result.c_str(), 1, result.length(), p_out);
    adaptiveFlush(p_out);
}

void Console::group(const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (args.Length() > 0) {
        log(args);
    }
    m_indentation_level++;
}

void Console::groupCollapsed(const v8::FunctionCallbackInfo<v8::Value>& args) {
    group(args); // CLI doesn't support collapse, same as Group
}

void Console::groupEnd(const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (m_indentation_level > 0) {
        m_indentation_level--;
    }
}

static std::map<std::string, std::chrono::steady_clock::time_point> s_console_timers;

void Console::time(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    std::string label = "default";
    if (args.Length() > 0 && !args[0]->IsUndefined()) {
        v8::String::Utf8Value utf8(p_isolate, args[0]);
        if (*utf8)
            label = *utf8;
    }
    s_console_timers[label] = std::chrono::steady_clock::now();
}

void Console::timeLog(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    std::string label = "default";
    if (args.Length() > 0 && !args[0]->IsUndefined()) {
        v8::String::Utf8Value utf8(p_isolate, args[0]);
        if (*utf8)
            label = *utf8;
    }

    auto it = s_console_timers.find(label);
    if (it == s_console_timers.end()) {
        std::string msg = "Timer '" + label + "' does not exist\n";
        fwrite(msg.c_str(), 1, msg.length(), stderr);
        fflush(stderr);
        return;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<double, std::milli>(now - it->second).count();

    FILE* p_out = stdout;
    for (int32_t i = 0; i < m_indentation_level; i++)
        fputs("  ", p_out);

    char buf[128];
    snprintf(buf, sizeof(buf), "%s: %.3fms\n", label.c_str(), elapsed);
    fwrite(buf, 1, strlen(buf), p_out);
    adaptiveFlush(p_out);
}

void Console::timeEnd(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    std::string label = "default";
    if (args.Length() > 0 && !args[0]->IsUndefined()) {
        v8::String::Utf8Value utf8(p_isolate, args[0]);
        if (*utf8)
            label = *utf8;
    }

    auto it = s_console_timers.find(label);
    if (it == s_console_timers.end()) {
        std::string msg = "Timer '" + label + "' does not exist\n";
        fwrite(msg.c_str(), 1, msg.length(), stderr);
        fflush(stderr);
        return;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<double, std::milli>(now - it->second).count();
    s_console_timers.erase(it);

    FILE* p_out = stdout;
    for (int32_t i = 0; i < m_indentation_level; i++)
        fputs("  ", p_out);

    char buf[128];
    snprintf(buf, sizeof(buf), "%s: %.3fms\n", label.c_str(), elapsed);
    fwrite(buf, 1, strlen(buf), p_out);
    adaptiveFlush(p_out);
}

void Console::trace(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::HandleScope handle_scope(p_isolate);
    v8::Local<v8::Context> p_context = p_isolate->GetCurrentContext();
    FILE* p_out = stderr;

    // Apply indentation
    for (int32_t i = 0; i < m_indentation_level; i++)
        fputs("  ", p_out);

    fputs("Trace:", p_out);
    int32_t len = args.Length();
    for (int32_t i = 0; i < len; i++) {
        fputc(' ', p_out);
        v8::String::Utf8Value utf8(p_isolate, args[i]);
        if (*utf8) {
            fwrite(*utf8, 1, utf8.length(), p_out);
        } else {
            fputs("undefined", p_out);
        }
    }
    fputc('\n', p_out);

    v8::Local<v8::StackTrace> stack_trace = v8::StackTrace::CurrentStackTrace(p_isolate, 10);
    for (int32_t i = 0; i < stack_trace->GetFrameCount(); i++) {
        v8::Local<v8::StackFrame> frame = stack_trace->GetFrame(p_isolate, i);
        v8::String::Utf8Value script_name(p_isolate, frame->GetScriptName());
        v8::String::Utf8Value function_name(p_isolate, frame->GetFunctionName());
        int32_t line = frame->GetLineNumber();
        int32_t column = frame->GetColumn();

        for (int32_t j = 0; j < m_indentation_level + 1; j++)
            fputs("  ", p_out);

        const char* p_fn_name = *function_name ? *function_name : "(anonymous)";
        const char* p_sn_name = *script_name ? *script_name : "unknown";

        char buf[512];
        snprintf(buf, sizeof(buf), "at %s (%s:%d:%d)\n", p_fn_name, p_sn_name, line, column);
        fwrite(buf, 1, strlen(buf), p_out);
    }

    fflush(p_out);
}

void Console::clear(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void Console::write(const v8::FunctionCallbackInfo<v8::Value>& args, const char* p_color_code, bool is_error) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::HandleScope handle_scope(p_isolate);
    FILE* p_out = is_error ? stderr : stdout;
    bool use_color = p_color_code && ShouldUseColors(p_out);
    bool global_colors = ShouldUseColors(p_out);

    // Apply indentation
    for (int32_t i = 0; i < m_indentation_level; i++)
        fputs("  ", p_out);

    if (use_color)
        fputs(p_color_code, p_out);
    int32_t len = args.Length();
    for (int32_t i = 0; i < len; i++) {
        if (args[i]->IsObject() && !args[i]->IsString() && !args[i]->IsNumber() && !args[i]->IsBoolean() &&
            !args[i]->IsNull()) {
            // Smart inspection for console.log: default depth 2 (like Node)
            std::string inspected = Inspect(p_isolate, args[i], 2, 0, global_colors);
            fwrite(inspected.c_str(), 1, inspected.length(), p_out);
        } else {
            v8::String::Utf8Value utf8(p_isolate, args[i]);
            if (*utf8) {
                fwrite(*utf8, 1, utf8.length(), p_out);
            } else {
                fputs("undefined", p_out);
            }
        }
        if (i < len - 1)
            fputc(' ', p_out);
    }
    if (use_color)
        fputs("\x1b[0m\n", p_out);
    else
        fputc('\n', p_out);
    if (is_error)
        fflush(p_out);
    else
        adaptiveFlush(p_out);
}

void Console::adaptiveFlush(FILE* p_out) {
    static auto last_flush = std::chrono::steady_clock::now();
    static int32_t calls_in_burst = 0;
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_flush).count();
    if (elapsed < 50)
        calls_in_burst++;
    else {
        calls_in_burst = 0;
        last_flush = now;
    }

    // Low latency mode: flush immediately during bursts
    // High throughput mode: let the 64KB buffer handle it
    if (calls_in_burst < 20 && isatty(fileno(p_out))) {
        if (p_out == stderr)
            fflush(stdout); // Always push stdout before stderr
        fflush(p_out);
    }
}

} // namespace module
} // namespace z8
