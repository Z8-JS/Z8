$path = 'd:\Z8\Z8-app\src\module\node\stream\stream.cpp'
$raw = Get-Content -Raw -Path $path

$newRD = 'void Stream::streamDestroy(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    v8::Local<v8::Object> self = args.This();
    v8::Local<v8::Data> internal_data = self->GetInternalField(0);
    if (internal_data.IsEmpty() || !internal_data->IsValue() || !internal_data.As<v8::Value>()->IsExternal()) return;
    StreamInternal* p_internal = static_cast<StreamInternal*>(internal_data.As<v8::External>()->Value());

    if (p_internal->m_destroyed) {
        args.GetReturnValue().Set(self);
        return;
    }

    p_internal->m_destroyed = true;
    v8::Local<v8::Value> error = args.Length() > 0 ? args[0] : v8::Undefined(p_isolate).As<v8::Value>();

    if (!error->IsUndefined() && !error->IsNull()) {
        v8::Local<v8::Value> emit_val;
        if (self->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "emit")).ToLocal(&emit_val) && emit_val->IsFunction()) {
            v8::Local<v8::Value> argv[] = { v8::String::NewFromUtf8Literal(p_isolate, "error"), error };
            (void)emit_val.As<v8::Function>()->Call(context, self, 2, argv);
        }
    }

    v8::Local<v8::Value> destroy_val;
    if (self->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "_destroy")).ToLocal(&destroy_val) && destroy_val->IsFunction()) {
        v8::Local<v8::Function> cb = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            v8::Isolate* p_isolate_inner = args.GetIsolate();
            v8::Local<v8::Context> context_inner = p_isolate_inner->GetCurrentContext();
            v8::Local<v8::Object> self_inner = args.This();
            StreamInternal* p_internal_inner = static_cast<StreamInternal*>(self_inner->GetInternalField(0).As<v8::External>()->Value());
            p_internal_inner->m_closed = true;
            v8::Local<v8::Value> emit_val_inner;
            if (self_inner->Get(context_inner, v8::String::NewFromUtf8Literal(p_isolate_inner, "emit")).ToLocal(&emit_val_inner) && emit_val_inner->IsFunction()) {
                v8::Local<v8::Value> argv[] = { v8::String::NewFromUtf8Literal(p_isolate_inner, "close") };
                (void)emit_val_inner.As<v8::Function>()->Call(context_inner, self_inner, 1, argv);
            }
        }).ToLocalChecked();
        v8::Local<v8::Value> argv[] = { error, cb };
        (void)destroy_val.As<v8::Function>()->Call(context, self, 2, argv);
    } else {
        p_internal->m_closed = true;
        v8::Local<v8::Value> emit_val;
        if (self->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "emit")).ToLocal(&emit_val) && emit_val->IsFunction()) {
            v8::Local<v8::Value> argv[] = { v8::String::NewFromUtf8Literal(p_isolate, "close") };
            (void)emit_val.As<v8::Function>()->Call(context, self, 1, argv);
        }
    }
    args.GetReturnValue().Set(self);
}'

# Note: The previously applied bad patch is still there but with the line that failed to compile maybe?
# Wait, MSVC failed at that line, so the file WAS updated.
$currentBAD = 'void Stream::streamDestroy(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    v8::Local<v8::Object> self = args.This();
    v8::Local<v8::Value> internal_ext = self->GetInternalField(0);
    if (internal_ext.IsEmpty() || !internal_ext->IsExternal()) return;
    StreamInternal* p_internal = static_cast<StreamInternal*>(internal_ext.As<v8::External>()->Value());

    if (p_internal->m_destroyed) {
        args.GetReturnValue().Set(self);
        return;
    }

    p_internal->m_destroyed = true;
    v8::Local<v8::Value> error = args.Length() > 0 ? args[0] : v8::Undefined(p_isolate).As<v8::Value>();

    if (!error->IsUndefined() && !error->IsNull()) {
        v8::Local<v8::Value> emit_val;
        if (self->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "emit")).ToLocal(&emit_val) && emit_val->IsFunction()) {
            v8::Local<v8::Value> argv[] = { v8::String::NewFromUtf8Literal(p_isolate, "error"), error };
            (void)emit_val.As<v8::Function>()->Call(context, self, 2, argv);
        }
    }

    v8::Local<v8::Value> destroy_val;
    if (self->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "_destroy")).ToLocal(&destroy_val) && destroy_val->IsFunction()) {
        v8::Local<v8::Function> cb = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            v8::Isolate* p_isolate_inner = args.GetIsolate();
            v8::Local<v8::Context> context_inner = p_isolate_inner->GetCurrentContext();
            v8::Local<v8::Object> self_inner = args.This();
            StreamInternal* p_internal_inner = static_cast<StreamInternal*>(self_inner->GetInternalField(0).As<v8::External>()->Value());
            p_internal_inner->m_closed = true;
            v8::Local<v8::Value> emit_val_inner;
            if (self_inner->Get(context_inner, v8::String::NewFromUtf8Literal(p_isolate_inner, "emit")).ToLocal(&emit_val_inner) && emit_val_inner->IsFunction()) {
                v8::Local<v8::Value> argv[] = { v8::String::NewFromUtf8Literal(p_isolate_inner, "close") };
                (void)emit_val_inner.As<v8::Function>()->Call(context_inner, self_inner, 1, argv);
            }
        }).ToLocalChecked();
        v8::Local<v8::Value> argv[] = { error, cb };
        (void)destroy_val.As<v8::Function>()->Call(context, self, 2, argv);
    } else {
        p_internal->m_closed = true;
        v8::Local<v8::Value> emit_val;
        if (self->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "emit")).ToLocal(&emit_val) && emit_val->IsFunction()) {
            v8::Local<v8::Value> argv[] = { v8::String::NewFromUtf8Literal(p_isolate, "close") };
            (void)emit_val.As<v8::Function>()->Call(context, self, 1, argv);
        }
    }
    args.GetReturnValue().Set(self);
}'

$raw.Replace($currentBAD, $newRD) | Set-Content -Path $path
