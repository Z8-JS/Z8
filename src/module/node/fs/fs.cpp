#include "fs.h"
#include <v8-isolate.h>
#include <fstream>
#include <filesystem>
#include <string>
#include <chrono>
#include <fcntl.h> // For O_* constants
#include "thread_pool.h"
#include "task_queue.h"
#include <v8-promise.h>

namespace fs = std::filesystem;

namespace z8 {
namespace module {

// Helper to convert file_time_type to V8 Date
v8::Local<v8::Value> FileTimeToV8Date(v8::Isolate* isolate, fs::file_time_type ftime) {
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
    double ms = static_cast<double>(std::chrono::time_point_cast<std::chrono::milliseconds>(sctp).time_since_epoch().count());
    return v8::Date::New(isolate->GetCurrentContext(), ms).ToLocalChecked();
}

v8::Local<v8::ObjectTemplate> FS::CreateTemplate(v8::Isolate* isolate) {
    v8::Local<v8::ObjectTemplate> tmpl = v8::ObjectTemplate::New(isolate);

    tmpl->Set(v8::String::NewFromUtf8(isolate, "readFileSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, ReadFileSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "writeFileSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, WriteFileSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "existsSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, ExistsSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "appendFileSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, AppendFileSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "statSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, StatSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "mkdirSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, MkdirSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "rmSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, RmSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "rmdirSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, RmdirSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "unlinkSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, UnlinkSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "lstatSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, LstatSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "readdirSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, ReaddirSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "renameSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, RenameSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "copyFileSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, CopyFileSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "realpathSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, RealpathSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "accessSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, AccessSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "chmodSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, ChmodSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "chownSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, ChownSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "utimesSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, UtimesSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "readlinkSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, ReadlinkSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "symlinkSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, SymlinkSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "linkSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, LinkSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "truncateSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, TruncateSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "openSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, OpenSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "readSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, ReadSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "writeSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, WriteSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "closeSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, CloseSync));

    // Add fs.constants
    v8::Local<v8::ObjectTemplate> constants_tmpl = v8::ObjectTemplate::New(isolate);
    constants_tmpl->Set(isolate, "F_OK", v8::Integer::New(isolate, 0));
    constants_tmpl->Set(isolate, "R_OK", v8::Integer::New(isolate, 4));
    constants_tmpl->Set(isolate, "W_OK", v8::Integer::New(isolate, 2));
    constants_tmpl->Set(isolate, "X_OK", v8::Integer::New(isolate, 1));
    constants_tmpl->Set(isolate, "O_RDONLY", v8::Integer::New(isolate, 0));
    constants_tmpl->Set(isolate, "O_WRONLY", v8::Integer::New(isolate, 1));
    constants_tmpl->Set(isolate, "O_RDWR", v8::Integer::New(isolate, 2));
    constants_tmpl->Set(isolate, "O_CREAT", v8::Integer::New(isolate, 0x0100));
    constants_tmpl->Set(isolate, "O_EXCL", v8::Integer::New(isolate, 0x0200));
    constants_tmpl->Set(isolate, "O_TRUNC", v8::Integer::New(isolate, 0x0400));
    constants_tmpl->Set(isolate, "O_APPEND", v8::Integer::New(isolate, 0x0008));
    
    tmpl->Set(isolate, "constants", constants_tmpl);

    // Async methods
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "readFile"), v8::FunctionTemplate::New(isolate, ReadFile));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "writeFile"), v8::FunctionTemplate::New(isolate, WriteFile));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "stat"), v8::FunctionTemplate::New(isolate, Stat));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "unlink"), v8::FunctionTemplate::New(isolate, Unlink));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "mkdir"), v8::FunctionTemplate::New(isolate, Mkdir));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "readdir"), v8::FunctionTemplate::New(isolate, Readdir));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "rmdir"), v8::FunctionTemplate::New(isolate, Rmdir));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "rename"), v8::FunctionTemplate::New(isolate, Rename));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "copyFile"), v8::FunctionTemplate::New(isolate, CopyFile));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "access"), v8::FunctionTemplate::New(isolate, Access));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "appendFile"), v8::FunctionTemplate::New(isolate, AppendFile));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "realpath"), v8::FunctionTemplate::New(isolate, Realpath));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "chmod"), v8::FunctionTemplate::New(isolate, Chmod));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "readlink"), v8::FunctionTemplate::New(isolate, Readlink));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "symlink"), v8::FunctionTemplate::New(isolate, Symlink));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "lstat"), v8::FunctionTemplate::New(isolate, Lstat));

    // Expose fs.promises
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "promises"), CreatePromisesTemplate(isolate));

    return tmpl;
}

v8::Local<v8::ObjectTemplate> FS::CreatePromisesTemplate(v8::Isolate* isolate) {
    v8::Local<v8::ObjectTemplate> tmpl = v8::ObjectTemplate::New(isolate);
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "readFile"), v8::FunctionTemplate::New(isolate, ReadFilePromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "writeFile"), v8::FunctionTemplate::New(isolate, WriteFilePromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "stat"), v8::FunctionTemplate::New(isolate, StatPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "unlink"), v8::FunctionTemplate::New(isolate, UnlinkPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "mkdir"), v8::FunctionTemplate::New(isolate, MkdirPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "readdir"), v8::FunctionTemplate::New(isolate, ReaddirPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "rmdir"), v8::FunctionTemplate::New(isolate, RmdirPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "rename"), v8::FunctionTemplate::New(isolate, RenamePromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "copyFile"), v8::FunctionTemplate::New(isolate, CopyFilePromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "access"), v8::FunctionTemplate::New(isolate, AccessPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "appendFile"), v8::FunctionTemplate::New(isolate, AppendFilePromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "realpath"), v8::FunctionTemplate::New(isolate, RealpathPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "chmod"), v8::FunctionTemplate::New(isolate, ChmodPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "readlink"), v8::FunctionTemplate::New(isolate, ReadlinkPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "symlink"), v8::FunctionTemplate::New(isolate, SymlinkPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "lstat"), v8::FunctionTemplate::New(isolate, LstatPromise));
    return tmpl;
}


void FS::ReadFileSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "TypeError: Path must be a string").ToLocalChecked());
        return;
    }

    v8::String::Utf8Value path_val(isolate, args[0]);
    std::string path(*path_val);

    std::string encoding = "";
    if (args.Length() >= 2 && args[1]->IsString()) {
        v8::String::Utf8Value enc_val(isolate, args[1]);
        encoding = *enc_val;
    } else if (args.Length() >= 2 && args[1]->IsObject()) {
        v8::Local<v8::Object> options = args[1].As<v8::Object>();
        v8::Local<v8::Value> enc_opt;
        if (options->Get(context, v8::String::NewFromUtf8Literal(isolate, "encoding")).ToLocal(&enc_opt) && enc_opt->IsString()) {
            v8::String::Utf8Value enc_val(isolate, enc_opt);
            encoding = *enc_val;
        }
    }

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "Error: ENOENT: no such file or directory").ToLocalChecked());
        return;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (encoding == "utf8") {
        std::string content(size, '\0');
        if (file.read(&content[0], size)) {
            args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, content.c_str(), v8::NewStringType::kNormal, (int)size).ToLocalChecked());
        }
    } else {
        v8::Local<v8::ArrayBuffer> ab = v8::ArrayBuffer::New(isolate, size);
        std::shared_ptr<v8::BackingStore> backing_store = ab->GetBackingStore();
        if (file.read(static_cast<char*>(backing_store->Data()), size)) {
            v8::Local<v8::Uint8Array> ui8 = v8::Uint8Array::New(ab, 0, size);
            args.GetReturnValue().Set(ui8);
        }
    }
}

