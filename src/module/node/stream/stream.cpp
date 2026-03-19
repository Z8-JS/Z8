#include "stream.h"
#include <cstring>
#include <iostream>
#include <vector>
#include <string>
 
template<typename T>
static void StreamWeakCallback(const v8::WeakCallbackInfo<T>& data) {
    delete data.GetParameter();
}

namespace z8 {
namespace module {

v8::Persistent<v8::FunctionTemplate> Stream::m_readable_tmpl;
v8::Persistent<v8::FunctionTemplate> Stream::m_writable_tmpl;
v8::Persistent<v8::FunctionTemplate> Stream::m_duplex_tmpl;
v8::Persistent<v8::FunctionTemplate> Stream::m_transform_tmpl;

v8::Local<v8::ObjectTemplate> Stream::createTemplate(v8::Isolate* p_isolate) {
    v8::Local<v8::ObjectTemplate> tmpl = v8::ObjectTemplate::New(p_isolate);
    
    v8::Local<v8::FunctionTemplate> readable_tmpl = getReadableTemplate(p_isolate);
    v8::Local<v8::FunctionTemplate> writable_tmpl = getWritableTemplate(p_isolate);
    v8::Local<v8::FunctionTemplate> duplex_tmpl = getDuplexTemplate(p_isolate);
    v8::Local<v8::FunctionTemplate> transform_tmpl = getTransformTemplate(p_isolate);

    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "Readable"), readable_tmpl);
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "Writable"), writable_tmpl);
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "Duplex"), duplex_tmpl);
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "Transform"), transform_tmpl);
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "pipeline"), v8::FunctionTemplate::New(p_isolate, pipeline));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "finished"), v8::FunctionTemplate::New(p_isolate, finished));

    return tmpl;
}

v8::Local<v8::FunctionTemplate> Stream::getReadableTemplate(v8::Isolate* p_isolate) {
    if (m_readable_tmpl.IsEmpty()) {
        v8::Local<v8::FunctionTemplate> ee_tmpl = Events::getEventEmitterTemplate(p_isolate);
        m_readable_tmpl.Reset(p_isolate, createReadableTemplate(p_isolate, ee_tmpl));
    }
    return m_readable_tmpl.Get(p_isolate);
}

v8::Local<v8::FunctionTemplate> Stream::getWritableTemplate(v8::Isolate* p_isolate) {
    if (m_writable_tmpl.IsEmpty()) {
        v8::Local<v8::FunctionTemplate> ee_tmpl = Events::getEventEmitterTemplate(p_isolate);
        m_writable_tmpl.Reset(p_isolate, createWritableTemplate(p_isolate, ee_tmpl));
    }
    return m_writable_tmpl.Get(p_isolate);
}

v8::Local<v8::FunctionTemplate> Stream::getDuplexTemplate(v8::Isolate* p_isolate) {
    if (m_duplex_tmpl.IsEmpty()) {
        v8::Local<v8::FunctionTemplate> readable_tmpl = getReadableTemplate(p_isolate);
        v8::Local<v8::FunctionTemplate> writable_tmpl = getWritableTemplate(p_isolate);
        m_duplex_tmpl.Reset(p_isolate, createDuplexTemplate(p_isolate, readable_tmpl, writable_tmpl));
    }
    return m_duplex_tmpl.Get(p_isolate);
}

v8::Local<v8::FunctionTemplate> Stream::getTransformTemplate(v8::Isolate* p_isolate) {
    if (m_transform_tmpl.IsEmpty()) {
        v8::Local<v8::FunctionTemplate> duplex_tmpl = getDuplexTemplate(p_isolate);
        m_transform_tmpl.Reset(p_isolate, createTransformTemplate(p_isolate, duplex_tmpl));
    }
    return m_transform_tmpl.Get(p_isolate);
}

// --- Readable ---

