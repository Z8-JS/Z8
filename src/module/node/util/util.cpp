#include "util.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace z8 {
namespace module {

v8::Local<v8::ObjectTemplate> Util::createTemplate(v8::Isolate* p_isolate) {
    v8::Local<v8::ObjectTemplate> tmpl = v8::ObjectTemplate::New(p_isolate);

    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "format"), v8::FunctionTemplate::New(p_isolate, format));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "promisify"), v8::FunctionTemplate::New(p_isolate, promisify));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "callbackify"),
              v8::FunctionTemplate::New(p_isolate, callbackify));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "inherits"), v8::FunctionTemplate::New(p_isolate, inherits));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "inspect"), v8::FunctionTemplate::New(p_isolate, inspect));

    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "types"), createTypesTemplate(p_isolate));

    return tmpl;
}

v8::Local<v8::ObjectTemplate> Util::createTypesTemplate(v8::Isolate* p_isolate) {
    v8::Local<v8::ObjectTemplate> tmpl = v8::ObjectTemplate::New(p_isolate);

    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isDate"), v8::FunctionTemplate::New(p_isolate, isDate));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isRegExp"), v8::FunctionTemplate::New(p_isolate, isRegExp));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isPromise"), v8::FunctionTemplate::New(p_isolate, isPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isTypedArray"),
              v8::FunctionTemplate::New(p_isolate, isTypedArray));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isUint8Array"),
              v8::FunctionTemplate::New(p_isolate, isUint8Array));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isMap"), v8::FunctionTemplate::New(p_isolate, isMap));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isSet"), v8::FunctionTemplate::New(p_isolate, isSet));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isNativeError"),
              v8::FunctionTemplate::New(p_isolate, isNativeError));

    return tmpl;
}

void Util::format(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::HandleScope handle_scope(p_isolate);

    if (args.Length() == 0) {
        args.GetReturnValue().Set(v8::String::Empty(p_isolate));
        return;
    }

    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    v8::String::Utf8Value first_arg(p_isolate, args[0]);
    std::string fmt_str(*first_arg);

    std::stringstream result;
    size_t arg_index = 1;

    for (size_t i = 0; i < fmt_str.length(); ++i) {
        if (fmt_str[i] == '%' && i + 1 < fmt_str.length() && arg_index < (size_t) args.Length()) {
            char spec = fmt_str[i + 1];
            v8::Local<v8::Value> arg = args[(int) arg_index++];

            if (spec == 's') {
                v8::String::Utf8Value val(p_isolate, arg);
                result << *val;
                i++;
            } else if (spec == 'd' || spec == 'i') {
                result << arg->NumberValue(context).FromMaybe(0.0);
                i++;
            } else if (spec == 'j') {
                v8::Local<v8::String> json;
                if (v8::JSON::Stringify(context, arg).ToLocal(&json)) {
                    v8::String::Utf8Value val(p_isolate, json);
                    result << *val;
                } else {
                    result << "[Circular]";
                }
                i++;
            } else {
                result << '%';
            }
        } else {
            result << fmt_str[i];
        }
    }

    // Append remaining arguments
    for (; arg_index < (size_t) args.Length(); ++arg_index) {
        v8::String::Utf8Value val(p_isolate, args[(int) arg_index]);
        result << " " << *val;
    }

    args.GetReturnValue().Set(v8::String::NewFromUtf8(p_isolate, result.str().c_str()).ToLocalChecked());
}

