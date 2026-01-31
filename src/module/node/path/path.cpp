#include "path.h"
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace z8 {
namespace module {

v8::Local<v8::ObjectTemplate> Path::createTemplate(v8::Isolate* p_isolate) {
    v8::Local<v8::ObjectTemplate> tmpl = v8::ObjectTemplate::New(p_isolate);

    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "resolve"), v8::FunctionTemplate::New(p_isolate, resolve));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "join"), v8::FunctionTemplate::New(p_isolate, join));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "normalize"), v8::FunctionTemplate::New(p_isolate, normalize));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isAbsolute"),
              v8::FunctionTemplate::New(p_isolate, isAbsolute));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "relative"), v8::FunctionTemplate::New(p_isolate, relative));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "dirname"), v8::FunctionTemplate::New(p_isolate, dirname));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "basename"), v8::FunctionTemplate::New(p_isolate, basename));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "extname"), v8::FunctionTemplate::New(p_isolate, extname));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "parse"), v8::FunctionTemplate::New(p_isolate, parse));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "format"), v8::FunctionTemplate::New(p_isolate, format));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "toNamespacedPath"),
              v8::FunctionTemplate::New(p_isolate, toNamespacedPath));

    // Constants
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "sep"), v8::String::NewFromUtf8Literal(p_isolate, "\\"));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "delimiter"), v8::String::NewFromUtf8Literal(p_isolate, ";"));

    return tmpl;
}

void Path::resolve(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    fs::path resolved;
    bool absolute_found = false;

    for (int i = args.Length() - 1; i >= 0; --i) {
        v8::String::Utf8Value segment(p_isolate, args[i]);
        if (*segment == nullptr)
            continue;
        fs::path p_segment(*segment);
        if (p_segment.is_absolute()) {
            resolved = p_segment / resolved;
            absolute_found = true;
            break;
        } else {
            resolved = p_segment / resolved;
        }
    }

    if (!absolute_found) {
        resolved = fs::current_path() / resolved;
    }

    std::string result = resolved.lexically_normal().make_preferred().string();
    // Remove trailing slash if not root
    if (result.length() > 3 && (result.back() == '\\' || result.back() == '/')) {
        result.pop_back();
    }

    args.GetReturnValue().Set(v8::String::NewFromUtf8(p_isolate, result.c_str()).ToLocalChecked());
}

void Path::join(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    fs::path joined;

    for (int i = 0; i < args.Length(); ++i) {
        v8::String::Utf8Value segment(p_isolate, args[i]);
        if (*segment == nullptr || strlen(*segment) == 0)
            continue;
        if (i == 0) {
            joined = *segment;
        } else {
            joined /= *segment;
        }
    }

    std::string result = joined.lexically_normal().make_preferred().string();
    if (result.length() > 3 && (result.back() == '\\' || result.back() == '/')) {
        result.pop_back();
    }
    if (result.empty())
        result = ".";

    args.GetReturnValue().Set(v8::String::NewFromUtf8(p_isolate, result.c_str()).ToLocalChecked());
}

void Path::normalize(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    if (args.Length() < 1)
        return;
    v8::String::Utf8Value path_val(p_isolate, args[0]);
    fs::path p(*path_val);
    std::string result = p.lexically_normal().make_preferred().string();
    if (result.length() > 3 && (result.back() == '\\' || result.back() == '/')) {
        result.pop_back();
    }
    args.GetReturnValue().Set(v8::String::NewFromUtf8(p_isolate, result.c_str()).ToLocalChecked());
}

void Path::isAbsolute(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    if (args.Length() < 1)
        return;
    v8::String::Utf8Value path_val(p_isolate, args[0]);
    fs::path p(*path_val);
    args.GetReturnValue().Set(v8::Boolean::New(p_isolate, p.is_absolute()));
}

void Path::relative(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    if (args.Length() < 2)
        return;
    v8::String::Utf8Value from_val(p_isolate, args[0]);
    v8::String::Utf8Value to_val(p_isolate, args[1]);

    fs::path from = fs::absolute(*from_val);
    fs::path to = fs::absolute(*to_val);

    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(p_isolate, fs::relative(to, from).make_preferred().string().c_str()).ToLocalChecked());
}

