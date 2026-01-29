#include "fs.h"
#include <v8-isolate.h>
#include <fstream>
#include <filesystem>
#include <string>
#include <chrono>
#include <fcntl.h> // For O_* constants
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#include <windows.h>
#include <process.h>
#define ssize_t long long

static ssize_t pread(int fd, void* buf, size_t count, int64_t offset) {
    HANDLE h = (HANDLE)_get_osfhandle(fd);
    if (h == INVALID_HANDLE_VALUE) return -1;
    OVERLAPPED ov = {0};
    ov.Offset = (DWORD)(offset & 0xFFFFFFFF);
    ov.OffsetHigh = (DWORD)(offset >> 32);
    DWORD bytesRead = 0;
    if (ReadFile(h, buf, (DWORD)count, &bytesRead, &ov)) return (ssize_t)bytesRead;
    if (GetLastError() == ERROR_HANDLE_EOF) return 0;
    return -1;
}

static ssize_t pwrite(int fd, const void* buf, size_t count, int64_t offset) {
    HANDLE h = (HANDLE)_get_osfhandle(fd);
    if (h == INVALID_HANDLE_VALUE) return -1;
    OVERLAPPED ov = {0};
    ov.Offset = (DWORD)(offset & 0xFFFFFFFF);
    ov.OffsetHigh = (DWORD)(offset >> 32);
    DWORD bytesWritten = 0;
    if (WriteFile(h, buf, (DWORD)count, &bytesWritten, &ov)) return (ssize_t)bytesWritten;
    return -1;
}
#else
#include <unistd.h>
#endif
#include "thread_pool.h"
#include "task_queue.h"
#include <v8-promise.h>

#ifdef _WIN32
#undef CopyFile
#endif

namespace fs = std::filesystem;

namespace z8 {
namespace module {

// Helper to convert file_time_type to V8 Date
// Helper to convert V8 milliseconds to file_time_type
fs::file_time_type V8MillisecondsToFileTime(double ms) {
    auto system_time = std::chrono::system_clock::time_point(std::chrono::milliseconds(static_cast<long long>(ms)));
    return fs::file_time_type::clock::now() + (system_time - std::chrono::system_clock::now());
}

v8::Local<v8::Value> FileTimeToV8Date(v8::Isolate* isolate, fs::file_time_type ftime) {
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
    double ms = static_cast<double>(std::chrono::time_point_cast<std::chrono::milliseconds>(sctp).time_since_epoch().count());
    return v8::Date::New(isolate->GetCurrentContext(), ms).ToLocalChecked();
}

v8::Local<v8::ObjectTemplate> FS::CreateTemplate(v8::Isolate* isolate) {
    v8::Local<v8::ObjectTemplate> tmpl = v8::ObjectTemplate::New(isolate);

    tmpl->Set(v8::String::NewFromUtf8(isolate, "readFileSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, FS::ReadFileSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "writeFileSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, FS::WriteFileSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "existsSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, FS::ExistsSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "appendFileSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, FS::AppendFileSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "statSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, FS::StatSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "mkdirSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, FS::MkdirSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "rmSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, FS::RmSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "rmdirSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, FS::RmdirSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "unlinkSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, FS::UnlinkSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "lstatSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, FS::LstatSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "readdirSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, FS::ReaddirSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "renameSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, FS::RenameSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "copyFileSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, FS::CopyFileSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "realpathSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, FS::RealpathSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "accessSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, FS::AccessSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "chmodSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, FS::ChmodSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "chownSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, FS::ChownSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "utimesSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, FS::UtimesSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "readlinkSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, FS::ReadlinkSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "symlinkSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, FS::SymlinkSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "linkSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, FS::LinkSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "truncateSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, FS::TruncateSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "openSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, FS::OpenSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "readSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, FS::ReadSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "writeSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, FS::WriteSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "closeSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, FS::CloseSync));
    tmpl->Set(v8::String::NewFromUtf8(isolate, "fstatSync").ToLocalChecked(), v8::FunctionTemplate::New(isolate, FS::FstatSync));

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
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "readFile"), v8::FunctionTemplate::New(isolate, FS::ReadFile));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "writeFile"), v8::FunctionTemplate::New(isolate, FS::WriteFile));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "stat"), v8::FunctionTemplate::New(isolate, FS::Stat));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "unlink"), v8::FunctionTemplate::New(isolate, FS::Unlink));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "mkdir"), v8::FunctionTemplate::New(isolate, FS::Mkdir));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "readdir"), v8::FunctionTemplate::New(isolate, FS::Readdir));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "rmdir"), v8::FunctionTemplate::New(isolate, FS::Rmdir));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "rename"), v8::FunctionTemplate::New(isolate, FS::Rename));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "copyFile"), v8::FunctionTemplate::New(isolate, FS::CopyFile));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "access"), v8::FunctionTemplate::New(isolate, FS::Access));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "appendFile"), v8::FunctionTemplate::New(isolate, FS::AppendFile));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "realpath"), v8::FunctionTemplate::New(isolate, FS::Realpath));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "chmod"), v8::FunctionTemplate::New(isolate, FS::Chmod));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "readlink"), v8::FunctionTemplate::New(isolate, FS::Readlink));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "symlink"), v8::FunctionTemplate::New(isolate, FS::Symlink));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "lstat"), v8::FunctionTemplate::New(isolate, FS::Lstat));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "utimes"), v8::FunctionTemplate::New(isolate, FS::Utimes));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "link"), v8::FunctionTemplate::New(isolate, FS::Link));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "truncate"), v8::FunctionTemplate::New(isolate, FS::Truncate));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "open"), v8::FunctionTemplate::New(isolate, FS::Open));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "read"), v8::FunctionTemplate::New(isolate, FS::Read));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "write"), v8::FunctionTemplate::New(isolate, FS::Write));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "close"), v8::FunctionTemplate::New(isolate, FS::Close));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "fstat"), v8::FunctionTemplate::New(isolate, FS::Fstat));

    // Expose fs.promises
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "promises"), CreatePromisesTemplate(isolate));

    return tmpl;
}