v8::Local<v8::FunctionTemplate> Stream::createReadableTemplate(v8::Isolate* p_isolate, v8::Local<v8::FunctionTemplate> ee_tmpl) {
    v8::Local<v8::FunctionTemplate> tmpl = v8::FunctionTemplate::New(p_isolate, readableConstructor);
    tmpl->SetClassName(v8::String::NewFromUtf8Literal(p_isolate, "Readable"));
    tmpl->Inherit(ee_tmpl);
    tmpl->InstanceTemplate()->SetInternalFieldCount(1);

    v8::Local<v8::ObjectTemplate> proto = tmpl->PrototypeTemplate();
    proto->Set(v8::String::NewFromUtf8Literal(p_isolate, "read"), v8::FunctionTemplate::New(p_isolate, readableRead));
    proto->Set(v8::String::NewFromUtf8Literal(p_isolate, "push"), v8::FunctionTemplate::New(p_isolate, readablePush));
    proto->Set(v8::String::NewFromUtf8Literal(p_isolate, "pause"), v8::FunctionTemplate::New(p_isolate, readablePause));
    proto->Set(v8::String::NewFromUtf8Literal(p_isolate, "resume"), v8::FunctionTemplate::New(p_isolate, readableResume));
    proto->Set(v8::String::NewFromUtf8Literal(p_isolate, "destroy"), v8::FunctionTemplate::New(p_isolate, readableDestroy));
    proto->Set(v8::String::NewFromUtf8Literal(p_isolate, "setEncoding"), v8::FunctionTemplate::New(p_isolate, readableSetEncoding));
    proto->Set(v8::String::NewFromUtf8Literal(p_isolate, "pipe"), v8::FunctionTemplate::New(p_isolate, streamPipe));
    proto->Set(v8::String::NewFromUtf8Literal(p_isolate, "unpipe"), v8::FunctionTemplate::New(p_isolate, streamUnpipe));

    return tmpl;
}

void Stream::readableConstructor(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    v8::Local<v8::Object> self = args.This();

    // Initialize EventEmitter part
    Events::eeConstructor(args);

    if (args.Length() > 0 && args[0]->IsObject()) {
        v8::Local<v8::Object> options = args[0].As<v8::Object>();
        v8::Local<v8::Value> internal_val;
        if (options->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "_internal")).ToLocal(&internal_val) && internal_val->IsExternal()) {
            self->SetInternalField(0, internal_val);
            // Also handle highWaterMark and read() if provided in options
            v8::Local<v8::Value> hwm;
            if (options->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "highWaterMark")).ToLocal(&hwm) && hwm->IsNumber()) {
                StreamInternal* p_internal = static_cast<StreamInternal*>(internal_val.As<v8::External>()->Value());
                p_internal->m_high_water_mark = static_cast<size_t>(hwm->Uint32Value(context).FromMaybe(16384));
            }
            v8::Local<v8::Value> read_fn;
            if (options->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "read")).ToLocal(&read_fn) && read_fn->IsFunction()) {
                (void)self->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "_read"), read_fn);
            }
            return;
        }
    }

    StreamInternal* p_internal = new StreamInternal();
    p_internal->m_is_readable = true;
    
    if (args.Length() > 0 && args[0]->IsObject()) {
        v8::Local<v8::Object> options = args[0].As<v8::Object>();
        v8::Local<v8::Value> hwm;
        if (options->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "highWaterMark")).ToLocal(&hwm) && hwm->IsNumber()) {
            p_internal->m_high_water_mark = static_cast<size_t>(hwm->Uint32Value(context).FromMaybe(16384));
        }
        v8::Local<v8::Value> read_fn;
        if (options->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "read")).ToLocal(&read_fn) && read_fn->IsFunction()) {
            (void)self->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "_read"), read_fn);
        }
    }

    self->SetInternalField(0, v8::External::New(p_isolate, p_internal));

    // Cleanup on GC
    v8::Global<v8::Object> global_self(p_isolate, self);
    global_self.SetWeak(p_internal, StreamWeakCallback<StreamInternal>, v8::WeakCallbackType::kParameter);
}