void Util::promisify(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    if (args.Length() < 1 || !args[0]->IsFunction()) {
        p_isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(p_isolate, "The \"original\" argument must be of type function")));
        return;
    }

    v8::Local<v8::Function> original = args[0].As<v8::Function>();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();

    // Basic implementation of promisify in C++
    // Returns a function that returns a Promise
    auto promisified_callback = [](const v8::FunctionCallbackInfo<v8::Value>& args) {
        v8::Isolate* p_isolate = args.GetIsolate();
        v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
        v8::Local<v8::Function> original = v8::Local<v8::Function>::Cast(args.Data());

        v8::Local<v8::Promise::Resolver> resolver =
            v8::Local<v8::Promise::Resolver>::New(p_isolate, v8::Promise::Resolver::New(context).ToLocalChecked());

        // Create the callback: (err, value) => { if (err) reject(err); else resolve(value); }
        struct CallbackData {
            v8::Global<v8::Promise::Resolver> resolver;
        };
        auto* data = new CallbackData{v8::Global<v8::Promise::Resolver>(p_isolate, resolver)};

        auto callback_wrapper = [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            v8::Isolate* p_isolate = args.GetIsolate();
            v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
            auto* data = static_cast<CallbackData*>(v8::Local<v8::External>::Cast(args.Data())->Value());

            v8::Local<v8::Promise::Resolver> resolver = data->resolver.Get(p_isolate);

            if (args.Length() > 0 && !args[0]->IsNull() && !args[0]->IsUndefined()) {
                resolver->Reject(context, args[0]).Check();
            } else {
                v8::Local<v8::Value> result = v8::Undefined(p_isolate);
                if (args.Length() > 1) {
                    result = args[1];
                }
                resolver->Resolve(context, result).Check();
            }

            data->resolver.Reset();
            delete data;
        };

        v8::Local<v8::Function> callback =
            v8::Function::New(context, callback_wrapper, v8::External::New(p_isolate, data)).ToLocalChecked();

        // Call original with (args..., callback)
        int argc = args.Length();
        std::vector<v8::Local<v8::Value>> call_args;
        for (int i = 0; i < argc; i++) {
            call_args.push_back(args[i]);
        }
        call_args.push_back(callback);

        v8::TryCatch try_catch(p_isolate);
        v8::MaybeLocal<v8::Value> result =
            original->Call(context, v8::Undefined(p_isolate), (int) call_args.size(), call_args.data());

        if (try_catch.HasCaught()) {
            resolver->Reject(context, try_catch.Exception()).Check();
        }

        args.GetReturnValue().Set(resolver->GetPromise());
    };

    v8::Local<v8::Function> result = v8::Function::New(context, promisified_callback, original).ToLocalChecked();
    args.GetReturnValue().Set(result);
}

void Util::callbackify(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // Placeholder - implementation of callbackify
    v8::Isolate* p_isolate = args.GetIsolate();
    args.GetReturnValue().Set(v8::Undefined(p_isolate));
}

void Util::inherits(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    if (args.Length() < 2)
        return;

    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    v8::Local<v8::Object> constructor = args[0]->ToObject(context).ToLocalChecked();
    v8::Local<v8::Object> superConstructor = args[1]->ToObject(context).ToLocalChecked();

    v8::Local<v8::Value> prototype;
    if (superConstructor->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "prototype")).ToLocal(&prototype)) {
        constructor->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "super_"), superConstructor).Check();

        // This is a simplified inherits. In Node it's:
        // constructor.prototype = Object.create(superConstructor.prototype, { ... });
        // For Z8 we can do a basic prototype chain setup
        v8::Local<v8::Object> proto_obj = v8::Object::New(p_isolate);
        proto_obj->SetPrototypeV2(context, prototype).Check();
        constructor->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "prototype"), proto_obj).Check();
    }
}

void Util::inspect(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    if (args.Length() < 1)
        return;

    // Simplistic inspect: just stringify or tostring
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    v8::Local<v8::String> str;
    if (args[0]->IsObject()) {
        if (!v8::JSON::Stringify(context, args[0]).ToLocal(&str)) {
            args[0]->ToString(context).ToLocal(&str);
        }
    } else {
        args[0]->ToString(context).ToLocal(&str);
    }
    args.GetReturnValue().Set(str);
}

// Types
void Util::isDate(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsDate()));
}

void Util::isRegExp(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsRegExp()));
}

void Util::isPromise(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsPromise()));
}

void Util::isTypedArray(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsTypedArray()));
}

void Util::isUint8Array(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsUint8Array()));
}

void Util::isMap(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsMap()));
}

void Util::isSet(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsSet()));
}

void Util::isNativeError(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsNativeError()));
}

} // namespace module
} // namespace z8