v8::Local<v8::ObjectTemplate> FS::CreatePromisesTemplate(v8::Isolate* isolate) {
    v8::Local<v8::ObjectTemplate> tmpl = v8::ObjectTemplate::New(isolate);
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "readFile"), v8::FunctionTemplate::New(isolate, FS::ReadFilePromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "writeFile"), v8::FunctionTemplate::New(isolate, FS::WriteFilePromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "stat"), v8::FunctionTemplate::New(isolate, FS::StatPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "unlink"), v8::FunctionTemplate::New(isolate, FS::UnlinkPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "mkdir"), v8::FunctionTemplate::New(isolate, FS::MkdirPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "readdir"), v8::FunctionTemplate::New(isolate, FS::ReaddirPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "rmdir"), v8::FunctionTemplate::New(isolate, FS::RmdirPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "rename"), v8::FunctionTemplate::New(isolate, FS::RenamePromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "copyFile"), v8::FunctionTemplate::New(isolate, FS::CopyFilePromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "access"), v8::FunctionTemplate::New(isolate, FS::AccessPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "appendFile"), v8::FunctionTemplate::New(isolate, FS::AppendFilePromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "realpath"), v8::FunctionTemplate::New(isolate, FS::RealpathPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "chmod"), v8::FunctionTemplate::New(isolate, FS::ChmodPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "readlink"), v8::FunctionTemplate::New(isolate, FS::ReadlinkPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "symlink"), v8::FunctionTemplate::New(isolate, FS::SymlinkPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "lstat"), v8::FunctionTemplate::New(isolate, FS::LstatPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "utimes"), v8::FunctionTemplate::New(isolate, FS::UtimesPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "link"), v8::FunctionTemplate::New(isolate, FS::LinkPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "truncate"), v8::FunctionTemplate::New(isolate, FS::TruncatePromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "open"), v8::FunctionTemplate::New(isolate, FS::OpenPromise));
    tmpl->Set(v8::String::NewFromUtf8Literal(isolate, "fstat"), v8::FunctionTemplate::New(isolate, FS::FstatPromise));
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
    
    auto mtime = follow_symlink ? fs::last_write_time(path, ec) : fs::last_write_time(path, ec); // std::filesystem doesn't have symlink_last_write_time? 
    if (!ec) {
        // Correct conversion to UNIX timestamp (ms)
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(mtime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
        double ms = static_cast<double>(std::chrono::time_point_cast<std::chrono::milliseconds>(sctp).time_since_epoch().count());

        stats->Set(context, v8::String::NewFromUtf8Literal(isolate, "mtime"), v8::Date::New(context, ms).ToLocalChecked()).Check();
        stats->Set(context, v8::String::NewFromUtf8Literal(isolate, "mtimeMs"), v8::Number::New(isolate, ms)).Check();
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
    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "TypeError: Path must be a string").ToLocalChecked());
        return;
    }
    v8::String::Utf8Value path(isolate, args[0]);
    std::error_code ec;
    fs::remove(*path, ec);
    if (ec) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, ("Error rmdir: " + ec.message()).c_str()).ToLocalChecked());
    }
}

void FS::UnlinkSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "TypeError: Path must be a string").ToLocalChecked());
        return;
    }
    v8::String::Utf8Value path(isolate, args[0]);
    std::error_code ec;
    fs::remove(*path, ec);
    if (ec) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, ("Error unlink: " + ec.message()).c_str()).ToLocalChecked());
    }
}

void FS::ChmodSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsInt32()) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "TypeError: Path must be a string and mode must be an integer").ToLocalChecked());
        return;
    }
    v8::String::Utf8Value path(isolate, args[0]);
    int mode = args[1]->Int32Value(context).FromMaybe(0);
    std::error_code ec;
    fs::permissions(*path, static_cast<fs::perms>(mode), ec);
    if (ec) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, ("Error chmod: " + ec.message()).c_str()).ToLocalChecked());
        return;
    }
}

void FS::ChownSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // std::filesystem doesn't support chown easily, would need platform specific code (POSIX)
    v8::Isolate* isolate = args.GetIsolate();
    isolate->ThrowException(v8::String::NewFromUtf8(isolate, "Error: chown is not supported on this platform or by std::filesystem").ToLocalChecked());
    return;
}

void FS::UtimesSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    if (args.Length() < 3 || !args[0]->IsString() || !args[1]->IsNumber() || !args[2]->IsNumber()) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "TypeError: Path must be a string, atime and mtime must be numbers").ToLocalChecked());
        return;
    }
    v8::String::Utf8Value path(isolate, args[0]);
    double atime = args[1]->NumberValue(context).FromMaybe(0);
    double mtime = args[2]->NumberValue(context).FromMaybe(0);
    
    std::error_code ec;
    // Note: std::filesystem only supports last_write_time (mtime), not atime directly.
    // For full utimes functionality, platform-specific APIs would be needed.
    fs::last_write_time(*path, V8MillisecondsToFileTime(mtime), ec); 
    if (ec) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, ("Error setting times: " + ec.message()).c_str()).ToLocalChecked());
        return;
    }
}