void Stream::readableRead(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // Basic implementation: emit 'data' if there's data in buffer
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Object> self = args.This();
    // StreamInternal* p_internal = static_cast<StreamInternal*>(v8::Local<v8::External>::Cast(self->GetInternalField(0))->Value());
    
    // In a real stream, this would call _read() if needed
    // For now we just return null or the buffer
    args.GetReturnValue().SetNull();
}

void Stream::readablePush(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    v8::Local<v8::Object> self = args.This();
    
    if (args.Length() == 0 || args[0]->IsNull()) {
        // EOF
        v8::Local<v8::Value> emit_val;
        if (self->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "emit")).ToLocal(&emit_val) && emit_val->IsFunction()) {
            v8::Local<v8::Value> argv[] = { v8::String::NewFromUtf8Literal(p_isolate, "end") };
            (void)emit_val.As<v8::Function>()->Call(context, self, 1, argv);
        }
        args.GetReturnValue().Set(false);
        return;
    }

    // Emit 'data'
    v8::Local<v8::Value> emit_val;
    if (self->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "emit")).ToLocal(&emit_val) && emit_val->IsFunction()) {
        v8::Local<v8::Value> argv[] = { v8::String::NewFromUtf8Literal(p_isolate, "data"), args[0] };
        (void)emit_val.As<v8::Function>()->Call(context, self, 2, argv);
    }

    args.GetReturnValue().Set(true);
}

void Stream::readablePause(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Local<v8::Object> self = args.This();
    v8::Local<v8::External> ext = self->GetInternalField(0).As<v8::External>();
    StreamInternal* p_internal = static_cast<StreamInternal*>(ext->Value());
    p_internal->m_paused = true;
    args.GetReturnValue().Set(self);
}

void Stream::readableResume(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Local<v8::Object> self = args.This();
    v8::Local<v8::External> ext = self->GetInternalField(0).As<v8::External>();
    StreamInternal* p_internal = static_cast<StreamInternal*>(ext->Value());
    p_internal->m_paused = false;
    
    // In a real implementation, this might trigger reading from the source
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    v8::Local<v8::Value> emit_val;
    if (self->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "emit")).ToLocal(&emit_val) && emit_val->IsFunction()) {
        v8::Local<v8::Value> argv[] = { v8::String::NewFromUtf8Literal(p_isolate, "resume") };
        (void)emit_val.As<v8::Function>()->Call(context, self, 1, argv);
    }

    args.GetReturnValue().Set(self);
}

void Stream::readableSetEncoding(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // Simplified: we don't store encoding in StreamInternal yet
    args.GetReturnValue().Set(args.This());
}

void Stream::streamUnpipe(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // Simplified: unpipe logic would require tracking piped destinations
    args.GetReturnValue().Set(args.This());
}

void Stream::writableCork(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(args.This());
}

void Stream::writableUncork(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(args.This());
}

void Stream::writableSetDefaultEncoding(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(args.This());
}

void Stream::readableDestroy(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    v8::Local<v8::Object> self = args.This();
    
    v8::Local<v8::Value> emit_val;
    if (self->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "emit")).ToLocal(&emit_val) && emit_val->IsFunction()) {
        v8::Local<v8::Value> argv[] = { v8::String::NewFromUtf8Literal(p_isolate, "close") };
        (void)emit_val.As<v8::Function>()->Call(context, self, 1, argv);
    }
    args.GetReturnValue().Set(args.This());
}

// --- Writable ---

