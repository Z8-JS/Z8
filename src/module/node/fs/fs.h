#ifndef Z8_MODULE_FS_H
#define Z8_MODULE_FS_H

#include "v8.h"
#include <filesystem>
#include <string>
#include <system_error>

namespace z8 {
namespace module {

class FS {
public:
    static v8::Local<v8::ObjectTemplate> CreateTemplate(v8::Isolate* isolate);
    static v8::Local<v8::ObjectTemplate> CreatePromisesTemplate(v8::Isolate* isolate);
    static v8::Local<v8::Object> CreateStats(v8::Isolate* isolate, const std::filesystem::path& path, std::error_code& ec, bool follow_symlink);

    // Sync methods
    static void ReadFileSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void WriteFileSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void ExistsSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void AppendFileSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void StatSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void LstatSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void MkdirSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void RmSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void RmdirSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void UnlinkSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void ReaddirSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void RenameSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void CopyFileSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void RealpathSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void AccessSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void ChmodSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void ChownSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void UtimesSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void ReadlinkSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void SymlinkSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void LinkSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void TruncateSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void OpenSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void ReadSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void WriteSync(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void CloseSync(const v8::FunctionCallbackInfo<v8::Value>& args);

    static void ReadFilePromise(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void WriteFilePromise(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void StatPromise(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void UnlinkPromise(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void MkdirPromise(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void ReaddirPromise(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void RmdirPromise(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void RenamePromise(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void CopyFilePromise(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void AccessPromise(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void AppendFilePromise(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void RealpathPromise(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void ChmodPromise(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void ReadlinkPromise(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void SymlinkPromise(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void LstatPromise(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void UtimesPromise(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void LinkPromise(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void TruncatePromise(const v8::FunctionCallbackInfo<v8::Value>& args);

    // Async methods (Callback-based)
    static void ReadFile(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void WriteFile(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Stat(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Unlink(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Mkdir(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Readdir(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Rmdir(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Rename(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void CopyFile(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Access(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void AppendFile(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Realpath(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Chmod(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Readlink(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Symlink(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Lstat(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Utimes(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Link(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Truncate(const v8::FunctionCallbackInfo<v8::Value>& args);
};

} // namespace module
} // namespace z8

#endif