void FS::ReadlinkSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "TypeError: Path must be a string").ToLocalChecked());
        return;
    }
    v8::String::Utf8Value path(isolate, args[0]);
    std::error_code ec;
    fs::path target = fs::read_symlink(*path, ec);
    if (ec) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, ("Error readlink: " + ec.message()).c_str()).ToLocalChecked());
        return;
    }
    args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, target.string().c_str()).ToLocalChecked());
}

void FS::SymlinkSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsString()) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "TypeError: Target and path must be strings").ToLocalChecked());
        return;
    }
    v8::String::Utf8Value target_val(isolate, args[0]);
    v8::String::Utf8Value path_val(isolate, args[1]);
    std::string target_str = *target_val;
    std::string path_str = *path_val;
    std::error_code ec;

    // Optional 'type' argument (args[2])
    // For simplicity, we'll infer type from target or default to file symlink.
    // A more robust implementation would parse "dir", "file", "junction" from args[2].
    bool is_dir_symlink = false;
    if (args.Length() >= 3 && args[2]->IsString()) {
        v8::String::Utf8Value type_val(isolate, args[2]);
        std::string type_str = *type_val;
        if (type_str == "dir" || type_str == "junction") {
            is_dir_symlink = true;
        } else if (type_str == "file") {
            is_dir_symlink = false;
        }
    } else {
        // If type is not specified, try to infer from target
        is_dir_symlink = fs::is_directory(target_str, ec);
        if (ec) {
            // If is_directory check fails, proceed assuming file symlink or throw if critical
            ec.clear(); // Clear error for is_directory check if we're just inferring
        }
    }

    if (is_dir_symlink) {
        fs::create_directory_symlink(target_str, path_str, ec);
    } else {
        fs::create_symlink(target_str, path_str, ec);
    }
    
    if (ec) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, ("Error symlink: " + ec.message()).c_str()).ToLocalChecked());
        return;
    }
}

void FS::LinkSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsString()) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "TypeError: Target and path must be strings").ToLocalChecked());
        return;
    }
    v8::String::Utf8Value target(isolate, args[0]);
    v8::String::Utf8Value path(isolate, args[1]);
    std::error_code ec;
    fs::create_hard_link(*target, *path, ec);
    if (ec) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, ("Error link: " + ec.message()).c_str()).ToLocalChecked());
        return;
    }
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
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    auto context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "TypeError: Path must be a string").ToLocalChecked());
        return;
    }

    v8::String::Utf8Value path(isolate, args[0]);
    int flags = 0;
    
    if (args.Length() >= 2) {
        if (args[1]->IsInt32()) {
            flags = args[1]->Int32Value(context).FromMaybe(0);
        } else if (args[1]->IsString()) {
            v8::String::Utf8Value flags_str(isolate, args[1]);
            std::string f(*flags_str);
            if (f == "r") flags = O_RDONLY;
            else if (f == "r+") flags = O_RDWR;
            else if (f == "w") flags = O_WRONLY | O_CREAT | O_TRUNC;
            else if (f == "w+") flags = O_RDWR | O_CREAT | O_TRUNC;
            else if (f == "a") flags = O_WRONLY | O_CREAT | O_APPEND;
            else if (f == "a+") flags = O_RDWR | O_CREAT | O_APPEND;
            else flags = O_RDONLY;
        }
    } else {
        flags = O_RDONLY;
    }

    int mode = 0666;
    if (args.Length() >= 3 && args[2]->IsInt32()) {
        mode = args[2]->Int32Value(context).FromMaybe(0666);
    }

#ifdef _WIN32
    // On Windows, we need _O_BINARY usually for node compatibility
    int fd = _open(*path, flags | _O_BINARY, mode);
#else
    int fd = open(*path, flags, mode);
#endif

    if (fd == -1) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "Error: Could not open file").ToLocalChecked());
        return;
    }

    args.GetReturnValue().Set(fd);
}

void FS::ReadSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    auto context = isolate->GetCurrentContext();

    if (args.Length() < 2 || !args[0]->IsInt32() || !args[1]->IsUint8Array()) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "TypeError: Invalid arguments").ToLocalChecked());
        return;
    }

    int fd = args[0]->Int32Value(context).FromMaybe(-1);
    v8::Local<v8::Uint8Array> buffer = args[1].As<v8::Uint8Array>();
    
    size_t offset = 0;
    if (args.Length() >= 3 && args[2]->IsNumber()) {
        offset = static_cast<size_t>(args[2]->NumberValue(context).FromMaybe(0));
    }

    size_t length = buffer->ByteLength() - offset;
    if (args.Length() >= 4 && args[3]->IsNumber()) {
        length = static_cast<size_t>(args[3]->NumberValue(context).FromMaybe(length));
    }

    int64_t position = -1;
    if (args.Length() >= 5 && args[4]->IsNumber()) {
        position = static_cast<int64_t>(args[4]->NumberValue(context).FromMaybe(-1));
    }

    char* data = static_cast<char*>(buffer->Buffer()->GetBackingStore()->Data()) + buffer->ByteOffset() + offset;

#ifdef _WIN32
    if (position != -1) {
        _lseeki64(fd, position, SEEK_SET);
    }
    int bytesRead = _read(fd, data, (unsigned int)length);
#else
    ssize_t bytesRead;
    if (position != -1) {
        bytesRead = pread(fd, data, length, position);
    } else {
        bytesRead = read(fd, data, length);
    }
#endif

    if (bytesRead == -1) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "Error: Could not read from file").ToLocalChecked());
        return;
    }

    args.GetReturnValue().Set(bytesRead);
}

