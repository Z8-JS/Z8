#ifndef Z8_MODULE_STREAM_H
#define Z8_MODULE_STREAM_H

#include "v8.h"
#include "../events/events.h"
#include <vector> // Added for std::vector

namespace z8 {
namespace module {

// Internal state for C++ backed streams
struct StreamInternal {
    virtual ~StreamInternal() = default;
    bool m_is_readable = false;
    bool m_is_writable = false;
    bool m_reading = false;
    bool m_writing = false;
    bool m_ended = false;
    bool m_destroyed = false;
    bool m_paused = true;
    size_t m_high_water_mark = 16 * 1024; // 16KB default
    std::vector<uint8_t> m_buffer;
    uint64_t m_bytes_read = 0;
    uint64_t m_bytes_written = 0;
};

class Stream {
  private:
    static v8::Persistent<v8::FunctionTemplate> m_readable_tmpl;
    static v8::Persistent<v8::FunctionTemplate> m_writable_tmpl;
    static v8::Persistent<v8::FunctionTemplate> m_duplex_tmpl;
    static v8::Persistent<v8::FunctionTemplate> m_transform_tmpl;

  public:
    static v8::Local<v8::ObjectTemplate> createTemplate(v8::Isolate* p_isolate);

    // Stream base class methods
    static void streamConstructor(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void streamPipe(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void streamUnpipe(const v8::FunctionCallbackInfo<v8::Value>& args);

    // Readable methods
    static v8::Local<v8::FunctionTemplate> createReadableTemplate(v8::Isolate* p_isolate, v8::Local<v8::FunctionTemplate> ee_tmpl);
    static void readableConstructor(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void readableRead(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void readablePush(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void readablePause(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void readableResume(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void readableDestroy(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void readableSetEncoding(const v8::FunctionCallbackInfo<v8::Value>& args);

    // Helpers for other modules
    static v8::Local<v8::FunctionTemplate> getReadableTemplate(v8::Isolate* p_isolate);
    static v8::Local<v8::FunctionTemplate> getWritableTemplate(v8::Isolate* p_isolate);
    static v8::Local<v8::FunctionTemplate> getDuplexTemplate(v8::Isolate* p_isolate);
    static v8::Local<v8::FunctionTemplate> getTransformTemplate(v8::Isolate* p_isolate);

    // Writable methods
    static v8::Local<v8::FunctionTemplate> createWritableTemplate(v8::Isolate* p_isolate, v8::Local<v8::FunctionTemplate> ee_tmpl);
    static void writableConstructor(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void writableWrite(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void writableEnd(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void writableDestroy(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void writableCork(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void writableUncork(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void writableSetDefaultEncoding(const v8::FunctionCallbackInfo<v8::Value>& args);

    // Duplex and Transform
    static v8::Local<v8::FunctionTemplate> createDuplexTemplate(v8::Isolate* p_isolate, v8::Local<v8::FunctionTemplate> readable_tmpl, v8::Local<v8::FunctionTemplate> writable_tmpl);
    static v8::Local<v8::FunctionTemplate> createTransformTemplate(v8::Isolate* p_isolate, v8::Local<v8::FunctionTemplate> duplex_tmpl);
    static void duplexConstructor(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void transformConstructor(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void transform_transform(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void transform_flush(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void transform_write(const v8::FunctionCallbackInfo<v8::Value>& args);

    // Internal Helpers
    static void pipeline(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void finished(const v8::FunctionCallbackInfo<v8::Value>& args);
};

} // namespace module
} // namespace z8

#endif // Z8_MODULE_STREAM_H