v8::Local<v8::FunctionTemplate> Stream::createWritableTemplate(v8::Isolate* p_isolate, v8::Local<v8::FunctionTemplate> ee_tmpl) {
    v8::Local<v8::FunctionTemplate> tmpl = v8::FunctionTemplate::New(p_isolate, writableConstructor);
    tmpl->SetClassName(v8::String::NewFromUtf8Literal(p_isolate, "Writable"));
    tmpl->Inherit(ee_tmpl);
    tmpl->InstanceTemplate()->SetInternalFieldCount(1);

    v8::Local<v8::ObjectTemplate> proto = tmpl->PrototypeTemplate();
    proto->Set(v8::String::NewFromUtf8Literal(p_isolate, "write"), v8::FunctionTemplate::New(p_isolate, writableWrite));
    proto->Set(v8::String::NewFromUtf8Literal(p_isolate, "end"), v8::FunctionTemplate::New(p_isolate, writableEnd));
    proto->Set(v8::String::NewFromUtf8Literal(p_isolate, "destroy"), v8::FunctionTemplate::New(p_isolate, writableDestroy));
    proto->Set(v8::String::NewFromUtf8Literal(p_isolate, "cork"), v8::FunctionTemplate::New(p_isolate, writableCork));
    proto->Set(v8::String::NewFromUtf8Literal(p_isolate, "uncork"), v8::FunctionTemplate::New(p_isolate, writableUncork));
    proto->Set(v8::String::NewFromUtf8Literal(p_isolate, "setDefaultEncoding"), v8::FunctionTemplate::New(p_isolate, writableSetDefaultEncoding));

    return tmpl;
}

void Stream::writableConstructor(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Object> self = args.This();

    Events::eeConstructor(args);

    StreamInternal* p_internal = new StreamInternal();
    p_internal->m_is_writable = true;
    self->SetInternalField(0, v8::External::New(p_isolate, p_internal));

    if (args.Length() > 0 && args[0]->IsObject()) {
        v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
        v8::Local<v8::Object> options = args[0].As<v8::Object>();
        v8::Local<v8::Value> write_fn;
        if (options->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "write")).ToLocal(&write_fn) && write_fn->IsFunction()) {
            (void)self->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "_write"), write_fn);
        }
    }
 
    v8::Global<v8::Object> global_self(p_isolate, self);
    global_self.SetWeak(p_internal, StreamWeakCallback<StreamInternal>, v8::WeakCallbackType::kParameter);
}

void Stream::writableWrite(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    v8::Local<v8::Object> self = args.This();

    // If _write is defined, call it. Otherwise, this is a base class that does nothing.
    v8::Local<v8::Value> write_val;
    if (self->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "_write")).ToLocal(&write_val) && write_val->IsFunction()) {
        // args[0] is chunk, args[1] is encoding, args[2] is callback
        v8::Local<v8::Value> chunk = args[0];
        v8::Local<v8::Value> encoding = args.Length() > 1 ? args[1] : v8::Undefined(p_isolate).As<v8::Value>();
        v8::Local<v8::Value> callback = args.Length() > 2 ? args[2] : v8::Undefined(p_isolate).As<v8::Value>();
        
        if (!callback->IsFunction()) {
            // Provide a dummy callback if none was passed
            callback = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {}).ToLocalChecked();
        }

        v8::Local<v8::Value> argv[] = { chunk, encoding, callback };
        (void)write_val.As<v8::Function>()->Call(context, self, 3, argv);
    } else {
        // Default: If no callback, just return true
        if (args.Length() > 2 && args[2]->IsFunction()) {
            v8::Local<v8::Function> cb = args[2].As<v8::Function>();
            (void)cb->Call(context, self, 0, nullptr);
        }
    }

    args.GetReturnValue().Set(true);
}

void Stream::writableEnd(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    v8::Local<v8::Object> self = args.This();

    if (args.Length() > 0 && !args[0]->IsUndefined() && !args[0]->IsFunction()) {
        v8::Local<v8::Value> write_val;
        if (self->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "write")).ToLocal(&write_val) && write_val->IsFunction()) {
             v8::Local<v8::Value> chunk = args[0];
             v8::Local<v8::Value> write_argv[] = { chunk };
             (void)write_val.As<v8::Function>()->Call(context, self, 1, write_argv);
        }
    }

    v8::Local<v8::Value> emit_val;
    if (self->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "emit")).ToLocal(&emit_val) && emit_val->IsFunction()) {
        v8::Local<v8::Value> argv[] = { v8::String::NewFromUtf8Literal(p_isolate, "finish") };
        (void)emit_val.As<v8::Function>()->Call(context, self, 1, argv);
    }

    if (args.Length() > 0 && args[args.Length()-1]->IsFunction()) {
        v8::Local<v8::Function> cb = args[args.Length()-1].As<v8::Function>();
        (void)cb->Call(context, self, 0, nullptr);
    }

    args.GetReturnValue().Set(args.This());
}

