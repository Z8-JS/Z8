#include "buffer.h"
#include <cstring>
#include <vector>

namespace z8 {
namespace module {

// Helper for Hex
static int32_t hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static std::string bytesToHex(const uint8_t* p_data, size_t len) {
    static const char hex_chars[] = "0123456789abcdef";
    std::string res;
    res.reserve(len * 2);
    for (size_t i = 0; i < len; i++) {
        res += hex_chars[p_data[i] >> 4];
        res += hex_chars[p_data[i] & 0x0F];
    }
    return res;
}

// Helper for Base64 (Basic)
static const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

static std::string bytesToBase64(const uint8_t* p_data, size_t len) {
    std::string res;
    int32_t i = 0;
    int32_t j = 0;
    uint8_t char_array_3[3];
    uint8_t char_array_4[4];

    while (len--) {
        char_array_3[i++] = *(p_data++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; (i <4) ; i++)
                res += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for(j = i; j < 3; j++) char_array_3[j] = '\0';
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;
        for (j = 0; (j < i + 1); j++) res += base64_chars[char_array_4[j]];
        while((i++ < 3)) res += '=';
    }
    return res;
}

static std::vector<uint8_t> base64ToBytes(const std::string& encoded_string) {
    int32_t in_len = static_cast<int32_t>(encoded_string.size());
    int32_t i = 0;
    int32_t j = 0;
    int32_t in_ = 0;
    uint8_t char_array_4[4], char_array_3[3];
    std::vector<uint8_t> ret;

    while (in_len-- && ( encoded_string[in_] != '=') && (isalnum(encoded_string[in_]) || (encoded_string[in_] == '+') || (encoded_string[in_] == '/'))) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i == 4) {
            for (i = 0; i <4; i++)
                char_array_4[i] = static_cast<uint8_t>(base64_chars.find(char_array_4[i]));

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++) ret.push_back(char_array_3[i]);
            i = 0;
        }
    }

    if (i) {
        for (j = i; j <4; j++) char_array_4[j] = 0;
        for (j = 0; j <4; j++) char_array_4[j] = (uint8_t)base64_chars.find(char_array_4[j]);
        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
        for (j = 0; (j < i - 1); j++) ret.push_back(char_array_3[j]);
    }
    return ret;
}

v8::Local<v8::FunctionTemplate> Buffer::createTemplate(v8::Isolate* p_isolate) {
    v8::Local<v8::FunctionTemplate> tmpl = v8::FunctionTemplate::New(p_isolate, from);

    tmpl->SetClassName(v8::String::NewFromUtf8Literal(p_isolate, "Buffer"));

    // Static methods
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "alloc"), v8::FunctionTemplate::New(p_isolate, alloc));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "allocUnsafe"), v8::FunctionTemplate::New(p_isolate, allocUnsafe));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "from"), v8::FunctionTemplate::New(p_isolate, from));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "concat"), v8::FunctionTemplate::New(p_isolate, concat));
    tmpl->Set(v8::String::NewFromUtf8Literal(p_isolate, "isBuffer"), v8::FunctionTemplate::New(p_isolate, isBuffer));

    // Prototype methods (will be attached to instances)
    v8::Local<v8::ObjectTemplate> proto = tmpl->PrototypeTemplate();
    proto->Set(v8::String::NewFromUtf8Literal(p_isolate, "toString"), v8::FunctionTemplate::New(p_isolate, toString));
    proto->Set(v8::String::NewFromUtf8Literal(p_isolate, "write"), v8::FunctionTemplate::New(p_isolate, write));
    proto->Set(v8::String::NewFromUtf8Literal(p_isolate, "fill"), v8::FunctionTemplate::New(p_isolate, fill));
    proto->Set(v8::String::NewFromUtf8Literal(p_isolate, "copy"), v8::FunctionTemplate::New(p_isolate, copy));
    proto->Set(v8::String::NewFromUtf8Literal(p_isolate, "slice"), v8::FunctionTemplate::New(p_isolate, slice));

    return tmpl;
}

