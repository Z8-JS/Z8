#include "console.h"
#include <string.h>
#ifdef _WIN32
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif
#include <stdlib.h>
#include <iostream>
#include <chrono>
#include <map>
#include <string>
#include <signal.h>

namespace z8 {
namespace module {

// Global exit/crash handler to rescue logs
static void FlushAll() {
    fflush(stdout);
    fflush(stderr);
}

static void HandleCrash(int sig) {
    // Attempt one last flush before dying
    FlushAll();
    // Re-raise the signal to allow default OS behavior (core dump, etc.)
    signal(sig, SIG_DFL);
    raise(sig);
}

int Console::indentation_level = 0;

bool ShouldUseColors(FILE* stream) {
    static std::map<FILE*, bool> cache;
    auto it = cache.find(stream);
    if (it != cache.end()) return it->second;

    bool result = true;
    if (!isatty(fileno(stream))) result = false;
    else {
        const char* no_color = getenv("NO_COLOR");
        if (no_color != nullptr && strlen(no_color) > 0) result = false;
        else {
            const char* term = getenv("TERM");
            if (term != nullptr && strcmp(term, "dumb") == 0) result = false;
        }
    }
    
    cache[stream] = result;
    return result;
}

v8::Local<v8::ObjectTemplate> Console::CreateTemplate(v8::Isolate* isolate) {
    static bool buffered = []() {
        setvbuf(stdout, NULL, _IOFBF, 64 * 1024);
        setvbuf(stderr, NULL, _IOFBF, 64 * 1024);
        
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

    v8::Local<v8::ObjectTemplate> console = v8::ObjectTemplate::New(isolate);
    console->Set(v8::String::NewFromUtf8(isolate, "log").ToLocalChecked(), v8::FunctionTemplate::New(isolate, Log));
    console->Set(v8::String::NewFromUtf8(isolate, "error").ToLocalChecked(), v8::FunctionTemplate::New(isolate, Error));
    console->Set(v8::String::NewFromUtf8(isolate, "warn").ToLocalChecked(), v8::FunctionTemplate::New(isolate, Warn));
    console->Set(v8::String::NewFromUtf8(isolate, "info").ToLocalChecked(), v8::FunctionTemplate::New(isolate, Info));
    console->Set(v8::String::NewFromUtf8(isolate, "assert").ToLocalChecked(), v8::FunctionTemplate::New(isolate, Assert));
    console->Set(v8::String::NewFromUtf8(isolate, "count").ToLocalChecked(), v8::FunctionTemplate::New(isolate, Count));
    console->Set(v8::String::NewFromUtf8(isolate, "countReset").ToLocalChecked(), v8::FunctionTemplate::New(isolate, CountReset));
    console->Set(v8::String::NewFromUtf8(isolate, "dir").ToLocalChecked(), v8::FunctionTemplate::New(isolate, Dir));
    console->Set(v8::String::NewFromUtf8(isolate, "dirxml").ToLocalChecked(), v8::FunctionTemplate::New(isolate, Log));
    console->Set(v8::String::NewFromUtf8(isolate, "group").ToLocalChecked(), v8::FunctionTemplate::New(isolate, Group));
    console->Set(v8::String::NewFromUtf8(isolate, "groupCollapsed").ToLocalChecked(), v8::FunctionTemplate::New(isolate, GroupCollapsed));
    console->Set(v8::String::NewFromUtf8(isolate, "groupEnd").ToLocalChecked(), v8::FunctionTemplate::New(isolate, GroupEnd));
    console->Set(v8::String::NewFromUtf8(isolate, "time").ToLocalChecked(), v8::FunctionTemplate::New(isolate, Time));
    console->Set(v8::String::NewFromUtf8(isolate, "timeLog").ToLocalChecked(), v8::FunctionTemplate::New(isolate, TimeLog));
    console->Set(v8::String::NewFromUtf8(isolate, "timeEnd").ToLocalChecked(), v8::FunctionTemplate::New(isolate, TimeEnd));
    console->Set(v8::String::NewFromUtf8(isolate, "trace").ToLocalChecked(), v8::FunctionTemplate::New(isolate, Trace));
    console->Set(v8::String::NewFromUtf8(isolate, "clear").ToLocalChecked(), v8::FunctionTemplate::New(isolate, Clear));
    return console;
}

void Console::Log(const v8::FunctionCallbackInfo<v8::Value>& args) {
    Write(args, nullptr);
}

void Console::Error(const v8::FunctionCallbackInfo<v8::Value>& args) {
    Write(args, "\x1b[31m", true); 
}

void Console::Warn(const v8::FunctionCallbackInfo<v8::Value>& args) {
    Write(args, "\x1b[33m"); 
}

void Console::Info(const v8::FunctionCallbackInfo<v8::Value>& args) {
    Write(args, "\x1b[36m"); 
}

void Console::Assert(const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (args.Length() > 0 && args[0]->BooleanValue(args.GetIsolate())) {
        return;
    }

    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    FILE* out = stderr;
    bool use_color = ShouldUseColors(out);

    if (use_color) fputs("\x1b[31m", out);
    fputs("Assertion failed:", out);

    int len = args.Length();
    if (len > 1) {
        fputc(' ', out);
        for (int i = 1; i < len; i++) {
            v8::String::Utf8Value utf8(isolate, args[i]);
            if (*utf8) {
                fwrite(*utf8, 1, utf8.length(), out);
            } else {
                fputs("undefined", out);
            }
            if (i < len - 1) fputc(' ', out);
        }
    } else {
        fputs(" console.assert", out);
    }

    if (use_color) fputs("\x1b[0m\n", out);
    else fputc('\n', out);
    fflush(out);
}

static std::map<std::string, int> console_counts;

void Console::Count(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    std::string label = "default";
    
    if (args.Length() > 0 && !args[0]->IsUndefined()) {
        v8::String::Utf8Value utf8(isolate, args[0]);
        if (*utf8) label = *utf8;
    }

    int count = ++console_counts[label];
    FILE* out = stdout;
    
    // Apply indentation
    for (int i = 0; i < indentation_level; i++) fputs("  ", out);

    std::string output = label + ": " + std::to_string(count) + "\n";
    fwrite(output.c_str(), 1, output.length(), out);
    AdaptiveFlush(out);
}

void Console::CountReset(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    std::string label = "default";
    
    if (args.Length() > 0 && !args[0]->IsUndefined()) {
        v8::String::Utf8Value utf8(isolate, args[0]);
        if (*utf8) label = *utf8;
    }

    if (console_counts.find(label) != console_counts.end()) {
        console_counts[label] = 0;
    } else {
        std::string msg = "Count for '" + label + "' does not exist\n";
        fwrite(msg.c_str(), 1, msg.length(), stderr);
        fflush(stderr);
    }
}

static std::string Inspect(v8::Isolate* isolate, v8::Local<v8::Value> value, int depth, int current_depth, bool colors) {
    if (value->IsUndefined()) return colors ? "\x1b[90mundefined\x1b[0m" : "undefined";
    if (value->IsNull()) return colors ? "\x1b[90mnull\x1b[0m" : "null";
    if (value->IsBoolean()) {
        bool val = value->BooleanValue(isolate);
        return colors ? "\x1b[33m" + std::string(val ? "true" : "false") + "\x1b[0m" : (val ? "true" : "false");
    }
    if (value->IsNumber()) {
        v8::String::Utf8Value str(isolate, value);
        return colors ? "\x1b[33m" + std::string(*str) + "\x1b[0m" : *str;
    }
    if (value->IsString()) {
        v8::String::Utf8Value str(isolate, value);
        return colors ? "\x1b[32m\""+ std::string(*str) + "\"\x1b[0m" : "\"" + std::string(*str) + "\"";
    }

    if (value->IsObject()) {
        if (depth != -1 && current_depth >= depth) return colors ? "\x1b[38;5;242m[Object]\x1b[0m" : "[Object]";

        v8::Local<v8::Object> obj = value.As<v8::Object>();
        v8::Local<v8::Context> context = isolate->GetCurrentContext();
        v8::Local<v8::Array> props;
        if (!obj->GetPropertyNames(context).ToLocal(&props)) return "[Object]";

        std::string indent = "";
        for(int i=0; i<current_depth + 1; i++) indent += "  ";

        std::string result = "{\n";
        for (uint32_t i = 0; i < props->Length(); i++) {
            v8::Local<v8::Value> key;
            if (!props->Get(context, i).ToLocal(&key)) continue;
            v8::Local<v8::Value> val;
            if (!obj->Get(context, key).ToLocal(&val)) continue;

            v8::String::Utf8Value key_str(isolate, key);
            result += indent + (colors ? "\x1b[36m" + std::string(*key_str) + "\x1b[0m" : *key_str) + ": ";
            result += Inspect(isolate, val, depth, current_depth + 1, colors);
            if (i < props->Length() - 1) result += ",";
            result += "\n";
        }
        std::string closing_indent = "";
        for(int i=0; i<current_depth; i++) closing_indent += "  ";
        result += closing_indent + "}";
        return result;
    }

    return "[Unknown]";
}

void Console::Dir(const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (args.Length() == 0) return;
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    int depth = 2;
    bool colors = ShouldUseColors(stdout);

    if (args.Length() > 1 && args[1]->IsObject()) {
        v8::Local<v8::Object> options = args[1].As<v8::Object>();
        v8::Local<v8::Value> depth_val;
        if (options->Get(context, v8::String::NewFromUtf8(isolate, "depth").ToLocalChecked()).ToLocal(&depth_val)) {
            if (depth_val->IsNull()) depth = -1;
            else if (depth_val->IsNumber()) depth = (int)depth_val->NumberValue(context).FromMaybe(2.0);
        }
        v8::Local<v8::Value> colors_val;
        if (options->Get(context, v8::String::NewFromUtf8(isolate, "colors").ToLocalChecked()).ToLocal(&colors_val)) {
            if (colors_val->IsBoolean()) colors = colors_val->BooleanValue(isolate);
        }
    }

    std::string result = Inspect(isolate, args[0], depth, 0, colors) + "\n";
    FILE* out = stdout;
    
    // Apply indentation to each line of the result
    std::string indent = "";
    for (int i = 0; i < indentation_level; i++) indent += "  ";
    
    // Prepend indent to the first line
    fwrite(indent.c_str(), 1, indent.length(), out);
    
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

    fwrite(result.c_str(), 1, result.length(), out);
    AdaptiveFlush(out);
}

void Console::Group(const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (args.Length() > 0) {
        Log(args);
    }
    indentation_level++;
}

void Console::GroupCollapsed(const v8::FunctionCallbackInfo<v8::Value>& args) {
    Group(args); // CLI doesn't support collapse, same as Group
}

void Console::GroupEnd(const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (indentation_level > 0) {
        indentation_level--;
    }
}

static std::map<std::string, std::chrono::steady_clock::time_point> console_timers;

void Console::Time(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    std::string label = "default";
    if (args.Length() > 0 && !args[0]->IsUndefined()) {
        v8::String::Utf8Value utf8(isolate, args[0]);
        if (*utf8) label = *utf8;
    }
    console_timers[label] = std::chrono::steady_clock::now();
}

void Console::TimeLog(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    std::string label = "default";
    if (args.Length() > 0 && !args[0]->IsUndefined()) {
        v8::String::Utf8Value utf8(isolate, args[0]);
        if (*utf8) label = *utf8;
    }

    auto it = console_timers.find(label);
    if (it == console_timers.end()) {
        std::string msg = "Timer '" + label + "' does not exist\n";
        fwrite(msg.c_str(), 1, msg.length(), stderr);
        fflush(stderr);
        return;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<double, std::milli>(now - it->second).count();
    
    FILE* out = stdout;
    for (int i = 0; i < indentation_level; i++) fputs("  ", out);
    
    char buf[128];
    snprintf(buf, sizeof(buf), "%s: %.3fms\n", label.c_str(), elapsed);
    fwrite(buf, 1, strlen(buf), out);
    AdaptiveFlush(out);
}

void Console::TimeEnd(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    std::string label = "default";
    if (args.Length() > 0 && !args[0]->IsUndefined()) {
        v8::String::Utf8Value utf8(isolate, args[0]);
        if (*utf8) label = *utf8;
    }

    auto it = console_timers.find(label);
    if (it == console_timers.end()) {
        std::string msg = "Timer '" + label + "' does not exist\n";
        fwrite(msg.c_str(), 1, msg.length(), stderr);
        fflush(stderr);
        return;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<double, std::milli>(now - it->second).count();
    console_timers.erase(it);

    FILE* out = stdout;
    for (int i = 0; i < indentation_level; i++) fputs("  ", out);
    
    char buf[128];
    snprintf(buf, sizeof(buf), "%s: %.3fms\n", label.c_str(), elapsed);
    fwrite(buf, 1, strlen(buf), out);
    AdaptiveFlush(out);
}

void Console::Trace(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    FILE* out = stderr;

    // Apply indentation
    for (int i = 0; i < indentation_level; i++) fputs("  ", out);

    fputs("Trace:", out);
    int len = args.Length();
    for (int i = 0; i < len; i++) {
        fputc(' ', out);
        v8::String::Utf8Value utf8(isolate, args[i]);
        if (*utf8) { fwrite(*utf8, 1, utf8.length(), out); }
        else { fputs("undefined", out); }
    }
    fputc('\n', out);

    v8::Local<v8::StackTrace> stack_trace = v8::StackTrace::CurrentStackTrace(isolate, 10);
    for (int i = 0; i < stack_trace->GetFrameCount(); i++) {
        v8::Local<v8::StackFrame> frame = stack_trace->GetFrame(isolate, i);
        v8::String::Utf8Value script_name(isolate, frame->GetScriptName());
        v8::String::Utf8Value function_name(isolate, frame->GetFunctionName());
        int line = frame->GetLineNumber();
        int column = frame->GetColumn();

        for (int j = 0; j < indentation_level + 1; j++) fputs("  ", out);
        
        const char* fn_name = *function_name ? *function_name : "(anonymous)";
        const char* sn_name = *script_name ? *script_name : "unknown";
        
        char buf[512];
        snprintf(buf, sizeof(buf), "at %s (%s:%d:%d)\n", fn_name, sn_name, line, column);
        fwrite(buf, 1, strlen(buf), out);
    }
    
    fflush(out);
}

void Console::Clear(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void Console::Write(const v8::FunctionCallbackInfo<v8::Value>& args, const char* color_code, bool is_error) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    FILE* out = is_error ? stderr : stdout;
    bool use_color = color_code && ShouldUseColors(out);
    bool global_colors = ShouldUseColors(out);

    // Apply indentation
    for (int i = 0; i < indentation_level; i++) fputs("  ", out);

    if (use_color) fputs(color_code, out);
    int len = args.Length();
    for (int i = 0; i < len; i++) {
        if (args[i]->IsObject() && !args[i]->IsString() && !args[i]->IsNumber() && !args[i]->IsBoolean() && !args[i]->IsNull()) {
            // Smart inspection for console.log: default depth 2 (like Node)
            std::string inspected = Inspect(isolate, args[i], 2, 0, global_colors);
            fwrite(inspected.c_str(), 1, inspected.length(), out);
        } else {
            v8::String::Utf8Value utf8(isolate, args[i]);
            if (*utf8) { fwrite(*utf8, 1, utf8.length(), out); }
            else { fputs("undefined", out); }
        }
        if (i < len - 1) fputc(' ', out);
    }
    if (use_color) fputs("\x1b[0m\n", out);
    else fputc('\n', out);
    if (is_error) fflush(out);
    else AdaptiveFlush(out);
}

void Console::AdaptiveFlush(FILE* out) {
    static auto last_flush = std::chrono::steady_clock::now();
    static int calls_in_burst = 0;
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_flush).count();
    if (elapsed < 50) calls_in_burst++;
    else { calls_in_burst = 0; last_flush = now; }
    
    // Low latency mode: flush immediately during bursts
    // High throughput mode: let the 64KB buffer handle it
    if (calls_in_burst < 20 && isatty(fileno(out))) {
        if (out == stderr) fflush(stdout); // Always push stdout before stderr
        fflush(out);
    }
}

} // namespace module
} // namespace z8
