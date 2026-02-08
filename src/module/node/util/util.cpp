#include "util.h"
#include <cstdint>
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

    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isAnyArrayBuffer"),
              v8::FunctionTemplate::New(p_isolate, isAnyArrayBuffer));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isArgumentsObject"),
              v8::FunctionTemplate::New(p_isolate, isArgumentsObject));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isArrayBuffer"),
              v8::FunctionTemplate::New(p_isolate, isArrayBuffer));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isAsyncFunction"),
              v8::FunctionTemplate::New(p_isolate, isAsyncFunction));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isBigInt64Array"),
              v8::FunctionTemplate::New(p_isolate, isBigInt64Array));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isBigUint64Array"),
              v8::FunctionTemplate::New(p_isolate, isBigUint64Array));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isBooleanObject"),
              v8::FunctionTemplate::New(p_isolate, isBooleanObject));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isBoxedPrimitive"),
              v8::FunctionTemplate::New(p_isolate, isBoxedPrimitive));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isDataView"),
              v8::FunctionTemplate::New(p_isolate, isDataView));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isDate"), v8::FunctionTemplate::New(p_isolate, isDate));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isExternal"),
              v8::FunctionTemplate::New(p_isolate, isExternal));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isFloat32Array"),
              v8::FunctionTemplate::New(p_isolate, isFloat32Array));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isFloat64Array"),
              v8::FunctionTemplate::New(p_isolate, isFloat64Array));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isGeneratorFunction"),
              v8::FunctionTemplate::New(p_isolate, isGeneratorFunction));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isGeneratorObject"),
              v8::FunctionTemplate::New(p_isolate, isGeneratorObject));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isInt8Array"),
              v8::FunctionTemplate::New(p_isolate, isInt8Array));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isInt16Array"),
              v8::FunctionTemplate::New(p_isolate, isInt16Array));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isInt32Array"),
              v8::FunctionTemplate::New(p_isolate, isInt32Array));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isMap"), v8::FunctionTemplate::New(p_isolate, isMap));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isMapIterator"),
              v8::FunctionTemplate::New(p_isolate, isMapIterator));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isModuleNamespaceObject"),
              v8::FunctionTemplate::New(p_isolate, isModuleNamespaceObject));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isNativeError"),
              v8::FunctionTemplate::New(p_isolate, isNativeError));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isNumberObject"),
              v8::FunctionTemplate::New(p_isolate, isNumberObject));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isPromise"), v8::FunctionTemplate::New(p_isolate, isPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isProxy"), v8::FunctionTemplate::New(p_isolate, isProxy));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isRegExp"), v8::FunctionTemplate::New(p_isolate, isRegExp));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isSet"), v8::FunctionTemplate::New(p_isolate, isSet));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isSetIterator"),
              v8::FunctionTemplate::New(p_isolate, isSetIterator));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isSharedArrayBuffer"),
              v8::FunctionTemplate::New(p_isolate, isSharedArrayBuffer));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isStringObject"),
              v8::FunctionTemplate::New(p_isolate, isStringObject));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isSymbolObject"),
              v8::FunctionTemplate::New(p_isolate, isSymbolObject));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isTypedArray"),
              v8::FunctionTemplate::New(p_isolate, isTypedArray));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isUint8Array"),
              v8::FunctionTemplate::New(p_isolate, isUint8Array));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isUint8ClampedArray"),
              v8::FunctionTemplate::New(p_isolate, isUint8ClampedArray));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isUint16Array"),
              v8::FunctionTemplate::New(p_isolate, isUint16Array));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isUint32Array"),
              v8::FunctionTemplate::New(p_isolate, isUint32Array));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isWeakMap"), v8::FunctionTemplate::New(p_isolate, isWeakMap));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isWeakSet"), v8::FunctionTemplate::New(p_isolate, isWeakSet));

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
    int32_t arg_index = 1;

    for (size_t i = 0; i < fmt_str.length(); ++i) {
        if (fmt_str[i] == '%' && i + 1 < fmt_str.length() && arg_index < args.Length()) {
            char spec = fmt_str[i + 1];
            v8::Local<v8::Value> arg = args[arg_index++];

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
    for (; arg_index < args.Length(); ++arg_index) {
        v8::String::Utf8Value val(p_isolate, args[arg_index]);
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

    auto promisified_callback = [](const v8::FunctionCallbackInfo<v8::Value>& args) {
        v8::Isolate* p_isolate = args.GetIsolate();
        v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
        v8::Local<v8::Function> original = v8::Local<v8::Function>::Cast(args.Data());

        v8::Local<v8::Promise::Resolver> resolver =
            v8::Local<v8::Promise::Resolver>::New(p_isolate, v8::Promise::Resolver::New(context).ToLocalChecked());

        struct CallbackData {
            v8::Global<v8::Promise::Resolver> resolver;
        };
        auto* data = new CallbackData{v8::Global<v8::Promise::Resolver>(p_isolate, resolver)};

        auto callback_wrapper = [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            v8::Isolate* p_isolate = args.GetIsolate();
            v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
            auto* p_cb_data = static_cast<CallbackData*>(v8::Local<v8::External>::Cast(args.Data())->Value());

            v8::Local<v8::Promise::Resolver> resolver = p_cb_data->resolver.Get(p_isolate);

            if (args.Length() > 0 && !args[0]->IsNull() && !args[0]->IsUndefined()) {
                resolver->Reject(context, args[0]).Check();
            } else {
                v8::Local<v8::Value> result = v8::Undefined(p_isolate);
                if (args.Length() > 1) {
                    result = args[1];
                }
                resolver->Resolve(context, result).Check();
            }

            p_cb_data->resolver.Reset();
            delete p_cb_data;
        };

        v8::Local<v8::Function> callback =
            v8::Function::New(context, callback_wrapper, v8::External::New(p_isolate, data)).ToLocalChecked();

        std::vector<v8::Local<v8::Value>> call_args;
        for (int32_t i = 0; i < args.Length(); i++) {
            call_args.push_back(args[i]);
        }
        call_args.push_back(callback);

        v8::TryCatch try_catch(p_isolate);
        v8::MaybeLocal<v8::Value> result =
            original->Call(context, v8::Undefined(p_isolate), (int32_t) call_args.size(), call_args.data());

        if (try_catch.HasCaught()) {
            resolver->Reject(context, try_catch.Exception()).Check();
        }

        args.GetReturnValue().Set(resolver->GetPromise());
    };

    v8::Local<v8::Function> result = v8::Function::New(context, promisified_callback, original).ToLocalChecked();
    args.GetReturnValue().Set(result);
}

void Util::callbackify(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    if (args.Length() < 1 || !args[0]->IsFunction()) {
        p_isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(p_isolate, "The \"original\" argument must be of type function")));
        return;
    }

    v8::Local<v8::Function> original = args[0].As<v8::Function>();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();

    auto callbackified_wrapper = [](const v8::FunctionCallbackInfo<v8::Value>& args) {
        v8::Isolate* p_isolate = args.GetIsolate();
        v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
        v8::Local<v8::Function> original = v8::Local<v8::Function>::Cast(args.Data());

        if (args.Length() < 1 || !args[args.Length() - 1]->IsFunction()) {
            p_isolate->ThrowException(v8::Exception::TypeError(
                v8::String::NewFromUtf8Literal(p_isolate, "The last argument must be a function")));
            return;
        }

        v8::Local<v8::Function> callback = args[args.Length() - 1].As<v8::Function>();

        std::vector<v8::Local<v8::Value>> call_args;
        for (int32_t i = 0; i < args.Length() - 1; i++) {
            call_args.push_back(args[i]);
        }

        v8::TryCatch try_catch(p_isolate);
        v8::MaybeLocal<v8::Value> result_maybe =
            original->Call(context, v8::Undefined(p_isolate), (int32_t) call_args.size(), call_args.data());

        if (try_catch.HasCaught()) {
            v8::Local<v8::Value> cb_args[] = {try_catch.Exception()};
            v8::MaybeLocal<v8::Value> unused = callback->Call(context, v8::Undefined(p_isolate), 1, cb_args);
            return;
        }

        v8::Local<v8::Value> result = result_maybe.ToLocalChecked();
        if (result->IsPromise()) {
            v8::Local<v8::Promise> promise = result.As<v8::Promise>();

            struct CallbackData {
                v8::Isolate* p_isolate;
                v8::Global<v8::Function> callback;
            };
            auto* p_data = new CallbackData{p_isolate, v8::Global<v8::Function>(p_isolate, callback)};

            auto on_resolved = [](const v8::FunctionCallbackInfo<v8::Value>& args) {
                auto* p_data = static_cast<CallbackData*>(v8::Local<v8::External>::Cast(args.Data())->Value());
                v8::Isolate* p_isolate = p_data->p_isolate;
                v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
                v8::Local<v8::Function> callback = p_data->callback.Get(p_isolate);

                v8::Local<v8::Value> cb_args[] = {v8::Null(p_isolate), args[0]};
                v8::MaybeLocal<v8::Value> unused = callback->Call(context, v8::Undefined(p_isolate), 2, cb_args);

                p_data->callback.Reset();
                delete p_data;
            };

            auto on_rejected = [](const v8::FunctionCallbackInfo<v8::Value>& args) {
                auto* p_data = static_cast<CallbackData*>(v8::Local<v8::External>::Cast(args.Data())->Value());
                v8::Isolate* p_isolate = p_data->p_isolate;
                v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
                v8::Local<v8::Function> callback = p_data->callback.Get(p_isolate);

                v8::Local<v8::Value> cb_args[] = {args[0]};
                v8::MaybeLocal<v8::Value> unused = callback->Call(context, v8::Undefined(p_isolate), 1, cb_args);

                p_data->callback.Reset();
                delete p_data;
            };

            v8::Local<v8::Function> resolve_fn =
                v8::Function::New(context, on_resolved, v8::External::New(p_isolate, p_data)).ToLocalChecked();
            v8::Local<v8::Function> reject_fn =
                v8::Function::New(context, on_rejected, v8::External::New(p_isolate, p_data)).ToLocalChecked();

            v8::MaybeLocal<v8::Promise> unused = promise->Then(context, resolve_fn, reject_fn);
        } else {
            v8::Local<v8::Value> cb_args[] = {v8::Null(p_isolate), result};
            v8::MaybeLocal<v8::Value> unused = callback->Call(context, v8::Undefined(p_isolate), 2, cb_args);
        }
    };

    v8::Local<v8::Function> result = v8::Function::New(context, callbackified_wrapper, original).ToLocalChecked();
    args.GetReturnValue().Set(result);
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
        v8::Local<v8::Object> proto_obj = v8::Object::New(p_isolate);
        proto_obj->SetPrototypeV2(context, prototype).Check();
        constructor->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "prototype"), proto_obj).Check();
    }
}