void FS::WriteSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    auto context = isolate->GetCurrentContext();

    if (args.Length() < 2 || !args[0]->IsInt32()) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "TypeError: Invalid arguments").ToLocalChecked());
        return;
    }

    int fd = args[0]->Int32Value(context).FromMaybe(-1);
    
    const char* data = nullptr;
    size_t length = 0;
    std::string str_data;

    if (args[1]->IsString()) {
        v8::String::Utf8Value utf8(isolate, args[1]);
        str_data = *utf8;
        data = str_data.c_str();
        length = str_data.length();
    } else if (args[1]->IsUint8Array()) {
        v8::Local<v8::Uint8Array> buffer = args[1].As<v8::Uint8Array>();
        data = static_cast<const char*>(buffer->Buffer()->GetBackingStore()->Data()) + buffer->ByteOffset();
        length = buffer->ByteLength();
    } else {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "TypeError: Data must be a string or Uint8Array").ToLocalChecked());
        return;
    }

    int64_t position = -1;
    if (args.Length() >= 3 && args[2]->IsNumber()) {
        position = static_cast<int64_t>(args[2]->NumberValue(context).FromMaybe(-1));
    }

#ifdef _WIN32
    if (position != -1) {
        _lseeki64(fd, position, SEEK_SET);
    }
    int bytesWritten = _write(fd, data, (unsigned int)length);
#else
    ssize_t bytesWritten;
    if (position != -1) {
        bytesWritten = pwrite(fd, data, length, position);
    } else {
        bytesWritten = write(fd, data, length);
    }
#endif

    if (bytesWritten == -1) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "Error: Could not write to file").ToLocalChecked());
        return;
    }

    args.GetReturnValue().Set(bytesWritten);
}

void FS::CloseSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    auto context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsInt32()) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "TypeError: FD must be an integer").ToLocalChecked());
        return;
    }

    int fd = args[0]->Int32Value(context).FromMaybe(-1);
#ifdef _WIN32
    int result = _close(fd);
#else
    int result = close(fd);
#endif

    if (result == -1) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "Error: Could not close file").ToLocalChecked());
        return;
    }
}

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

// --- Utimes ---
struct UtimesContext {
    std::string path;
    double atime;
    double mtime;
    bool is_error = false;
    std::string error_msg;
};

