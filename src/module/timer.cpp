#include "timer.h"
#include <algorithm>

namespace z8 {
namespace module {

std::map<int, std::unique_ptr<Timer::TimerData>> Timer::timers;
int Timer::next_timer_id = 1;
int Timer::running_timer_id = -1;
bool Timer::running_timer_cleared = false;

void Timer::Initialize(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::Local<v8::Object> global = context->Global();

    global->Set(context, 
                v8::String::NewFromUtf8(isolate, "setTimeout").ToLocalChecked(),
                v8::FunctionTemplate::New(isolate, SetTimeout)->GetFunction(context).ToLocalChecked()).Check();
                
    global->Set(context, 
                v8::String::NewFromUtf8(isolate, "clearTimeout").ToLocalChecked(),
                v8::FunctionTemplate::New(isolate, ClearTimeout)->GetFunction(context).ToLocalChecked()).Check();

    global->Set(context, 
                v8::String::NewFromUtf8(isolate, "setInterval").ToLocalChecked(),
                v8::FunctionTemplate::New(isolate, SetInterval)->GetFunction(context).ToLocalChecked()).Check();

    global->Set(context, 
                v8::String::NewFromUtf8(isolate, "clearInterval").ToLocalChecked(),
                v8::FunctionTemplate::New(isolate, ClearInterval)->GetFunction(context).ToLocalChecked()).Check();
}

void Timer::SetTimeout(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 1 || !args[0]->IsFunction()) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "First argument must be a function").ToLocalChecked());
        return;
    }

    int delay = 0;
    if (args.Length() >= 2 && args[1]->IsNumber()) {
        delay = args[1]->Int32Value(isolate->GetCurrentContext()).FromMaybe(0);
    }
    if (delay < 0) delay = 0;

    int id = next_timer_id++;
    auto timer = std::make_unique<TimerData>();
    timer->id = id;
    timer->callback.Reset(isolate, args[0].As<v8::Function>());
    timer->expiry = std::chrono::steady_clock::now() + std::chrono::milliseconds(delay);
    timer->is_interval = false;
    timer->interval_ms = 0;

    // Capture extra arguments
    for (int i = 2; i < args.Length(); i++) {
        timer->args.emplace_back(isolate, args[i]);
    }

    timers[id] = std::move(timer);
    args.GetReturnValue().Set(id);
}

void Timer::SetInterval(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 1 || !args[0]->IsFunction()) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "First argument must be a function").ToLocalChecked());
        return;
    }

    int delay = 0;
    if (args.Length() >= 2 && args[1]->IsNumber()) {
        delay = args[1]->Int32Value(isolate->GetCurrentContext()).FromMaybe(0);
    }
    if (delay < 1) delay = 1; // Minimum interval is 1ms to prevent infinite synchronous loops

    int id = next_timer_id++;
    auto timer = std::make_unique<TimerData>();
    timer->id = id;
    timer->callback.Reset(isolate, args[0].As<v8::Function>());
    timer->expiry = std::chrono::steady_clock::now() + std::chrono::milliseconds(delay);
    timer->is_interval = true;
    timer->interval_ms = delay;

    // Capture extra arguments
    for (int i = 2; i < args.Length(); i++) {
        timer->args.emplace_back(isolate, args[i]);
    }

    timers[id] = std::move(timer);
    args.GetReturnValue().Set(id);
}

void Timer::ClearInterval(const v8::FunctionCallbackInfo<v8::Value>& args) {
    ClearTimeout(args);
}



void Timer::ClearTimeout(const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (args.Length() < 1 || !args[0]->IsNumber()) return;
    int id = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(-1);
    
    if (id == running_timer_id) {
        running_timer_cleared = true;
    }
    
    timers.erase(id);
}

bool Timer::Tick(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    if (timers.empty()) return false;

    auto now = std::chrono::steady_clock::now();
    std::vector<int> to_run;

    for (auto const& [id, timer] : timers) {
        if (timer->expiry <= now) {
            to_run.push_back(id);
        }
    }

    // Sort by expiry to maintain order for same-time timers
    std::sort(to_run.begin(), to_run.end(), [](int a, int b) {
        return timers[a]->expiry < timers[b]->expiry;
    });
    
    for (int id : to_run) {
        auto it = timers.find(id);
        if (it == timers.end()) continue;

        auto timer = std::move(it->second);
        timers.erase(it);

        running_timer_id = id;
        running_timer_cleared = false;

        v8::Local<v8::Function> cb = timer->callback.Get(isolate);
        
        std::vector<v8::Local<v8::Value>> js_args;
        for (auto& arg : timer->args) {
            js_args.push_back(arg.Get(isolate));
        }

        // Call the callback. If it throws, the isolation outer TryCatch in main.cpp will see it.
        v8::MaybeLocal<v8::Value> result = cb->Call(context, context->Global(), static_cast<int>(js_args.size()), js_args.data());
        
        running_timer_id = -1;

        if (result.IsEmpty()) {
            return false; // Stop the event loop as something went wrong
        }

        // Reschedule if it's an interval and wasn't cleared during execution
        if (timer->is_interval && !running_timer_cleared) {
            timer->expiry = std::chrono::steady_clock::now() + std::chrono::milliseconds(timer->interval_ms);
            timers[timer->id] = std::move(timer);
        }
    }

    return !timers.empty();
}

bool Timer::HasActiveTimers() {
    return !timers.empty();
}

std::chrono::milliseconds Timer::GetNextDelay() {
    if (timers.empty()) return std::chrono::milliseconds(0);
    
    auto now = std::chrono::steady_clock::now();
    auto min_expiry = std::chrono::steady_clock::time_point::max();

    for (auto const& [id, timer] : timers) {
        if (timer->expiry < min_expiry) {
            min_expiry = timer->expiry;
        }
    }

    if (min_expiry <= now) return std::chrono::milliseconds(0);
    return std::chrono::duration_cast<std::chrono::milliseconds>(min_expiry - now);
}

} // namespace module
} // namespace z8