void FS::WriteFileSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);

    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsString()) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "TypeError: Path and data must be strings").ToLocalChecked());
        return;
    }

    v8::String::Utf8Value path_val(isolate, args[0]);
    v8::String::Utf8Value data_val(isolate, args[1]);

    std::ofstream file(*path_val, std::ios::binary);
    if (!file.is_open()) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "Error: Could not open file for writing").ToLocalChecked());
        return;
    }

    file.write(*data_val, data_val.length());
    file.close();
}

void FS::AppendFileSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);

    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsString()) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "TypeError: Path and data must be strings").ToLocalChecked());
        return;
    }

    v8::String::Utf8Value path_val(isolate, args[0]);
    v8::String::Utf8Value data_val(isolate, args[1]);

    std::ofstream file(*path_val, std::ios::binary | std::ios::app);
    if (!file.is_open()) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "Error: Could not open file for appending").ToLocalChecked());
        return;
    }

    file.write(*data_val, data_val.length());
    file.close();
}


void FS::ExistsSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);

    if (args.Length() < 1 || !args[0]->IsString()) {
        args.GetReturnValue().Set(false);
        return;
    }

    v8::String::Utf8Value path_val(isolate, args[0]);
    args.GetReturnValue().Set(fs::exists(std::string(*path_val)));
}

v8::Local<v8::Object> FS::CreateStats(v8::Isolate* isolate, const fs::path& path, std::error_code& ec, bool follow_symlink) {
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    auto status = follow_symlink ? fs::status(path, ec) : fs::symlink_status(path, ec);
    
    v8::Local<v8::Object> stats = v8::Object::New(isolate);
    
    // Add properties
    stats->Set(context, v8::String::NewFromUtf8Literal(isolate, "size"), v8::Number::New(isolate, static_cast<double>(fs::exists(status) && fs::is_regular_file(status) ? fs::file_size(path, ec) : 0))).Check();
    
    auto mtime = fs::last_write_time(path, ec);
    if (!ec) {
        stats->Set(context, v8::String::NewFromUtf8Literal(isolate, "mtime"), FileTimeToV8Date(isolate, mtime)).Check();
        stats->Set(context, v8::String::NewFromUtf8Literal(isolate, "mtimeMs"), v8::Number::New(isolate, static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(mtime.time_since_epoch()).count()))).Check();
    }

    // Add methods (Simplified)
    stats->Set(context, v8::String::NewFromUtf8Literal(isolate, "isDirectory"), 
        v8::FunctionTemplate::New(isolate, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            args.GetReturnValue().Set(args.Data().As<v8::Boolean>());
        }, v8::Boolean::New(isolate, fs::is_directory(status)))->GetFunction(context).ToLocalChecked()).Check();

    stats->Set(context, v8::String::NewFromUtf8Literal(isolate, "isFile"), 
        v8::FunctionTemplate::New(isolate, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            args.GetReturnValue().Set(args.Data().As<v8::Boolean>());
        }, v8::Boolean::New(isolate, fs::is_regular_file(status)))->GetFunction(context).ToLocalChecked()).Check();

    stats->Set(context, v8::String::NewFromUtf8Literal(isolate, "isSymbolicLink"), 
        v8::FunctionTemplate::New(isolate, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            args.GetReturnValue().Set(args.Data().As<v8::Boolean>());
        }, v8::Boolean::New(isolate, fs::is_symlink(status)))->GetFunction(context).ToLocalChecked()).Check();

    return stats;
}

void FS::StatSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "TypeError: Path must be a string").ToLocalChecked());
        return;
    }

    v8::String::Utf8Value path_val(isolate, args[0]);
    std::error_code ec;
    auto stats = CreateStats(isolate, *path_val, ec, true);
    
    if (ec) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, ("Error: " + ec.message()).c_str()).ToLocalChecked());
        return;
    }

    args.GetReturnValue().Set(stats);
}

void FS::MkdirSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "TypeError: Path must be a string").ToLocalChecked());
        return;
    }

    v8::String::Utf8Value path_val(isolate, args[0]);
    std::string path(*path_val);
    std::error_code ec;

    // Equivalent to mkdir -p
    fs::create_directories(path, ec);

    if (ec) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, ("Error creating directory: " + ec.message()).c_str()).ToLocalChecked());
        return;
    }
}

void FS::RmSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "TypeError: Path must be a string").ToLocalChecked());
        return;
    }

    v8::String::Utf8Value path_val(isolate, args[0]);
    std::string path(*path_val);
    std::error_code ec;

    // Equivalent to rm -rf
    fs::remove_all(path, ec);

    if (ec) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, ("Error removing path: " + ec.message()).c_str()).ToLocalChecked());
        return;
    }
}


void FS::LstatSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "TypeError: Path must be a string").ToLocalChecked());
        return;
    }

    v8::String::Utf8Value path_val(isolate, args[0]);
    std::error_code ec;
    auto stats = CreateStats(isolate, *path_val, ec, false);
    
    if (ec) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, ("Error: " + ec.message()).c_str()).ToLocalChecked());
        return;
    }

    args.GetReturnValue().Set(stats);
}

void FS::ReaddirSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "TypeError: Path must be a string").ToLocalChecked());
        return;
    }

    v8::String::Utf8Value path_val(isolate, args[0]);
    std::string path(*path_val);
    std::error_code ec;

    std::vector<v8::Local<v8::Value>> entries;
    for (const auto& entry : fs::directory_iterator(path, ec)) {
        entries.push_back(v8::String::NewFromUtf8(isolate, entry.path().filename().string().c_str()).ToLocalChecked());
    }

    if (ec) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, ("Error reading directory: " + ec.message()).c_str()).ToLocalChecked());
        return;
    }

    args.GetReturnValue().Set(v8::Array::New(isolate, entries.data(), (int)entries.size()));
}

void FS::RenameSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);

    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsString()) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "TypeError: Old and new paths must be strings").ToLocalChecked());
        return;
    }

    v8::String::Utf8Value old_path(isolate, args[0]);
    v8::String::Utf8Value new_path(isolate, args[1]);
    std::error_code ec;

    fs::rename(*old_path, *new_path, ec);
    if (ec) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, ("Error renaming: " + ec.message()).c_str()).ToLocalChecked());
    }
}

void FS::CopyFileSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);

    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsString()) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "TypeError: Source and destination paths must be strings").ToLocalChecked());
        return;
    }

    v8::String::Utf8Value src(isolate, args[0]);
    v8::String::Utf8Value dest(isolate, args[1]);
    std::error_code ec;

    fs::copy_file(*src, *dest, fs::copy_options::overwrite_existing, ec);
    if (ec) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, ("Error copying file: " + ec.message()).c_str()).ToLocalChecked());
    }
}

void FS::RealpathSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "TypeError: Path must be a string").ToLocalChecked());
        return;
    }

    v8::String::Utf8Value path(isolate, args[0]);
    std::error_code ec;

    fs::path p = fs::canonical(*path, ec); // Canonical is good for realpath
    if (ec) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, ("Error resolving realpath: " + ec.message()).c_str()).ToLocalChecked());
        return;
    }

    args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, p.string().c_str()).ToLocalChecked());
}

void FS::AccessSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "TypeError: Path must be a string").ToLocalChecked());
        return;
    }

    v8::String::Utf8Value path(isolate, args[0]);
    std::error_code ec;

    if (!fs::exists(*path, ec)) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "Error: ENOENT: no such file or directory").ToLocalChecked());
        return;
    }
}