void FS::Utimes(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 4 || !args[0]->IsString() || !args[1]->IsNumber() || !args[2]->IsNumber() || !args[args.Length()-1]->IsFunction()) return;

    v8::String::Utf8Value path(isolate, args[0]);
    double atime = args[1]->NumberValue(isolate->GetCurrentContext()).FromMaybe(0);
    double mtime = args[2]->NumberValue(isolate->GetCurrentContext()).FromMaybe(0);
    v8::Local<v8::Function> cb = args[args.Length()-1].As<v8::Function>();

    auto ctx = new UtimesContext();
    ctx->path = *path;
    ctx->atime = atime;
    ctx->mtime = mtime;

    Task* task = new Task();
    task->callback.Reset(isolate, cb);
    task->is_promise = false;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<UtimesContext*>(task->data);
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
        fs::last_write_time(ctx->path, V8MillisecondsToFileTime(ctx->mtime), ec);
        if (ec) {
            ctx->is_error = true;
            ctx->error_msg = ec.message();
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

void FS::UtimesPromise(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    if (args.Length() < 3 || !args[0]->IsString() || !args[1]->IsNumber() || !args[2]->IsNumber()) return;

    v8::Local<v8::Promise::Resolver> resolver;
    if (!v8::Promise::Resolver::New(context).ToLocal(&resolver)) return;
    args.GetReturnValue().Set(resolver->GetPromise());

    v8::String::Utf8Value path(isolate, args[0]);
    double atime = args[1]->NumberValue(context).FromMaybe(0);
    double mtime = args[2]->NumberValue(context).FromMaybe(0);

    auto ctx = new UtimesContext();
    ctx->path = *path;
    ctx->atime = atime;
    ctx->mtime = mtime;

    Task* task = new Task();
    task->resolver.Reset(isolate, resolver);
    task->is_promise = true;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<UtimesContext*>(task->data);
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
        fs::last_write_time(ctx->path, V8MillisecondsToFileTime(ctx->mtime), ec);
        if (ec) {
            ctx->is_error = true;
            ctx->error_msg = ec.message();
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

// --- Link ---
struct LinkContext {
    std::string existing_path;
    std::string new_path;
    bool is_error = false;
    std::string error_msg;
};

void FS::Link(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 3 || !args[0]->IsString() || !args[1]->IsString() || !args[args.Length()-1]->IsFunction()) return;

    v8::String::Utf8Value existing_path(isolate, args[0]);
    v8::String::Utf8Value new_path(isolate, args[1]);
    v8::Local<v8::Function> cb = args[args.Length()-1].As<v8::Function>();

    auto ctx = new LinkContext();
    ctx->existing_path = *existing_path;
    ctx->new_path = *new_path;

    Task* task = new Task();
    task->callback.Reset(isolate, cb);
    task->is_promise = false;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<LinkContext*>(task->data);
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
        fs::create_hard_link(ctx->existing_path, ctx->new_path, ec);
        if (ec) {
            ctx->is_error = true;
            ctx->error_msg = ec.message();
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

void FS::LinkPromise(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsString()) return;

    v8::Local<v8::Promise::Resolver> resolver;
    if (!v8::Promise::Resolver::New(context).ToLocal(&resolver)) return;
    args.GetReturnValue().Set(resolver->GetPromise());

    v8::String::Utf8Value existing_path(isolate, args[0]);
    v8::String::Utf8Value new_path(isolate, args[1]);

    auto ctx = new LinkContext();
    ctx->existing_path = *existing_path;
    ctx->new_path = *new_path;

    Task* task = new Task();
    task->resolver.Reset(isolate, resolver);
    task->is_promise = true;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<LinkContext*>(task->data);
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
        fs::create_hard_link(ctx->existing_path, ctx->new_path, ec);
        if (ec) {
            ctx->is_error = true;
            ctx->error_msg = ec.message();
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

// --- Truncate ---
struct TruncateContext {
    std::string path;
    uintmax_t length;
    bool is_error = false;
    std::string error_msg;
};

void FS::Truncate(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 2 || !args[0]->IsString()) return;

    v8::String::Utf8Value path(isolate, args[0]);
    uintmax_t length = 0;
    v8::Local<v8::Function> cb;

    if (args[1]->IsFunction()) {
        cb = args[1].As<v8::Function>();
    } else if (args.Length() > 2 && args[1]->IsNumber() && args[2]->IsFunction()) {
        length = static_cast<uintmax_t>(args[1]->NumberValue(isolate->GetCurrentContext()).FromMaybe(0));
        cb = args[2].As<v8::Function>();
    } else {
        return;
    }

    auto ctx = new TruncateContext();
    ctx->path = *path;
    ctx->length = length;

    Task* task = new Task();
    task->callback.Reset(isolate, cb);
    task->is_promise = false;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<TruncateContext*>(task->data);
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
        fs::resize_file(ctx->path, ctx->length, ec);
        if (ec) {
            ctx->is_error = true;
            ctx->error_msg = ec.message();
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

void FS::TruncatePromise(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    if (args.Length() < 1 || !args[0]->IsString()) return;

    v8::Local<v8::Promise::Resolver> resolver;
    if (!v8::Promise::Resolver::New(context).ToLocal(&resolver)) return;
    args.GetReturnValue().Set(resolver->GetPromise());

    v8::String::Utf8Value path(isolate, args[0]);
    uintmax_t length = 0;
    if (args.Length() > 1 && args[1]->IsNumber()) {
        length = static_cast<uintmax_t>(args[1]->NumberValue(context).FromMaybe(0));
    }

    auto ctx = new TruncateContext();
    ctx->path = *path;
    ctx->length = length;

    Task* task = new Task();
    task->resolver.Reset(isolate, resolver);
    task->is_promise = true;
    task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<TruncateContext*>(task->data);
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
        fs::resize_file(ctx->path, ctx->length, ec);
        if (ec) {
            ctx->is_error = true;
            ctx->error_msg = ec.message();
        }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

void FS::FstatSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    auto context = isolate->GetCurrentContext();
    if (args.Length() < 1 || !args[0]->IsInt32()) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "TypeError: FD must be an integer").ToLocalChecked());
        return;
    }
    int fd = args[0]->Int32Value(context).FromMaybe(-1);
#ifdef _WIN32
    struct _stat64 st;
    if (_fstat64(fd, &st) != 0) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "Error: Could not stat file descriptor").ToLocalChecked());
        return;
    }
#else
    struct stat st;
    if (fstat(fd, &st) != 0) {
        isolate->ThrowException(v8::String::NewFromUtf8(isolate, "Error: Could not stat file descriptor").ToLocalChecked());
        return;
    }
#endif
    v8::Local<v8::Object> stats = v8::Object::New(isolate);
    stats->Set(context, v8::String::NewFromUtf8Literal(isolate, "size"), v8::Number::New(isolate, static_cast<double>(st.st_size))).Check();
    stats->Set(context, v8::String::NewFromUtf8Literal(isolate, "mtimeMs"), v8::Number::New(isolate, static_cast<double>(st.st_mtime) * 1000)).Check();
    bool is_dir = (st.st_mode & S_IFMT) == S_IFDIR;
    bool is_file = (st.st_mode & S_IFMT) == S_IFREG;
    stats->Set(context, v8::String::NewFromUtf8Literal(isolate, "isDirectory"), 
        v8::FunctionTemplate::New(isolate, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            args.GetReturnValue().Set(args.Data().As<v8::Boolean>());
        }, v8::Boolean::New(isolate, is_dir))->GetFunction(context).ToLocalChecked()).Check();
    stats->Set(context, v8::String::NewFromUtf8Literal(isolate, "isFile"), 
        v8::FunctionTemplate::New(isolate, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            args.GetReturnValue().Set(args.Data().As<v8::Boolean>());
        }, v8::Boolean::New(isolate, is_file))->GetFunction(context).ToLocalChecked()).Check();
    args.GetReturnValue().Set(stats);
}

struct OpenContext {
    std::string path; int flags; int mode; int result_fd = -1;
    bool is_error = false; std::string error_msg;
};

