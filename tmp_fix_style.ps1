$path = 'd:\Z8\Z8-app\src\module\node\stream\stream.cpp'
$oldWrite = '    v8::Local<v8::Function> wrapper = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
        v8::Isolate* p_isolate_inner = args.GetIsolate();
        v8::Local<v8::Context> context_inner = p_isolate_inner->GetCurrentContext();
        v8::Local<v8::Object> d = args.Data().As<v8::Object>();
        
        v8::Local<v8::Object> s = d->Get(context_inner, v8::String::NewFromUtf8Literal(p_isolate_inner, "stream")).ToLocalChecked().As<v8::Object>();
        StreamInternal* pi = static_cast<StreamInternal*>(s->GetInternalField(0).As<v8::External>()->Value());
        
        size_t clen = static_cast<size_t>(d->Get(context_inner, v8::String::NewFromUtf8Literal(p_isolate_inner, "chunk_len")).ToLocalChecked()->NumberValue(context_inner).FromMaybe(0.0));
        
        bool was_full = pi->m_writable_len >= pi->m_high_water_mark;
        pi->m_writable_len -= clen;
        
        v8::Local<v8::Value> user_cb;
        if (d->Get(context_inner, v8::String::NewFromUtf8Literal(p_isolate_inner, "user_cb")).ToLocal(&user_cb) && user_cb->IsFunction()) {
            (void)user_cb.As<v8::Function>()->Call(context_inner, s, 0, nullptr);
        }
        
        if (was_full && pi->m_writable_len < pi->m_high_water_mark) {
            v8::Local<v8::Value> emit_val;
            if (s->Get(context_inner, v8::String::NewFromUtf8Literal(p_isolate_inner, "emit")).ToLocal(&emit_val) && emit_val->IsFunction()) {
                v8::Local<v8::Value> argv[] = { v8::String::NewFromUtf8Literal(p_isolate_inner, "drain") };
                (void)emit_val.As<v8::Function>()->Call(context_inner, s, 1, argv);
            }
        }
    }, data).ToLocalChecked();'

$newWrite = '    v8::Local<v8::Function> wrapper = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
        v8::Isolate* p_isolate_inner = args.GetIsolate();
        v8::Local<v8::Context> context_inner = p_isolate_inner->GetCurrentContext();
        v8::Local<v8::Object> data_inner = args.Data().As<v8::Object>();
        
        v8::Local<v8::Object> self_obj = data_inner->Get(context_inner, v8::String::NewFromUtf8Literal(p_isolate_inner, "stream")).ToLocalChecked().As<v8::Object>();
        StreamInternal* p_internal_inner = static_cast<StreamInternal*>(self_obj->GetInternalField(0).As<v8::External>()->Value());
        
        size_t chunk_len_inner = static_cast<size_t>(data_inner->Get(context_inner, v8::String::NewFromUtf8Literal(p_isolate_inner, "chunk_len")).ToLocalChecked()->NumberValue(context_inner).FromMaybe(0.0));
        
        bool was_full = p_internal_inner->m_writable_len >= p_internal_inner->m_high_water_mark;
        p_internal_inner->m_writable_len -= chunk_len_inner;
        
        v8::Local<v8::Value> user_cb;
        if (data_inner->Get(context_inner, v8::String::NewFromUtf8Literal(p_isolate_inner, "user_cb")).ToLocal(&user_cb) && user_cb->IsFunction()) {
            (void)user_cb.As<v8::Function>()->Call(context_inner, self_obj, 0, nullptr);
        }
        
        if (was_full && p_internal_inner->m_writable_len < p_internal_inner->m_high_water_mark) {
            v8::Local<v8::Value> emit_val;
            if (self_obj->Get(context_inner, v8::String::NewFromUtf8Literal(p_isolate_inner, "emit")).ToLocal(&emit_val) && emit_val->IsFunction()) {
                v8::Local<v8::Value> argv[] = { v8::String::NewFromUtf8Literal(p_isolate_inner, "drain") };
                (void)emit_val.As<v8::Function>()->Call(context_inner, self_obj, 1, argv);
            }
        }
    }, data).ToLocalChecked();'

(Get-Content -Raw -Path $path).Replace($oldWrite, $newWrite) | Set-Content -Path $path
