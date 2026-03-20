$path = 'd:\Z8\Z8-app\src\module\node\stream\stream.cpp'
$old = '    self->SetInternalField(0, v8::External::New(p_isolate, p_internal));

    // Cleanup on GC
    v8::Global<v8::Object> global_self(p_isolate, self);'
$new = '    // Listen for ''newListener'' to auto-resume on ''data''
    v8::Local<v8::Value> on_fn;
    if (self->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "on")).ToLocal(&on_fn) && on_fn->IsFunction()) {
        v8::Local<v8::Object> data = v8::Object::New(p_isolate);
        data->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "stream"), self).Check();
        
        v8::Local<v8::Function> nl_handler = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            v8::Isolate* p_isolate_inner = args.GetIsolate();
            v8::Local<v8::Context> context_inner = p_isolate_inner->GetCurrentContext();
            v8::Local<v8::Value> type = args[0];
            if (type->IsString() && type->ToString(context_inner).ToLocalChecked()->StringEquals(v8::String::NewFromUtf8Literal(p_isolate_inner, "data"))) {
                v8::Local<v8::Object> self_obj = args.Data().As<v8::Object>()->Get(context_inner, v8::String::NewFromUtf8Literal(p_isolate_inner, "stream")).ToLocalChecked().As<v8::Object>();
                v8::Local<v8::Value> resume_fn;
                if (self_obj->Get(context_inner, v8::String::NewFromUtf8Literal(p_isolate_inner, "resume")).ToLocal(&resume_fn) && resume_fn->IsFunction()) {
                    (void)resume_fn.As<v8::Function>()->Call(context_inner, self_obj, 0, nullptr);
                }
            }
        }, data).ToLocalChecked();
        
        v8::Local<v8::Value> on_argv[] = { v8::String::NewFromUtf8Literal(p_isolate, "newListener"), nl_handler };
        (void)on_fn.As<v8::Function>()->Call(context, self, 2, on_argv);
    }

    self->SetInternalField(0, v8::External::New(p_isolate, p_internal));

    // Cleanup on GC
    v8::Global<v8::Object> global_self(p_isolate, self);'
(Get-Content -Raw -Path $path) -replace [regex]::Escape($old), $new | Set-Content -Path $path
