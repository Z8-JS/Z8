#ifndef Z8_MODULE_EVENTS_H
#define Z8_MODULE_EVENTS_H

#include "v8.h"

namespace z8 {
namespace module {

class Events {
  public:
    static v8::Local<v8::ObjectTemplate> createTemplate(v8::Isolate* p_isolate);

    // Static utilities
    static void once(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void on(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void listenerCount(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void getEventListeners(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void getMaxListeners(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void setMaxListeners(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void addAbortListener(const v8::FunctionCallbackInfo<v8::Value>& args);

    // EventEmitter class
    static v8::Local<v8::FunctionTemplate> createEventEmitterTemplate(v8::Isolate* p_isolate);
    
    // EventEmitter prototype methods
    static void eeConstructor(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void eeOn(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void eeOnce(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void eeEmit(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void eeRemoveListener(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void eeRemoveAllListeners(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void eeSetMaxListeners(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void eeGetMaxListeners(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void eeListeners(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void eeRawListeners(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void eeListenerCount(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void eePrependListener(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void eePrependOnceListener(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void eeEventNames(const v8::FunctionCallbackInfo<v8::Value>& args);
};

} // namespace module
} // namespace z8

#endif // Z8_MODULE_EVENTS_H