void FS::Open(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 2 || !args[0]->IsString() || !args[args.Length()-1]->IsFunction()) return;
    v8::String::Utf8Value path(isolate, args[0]);
    v8::Local<v8::Function> cb = args[args.Length()-1].As<v8::Function>();
    int flags = 0;
    if (args.Length() >= 2) {
        if (args[1]->IsInt32()) flags = args[1]->Int32Value(isolate->GetCurrentContext()).FromMaybe(O_RDONLY);
        else if (args[1]->IsString()) {
            v8::String::Utf8Value f_str(isolate, args[1]); std::string f(*f_str);
            if (f == "r") flags = O_RDONLY; else if (f == "r+") flags = O_RDWR;
            else if (f == "w") flags = O_WRONLY | O_CREAT | O_TRUNC;
            else if (f == "w+") flags = O_RDWR | O_CREAT | O_TRUNC;
            else if (f == "a") flags = O_WRONLY | O_CREAT | O_APPEND;
            else if (f == "a+") flags = O_RDWR | O_CREAT | O_APPEND;
            else flags = O_RDONLY;
        }
    }
    int mode = 0666;
    if (args.Length() >= 3 && args[2]->IsInt32()) mode = args[2]->Int32Value(isolate->GetCurrentContext()).FromMaybe(0666);
    auto ctx = new OpenContext(); ctx->path = *path; ctx->flags = flags; ctx->mode = mode;
    Task* task = new Task(); task->callback.Reset(isolate, cb); task->is_promise = false; task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<OpenContext*>(task->data);
        v8::Local<v8::Value> argv[2];
        if (ctx->is_error) {
            argv[0] = v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked());
            argv[1] = v8::Undefined(isolate);
        } else {
            argv[0] = v8::Null(isolate); argv[1] = v8::Integer::New(isolate, ctx->result_fd);
        }
        (void)task->callback.Get(isolate)->Call(context, context->Global(), 2, argv);
        delete ctx;
    };
    ThreadPool::GetInstance().Enqueue([task, ctx]() {
#ifdef _WIN32
        ctx->result_fd = _open(ctx->path.c_str(), ctx->flags | _O_BINARY, ctx->mode);
#else
        ctx->result_fd = open(ctx->path.c_str(), ctx->flags, ctx->mode);
#endif
        if (ctx->result_fd == -1) { ctx->is_error = true; ctx->error_msg = "Could not open file"; }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

void FS::OpenPromise(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate(); v8::Local<v8::Context> context = isolate->GetCurrentContext();
    if (args.Length() < 1 || !args[0]->IsString()) return;
    v8::Local<v8::Promise::Resolver> resolver;
    if (!v8::Promise::Resolver::New(context).ToLocal(&resolver)) return;
    args.GetReturnValue().Set(resolver->GetPromise());
    v8::String::Utf8Value path(isolate, args[0]);
    int flags = O_RDONLY;
    if (args.Length() >= 2 && args[1]->IsInt32()) flags = args[1]->Int32Value(context).FromMaybe(O_RDONLY);
    auto ctx = new OpenContext(); ctx->path = *path; ctx->flags = flags; ctx->mode = 0666;
    Task* task = new Task(); task->resolver.Reset(isolate, resolver); task->is_promise = true; task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<OpenContext*>(task->data);
        auto resolver = task->resolver.Get(isolate);
        if (ctx->is_error) resolver->Reject(context, v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked())).Check();
        else resolver->Resolve(context, v8::Integer::New(isolate, ctx->result_fd)).Check();
        delete ctx;
    };
    ThreadPool::GetInstance().Enqueue([task, ctx]() {
#ifdef _WIN32
        ctx->result_fd = _open(ctx->path.c_str(), ctx->flags | _O_BINARY, ctx->mode);
#else
        ctx->result_fd = open(ctx->path.c_str(), ctx->flags, ctx->mode);
#endif
        if (ctx->result_fd == -1) { ctx->is_error = true; ctx->error_msg = "Could not open file"; }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

struct RWContext {
    int fd; void* buffer_data; size_t length; int64_t position;
    int result_count = 0; bool is_error = false; std::string error_msg;
    v8::Global<v8::Value> buffer_keep_alive;
};

void FS::Read(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate(); v8::Local<v8::Context> context = isolate->GetCurrentContext();
    if (args.Length() < 2 || !args[0]->IsInt32() || !args[1]->IsUint8Array() || !args[args.Length()-1]->IsFunction()) return;
    int fd = args[0]->Int32Value(context).FromMaybe(-1);
    v8::Local<v8::Uint8Array> ui8 = args[1].As<v8::Uint8Array>();
    v8::Local<v8::Function> cb = args[args.Length()-1].As<v8::Function>();
    size_t offset = 0;
    if (args.Length() >= 3 && args[2]->IsNumber()) offset = static_cast<size_t>(args[2]->NumberValue(context).FromMaybe(0));
    size_t length = ui8->ByteLength() - offset;
    if (args.Length() >= 4 && args[3]->IsNumber()) length = static_cast<size_t>(args[3]->NumberValue(context).FromMaybe(length));
    int64_t position = -1;
    if (args.Length() >= 5 && args[4]->IsNumber()) position = static_cast<int64_t>(args[4]->NumberValue(context).FromMaybe(-1));
    auto ctx = new RWContext(); ctx->fd = fd;
    ctx->buffer_data = static_cast<char*>(ui8->Buffer()->GetBackingStore()->Data()) + ui8->ByteOffset() + offset;
    ctx->length = length; ctx->position = position; ctx->buffer_keep_alive.Reset(isolate, ui8);
    Task* task = new Task(); task->callback.Reset(isolate, cb); task->is_promise = false; task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<RWContext*>(task->data); v8::Local<v8::Value> argv[3];
        if (ctx->is_error) {
            argv[0] = v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked());
            argv[1] = v8::Undefined(isolate); argv[2] = v8::Undefined(isolate);
        } else {
            argv[0] = v8::Null(isolate); argv[1] = v8::Integer::New(isolate, ctx->result_count); argv[2] = ctx->buffer_keep_alive.Get(isolate);
        }
        (void)task->callback.Get(isolate)->Call(context, context->Global(), 3, argv);
        ctx->buffer_keep_alive.Reset(); delete ctx;
    };
    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        if (ctx->position != -1) ctx->result_count = pread(ctx->fd, ctx->buffer_data, ctx->length, ctx->position);
        else {
#ifdef _WIN32
            ctx->result_count = _read(ctx->fd, ctx->buffer_data, (unsigned int)ctx->length);
#else
            ctx->result_count = read(ctx->fd, ctx->buffer_data, ctx->length);
#endif
        }
        if (ctx->result_count == -1) { ctx->is_error = true; ctx->error_msg = "Read error"; }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

void FS::Write(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate(); v8::Local<v8::Context> context = isolate->GetCurrentContext();
    if (args.Length() < 2 || !args[0]->IsInt32() || !args[args.Length()-1]->IsFunction()) return;
    int fd = args[0]->Int32Value(context).FromMaybe(-1);
    v8::Local<v8::Function> cb = args[args.Length()-1].As<v8::Function>();
    auto ctx = new RWContext(); ctx->fd = fd;
    if (args[1]->IsString()) {
        v8::String::Utf8Value utf8(isolate, args[1]); std::string str = *utf8;
        ctx->length = str.length(); ctx->buffer_data = malloc(ctx->length); memcpy(ctx->buffer_data, str.c_str(), ctx->length);
        ctx->position = -1;
    } else if (args[1]->IsUint8Array()) {
        v8::Local<v8::Uint8Array> ui8 = args[1].As<v8::Uint8Array>();
        ctx->length = ui8->ByteLength(); ctx->buffer_data = static_cast<char*>(ui8->Buffer()->GetBackingStore()->Data()) + ui8->ByteOffset();
        ctx->buffer_keep_alive.Reset(isolate, ui8); ctx->position = -1;
        if (args.Length() >= 3 && args[2]->IsNumber()) ctx->position = static_cast<int64_t>(args[2]->NumberValue(context).FromMaybe(-1));
    }
    Task* task = new Task(); task->callback.Reset(isolate, cb); task->is_promise = false; task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<RWContext*>(task->data); v8::Local<v8::Value> argv[3];
        if (ctx->is_error) {
            argv[0] = v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked());
            argv[1] = v8::Undefined(isolate); argv[2] = v8::Undefined(isolate);
        } else {
            argv[0] = v8::Null(isolate); argv[1] = v8::Integer::New(isolate, ctx->result_count);
            if (!ctx->buffer_keep_alive.IsEmpty()) argv[2] = ctx->buffer_keep_alive.Get(isolate);
            else argv[2] = v8::Undefined(isolate);
        }
        (void)task->callback.Get(isolate)->Call(context, context->Global(), 3, argv);
        if (!ctx->buffer_keep_alive.IsEmpty()) ctx->buffer_keep_alive.Reset(); else free(ctx->buffer_data);
        delete ctx;
    };
    ThreadPool::GetInstance().Enqueue([task, ctx]() {
        if (ctx->position != -1) ctx->result_count = pwrite(ctx->fd, ctx->buffer_data, ctx->length, ctx->position);
        else {
#ifdef _WIN32
            ctx->result_count = _write(ctx->fd, ctx->buffer_data, (unsigned int)ctx->length);
#else
            ctx->result_count = write(ctx->fd, ctx->buffer_data, ctx->length);
#endif
        }
        if (ctx->result_count == -1) { ctx->is_error = true; ctx->error_msg = "Write error"; }
        TaskQueue::GetInstance().Enqueue(task);
    });
}