void Stream::writableDestroy(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    v8::Local<v8::Object> self = args.This();
    
    v8::Local<v8::Value> emit_val;
    if (self->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "emit")).ToLocal(&emit_val) && emit_val->IsFunction()) {
        v8::Local<v8::Value> argv[] = { v8::String::NewFromUtf8Literal(p_isolate, "close") };
        (void)emit_val.As<v8::Function>()->Call(context, self, 1, argv);
    }
    args.GetReturnValue().Set(args.This());
}

// --- Duplex ---

v8::Local<v8::FunctionTemplate> Stream::createDuplexTemplate(v8::Isolate* p_isolate, v8::Local<v8::FunctionTemplate> readable_tmpl, v8::Local<v8::FunctionTemplate> writable_tmpl) {
    v8::Local<v8::FunctionTemplate> tmpl = v8::FunctionTemplate::New(p_isolate, duplexConstructor);
    tmpl->SetClassName(v8::String::NewFromUtf8Literal(p_isolate, "Duplex"));
    tmpl->Inherit(readable_tmpl); // Duplex inherits from Readable in Node
    tmpl->InstanceTemplate()->SetInternalFieldCount(1);
    
    v8::Local<v8::ObjectTemplate> proto = tmpl->PrototypeTemplate();
    // Mix in Writable methods
    proto->Set(v8::String::NewFromUtf8Literal(p_isolate, "write"), v8::FunctionTemplate::New(p_isolate, writableWrite));
    proto->Set(v8::String::NewFromUtf8Literal(p_isolate, "end"), v8::FunctionTemplate::New(p_isolate, writableEnd));
    
    return tmpl;
}

void Stream::duplexConstructor(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // Duplex is both readable and writable
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Object> self = args.This();
 
    Events::eeConstructor(args);
 
    StreamInternal* p_internal = new StreamInternal();
    p_internal->m_is_readable = true;
    p_internal->m_is_writable = true;
    self->SetInternalField(0, v8::External::New(p_isolate, p_internal));
 
    v8::Global<v8::Object> global_self(p_isolate, self);
    global_self.SetWeak(p_internal, StreamWeakCallback<StreamInternal>, v8::WeakCallbackType::kParameter);
}

// --- Transform ---

v8::Local<v8::FunctionTemplate> Stream::createTransformTemplate(v8::Isolate* p_isolate, v8::Local<v8::FunctionTemplate> duplex_tmpl) {
    v8::Local<v8::FunctionTemplate> tmpl = v8::FunctionTemplate::New(p_isolate, transformConstructor);
    tmpl->SetClassName(v8::String::NewFromUtf8Literal(p_isolate, "Transform"));
    tmpl->Inherit(duplex_tmpl);
    tmpl->InstanceTemplate()->SetInternalFieldCount(1);
    
    v8::Local<v8::ObjectTemplate> proto = tmpl->PrototypeTemplate();
    proto->Set(v8::String::NewFromUtf8Literal(p_isolate, "_write"), v8::FunctionTemplate::New(p_isolate, transform_write));
    proto->Set(v8::String::NewFromUtf8Literal(p_isolate, "_transform"), v8::FunctionTemplate::New(p_isolate, transform_transform));
    proto->Set(v8::String::NewFromUtf8Literal(p_isolate, "_flush"), v8::FunctionTemplate::New(p_isolate, transform_flush));
    
    return tmpl;
}

