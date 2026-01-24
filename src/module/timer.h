#ifndef Z8_TIMER_H
#define Z8_TIMER_H

#include "v8.h"
#include <chrono>
#include <vector>
#include <map>
#include <memory>

namespace z8 {
namespace module {

class Timer {
public:
    static void Initialize(v8::Isolate* isolate, v8::Local<v8::Context> context);
    
    // Global functions for JS
    static void SetTimeout(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void ClearTimeout(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void SetInterval(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void ClearInterval(const v8::FunctionCallbackInfo<v8::Value>& args);
    
    // Event loop integration
    static bool Tick(v8::Isolate* isolate, v8::Local<v8::Context> context);
    static bool HasActiveTimers();
    static std::chrono::milliseconds GetNextDelay();

private:
    struct TimerData {
        int id;
        v8::Global<v8::Function> callback;
        std::chrono::steady_clock::time_point expiry;
        std::vector<v8::Global<v8::Value>> args;
        int interval_ms;
        bool is_interval;
    };

    static std::map<int, std::unique_ptr<TimerData>> timers;
    static int next_timer_id;
    static int running_timer_id;
    static bool running_timer_cleared;
};

} // namespace module
} // namespace z8

#endif // Z8_TIMER_H