void Path::dirname(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    if (args.Length() < 1)
        return;
    v8::String::Utf8Value path_val(p_isolate, args[0]);
    fs::path p(*path_val);
    args.GetReturnValue().Set(v8::String::NewFromUtf8(p_isolate, p.parent_path().string().c_str()).ToLocalChecked());
}

void Path::basename(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    if (args.Length() < 1)
        return;
    v8::String::Utf8Value path_val(p_isolate, args[0]);
    fs::path p(*path_val);
    std::string base = p.filename().string();

    if (args.Length() > 1 && args[1]->IsString()) {
        v8::String::Utf8Value ext_val(p_isolate, args[1]);
        std::string ext(*ext_val);
        if (base.length() >= ext.length() && base.substr(base.length() - ext.length()) == ext) {
            base = base.substr(0, base.length() - ext.length());
        }
    }

    args.GetReturnValue().Set(v8::String::NewFromUtf8(p_isolate, base.c_str()).ToLocalChecked());
}

void Path::extname(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    if (args.Length() < 1)
        return;
    v8::String::Utf8Value path_val(p_isolate, args[0]);
    fs::path p(*path_val);
    args.GetReturnValue().Set(v8::String::NewFromUtf8(p_isolate, p.extension().string().c_str()).ToLocalChecked());
}

void Path::parse(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    if (args.Length() < 1)
        return;
    v8::String::Utf8Value path_val(p_isolate, args[0]);
    fs::path p(*path_val);

    v8::Local<v8::Object> obj = v8::Object::New(p_isolate);
    std::string root = p.root_path().make_preferred().string();
    // Node.js usually returns C:\ (with backslash)

    obj->Set(context,
             v8::String::NewFromUtf8Literal(p_isolate, "root"),
             v8::String::NewFromUtf8(p_isolate, root.c_str()).ToLocalChecked())
        .Check();
    obj->Set(context,
             v8::String::NewFromUtf8Literal(p_isolate, "dir"),
             v8::String::NewFromUtf8(p_isolate, p.parent_path().make_preferred().string().c_str()).ToLocalChecked())
        .Check();
    obj->Set(context,
             v8::String::NewFromUtf8Literal(p_isolate, "base"),
             v8::String::NewFromUtf8(p_isolate, p.filename().make_preferred().string().c_str()).ToLocalChecked())
        .Check();
    obj->Set(context,
             v8::String::NewFromUtf8Literal(p_isolate, "ext"),
             v8::String::NewFromUtf8(p_isolate, p.extension().make_preferred().string().c_str()).ToLocalChecked())
        .Check();
    obj->Set(context,
             v8::String::NewFromUtf8Literal(p_isolate, "name"),
             v8::String::NewFromUtf8(p_isolate, p.stem().make_preferred().string().c_str()).ToLocalChecked())
        .Check();

    args.GetReturnValue().Set(obj);
}

void Path::format(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    if (args.Length() < 1 || !args[0]->IsObject())
        return;

    v8::Local<v8::Object> obj = args[0].As<v8::Object>();
    v8::Local<v8::Value> dir, base, root, name, ext;

    obj->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "dir")).ToLocal(&dir);
    obj->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "base")).ToLocal(&base);
    obj->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "root")).ToLocal(&root);
    obj->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "name")).ToLocal(&name);
    obj->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "ext")).ToLocal(&ext);

    std::string result;
    if (dir->IsString()) {
        v8::String::Utf8Value dir_str(p_isolate, dir);
        result = *dir_str;
        if (result.back() != '\\' && result.back() != '/') {
            result += "\\";
        }
    } else if (root->IsString()) {
        v8::String::Utf8Value root_str(p_isolate, root);
        result = *root_str;
    }

    if (base->IsString()) {
        v8::String::Utf8Value base_str(p_isolate, base);
        result += *base_str;
    } else {
        if (name->IsString()) {
            v8::String::Utf8Value name_str(p_isolate, name);
            result += *name_str;
        }
        if (ext->IsString()) {
            v8::String::Utf8Value ext_str(p_isolate, ext);
            result += *ext_str;
        }
    }

    args.GetReturnValue().Set(v8::String::NewFromUtf8(p_isolate, result.c_str()).ToLocalChecked());
}

void Path::toNamespacedPath(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // Basic implementation for now
    args.GetReturnValue().Set(args[0]);
}

} // namespace module
} // namespace z8