void Stream::transformConstructor(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    readableConstructor(args);
    v8::Local<v8::Object> self = args.This();
    v8::Local<v8::External> ext = self->GetInternalField(0).As<v8::External>();
    StreamInternal* p_internal = static_cast<StreamInternal*>(ext->Value());
    p_internal->m_is_writable = true;

    // Handle options: transform, flush, etc.
    if (args.Length() > 0 && args[0]->IsObject()) {
        v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
        v8::Local<v8::Object> options = args[0].As<v8::Object>();
        v8::Local<v8::Value> transform_fn;
        if (options->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "transform")).ToLocal(&transform_fn) && transform_fn->IsFunction()) {
            (void)self->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "_transform"), transform_fn);
        }
        v8::Local<v8::Value> flush_fn;
        if (options->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "flush")).ToLocal(&flush_fn) && flush_fn->IsFunction()) {
            (void)self->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "_flush"), flush_fn);
        }
    }
}

void Stream::transform_write(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    v8::Local<v8::Object> self = args.This();
    
    // transform_write(chunk, encoding, callback) calls _transform(chunk, encoding, callback)
    v8::Local<v8::Value> transform_val;
    if (self->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "_transform")).ToLocal(&transform_val) && transform_val->IsFunction()) {
        v8::Local<v8::Value> argv[] = { args[0], args[1], args[2] };
        (void)transform_val.As<v8::Function>()->Call(context, self, 3, argv);
    }
}

void Stream::transform_transform(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // Default: push what you get (identity transform)
    readablePush(args);
    if (args.Length() > 2 && args[2]->IsFunction()) {
        v8::Local<v8::Function> cb = args[2].As<v8::Function>();
        (void)cb->Call(args.GetIsolate()->GetCurrentContext(), args.This(), 0, nullptr);
    }
}

void Stream::transform_flush(const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (args.Length() > 0 && args[0]->IsFunction()) {
        v8::Local<v8::Function> cb = args[0].As<v8::Function>();
        (void)cb->Call(args.GetIsolate()->GetCurrentContext(), args.This(), 0, nullptr);
    }
}

// --- Pipe ---

void Stream::streamPipe(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    v8::Local<v8::Object> self = args.This();
    
    if (args.Length() < 1 || !args[0]->IsObject()) {
        p_isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8Literal(p_isolate, "Dest must be a stream")));
        return;
    }
    
    v8::Local<v8::Object> dest = args[0].As<v8::Object>();
    
    // Very simplified pipe: listen for 'data' on source, call write() on dest
    v8::Local<v8::Value> on_val;
    if (self->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "on")).ToLocal(&on_val) && on_val->IsFunction()) {
        
        v8::Local<v8::Object> data = v8::Object::New(p_isolate);
        data->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "dest"), dest).Check();
        
        v8::Local<v8::Function> data_handler = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            v8::Local<v8::Object> dest_obj = args.Data().As<v8::Object>()->Get(args.GetIsolate()->GetCurrentContext(), v8::String::NewFromUtf8Literal(args.GetIsolate(), "dest")).ToLocalChecked().As<v8::Object>();
            v8::Local<v8::Value> write_fn;
            if (dest_obj->Get(args.GetIsolate()->GetCurrentContext(), v8::String::NewFromUtf8Literal(args.GetIsolate(), "write")).ToLocal(&write_fn) && write_fn->IsFunction()) {
                v8::Local<v8::Value> chunk = args[0];
                // Pass a dummy callback to write()
                v8::Local<v8::Value> cb = v8::Function::New(args.GetIsolate()->GetCurrentContext(), [](const v8::FunctionCallbackInfo<v8::Value>& args) {}).ToLocalChecked();
                v8::Local<v8::Value> write_argv[] = { chunk, v8::Undefined(args.GetIsolate()), cb };
                (void)write_fn.As<v8::Function>()->Call(args.GetIsolate()->GetCurrentContext(), dest_obj, 3, write_argv);
            }
        }, data).ToLocalChecked();
        
        v8::Local<v8::Value> on_argv[] = { v8::String::NewFromUtf8Literal(p_isolate, "data"), data_handler };
        (void)on_val.As<v8::Function>()->Call(context, self, 2, on_argv);
        
        // Handle 'end' -> dest.end()
        v8::Local<v8::Function> end_handler = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            v8::Local<v8::Object> dest_obj = args.Data().As<v8::Object>()->Get(args.GetIsolate()->GetCurrentContext(), v8::String::NewFromUtf8Literal(args.GetIsolate(), "dest")).ToLocalChecked().As<v8::Object>();
            v8::Local<v8::Value> end_fn;
            if (dest_obj->Get(args.GetIsolate()->GetCurrentContext(), v8::String::NewFromUtf8Literal(args.GetIsolate(), "end")).ToLocal(&end_fn) && end_fn->IsFunction()) {
                (void)end_fn.As<v8::Function>()->Call(args.GetIsolate()->GetCurrentContext(), dest_obj, 0, nullptr);
            }
        }, data).ToLocalChecked();
        
        v8::Local<v8::Value> end_argv[] = { v8::String::NewFromUtf8Literal(p_isolate, "end"), end_handler };
        (void)on_val.As<v8::Function>()->Call(context, self, 2, end_argv);
    }
    
    args.GetReturnValue().Set(dest);
}