void FS::Close(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 2 || !args[0]->IsInt32() || !args[args.Length()-1]->IsFunction()) return;
    int fd = args[0]->Int32Value(isolate->GetCurrentContext()).FromMaybe(-1);
    v8::Local<v8::Function> cb = args[args.Length()-1].As<v8::Function>();
    struct CloseCtx { int fd; bool is_error = false; };
    auto ctx = new CloseCtx(); ctx->fd = fd;
    Task* task = new Task(); task->callback.Reset(isolate, cb); task->is_promise = false; task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<CloseCtx*>(task->data); v8::Local<v8::Value> argv[1];
        if (ctx->is_error) argv[0] = v8::Exception::Error(v8::String::NewFromUtf8(isolate, "Close error").ToLocalChecked());
        else argv[0] = v8::Null(isolate);
        (void)task->callback.Get(isolate)->Call(context, context->Global(), 1, argv);
        delete ctx;
    };
    ThreadPool::GetInstance().Enqueue([task, ctx]() {
#ifdef _WIN32
        if (_close(ctx->fd) != 0) ctx->is_error = true;
#else
        if (close(ctx->fd) != 0) ctx->is_error = true;
#endif
        TaskQueue::GetInstance().Enqueue(task);
    });
}

void FS::Fstat(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 2 || !args[0]->IsInt32() || !args[args.Length()-1]->IsFunction()) return;
    int fd = args[0]->Int32Value(isolate->GetCurrentContext()).FromMaybe(-1);
    v8::Local<v8::Function> cb = args[args.Length()-1].As<v8::Function>();
    struct FstatCtx {
        int fd; bool is_error = false; std::string error_msg;
        uint64_t size; double mtime; bool is_dir; bool is_file;
    };
    auto ctx = new FstatCtx(); ctx->fd = fd;
    Task* task = new Task(); task->callback.Reset(isolate, cb); task->is_promise = false; task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<FstatCtx*>(task->data); v8::Local<v8::Value> argv[2];
        if (ctx->is_error) {
            argv[0] = v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked());
            argv[1] = v8::Undefined(isolate);
        } else {
            argv[0] = v8::Null(isolate);
            v8::Local<v8::Object> stats = v8::Object::New(isolate);
            stats->Set(context, v8::String::NewFromUtf8Literal(isolate, "size"), v8::Number::New(isolate, (double)ctx->size)).Check();
            stats->Set(context, v8::String::NewFromUtf8Literal(isolate, "mtimeMs"), v8::Number::New(isolate, ctx->mtime)).Check();
            stats->Set(context, v8::String::NewFromUtf8Literal(isolate, "isDirectory"), 
                v8::FunctionTemplate::New(isolate, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
                    args.GetReturnValue().Set(args.Data().As<v8::Boolean>());
                }, v8::Boolean::New(isolate, ctx->is_dir))->GetFunction(context).ToLocalChecked()).Check();
            stats->Set(context, v8::String::NewFromUtf8Literal(isolate, "isFile"), 
                v8::FunctionTemplate::New(isolate, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
                    args.GetReturnValue().Set(args.Data().As<v8::Boolean>());
                }, v8::Boolean::New(isolate, ctx->is_file))->GetFunction(context).ToLocalChecked()).Check();
            argv[1] = stats;
        }
        (void)task->callback.Get(isolate)->Call(context, context->Global(), 2, argv);
        delete ctx;
    };
    ThreadPool::GetInstance().Enqueue([task, ctx]() {
#ifdef _WIN32
        struct _stat64 st;
        if (_fstat64(ctx->fd, &st) == 0) {
            ctx->size = st.st_size; ctx->mtime = (double)st.st_mtime * 1000;
            ctx->is_dir = (st.st_mode & S_IFMT) == S_IFDIR; ctx->is_file = (st.st_mode & S_IFMT) == S_IFREG;
        } else { ctx->is_error = true; ctx->error_msg = "fstat error"; }
#else
        struct stat st;
        if (fstat(ctx->fd, &st) == 0) {
            ctx->size = st.st_size; ctx->mtime = (double)st.st_mtime * 1000;
            ctx->is_dir = S_ISDIR(st.st_mode); ctx->is_file = S_ISREG(st.st_mode);
        } else { ctx->is_error = true; ctx->error_msg = "fstat error"; }
#endif
        TaskQueue::GetInstance().Enqueue(task);
    });
}