void Buffer::initialize(v8::Isolate* p_isolate, v8::Local<v8::Context> context) {
    v8::Local<v8::FunctionTemplate> tmpl = createTemplate(p_isolate);
    v8::Local<v8::Function> buffer_fn = tmpl->GetFunction(context).ToLocalChecked();
    
    // Set Buffer as a global
    context->Global()->Set(context, v8::String::NewFromUtf8Literal(p_isolate, "Buffer"), buffer_fn).Check();

    // In Node.js, Buffer.prototype inherits from Uint8Array.prototype
    // We can try to set the prototype of Buffer.prototype to Uint8Array.prototype
    v8::Local<v8::Value> u8_proto = context->Global()->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "Uint8Array"))
        .ToLocalChecked().As<v8::Function>()->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "prototype")).ToLocalChecked();
    
    v8::Local<v8::Value> buffer_proto = buffer_fn->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "prototype")).ToLocalChecked();
    buffer_proto.As<v8::Object>()->SetPrototypeV2(context, u8_proto).Check();
}

v8::Local<v8::Uint8Array> Buffer::createBuffer(v8::Isolate* p_isolate, size_t length) {
    v8::Local<v8::ArrayBuffer> ab = v8::ArrayBuffer::New(p_isolate, length);
    v8::Local<v8::Uint8Array> ui = v8::Uint8Array::New(ab, 0, length);
    
    // Set the prototype to Buffer.prototype so it gets the Buffer methods
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    v8::Local<v8::Value> buffer_val = context->Global()->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "Buffer")).ToLocalChecked();
    if (buffer_val->IsFunction()) {
        v8::Local<v8::Value> proto = buffer_val.As<v8::Function>()->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "prototype")).ToLocalChecked();
        ui->SetPrototypeV2(context, proto).Check();
    }
    
    return ui;
}

void Buffer::alloc(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    if (args.Length() < 1 || !args[0]->IsNumber()) {
        p_isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8Literal(p_isolate, "The \"size\" argument must be of type number")));
        return;
    }

    size_t length = static_cast<size_t>(args[0]->IntegerValue(p_isolate->GetCurrentContext()).FromMaybe(0));
    v8::Local<v8::Uint8Array> ui = createBuffer(p_isolate, length);

    if (args.Length() > 1) {
        // Handle fill
        // TODO: Implement filling logic
    }

    args.GetReturnValue().Set(ui);
}

void Buffer::allocUnsafe(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // For now, same as alloc in Z8 for simplicity
    alloc(args);
}

void Buffer::from(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();

    if (args.Length() < 1) {
        p_isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8Literal(p_isolate, "The first argument must be one of type string, Buffer, ArrayBuffer, Array, or Array-like object.")));
        return;
    }

    v8::Local<v8::Value> input = args[0];

    if (input->IsString()) {
        v8::String::Utf8Value str(p_isolate, input);
        std::string encoding = "utf8";
        if (args.Length() > 1 && args[1]->IsString()) {
            v8::String::Utf8Value enc_str(p_isolate, args[1]);
            encoding = *enc_str;
        }

        if (encoding == "hex") {
            std::string hex_str(*str);
            size_t len = hex_str.length() / 2;
            v8::Local<v8::Uint8Array> ui = createBuffer(p_isolate, len);
            uint8_t* p_data = static_cast<uint8_t*>(ui->Buffer()->GetBackingStore()->Data());
            for (size_t i = 0; i < len; i++) {
                int32_t high = hexValue(hex_str[i * 2]);
                int32_t low = hexValue(hex_str[i * 2 + 1]);
                if (high == -1 || low == -1) break;
                p_data[i] = static_cast<uint8_t>((high << 4) | low);
            }
            args.GetReturnValue().Set(ui);
            return;
        } else if (encoding == "base64") {
            std::vector<uint8_t> bytes = base64ToBytes(*str);
            v8::Local<v8::Uint8Array> ui = createBuffer(p_isolate, bytes.size());
            memcpy(ui->Buffer()->GetBackingStore()->Data(), bytes.data(), bytes.size());
            args.GetReturnValue().Set(ui);
            return;
        }

        size_t len = str.length();
        v8::Local<v8::Uint8Array> ui = createBuffer(p_isolate, len);
        void* p_data = ui->Buffer()->GetBackingStore()->Data();
        memcpy(p_data, *str, len);
        args.GetReturnValue().Set(ui);
        return;
    }

    if (input->IsArray()) {
        v8::Local<v8::Array> arr = input.As<v8::Array>();
        uint32_t len = arr->Length();
        v8::Local<v8::Uint8Array> ui = createBuffer(p_isolate, len);
        uint8_t* p_data = static_cast<uint8_t*>(ui->Buffer()->GetBackingStore()->Data());
        for (uint32_t i = 0; i < len; i++) {
            p_data[i] = static_cast<uint8_t>(arr->Get(context, i).ToLocalChecked()->Int32Value(context).FromMaybe(0));
        }
        args.GetReturnValue().Set(ui);
        return;
    }

    if (input->IsArrayBuffer()) {
        v8::Local<v8::ArrayBuffer> ab = input.As<v8::ArrayBuffer>();
        size_t byte_offset = 0;
        size_t length = ab->ByteLength();
        if (args.Length() > 1 && args[1]->IsNumber()) {
            byte_offset = static_cast<size_t>(args[1]->IntegerValue(context).FromMaybe(0));
        }
        if (args.Length() > 2 && args[2]->IsNumber()) {
            length = static_cast<size_t>(args[2]->IntegerValue(context).FromMaybe(0));
        }
        v8::Local<v8::Uint8Array> ui = v8::Uint8Array::New(ab, byte_offset, length);
        
        // Re-prototype it
        v8::Local<v8::Value> buffer_val = context->Global()->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "Buffer")).ToLocalChecked();
        if (buffer_val->IsFunction()) {
            v8::Local<v8::Value> proto = buffer_val.As<v8::Function>()->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "prototype")).ToLocalChecked();
            ui->SetPrototypeV2(context, proto).Check();
        }
        
        args.GetReturnValue().Set(ui);
        return;
    }
    
    if (input->IsUint8Array()) {
        v8::Local<v8::Uint8Array> src = input.As<v8::Uint8Array>();
        size_t len = src->ByteLength();
        v8::Local<v8::Uint8Array> ui = createBuffer(p_isolate, len);
        void* p_dst_data = ui->Buffer()->GetBackingStore()->Data();
        void* p_src_data = src->Buffer()->GetBackingStore()->Data();
        memcpy(p_dst_data, (uint8_t*)p_src_data + src->ByteOffset(), len);
        args.GetReturnValue().Set(ui);
        return;
    }

    p_isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8Literal(p_isolate, "Unsupported type for Buffer.from")));
}