void Stream::pipeline(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();

    if (args.Length() < 2) {
        p_isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8Literal(p_isolate, "Not enough arguments to pipeline()")));
        return;
    }

    // Basic pipeline implementation: connect all streams in the arguments
    v8::Local<v8::Value> current = args[0];
    v8::Local<v8::Value> callback = v8::Null(p_isolate);

    int32_t stream_count = args.Length();
    if (args[args.Length() - 1]->IsFunction()) {
        callback = args[args.Length() - 1];
        stream_count--;
    }

    for (int32_t i = 0; i < stream_count - 1; ++i) {
        v8::Local<v8::Value> source = args[i];
        v8::Local<v8::Value> dest = args[i + 1];

        if (source->IsObject() && dest->IsObject()) {
            v8::Local<v8::Object> source_obj = source.As<v8::Object>();
            v8::Local<v8::Value> pipe_fn;
            if (source_obj->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "pipe")).ToLocal(&pipe_fn) && pipe_fn->IsFunction()) {
                v8::Local<v8::Value> pipe_argv[] = { dest };
                (void)pipe_fn.As<v8::Function>()->Call(context, source_obj, 1, pipe_argv);
            }
        }
    }

    if (callback->IsFunction()) {
        // In a real implementation, we'd wait for the last stream to finish/error
        v8::Local<v8::Value> callback_argv[] = { v8::Null(p_isolate) };
        (void)callback.As<v8::Function>()->Call(context, v8::Null(p_isolate), 1, callback_argv);
    }

    args.GetReturnValue().Set(args[stream_count - 1]);
}

void Stream::finished(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsObject()) {
        p_isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8Literal(p_isolate, "First argument must be a stream")));
        return;
    }

    v8::Local<v8::Object> stream_obj = args[0].As<v8::Object>();
    v8::Local<v8::Value> callback = args.Length() > 1 ? args[1] : v8::Null(p_isolate).As<v8::Value>();

    if (callback->IsFunction()) {
        // Simplified: listen for 'end' or 'finish' or 'close' or 'error'
        v8::Local<v8::Value> on_fn;
        if (stream_obj->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "on")).ToLocal(&on_fn) && on_fn->IsFunction()) {
            v8::Local<v8::Value> events[] = {
                v8::String::NewFromUtf8Literal(p_isolate, "end"),
                v8::String::NewFromUtf8Literal(p_isolate, "finish"),
                v8::String::NewFromUtf8Literal(p_isolate, "close")
            };

            for (auto& evt : events) {
                v8::Local<v8::Value> on_argv[] = { evt, callback };
                (void)on_fn.As<v8::Function>()->Call(context, stream_obj, 2, on_argv);
            }

            // Handle error separately to pass the error object
            v8::Local<v8::Value> error_evt = v8::String::NewFromUtf8Literal(p_isolate, "error");
            v8::Local<v8::Value> error_argv[] = { error_evt, callback };
            (void)on_fn.As<v8::Function>()->Call(context, stream_obj, 2, error_argv);
        }
    }

    args.GetReturnValue().Set(v8::Undefined(p_isolate));
}

} // namespace module
} // namespace z8