void FS::FstatPromise(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate(); v8::Local<v8::Context> context = isolate->GetCurrentContext();
    if (args.Length() < 1 || !args[0]->IsInt32()) return;
    int fd = args[0]->Int32Value(context).FromMaybe(-1);
    v8::Local<v8::Promise::Resolver> resolver;
    if (!v8::Promise::Resolver::New(context).ToLocal(&resolver)) return;
    args.GetReturnValue().Set(resolver->GetPromise());

    struct FstatCtx {
        int fd; bool is_error = false; std::string error_msg;
        uint64_t size; double mtime; bool is_dir; bool is_file;
    };
    auto ctx = new FstatCtx(); ctx->fd = fd;

    Task* task = new Task(); task->resolver.Reset(isolate, resolver); task->is_promise = true; task->data = ctx;
    task->runner = [](v8::Isolate* isolate, v8::Local<v8::Context> context, Task* task) {
        auto ctx = static_cast<FstatCtx*>(task->data); auto resolver = task->resolver.Get(isolate);
        if (ctx->is_error) {
            resolver->Reject(context, v8::Exception::Error(v8::String::NewFromUtf8(isolate, ctx->error_msg.c_str()).ToLocalChecked())).Check();
        } else {
            v8::Local<v8::Object> stats = v8::Object::New(isolate);
            stats->Set(context, v8::String::NewFromUtf8Literal(isolate, "size"), v8::Number::New(isolate, (double)ctx->size)).Check();
            stats->Set(context, v8::String::NewFromUtf8Literal(isolate, "mtimeMs"), v8::Number::New(isolate, ctx->mtime)).Check();
            stats->Set(context, v8::String::NewFromUtf8Literal(isolate, "isDirectory"), 
                v8::FunctionTemplate::New(isolate, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
                    args.GetReturnValue().Set(args.Data().As<v8::Boolean>());
                }, v8::Boolean::New(isolate, ctx->is_dir))->GetFunction(context).ToLocalChecked()).Check();
            stats->Set(context, v8::String::NewFromUtf8Literal(isolate, "isFile"), 
                v8::FunctionTemplate::New(isolate, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
                    args.GetReturnValue().Set(args.Data().As<v8::Boolean>());
                }, v8::Boolean::New(isolate, ctx->is_file))->GetFunction(context).ToLocalChecked()).Check();
            resolver->Resolve(context, stats).Check();
        }
        delete ctx;
    };

    ThreadPool::GetInstance().Enqueue([task, ctx]() {
#ifdef _WIN32
        struct _stat64 st;
        if (_fstat64(ctx->fd, &st) == 0) {
            ctx->size = st.st_size; ctx->mtime = (double)st.st_mtime * 1000;
            ctx->is_dir = (st.st_mode & S_IFMT) == S_IFDIR; ctx->is_file = (st.st_mode & S_IFMT) == S_IFREG;
        } else { ctx->is_error = true; ctx->error_msg = "fstat error"; }
#else
        struct stat st;
        if (fstat(ctx->fd, &st) == 0) {
            ctx->size = st.st_size; ctx->mtime = (double)st.st_mtime * 1000;
            ctx->is_dir = S_ISDIR(st.st_mode); ctx->is_file = S_ISREG(st.st_mode);
        } else { ctx->is_error = true; ctx->error_msg = "fstat error"; }
#endif
        TaskQueue::GetInstance().Enqueue(task);
    });
}

} // namespace module
} // namespace z8