void Buffer::concat(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsArray()) {
        p_isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8Literal(p_isolate, "The \"list\" argument must be an instance of Array")));
        return;
    }

    v8::Local<v8::Array> list = args[0].As<v8::Array>();
    uint32_t list_len = list->Length();
    
    size_t total_length = 0;
    if (args.Length() > 1 && args[1]->IsNumber()) {
        total_length = static_cast<size_t>(args[1]->IntegerValue(context).FromMaybe(0));
    } else {
        for (uint32_t i = 0; i < list_len; i++) {
            v8::Local<v8::Value> item = list->Get(context, i).ToLocalChecked();
            if (item->IsUint8Array()) {
                total_length += item.As<v8::Uint8Array>()->ByteLength();
            }
        }
    }

    v8::Local<v8::Uint8Array> result = createBuffer(p_isolate, total_length);
    uint8_t* p_dst_data = static_cast<uint8_t*>(result->Buffer()->GetBackingStore()->Data());
    
    size_t offset = 0;
    for (uint32_t i = 0; i < list_len && offset < total_length; i++) {
        v8::Local<v8::Value> item = list->Get(context, i).ToLocalChecked();
        if (item->IsUint8Array()) {
            v8::Local<v8::Uint8Array> src = item.As<v8::Uint8Array>();
            size_t to_copy = std::min(src->ByteLength(), total_length - offset);
            void* p_src_data = src->Buffer()->GetBackingStore()->Data();
            memcpy(p_dst_data + offset, (uint8_t*)p_src_data + src->ByteOffset(), to_copy);
            offset += to_copy;
        }
    }

    args.GetReturnValue().Set(result);
}

void Buffer::isBuffer(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    if (args.Length() < 1) {
        args.GetReturnValue().Set(v8::Boolean::New(p_isolate, false));
        return;
    }
    
    v8::Local<v8::Value> val = args[0];
    if (!val->IsUint8Array()) {
        args.GetReturnValue().Set(v8::Boolean::New(p_isolate, false));
        return;
    }

    // Check if prototype is Buffer.prototype
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    v8::Local<v8::Value> buffer_val = context->Global()->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "Buffer")).ToLocalChecked();
    if (buffer_val->IsFunction()) {
        v8::Local<v8::Value> buffer_proto = buffer_val.As<v8::Function>()->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "prototype")).ToLocalChecked();
        v8::Local<v8::Value> obj_proto = val.As<v8::Object>()->GetPrototypeV2();
        args.GetReturnValue().Set(v8::Boolean::New(p_isolate, obj_proto->StrictEquals(buffer_proto)));
        return;
    }

    args.GetReturnValue().Set(v8::Boolean::New(p_isolate, false));
}