void FS::RmdirSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    if (args.Length() < 1 || !args[0]->IsString()) return;
    v8::String::Utf8Value path(isolate, args[0]);
    std::error_code ec;
    fs::remove(*path, ec);
}

void FS::UnlinkSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    if (args.Length() < 1 || !args[0]->IsString()) return;
    v8::String::Utf8Value path(isolate, args[0]);
    std::error_code ec;
    fs::remove(*path, ec);
}

void FS::ChmodSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsNumber()) return;
    v8::String::Utf8Value path(isolate, args[0]);
    int mode = args[1]->Int32Value(isolate->GetCurrentContext()).FromMaybe(0);
    std::error_code ec;
    fs::permissions(*path, static_cast<fs::perms>(mode), ec);
}

void FS::ChownSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // std::filesystem doesn't support chown easily, would need platform specific code (POSIX)
}

void FS::UtimesSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    if (args.Length() < 3 || !args[0]->IsString()) return;
    v8::String::Utf8Value path(isolate, args[0]);
    // Simplified: only setting mtime
    std::error_code ec;
    fs::last_write_time(*path, fs::file_time_type::clock::now(), ec); 
}

void FS::ReadlinkSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    if (args.Length() < 1 || !args[0]->IsString()) return;
    v8::String::Utf8Value path(isolate, args[0]);
    std::error_code ec;
    fs::path target = fs::read_symlink(*path, ec);
    if (!ec) args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, target.string().c_str()).ToLocalChecked());
}

void FS::SymlinkSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsString()) return;
    v8::String::Utf8Value target(isolate, args[0]);
    v8::String::Utf8Value path(isolate, args[1]);
    std::error_code ec;
    fs::create_symlink(*target, *path, ec);
}

void FS::LinkSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsString()) return;
    v8::String::Utf8Value target(isolate, args[0]);
    v8::String::Utf8Value path(isolate, args[1]);
    std::error_code ec;
    fs::create_hard_link(*target, *path, ec);
}

void FS::TruncateSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    if (args.Length() < 1 || !args[0]->IsString()) return;
    v8::String::Utf8Value path(isolate, args[0]);
    uintmax_t length = 0;
    if (args.Length() >= 2 && args[1]->IsNumber()) length = static_cast<uintmax_t>(args[1]->NumberValue(isolate->GetCurrentContext()).FromMaybe(0));
    std::error_code ec;
    fs::resize_file(*path, length, ec);
}

void FS::OpenSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // Needs proper FD management, for now just a placeholder or use platform fds
}

void FS::ReadSync(const v8::FunctionCallbackInfo<v8::Value>& args) {}
void FS::WriteSync(const v8::FunctionCallbackInfo<v8::Value>& args) {}
void FS::CloseSync(const v8::FunctionCallbackInfo<v8::Value>& args) {}

// Async Context for ReadFile
struct ReadFileContext {
    std::string path;
    std::string encoding;
    std::string content;
    std::vector<char> binary_content;
    bool is_error = false;
    std::string error_msg;
};

void FS::ReadFile(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 2 || !args[0]->IsString() || !args[args.Length()-1]->IsFunction()) return;

    v8::String::Utf8Value path(isolate, args[0]);
    v8::Local<v8::Function> cb = args[args.Length()-1].As<v8::Function>();

    auto ctx = new ReadFileContext();
    ctx->path = *path;

    if (args.Length() >= 2 && args[1]->IsString()) {
        v8::String::Utf8Value enc_val(isolate, args[1]);
        ctx->encoding = *enc_val;
    } else if (args.Length() >= 2 && args[1]->IsObject()) {
        v8::Local<v8::Object> options = args[1].As<v8::Object>();
        v8::Local<v8::Value> enc_opt;
        if (options->Get(isolate->GetCurrentContext(), v8::String::NewFromUtf8Literal(isolate, "encoding")).ToLocal(&enc_opt) && enc_opt->IsString()) {
            v8::String::Utf8Value enc_val(isolate, enc_opt);
            ctx->encoding = *enc_val;
        }
    }

    Task* task = new Task();
    task->callback.Reset(isolate, cb);
    task->is_promise = false;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<ReadFileContext*>(task->data);
        v8::Local<v8::Value> argv[2];
        if (ctx->is_error) {
            argv[0] = v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked());
            argv[1] = v8::Undefined(isolate);
        } else {
            argv[0] = v8::Null(isolate);
            if (ctx->encoding == "utf8") {
                argv[1] = v8::String::NewFromUtf8(isolate, ctx->binary_content.data(), v8::NewStringType::kNormal, (int)ctx->binary_content.size()).ToLocalChecked();
            } else {
                v8::Local<v8::ArrayBuffer> ab = v8::ArrayBuffer::New(isolate, ctx->binary_content.size());
                memcpy(ab->GetBackingStore()->Data(), ctx->binary_content.data(), ctx->binary_content.size());
                argv[1] = v8::Uint8Array::New(ab, 0, ctx->binary_content.size());
            }
        }
        (void)task->callback.Get(isolate)->Call(context, context->Global(), 2, argv);
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        std::ifstream file(ctx->path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            ctx->is_error = true;
            ctx->error_msg = "ENOENT: no such file or directory";
        } else {
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);
            ctx->binary_content.resize(size);
            file.read(ctx->binary_content.data(), size);
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

void FS::ReadFilePromise(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    if (args.Length() < 1 || !args[0]->IsString()) return;

    v8::Local<v8::Promise::Resolver> resolver;
    if (!v8::Promise::Resolver::New(context).ToLocal(&resolver)) return;
    args.GetReturnValue().Set(resolver->GetPromise());

    v8::String::Utf8Value path(isolate, args[0]);
    auto ctx = new ReadFileContext();
    ctx->path = *path;

    if (args.Length() >= 2 && args[1]->IsString()) {
        v8::String::Utf8Value enc_val(isolate, args[1]);
        ctx->encoding = *enc_val;
    } else if (args.Length() >= 2 && args[1]->IsObject()) {
        v8::Local<v8::Object> options = args[1].As<v8::Object>();
        v8::Local<v8::Value> enc_opt;
        if (options->Get(context, v8::String::NewFromUtf8Literal(isolate, "encoding")).ToLocal(&enc_opt) && enc_opt->IsString()) {
            v8::String::Utf8Value enc_val(isolate, enc_opt);
            ctx->encoding = *enc_val;
        }
    }

    Task* task = new Task();
    task->resolver.Reset(isolate, resolver);
    task->is_promise = true;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<ReadFileContext*>(task->data);
        auto resolver = task->resolver.Get(isolate);
        if (ctx->is_error) {
            resolver->Reject(context, v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked())).Check();
        } else {
            if (ctx->encoding == "utf8") {
                resolver->Resolve(context, v8::String::NewFromUtf8(isolate, ctx->binary_content.data(), v8::NewStringType::kNormal, (int)ctx->binary_content.size()).ToLocalChecked()).Check();
            } else {
                v8::Local<v8::ArrayBuffer> ab = v8::ArrayBuffer::New(isolate, ctx->binary_content.size());
                memcpy(ab->GetBackingStore()->Data(), ctx->binary_content.data(), ctx->binary_content.size());
                resolver->Resolve(context, v8::Uint8Array::New(ab, 0, ctx->binary_content.size())).Check();
            }
        }
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        std::ifstream file(ctx->path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            ctx->is_error = true;
            ctx->error_msg = "ENOENT: no such file or directory";
        } else {
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);
            ctx->binary_content.resize(size);
            file.read(ctx->binary_content.data(), size);
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