void Util::inspect(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    if (args.Length() < 1)
        return;

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

// Types Implementation
void Util::isAnyArrayBuffer(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(
        args.GetIsolate(), args.Length() > 0 && (args[0]->IsArrayBuffer() || args[0]->IsSharedArrayBuffer())));
}
void Util::isArgumentsObject(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsArgumentsObject()));
}
void Util::isArrayBuffer(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsArrayBuffer()));
}
void Util::isAsyncFunction(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsAsyncFunction()));
}
void Util::isBigInt64Array(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsBigInt64Array()));
}
void Util::isBigUint64Array(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsBigUint64Array()));
}
void Util::isBooleanObject(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsBooleanObject()));
}
void Util::isBoxedPrimitive(const v8::FunctionCallbackInfo<v8::Value>& args) {
    bool res = false;
    if (args.Length() > 0) {
        v8::Local<v8::Value> val = args[0];
        res = val->IsBooleanObject() || val->IsNumberObject() || val->IsStringObject() || val->IsSymbolObject() ||
              val->IsBigIntObject();
    }
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), res));
}
void Util::isDataView(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsDataView()));
}
void Util::isDate(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsDate()));
}
void Util::isExternal(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsExternal()));
}
void Util::isFloat32Array(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsFloat32Array()));
}
void Util::isFloat64Array(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsFloat64Array()));
}
void Util::isGeneratorFunction(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsGeneratorFunction()));
}
void Util::isGeneratorObject(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsGeneratorObject()));
}
void Util::isInt8Array(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsInt8Array()));
}
void Util::isInt16Array(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsInt16Array()));
}
void Util::isInt32Array(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsInt32Array()));
}
void Util::isMap(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsMap()));
}
void Util::isMapIterator(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsMapIterator()));
}
void Util::isModuleNamespaceObject(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(
        v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsModuleNamespaceObject()));
}
void Util::isNativeError(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsNativeError()));
}
void Util::isNumberObject(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsNumberObject()));
}
void Util::isPromise(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsPromise()));
}
void Util::isProxy(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsProxy()));
}
void Util::isRegExp(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsRegExp()));
}
void Util::isSet(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsSet()));
}
void Util::isSetIterator(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsSetIterator()));
}
void Util::isSharedArrayBuffer(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsSharedArrayBuffer()));
}
void Util::isStringObject(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsStringObject()));
}
void Util::isSymbolObject(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsSymbolObject()));
}
void Util::isTypedArray(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsTypedArray()));
}
void Util::isUint8Array(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsUint8Array()));
}
void Util::isUint8ClampedArray(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsUint8ClampedArray()));
}
void Util::isUint16Array(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsUint16Array()));
}
void Util::isUint32Array(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsUint32Array()));
}
void Util::isWeakMap(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsWeakMap()));
}
void Util::isWeakSet(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), args.Length() > 0 && args[0]->IsWeakSet()));
}

} // namespace module
} // namespace z8