void Buffer::toString(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Object> self = args.This();
    
    if (!self->IsUint8Array()) {
        p_isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8Literal(p_isolate, "Method toString called on incompatible receiver")));
        return;
    }

    v8::Local<v8::Uint8Array> ui = self.As<v8::Uint8Array>();
    uint8_t* p_data = static_cast<uint8_t*>(ui->Buffer()->GetBackingStore()->Data());
    size_t len = ui->ByteLength();
    size_t offset = ui->ByteOffset();

    std::string encoding = "utf8";
    if (args.Length() > 0 && args[0]->IsString()) {
        v8::String::Utf8Value enc_str(p_isolate, args[0]);
        encoding = *enc_str;
    }

    if (encoding == "hex") {
        std::string hex = bytesToHex(p_data + offset, len);
        args.GetReturnValue().Set(v8::String::NewFromUtf8(p_isolate, hex.c_str()).ToLocalChecked());
        return;
    } else if (encoding == "base64") {
        std::string b64 = bytesToBase64(p_data + offset, len);
        args.GetReturnValue().Set(v8::String::NewFromUtf8(p_isolate, b64.c_str()).ToLocalChecked());
        return;
    }

    // Default to utf8
    args.GetReturnValue().Set(v8::String::NewFromUtf8(p_isolate, (const char*)p_data + offset, v8::NewStringType::kNormal, static_cast<int32_t>(len)).ToLocalChecked());
}

void Buffer::write(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    if (args.Length() < 1 || !args[0]->IsString()) {
        args.GetReturnValue().Set(0);
        return;
    }

    v8::Local<v8::Uint8Array> self = args.This().As<v8::Uint8Array>();
    uint8_t* p_data = static_cast<uint8_t*>(self->Buffer()->GetBackingStore()->Data()) + self->ByteOffset();
    size_t length = self->ByteLength();

    size_t offset = 0;
    if (args.Length() > 1 && args[1]->IsNumber()) {
        offset = static_cast<size_t>(args[1]->IntegerValue(context).FromMaybe(0));
    }
    
    if (offset >= length) {
        args.GetReturnValue().Set(0);
        return;
    }

    v8::String::Utf8Value str(p_isolate, args[0]);
    size_t to_write = std::min((size_t)str.length(), length - offset);
    
    // Check for encoding
    std::string encoding = "utf8";
    if (args.Length() > 2 && args[2]->IsString()) {
        v8::String::Utf8Value enc_str(p_isolate, args[2]);
        encoding = *enc_str;
    }

    if (encoding == "hex") {
        std::string hex_str(*str);
        size_t hex_len = hex_str.length() / 2;
        to_write = std::min(hex_len, length - offset);
        for (size_t i = 0; i < to_write; i++) {
            int32_t high = hexValue(hex_str[i * 2]);
            int32_t low = hexValue(hex_str[i * 2 + 1]);
            if (high == -1 || low == -1) break;
            p_data[offset + i] = static_cast<uint8_t>((high << 4) | low);
        }
    } else if (encoding == "base64") {
        std::vector<uint8_t> bytes = base64ToBytes(*str);
        to_write = std::min(bytes.size(), length - offset);
        memcpy(p_data + offset, bytes.data(), to_write);
    } else {
        memcpy(p_data + offset, *str, to_write);
    }

    args.GetReturnValue().Set(v8::Integer::New(p_isolate, static_cast<int32_t>(to_write)));
}