struct WriteFileContext {
    std::string path;
    std::string content;
    std::vector<char> binary_content;
    bool is_binary = false;
    bool is_error = false;
    std::string error_msg;
};

void FS::WriteFile(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 3 || !args[0]->IsString() || !args[args.Length()-1]->IsFunction()) return;

    v8::String::Utf8Value path(isolate, args[0]);
    v8::Local<v8::Function> cb = args[args.Length()-1].As<v8::Function>();

    auto ctx = new WriteFileContext();
    ctx->path = *path;

    if (args[1]->IsString()) {
        v8::String::Utf8Value content(isolate, args[1]);
        ctx->content = *content;
        ctx->is_binary = false;
    } else if (args[1]->IsUint8Array()) {
        v8::Local<v8::Uint8Array> uint8 = args[1].As<v8::Uint8Array>();
        ctx->binary_content.resize(uint8->ByteLength());
        uint8->CopyContents(ctx->binary_content.data(), uint8->ByteLength());
        ctx->is_binary = true;
    }

    Task* task = new Task();
    task->callback.Reset(isolate, cb);
    task->is_promise = false;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<WriteFileContext*>(task->data);
        v8::Local<v8::Value> argv[1];
        if (ctx->is_error) {
            argv[0] = v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked());
        } else {
            argv[0] = v8::Null(isolate);
        }
        (void)task->callback.Get(isolate)->Call(context, context->Global(), 1, argv);
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        std::ofstream file(ctx->path, std::ios::binary);
        if (!file.is_open()) {
            ctx->is_error = true;
            ctx->error_msg = "Could not open file for writing";
        } else {
            if (ctx->is_binary) {
                file.write(ctx->binary_content.data(), ctx->binary_content.size());
            } else {
                file.write(ctx->content.c_str(), ctx->content.size());
            }
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

void FS::WriteFilePromise(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    if (args.Length() < 2 || !args[0]->IsString()) return;

    v8::Local<v8::Promise::Resolver> resolver;
    if (!v8::Promise::Resolver::New(context).ToLocal(&resolver)) return;
    args.GetReturnValue().Set(resolver->GetPromise());

    v8::String::Utf8Value path(isolate, args[0]);
    auto ctx = new WriteFileContext();
    ctx->path = *path;

    if (args[1]->IsString()) {
        v8::String::Utf8Value content(isolate, args[1]);
        ctx->content = *content;
        ctx->is_binary = false;
    } else if (args[1]->IsUint8Array()) {
        v8::Local<v8::Uint8Array> uint8 = args[1].As<v8::Uint8Array>();
        ctx->binary_content.resize(uint8->ByteLength());
        uint8->CopyContents(ctx->binary_content.data(), uint8->ByteLength());
        ctx->is_binary = true;
    }

    Task* task = new Task();
    task->resolver.Reset(isolate, resolver);
    task->is_promise = true;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<WriteFileContext*>(task->data);
        auto resolver = task->resolver.Get(isolate);
        if (ctx->is_error) {
            resolver->Reject(context, v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked())).Check();
        } else {
            resolver->Resolve(context, v8::Undefined(isolate)).Check();
        }
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        std::ofstream file(ctx->path, std::ios::binary);
        if (!file.is_open()) {
            ctx->is_error = true;
            ctx->error_msg = "Could not open file for writing";
        } else {
            if (ctx->is_binary) {
                file.write(ctx->binary_content.data(), ctx->binary_content.size());
            } else {
                file.write(ctx->content.c_str(), ctx->content.size());
            }
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

struct StatContext {
    std::string path;
    bool is_error = false;
    std::string error_msg;
    bool follow_symlink = true;
    std::error_code ec;
};

void FS::Stat(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 2 || !args[0]->IsString() || !args[args.Length()-1]->IsFunction()) return;

    v8::String::Utf8Value path(isolate, args[0]);
    v8::Local<v8::Function> cb = args[args.Length()-1].As<v8::Function>();

    auto ctx = new StatContext();
    ctx->path = *path;

    Task* task = new Task();
    task->callback.Reset(isolate, cb);
    task->is_promise = false;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<StatContext*>(task->data);
        v8::Local<v8::Value> argv[2];
        if (ctx->is_error) {
            argv[0] = v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked());
            argv[1] = v8::Undefined(isolate);
        } else {
            argv[0] = v8::Null(isolate);
            argv[1] = FS::CreateStats(isolate, ctx->path, ctx->ec, ctx->follow_symlink);
        }
        (void)task->callback.Get(isolate)->Call(context, context->Global(), 2, argv);
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        if (!fs::exists(ctx->path, ctx->ec)) {
            ctx->is_error = true;
            ctx->error_msg = "ENOENT: no such file or directory";
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

void FS::StatPromise(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    if (args.Length() < 1 || !args[0]->IsString()) return;

    v8::Local<v8::Promise::Resolver> resolver;
    if (!v8::Promise::Resolver::New(context).ToLocal(&resolver)) return;
    args.GetReturnValue().Set(resolver->GetPromise());

    v8::String::Utf8Value path(isolate, args[0]);
    auto ctx = new StatContext();
    ctx->path = *path;

    Task* task = new Task();
    task->resolver.Reset(isolate, resolver);
    task->is_promise = true;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<StatContext*>(task->data);
        auto resolver = task->resolver.Get(isolate);
        if (ctx->is_error) {
            resolver->Reject(context, v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked())).Check();
        } else {
            resolver->Resolve(context, FS::CreateStats(isolate, ctx->path, ctx->ec, ctx->follow_symlink)).Check();
        }
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        if (!fs::exists(ctx->path, ctx->ec)) {
            ctx->is_error = true;
            ctx->error_msg = "ENOENT: no such file or directory";
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

struct UnlinkContext {
    std::string path;
    bool is_error = false;
    std::string error_msg;
};

void FS::Unlink(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 2 || !args[0]->IsString() || !args[args.Length()-1]->IsFunction()) return;

    v8::String::Utf8Value path(isolate, args[0]);
    v8::Local<v8::Function> cb = args[args.Length()-1].As<v8::Function>();

    auto ctx = new UnlinkContext();
    ctx->path = *path;

    Task* task = new Task();
    task->callback.Reset(isolate, cb);
    task->is_promise = false;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<UnlinkContext*>(task->data);
        v8::Local<v8::Value> argv[1];
        if (ctx->is_error) {
            argv[0] = v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked());
        } else {
            argv[0] = v8::Null(isolate);
        }
        (void)task->callback.Get(isolate)->Call(context, context->Global(), 1, argv);
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        std::error_code ec;
        if (!fs::remove(ctx->path, ec)) {
            ctx->is_error = true;
            ctx->error_msg = ec ? ec.message() : "Failed to unlink file";
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

void FS::UnlinkPromise(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    if (args.Length() < 1 || !args[0]->IsString()) return;

    v8::Local<v8::Promise::Resolver> resolver;
    if (!v8::Promise::Resolver::New(context).ToLocal(&resolver)) return;
    args.GetReturnValue().Set(resolver->GetPromise());

    v8::String::Utf8Value path(isolate, args[0]);
    auto ctx = new UnlinkContext();
    ctx->path = *path;

    Task* task = new Task();
    task->resolver.Reset(isolate, resolver);
    task->is_promise = true;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<UnlinkContext*>(task->data);
        auto resolver = task->resolver.Get(isolate);
        if (ctx->is_error) {
            resolver->Reject(context, v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked())).Check();
        } else {
            resolver->Resolve(context, v8::Undefined(isolate)).Check();
        }
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        std::error_code ec;
        if (!fs::remove(ctx->path, ec)) {
            ctx->is_error = true;
            ctx->error_msg = ec ? ec.message() : "Failed to unlink file";
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

// --- Mkdir ---
struct MkdirContext {
    std::string path;
    bool is_error = false;
    std::string error_msg;
};

void FS::Mkdir(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 2 || !args[0]->IsString() || !args[args.Length()-1]->IsFunction()) return;

    v8::String::Utf8Value path(isolate, args[0]);
    v8::Local<v8::Function> cb = args[args.Length()-1].As<v8::Function>();

    auto ctx = new MkdirContext();
    ctx->path = *path;

    Task* task = new Task();
    task->callback.Reset(isolate, cb);
    task->is_promise = false;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<MkdirContext*>(task->data);
        v8::Local<v8::Value> argv[1];
        if (ctx->is_error) {
            argv[0] = v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked());
        } else {
            argv[0] = v8::Null(isolate);
        }
        (void)task->callback.Get(isolate)->Call(context, context->Global(), 1, argv);
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        std::error_code ec;
        fs::create_directories(ctx->path, ec); // Behaves like mkdir -p
        if (ec) {
            ctx->is_error = true;
            ctx->error_msg = ec.message();
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

void FS::MkdirPromise(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    if (args.Length() < 1 || !args[0]->IsString()) return;

    v8::Local<v8::Promise::Resolver> resolver;
    if (!v8::Promise::Resolver::New(context).ToLocal(&resolver)) return;
    args.GetReturnValue().Set(resolver->GetPromise());

    v8::String::Utf8Value path(isolate, args[0]);
    auto ctx = new MkdirContext();
    ctx->path = *path;

    Task* task = new Task();
    task->resolver.Reset(isolate, resolver);
    task->is_promise = true;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<MkdirContext*>(task->data);
        auto resolver = task->resolver.Get(isolate);
        if (ctx->is_error) {
            resolver->Reject(context, v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked())).Check();
        } else {
            resolver->Resolve(context, v8::Undefined(isolate)).Check();
        }
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        std::error_code ec;
        fs::create_directories(ctx->path, ec);
        if (ec) {
            ctx->is_error = true;
            ctx->error_msg = ec.message();
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

// --- Readdir ---
struct ReaddirContext {
    std::string path;
    std::vector<std::string> entries;
    bool is_error = false;
    std::string error_msg;
};

void FS::Readdir(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 2 || !args[0]->IsString() || !args[args.Length()-1]->IsFunction()) return;

    v8::String::Utf8Value path(isolate, args[0]);
    v8::Local<v8::Function> cb = args[args.Length()-1].As<v8::Function>();

    auto ctx = new ReaddirContext();
    ctx->path = *path;

    Task* task = new Task();
    task->callback.Reset(isolate, cb);
    task->is_promise = false;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<ReaddirContext*>(task->data);
        v8::Local<v8::Value> argv[2];
        if (ctx->is_error) {
            argv[0] = v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked());
            argv[1] = v8::Undefined(isolate);
        } else {
            argv[0] = v8::Null(isolate);
            v8::Local<v8::Array> array = v8::Array::New(isolate, (int)ctx->entries.size());
            for (size_t i = 0; i < ctx->entries.size(); ++i) {
                array->Set(context, (uint32_t)i, v8::String::NewFromUtf8(isolate, ctx->entries[i].c_str()).ToLocalChecked()).Check();
            }
            argv[1] = array;
        }
        (void)task->callback.Get(isolate)->Call(context, context->Global(), 2, argv);
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        std::error_code ec;
        for (const auto& entry : fs::directory_iterator(ctx->path, ec)) {
            ctx->entries.push_back(entry.path().filename().string());
        }
        if (ec) {
            ctx->is_error = true;
            ctx->error_msg = ec.message();
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

void FS::ReaddirPromise(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    if (args.Length() < 1 || !args[0]->IsString()) return;

    v8::Local<v8::Promise::Resolver> resolver;
    if (!v8::Promise::Resolver::New(context).ToLocal(&resolver)) return;
    args.GetReturnValue().Set(resolver->GetPromise());

    v8::String::Utf8Value path(isolate, args[0]);
    auto ctx = new ReaddirContext();
    ctx->path = *path;

    Task* task = new Task();
    task->resolver.Reset(isolate, resolver);
    task->is_promise = true;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<ReaddirContext*>(task->data);
        auto resolver = task->resolver.Get(isolate);
        if (ctx->is_error) {
            resolver->Reject(context, v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked())).Check();
        } else {
            v8::Local<v8::Array> array = v8::Array::New(isolate, (int)ctx->entries.size());
            for (size_t i = 0; i < ctx->entries.size(); ++i) {
                array->Set(context, (uint32_t)i, v8::String::NewFromUtf8(isolate, ctx->entries[i].c_str()).ToLocalChecked()).Check();
            }
            resolver->Resolve(context, array).Check();
        }
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        std::error_code ec;
        for (const auto& entry : fs::directory_iterator(ctx->path, ec)) {
            ctx->entries.push_back(entry.path().filename().string());
        }
        if (ec) {
            ctx->is_error = true;
            ctx->error_msg = ec.message();
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

// --- Rmdir ---
struct RmdirContext {
    std::string path;
    bool is_error = false;
    std::string error_msg;
};

void FS::Rmdir(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 2 || !args[0]->IsString() || !args[args.Length()-1]->IsFunction()) return;

    v8::String::Utf8Value path(isolate, args[0]);
    v8::Local<v8::Function> cb = args[args.Length()-1].As<v8::Function>();

    auto ctx = new RmdirContext();
    ctx->path = *path;

    Task* task = new Task();
    task->callback.Reset(isolate, cb);
    task->is_promise = false;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<RmdirContext*>(task->data);
        v8::Local<v8::Value> argv[1];
        if (ctx->is_error) {
            argv[0] = v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked());
        } else {
            argv[0] = v8::Null(isolate);
        }
        (void)task->callback.Get(isolate)->Call(context, context->Global(), 1, argv);
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        std::error_code ec;
        fs::remove(ctx->path, ec); // Standard rmdir (non-recursive in standard fs::remove unless it's empty?) actually fs::remove handles both file and empty dir
        // Node's rmdir usually expects empty directory. fs::remove also fails if not empty (usually).
        if (ec) {
            ctx->is_error = true;
            ctx->error_msg = ec.message();
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

void FS::RmdirPromise(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    if (args.Length() < 1 || !args[0]->IsString()) return;

    v8::Local<v8::Promise::Resolver> resolver;
    if (!v8::Promise::Resolver::New(context).ToLocal(&resolver)) return;
    args.GetReturnValue().Set(resolver->GetPromise());

    v8::String::Utf8Value path(isolate, args[0]);
    auto ctx = new RmdirContext();
    ctx->path = *path;

    Task* task = new Task();
    task->resolver.Reset(isolate, resolver);
    task->is_promise = true;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<RmdirContext*>(task->data);
        auto resolver = task->resolver.Get(isolate);
        if (ctx->is_error) {
            resolver->Reject(context, v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked())).Check();
        } else {
            resolver->Resolve(context, v8::Undefined(isolate)).Check();
        }
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        std::error_code ec;
        fs::remove(ctx->path, ec);
        if (ec) {
            ctx->is_error = true;
            ctx->error_msg = ec.message();
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

// --- Rename ---
struct RenameContext {
    std::string old_path;
    std::string new_path;
    bool is_error = false;
    std::string error_msg;
};

void FS::Rename(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 3 || !args[0]->IsString() || !args[1]->IsString() || !args[args.Length()-1]->IsFunction()) return;

    v8::String::Utf8Value old_path(isolate, args[0]);
    v8::String::Utf8Value new_path(isolate, args[1]);
    v8::Local<v8::Function> cb = args[args.Length()-1].As<v8::Function>();

    auto ctx = new RenameContext();
    ctx->old_path = *old_path;
    ctx->new_path = *new_path;

    Task* task = new Task();
    task->callback.Reset(isolate, cb);
    task->is_promise = false;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<RenameContext*>(task->data);
        v8::Local<v8::Value> argv[1];
        if (ctx->is_error) {
            argv[0] = v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked());
        } else {
            argv[0] = v8::Null(isolate);
        }
        (void)task->callback.Get(isolate)->Call(context, context->Global(), 1, argv);
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        std::error_code ec;
        fs::rename(ctx->old_path, ctx->new_path, ec);
        if (ec) {
            ctx->is_error = true;
            ctx->error_msg = ec.message();
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

void FS::RenamePromise(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsString()) return;

    v8::Local<v8::Promise::Resolver> resolver;
    if (!v8::Promise::Resolver::New(context).ToLocal(&resolver)) return;
    args.GetReturnValue().Set(resolver->GetPromise());

    v8::String::Utf8Value old_path(isolate, args[0]);
    v8::String::Utf8Value new_path(isolate, args[1]);
    auto ctx = new RenameContext();
    ctx->old_path = *old_path;
    ctx->new_path = *new_path;

    Task* task = new Task();
    task->resolver.Reset(isolate, resolver);
    task->is_promise = true;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<RenameContext*>(task->data);
        auto resolver = task->resolver.Get(isolate);
        if (ctx->is_error) {
            resolver->Reject(context, v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked())).Check();
        } else {
            resolver->Resolve(context, v8::Undefined(isolate)).Check();
        }
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        std::error_code ec;
        fs::rename(ctx->old_path, ctx->new_path, ec);
        if (ec) {
            ctx->is_error = true;
            ctx->error_msg = ec.message();
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

// --- CopyFile ---
struct CopyFileContext {
    std::string src;
    std::string dest;
    int flags = 0; // Default to overwrite (0)
    bool is_error = false;
    std::string error_msg;
};

void FS::CopyFile(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 3 || !args[0]->IsString() || !args[1]->IsString() || !args[args.Length()-1]->IsFunction()) return;

    v8::String::Utf8Value src(isolate, args[0]);
    v8::String::Utf8Value dest(isolate, args[1]);
    v8::Local<v8::Function> cb = args[args.Length()-1].As<v8::Function>();

    auto ctx = new CopyFileContext();
    ctx->src = *src;
    ctx->dest = *dest;
    
    if (args.Length() > 3 && args[2]->IsNumber()) {
        ctx->flags = args[2]->Int32Value(isolate->GetCurrentContext()).FromMaybe(0);
    }

    Task* task = new Task();
    task->callback.Reset(isolate, cb);
    task->is_promise = false;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<CopyFileContext*>(task->data);
        v8::Local<v8::Value> argv[1];
        if (ctx->is_error) {
            argv[0] = v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked());
        } else {
            argv[0] = v8::Null(isolate);
        }
        (void)task->callback.Get(isolate)->Call(context, context->Global(), 1, argv);
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        std::error_code ec;
        auto options = fs::copy_options::overwrite_existing; 
        // Need to check flags. Node.js COPYFILE_EXCL = 1, force = 0?
        // Basic impl: always overwrite for now unless we parse flags better
        // TODO: Map Node flags to std::filesystem copy_options
        
        fs::copy_file(ctx->src, ctx->dest, options, ec);
        if (ec) {
            ctx->is_error = true;
            ctx->error_msg = ec.message();
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

void FS::CopyFilePromise(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsString()) return;

    v8::Local<v8::Promise::Resolver> resolver;
    if (!v8::Promise::Resolver::New(context).ToLocal(&resolver)) return;
    args.GetReturnValue().Set(resolver->GetPromise());

    v8::String::Utf8Value src(isolate, args[0]);
    v8::String::Utf8Value dest(isolate, args[1]);
    auto ctx = new CopyFileContext();
    ctx->src = *src;
    ctx->dest = *dest;

    if (args.Length() > 2 && args[2]->IsNumber()) {
        ctx->flags = args[2]->Int32Value(context).FromMaybe(0);
    }

    Task* task = new Task();
    task->resolver.Reset(isolate, resolver);
    task->is_promise = true;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<CopyFileContext*>(task->data);
        auto resolver = task->resolver.Get(isolate);
        if (ctx->is_error) {
            resolver->Reject(context, v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked())).Check();
        } else {
            resolver->Resolve(context, v8::Undefined(isolate)).Check();
        }
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        std::error_code ec;
        fs::copy_file(ctx->src, ctx->dest, fs::copy_options::overwrite_existing, ec);
        if (ec) {
            ctx->is_error = true;
            ctx->error_msg = ec.message();
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

// --- Access ---
struct AccessContext {
    std::string path;
    int mode = 0; // F_OK
    bool is_error = false;
    std::string error_msg;
};

void FS::Access(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 2 || !args[0]->IsString() || !args[args.Length()-1]->IsFunction()) return;

    v8::String::Utf8Value path(isolate, args[0]);
    v8::Local<v8::Function> cb = args[args.Length()-1].As<v8::Function>();

    auto ctx = new AccessContext();
    ctx->path = *path;
    if (args.Length() > 2 && args[1]->IsNumber()) {
        ctx->mode = args[1]->Int32Value(isolate->GetCurrentContext()).FromMaybe(0);
    }

    Task* task = new Task();
    task->callback.Reset(isolate, cb);
    task->is_promise = false;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<AccessContext*>(task->data);
        v8::Local<v8::Value> argv[1];
        if (ctx->is_error) {
            argv[0] = v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked());
        } else {
            argv[0] = v8::Null(isolate);
        }
        (void)task->callback.Get(isolate)->Call(context, context->Global(), 1, argv);
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        std::error_code ec;
        if (!fs::exists(ctx->path, ec)) {
            ctx->is_error = true;
            ctx->error_msg = "ENOENT: no such file or directory";
        }
        // TODO: Check specific permissions (R_OK, W_OK) if mode > 0
        TaskQueue::GetInstance().Enqueue(task);
    });
}

void FS::AccessPromise(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    if (args.Length() < 1 || !args[0]->IsString()) return;

    v8::Local<v8::Promise::Resolver> resolver;
    if (!v8::Promise::Resolver::New(context).ToLocal(&resolver)) return;
    args.GetReturnValue().Set(resolver->GetPromise());

    v8::String::Utf8Value path(isolate, args[0]);
    auto ctx = new AccessContext();
    ctx->path = *path;
    if (args.Length() > 1 && args[1]->IsNumber()) {
        ctx->mode = args[1]->Int32Value(context).FromMaybe(0);
    }

    Task* task = new Task();
    task->resolver.Reset(isolate, resolver);
    task->is_promise = true;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<AccessContext*>(task->data);
        auto resolver = task->resolver.Get(isolate);
        if (ctx->is_error) {
            resolver->Reject(context, v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked())).Check();
        } else {
            resolver->Resolve(context, v8::Undefined(isolate)).Check();
        }
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        std::error_code ec;
        if (!fs::exists(ctx->path, ec)) {
            ctx->is_error = true;
            ctx->error_msg = "ENOENT: no such file or directory";
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

// --- AppendFile ---
struct AppendFileContext {
    std::string path;
    std::string content;
    std::vector<char> binary_content;
    bool is_binary = false;
    bool is_error = false;
    std::string error_msg;
};

void FS::AppendFile(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 3 || !args[0]->IsString() || !args[args.Length()-1]->IsFunction()) return;

    v8::String::Utf8Value path(isolate, args[0]);
    v8::Local<v8::Function> cb = args[args.Length()-1].As<v8::Function>();

    auto ctx = new AppendFileContext();
    ctx->path = *path;

    if (args[1]->IsString()) {
        v8::String::Utf8Value content(isolate, args[1]);
        ctx->content = *content;
        ctx->is_binary = false;
    } else if (args[1]->IsUint8Array()) {
        v8::Local<v8::Uint8Array> uint8 = args[1].As<v8::Uint8Array>();
        ctx->binary_content.resize(uint8->ByteLength());
        uint8->CopyContents(ctx->binary_content.data(), uint8->ByteLength());
        ctx->is_binary = true;
    }

    Task* task = new Task();
    task->callback.Reset(isolate, cb);
    task->is_promise = false;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<AppendFileContext*>(task->data);
        v8::Local<v8::Value> argv[1];
        if (ctx->is_error) {
            argv[0] = v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked());
        } else {
            argv[0] = v8::Null(isolate);
        }
        (void)task->callback.Get(isolate)->Call(context, context->Global(), 1, argv);
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        std::ofstream file(ctx->path, std::ios::binary | std::ios::app);
        if (!file.is_open()) {
            ctx->is_error = true;
            ctx->error_msg = "Could not open file for appending";
        } else {
            if (ctx->is_binary) {
                file.write(ctx->binary_content.data(), ctx->binary_content.size());
            } else {
                file.write(ctx->content.c_str(), ctx->content.size());
            }
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

void FS::AppendFilePromise(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    if (args.Length() < 2 || !args[0]->IsString()) return;

    v8::Local<v8::Promise::Resolver> resolver;
    if (!v8::Promise::Resolver::New(context).ToLocal(&resolver)) return;
    args.GetReturnValue().Set(resolver->GetPromise());

    v8::String::Utf8Value path(isolate, args[0]);
    auto ctx = new AppendFileContext();
    ctx->path = *path;

    if (args[1]->IsString()) {
        v8::String::Utf8Value content(isolate, args[1]);
        ctx->content = *content;
        ctx->is_binary = false;
    } else if (args[1]->IsUint8Array()) {
        v8::Local<v8::Uint8Array> uint8 = args[1].As<v8::Uint8Array>();
        ctx->binary_content.resize(uint8->ByteLength());
        uint8->CopyContents(ctx->binary_content.data(), uint8->ByteLength());
        ctx->is_binary = true;
    }

    Task* task = new Task();
    task->resolver.Reset(isolate, resolver);
    task->is_promise = true;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<AppendFileContext*>(task->data);
        auto resolver = task->resolver.Get(isolate);
        if (ctx->is_error) {
            resolver->Reject(context, v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked())).Check();
        } else {
            resolver->Resolve(context, v8::Undefined(isolate)).Check();
        }
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        std::ofstream file(ctx->path, std::ios::binary | std::ios::app);
        if (!file.is_open()) {
            ctx->is_error = true;
            ctx->error_msg = "Could not open file for appending";
        } else {
            if (ctx->is_binary) {
                file.write(ctx->binary_content.data(), ctx->binary_content.size());
            } else {
                file.write(ctx->content.c_str(), ctx->content.size());
            }
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

// --- Realpath ---
struct RealpathContext {
    std::string path;
    std::string result;
    bool is_error = false;
    std::string error_msg;
};

void FS::Realpath(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 2 || !args[0]->IsString() || !args[args.Length()-1]->IsFunction()) return;

    v8::String::Utf8Value path(isolate, args[0]);
    v8::Local<v8::Function> cb = args[args.Length()-1].As<v8::Function>();

    auto ctx = new RealpathContext();
    ctx->path = *path;

    Task* task = new Task();
    task->callback.Reset(isolate, cb);
    task->is_promise = false;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<RealpathContext*>(task->data);
        v8::Local<v8::Value> argv[2];
        if (ctx->is_error) {
            argv[0] = v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked());
            argv[1] = v8::Undefined(isolate);
        } else {
            argv[0] = v8::Null(isolate);
            argv[1] = v8::String::NewFromUtf8(isolate, ctx->result.c_str()).ToLocalChecked();
        }
        (void)task->callback.Get(isolate)->Call(context, context->Global(), 2, argv);
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        std::error_code ec;
        fs::path p = fs::canonical(ctx->path, ec);
        if (ec) {
            ctx->is_error = true;
            ctx->error_msg = ec.message();
        } else {
            ctx->result = p.string();
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

void FS::RealpathPromise(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    if (args.Length() < 1 || !args[0]->IsString()) return;

    v8::Local<v8::Promise::Resolver> resolver;
    if (!v8::Promise::Resolver::New(context).ToLocal(&resolver)) return;
    args.GetReturnValue().Set(resolver->GetPromise());

    v8::String::Utf8Value path(isolate, args[0]);
    auto ctx = new RealpathContext();
    ctx->path = *path;

    Task* task = new Task();
    task->resolver.Reset(isolate, resolver);
    task->is_promise = true;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<RealpathContext*>(task->data);
        auto resolver = task->resolver.Get(isolate);
        if (ctx->is_error) {
            resolver->Reject(context, v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked())).Check();
        } else {
            resolver->Resolve(context, v8::String::NewFromUtf8(isolate, ctx->result.c_str()).ToLocalChecked()).Check();
        }
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        std::error_code ec;
        fs::path p = fs::canonical(ctx->path, ec);
        if (ec) {
            ctx->is_error = true;
            ctx->error_msg = ec.message();
        } else {
            ctx->result = p.string();
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

// --- Chmod ---
struct ChmodContext {
    std::string path;
    int mode;
    bool is_error = false;
    std::string error_msg;
};

void FS::Chmod(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 3 || !args[0]->IsString() || !args[1]->IsNumber() || !args[args.Length()-1]->IsFunction()) return;

    v8::String::Utf8Value path(isolate, args[0]);
    int mode = args[1]->Int32Value(isolate->GetCurrentContext()).FromMaybe(0);
    v8::Local<v8::Function> cb = args[args.Length()-1].As<v8::Function>();

    auto ctx = new ChmodContext();
    ctx->path = *path;
    ctx->mode = mode;

    Task* task = new Task();
    task->callback.Reset(isolate, cb);
    task->is_promise = false;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<ChmodContext*>(task->data);
        v8::Local<v8::Value> argv[1];
        if (ctx->is_error) {
            argv[0] = v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked());
        } else {
            argv[0] = v8::Null(isolate);
        }
        (void)task->callback.Get(isolate)->Call(context, context->Global(), 1, argv);
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        std::error_code ec;
        fs::permissions(ctx->path, static_cast<fs::perms>(ctx->mode), ec);
        if (ec) {
            ctx->is_error = true;
            ctx->error_msg = ec.message();
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

void FS::ChmodPromise(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsNumber()) return;

    v8::Local<v8::Promise::Resolver> resolver;
    if (!v8::Promise::Resolver::New(context).ToLocal(&resolver)) return;
    args.GetReturnValue().Set(resolver->GetPromise());

    v8::String::Utf8Value path(isolate, args[0]);
    int mode = args[1]->Int32Value(context).FromMaybe(0);

    auto ctx = new ChmodContext();
    ctx->path = *path;
    ctx->mode = mode;

    Task* task = new Task();
    task->resolver.Reset(isolate, resolver);
    task->is_promise = true;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<ChmodContext*>(task->data);
        auto resolver = task->resolver.Get(isolate);
        if (ctx->is_error) {
            resolver->Reject(context, v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked())).Check();
        } else {
            resolver->Resolve(context, v8::Undefined(isolate)).Check();
        }
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        std::error_code ec;
        fs::permissions(ctx->path, static_cast<fs::perms>(ctx->mode), ec);
        if (ec) {
            ctx->is_error = true;
            ctx->error_msg = ec.message();
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

// --- Readlink ---
struct ReadlinkContext {
    std::string path;
    std::string result;
    bool is_error = false;
    std::string error_msg;
};

void FS::Readlink(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 2 || !args[0]->IsString() || !args[args.Length()-1]->IsFunction()) return;

    v8::String::Utf8Value path(isolate, args[0]);
    v8::Local<v8::Function> cb = args[args.Length()-1].As<v8::Function>();

    auto ctx = new ReadlinkContext();
    ctx->path = *path;

    Task* task = new Task();
    task->callback.Reset(isolate, cb);
    task->is_promise = false;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<ReadlinkContext*>(task->data);
        v8::Local<v8::Value> argv[2];
        if (ctx->is_error) {
            argv[0] = v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked());
            argv[1] = v8::Undefined(isolate);
        } else {
            argv[0] = v8::Null(isolate);
            argv[1] = v8::String::NewFromUtf8(isolate, ctx->result.c_str()).ToLocalChecked();
        }
        (void)task->callback.Get(isolate)->Call(context, context->Global(), 2, argv);
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        std::error_code ec;
        fs::path p = fs::read_symlink(ctx->path, ec);
        if (ec) {
            ctx->is_error = true;
            ctx->error_msg = ec.message();
        } else {
            ctx->result = p.string();
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

void FS::ReadlinkPromise(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    if (args.Length() < 1 || !args[0]->IsString()) return;

    v8::Local<v8::Promise::Resolver> resolver;
    if (!v8::Promise::Resolver::New(context).ToLocal(&resolver)) return;
    args.GetReturnValue().Set(resolver->GetPromise());

    v8::String::Utf8Value path(isolate, args[0]);
    auto ctx = new ReadlinkContext();
    ctx->path = *path;

    Task* task = new Task();
    task->resolver.Reset(isolate, resolver);
    task->is_promise = true;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<ReadlinkContext*>(task->data);
        auto resolver = task->resolver.Get(isolate);
        if (ctx->is_error) {
            resolver->Reject(context, v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked())).Check();
        } else {
            resolver->Resolve(context, v8::String::NewFromUtf8(isolate, ctx->result.c_str()).ToLocalChecked()).Check();
        }
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        std::error_code ec;
        fs::path p = fs::read_symlink(ctx->path, ec);
        if (ec) {
            ctx->is_error = true;
            ctx->error_msg = ec.message();
        } else {
            ctx->result = p.string();
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

// --- Symlink ---
struct SymlinkContext {
    std::string target;
    std::string path;
    bool is_error = false;
    std::string error_msg;
};

void FS::Symlink(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 3 || !args[0]->IsString() || !args[1]->IsString() || !args[args.Length()-1]->IsFunction()) return;

    v8::String::Utf8Value target(isolate, args[0]);
    v8::String::Utf8Value path(isolate, args[1]);
    v8::Local<v8::Function> cb = args[args.Length()-1].As<v8::Function>();

    auto ctx = new SymlinkContext();
    ctx->target = *target;
    ctx->path = *path;

    Task* task = new Task();
    task->callback.Reset(isolate, cb);
    task->is_promise = false;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<SymlinkContext*>(task->data);
        v8::Local<v8::Value> argv[1];
        if (ctx->is_error) {
            argv[0] = v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked());
        } else {
            argv[0] = v8::Null(isolate);
        }
        (void)task->callback.Get(isolate)->Call(context, context->Global(), 1, argv);
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        std::error_code ec;
        fs::create_symlink(ctx->target, ctx->path, ec);
        if (ec) {
            ctx->is_error = true;
            ctx->error_msg = ec.message();
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

void FS::SymlinkPromise(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsString()) return;

    v8::Local<v8::Promise::Resolver> resolver;
    if (!v8::Promise::Resolver::New(context).ToLocal(&resolver)) return;
    args.GetReturnValue().Set(resolver->GetPromise());

    v8::String::Utf8Value target(isolate, args[0]);
    v8::String::Utf8Value path(isolate, args[1]);

    auto ctx = new SymlinkContext();
    ctx->target = *target;
    ctx->path = *path;

    Task* task = new Task();
    task->resolver.Reset(isolate, resolver);
    task->is_promise = true;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<SymlinkContext*>(task->data);
        auto resolver = task->resolver.Get(isolate);
        if (ctx->is_error) {
            resolver->Reject(context, v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked())).Check();
        } else {
            resolver->Resolve(context, v8::Undefined(isolate)).Check();
        }
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        std::error_code ec;
        fs::create_symlink(ctx->target, ctx->path, ec);
        if (ec) {
            ctx->is_error = true;
            ctx->error_msg = ec.message();
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

// --- Lstat ---
void FS::Lstat(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 2 || !args[0]->IsString() || !args[args.Length()-1]->IsFunction()) return;

    v8::String::Utf8Value path(isolate, args[0]);
    v8::Local<v8::Function> cb = args[args.Length()-1].As<v8::Function>();

    auto ctx = new StatContext();
    ctx->path = *path;
    ctx->follow_symlink = false;

    Task* task = new Task();
    task->callback.Reset(isolate, cb);
    task->is_promise = false;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<StatContext*>(task->data);
        v8::Local<v8::Value> argv[2];
        if (ctx->is_error) {
            argv[0] = v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked());
            argv[1] = v8::Undefined(isolate);
        } else {
            argv[0] = v8::Null(isolate);
            argv[1] = FS::CreateStats(isolate, ctx->path, ctx->ec, ctx->follow_symlink);
        }
        (void)task->callback.Get(isolate)->Call(context, context->Global(), 2, argv);
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        std::error_code ec;
        if (!fs::exists(ctx->path, ec) && !fs::is_symlink(fs::symlink_status(ctx->path, ec))) {
            ctx->is_error = true;
            ctx->error_msg = "ENOENT: no such file or directory";
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

void FS::LstatPromise(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    if (args.Length() < 1 || !args[0]->IsString()) return;

    v8::Local<v8::Promise::Resolver> resolver;
    if (!v8::Promise::Resolver::New(context).ToLocal(&resolver)) return;
    args.GetReturnValue().Set(resolver->GetPromise());

    v8::String::Utf8Value path(isolate, args[0]);
    auto ctx = new StatContext();
    ctx->path = *path;
    ctx->follow_symlink = false;

    Task* task = new Task();
    task->resolver.Reset(isolate, resolver);
    task->is_promise = true;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<StatContext*>(task->data);
        auto resolver = task->resolver.Get(isolate);
        if (ctx->is_error) {
            resolver->Reject(context, v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked())).Check();
        } else {
            resolver->Resolve(context, FS::CreateStats(isolate, ctx->path, ctx->ec, ctx->follow_symlink)).Check();
        }
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        std::error_code ec;
        if (!fs::exists(ctx->path, ec) && !fs::is_symlink(fs::symlink_status(ctx->path, ec))) {
            ctx->is_error = true;
            ctx->error_msg = "ENOENT: no such file or directory";
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

} // namespace module
} // namespace z8