void Buffer::fill(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    v8::Local<v8::Uint8Array> self = args.This().As<v8::Uint8Array>();
    uint8_t* p_data = static_cast<uint8_t*>(self->Buffer()->GetBackingStore()->Data()) + self->ByteOffset();
    size_t length = self->ByteLength();

    if (args.Length() < 1) {
        memset(p_data, 0, length);
    } else {
        uint8_t val = 0;
        if (args[0]->IsNumber()) {
            val = static_cast<uint8_t>(args[0]->Uint32Value(context).FromMaybe(0));
        } else if (args[0]->IsString()) {
            v8::String::Utf8Value str(p_isolate, args[0]);
            if (str.length() > 0) val = static_cast<uint8_t>((*str)[0]);
        } else if (args[0]->IsUint8Array()) {
            // Fill with first byte of Uint8Array for simplicity
            v8::Local<v8::Uint8Array> fill_buf = args[0].As<v8::Uint8Array>();
            if (fill_buf->ByteLength() > 0) {
                val = static_cast<uint8_t*>(fill_buf->Buffer()->GetBackingStore()->Data())[fill_buf->ByteOffset()];
            }
        }
        memset(p_data, val, length);
    }
    args.GetReturnValue().Set(args.This());
}

void Buffer::copy(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsUint8Array()) {
        p_isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8Literal(p_isolate, "The \"target\" argument must be an instance of Uint8Array")));
        return;
    }

    v8::Local<v8::Uint8Array> self = args.This().As<v8::Uint8Array>();
    v8::Local<v8::Uint8Array> target = args[0].As<v8::Uint8Array>();

    size_t target_start = 0;
    size_t source_start = 0;
    size_t source_end = self->ByteLength();

    if (args.Length() > 1 && args[1]->IsNumber()) {
        target_start = static_cast<size_t>(args[1]->IntegerValue(context).FromMaybe(0));
    }
    if (args.Length() > 2 && args[2]->IsNumber()) {
        source_start = static_cast<size_t>(args[2]->IntegerValue(context).FromMaybe(0));
    }
    if (args.Length() > 3 && args[3]->IsNumber()) {
        source_end = static_cast<size_t>(args[3]->IntegerValue(context).FromMaybe(static_cast<int64_t>(source_end)));
    }

    if (source_start >= self->ByteLength() || target_start >= target->ByteLength()) {
        args.GetReturnValue().Set(v8::Integer::New(p_isolate, 0));
        return;
    }

    size_t length = std::min(source_end - source_start, self->ByteLength() - source_start);
    length = std::min(length, target->ByteLength() - target_start);

    if (length > 0) {
        uint8_t* p_src = static_cast<uint8_t*>(self->Buffer()->GetBackingStore()->Data()) + self->ByteOffset() + source_start;
        uint8_t* p_dst = static_cast<uint8_t*>(target->Buffer()->GetBackingStore()->Data()) + target->ByteOffset() + target_start;
        memmove(p_dst, p_src, length);
    }

    args.GetReturnValue().Set(v8::Integer::New(p_isolate, static_cast<int32_t>(length)));
}

void Buffer::slice(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* p_isolate = args.GetIsolate();
    v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
    v8::Local<v8::Object> self = args.This();

    if (!self->IsUint8Array()) {
        p_isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8Literal(p_isolate, "Method slice called on incompatible receiver")));
        return;
    }

    v8::Local<v8::Uint8Array> ui = self.As<v8::Uint8Array>();
    int64_t start = 0;
    int64_t end = ui->ByteLength();

    if (args.Length() > 0 && args[0]->IsNumber()) {
        start = args[0]->IntegerValue(context).FromMaybe(0);
        if (start < 0) start += ui->ByteLength();
        if (start < 0) start = 0;
    }

    if (args.Length() > 1 && args[1]->IsNumber()) {
        end = args[1]->IntegerValue(context).FromMaybe(ui->ByteLength());
        if (end < 0) end += ui->ByteLength();
        if (end > (int64_t)ui->ByteLength()) end = ui->ByteLength();
    }

    if (end < start) end = start;

    size_t length = end - start;
    v8::Local<v8::Uint8Array> result = v8::Uint8Array::New(ui->Buffer(), ui->ByteOffset() + start, length);
    
    // Re-prototype it
    v8::Local<v8::Value> buffer_val = context->Global()->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "Buffer")).ToLocalChecked();
    if (buffer_val->IsFunction()) {
        v8::Local<v8::Value> proto = buffer_val.As<v8::Function>()->Get(context, v8::String::NewFromUtf8Literal(p_isolate, "prototype")).ToLocalChecked();
        result->SetPrototypeV2(context, proto).Check();
    }

    args.GetReturnValue().Set(result);
}

} // namespace module
} // namespace z8
