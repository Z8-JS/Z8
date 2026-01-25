| API                                                          | Tiến độ     |
| ------------------------------------------------------------ | ----------- |
| fsPromises.stat(path[, options])                             | ✅ Done     |
| fsPromises.unlink(path)                                      | ✅ Done     |
| fsPromises.writeFile(file, data[, options])                  | ✅ Done     |
| fsPromises.constants                                         | ✅ Done     |
| fsPromises.chmod(path, mode)                                 |             |
| fsPromises.chown(path, uid, gid)                             |             |
| fsPromises.copyFile(src, dest[, mode])                       | ✅ Done     |
| fsPromises.cp(src, dest[, options])                          |             |
| fsPromises.glob(pattern[, options])                          |             |
| fsPromises.lchown(path, uid, gid)                            |             |
| fsPromises.lutimes(path, atime, mtime)                       |             |
| fsPromises.link(existingPath, newPath)                       |             |
| fsPromises.lstat(path[, options])                            |             |
| fsPromises.mkdir(path[, options])                            | ✅ Done     |
| fsPromises.mkdtemp(prefix[, options])                        |             |
| fsPromises.mkdtempDisposable(prefix[, options])              |             |
| fsPromises.open(path, flags[, mode])                         |             |
| fsPromises.opendir(path[, options])                          |             |
| fsPromises.readdir(path[, options])                          | ✅ Done     |
| fsPromises.readFile(path[, options])                         | ✅ Done     |
| fsPromises.readlink(path[, options])                         |             |
| fsPromises.realpath(path[, options])                         |             |
| fsPromises.rename(oldPath, newPath)                          | ✅ Done     |
| fsPromises.rmdir(path[, options])                            | ✅ Done     |
| fsPromises.rm(path[, options])                               |             |
| fsPromises.stat(path[, options])                             | ✅ Done     |
| fsPromises.statfs(path[, options])                           |             |
| fsPromises.symlink(target, path[, type])                     |             |
| fsPromises.truncate(path[, len])                             |             |
| fsPromises.unlink(path)                                      | ✅ Done     |
| fsPromises.utimes(path, atime, mtime)                        |             |
| fsPromises.watch(filename[, options])                        |             |
| fsPromises.writeFile(file, data[, options])                  | ✅ Done     |
| fsPromises.constants                                         | ✅ Done     |
| fs.access(path[, mode], callback)                            | ✅ Done     |
| fs.appendFile(path, data[, options], callback)               |             |
| fs.chmod(path, mode, callback)                               |             |
| fs.chown(path, uid, gid, callback)                           |             |
| fs.close(fd[, callback])                                     |             |
| fs.copyFile(src, dest[, mode], callback)                     | ✅ Done     |
| fs.cp(src, dest[, options], callback)                        |             |
| fs.createReadStream(path[, options])                         |             |
| fs.createWriteStream(path[, options])                        |             |
| fs.fchmod(fd, mode, callback)                                |             |
| fs.fchown(fd, uid, gid, callback)                            |             |
| fs.fdatasync(fd, callback)                                   |             |
| fs.fstat(fd[, options], callback)                            |             |
| fs.fsync(fd, callback)                                       |             |
| fs.ftruncate(fd[, len], callback)                            |             |
| fs.futimes(fd, atime, mtime, callback)                       |             |
| fs.glob(pattern[, options], callback)                        |             |
| fs.lchown(path, uid, gid, callback)                          |             |
| fs.lutimes(path, atime, mtime, callback)                     |             |
| fs.link(existingPath, newPath, callback)                     |             |
| fs.lstat(path[, options], callback)                          |             |
| fs.mkdir(path[, options], callback)                          | ✅ Done     |
| fs.mkdtemp(prefix[, options], callback)                      |             |
| fs.open(path, flags[, mode], callback)                       |             |
| fs.openAsBlob(path[, options])                               |             |
| fs.opendir(path[, options], callback)                        |             |
| fs.read(fd, buffer, offset, length, position, callback)      |             |
| fs.read(fd[, options], callback)                             |             |
| fs.read(fd, buffer[, options], callback)                     |             |
| fs.readdir(path[, options], callback)                        | ✅ Done     |
| fs.readFile(path[, options], callback)                       | ✅ Done     |
| fs.readlink(path[, options], callback)                       |             |
| fs.readv(fd, buffers[, position], callback)                  |             |
| fs.realpath(path[, options], callback)                       |             |
| fs.realpath.native(path[, options], callback)                |             |
| fs.rename(oldPath, newPath, callback)                        | ✅ Done     |
| fs.rmdir(path[, options], callback)                          | ✅ Done     |
| fs.rm(path[, options], callback)                             |             |
| fs.stat(path[, options], callback)                           | ✅ Done     |
| fs.statfs(path[, options], callback)                         |             |
| fs.symlink(target, path[, type], callback)                   |             |
| fs.truncate(path[, len], callback)                           |             |
| fs.unlink(path, callback)                                    | ✅ Done     |
| fs.unwatchFile(filename[, listener])                         |             |
| fs.utimes(path, atime, mtime, callback)                      |             |
| fs.watch(filename[, options][, listener])                    |             |
| fs.watchFile(filename[, options], listener)                  |             |
| fs.write(fd, buffer, offset[, length[, position]], callback) |             |
| fs.write(fd, buffer[, options], callback)                    |             |
| fs.write(fd, string[, position[, encoding]], callback)       |             |
| fs.writeFile(file, data[, options], callback)                | ✅ Done     |
| fs.writev(fd, buffers[, position], callback)                 |             |
| fs.accessSync(path[, mode])                                  | ✅ Done     |
| fs.appendFileSync(path, data[, options])                     | ✅ Done     |
| fs.chmodSync(path, mode)                                     | ✅ Done     |
| fs.chownSync(path, uid, gid)                                 | 🚧 Not Easy |
| fs.closeSync(fd)                                             | 🚧 Partial  |
| fs.copyFileSync(src, dest[, mode])                           | ✅ Done     |
| fs.cpSync(src, dest[, options])                              |             |
| fs.existsSync(path)                                          | ✅ Done     |
| fs.fchmodSync(fd, mode)                                      |             |
| fs.fchownSync(fd, uid, gid)                                  |             |
| fs.fdatasyncSync(fd)                                         |             |
| fs.fstatSync(fd[, options])                                  |             |
| fs.fsyncSync(fd)                                             |             |
| fs.ftruncateSync(fd[, len])                                  |             |
| fs.futimesSync(fd, atime, mtime)                             |             |
| fs.globSync(pattern[, options])                              |             |
| fs.lchownSync(path, uid, gid)                                |             |
| fs.lutimesSync(path, atime, mtime)                           |             |
| fs.linkSync(existingPath, newPath)                           | ✅ Done     |
| fs.lstatSync(path[, options])                                | ✅ Done     |
| fs.mkdirSync(path[, options])                                | ✅ Done     |
| fs.mkdtempSync(prefix[, options])                            |             |
| fs.mkdtempDisposableSync(prefix[, options])                  |             |
| fs.opendirSync(path[, options])                              |             |
| fs.openSync(path[, flags[, mode]])                           | 🚧 Partial  |
| fs.readdirSync(path[, options])                              | ✅ Done     |
| fs.readFileSync(path[, options])                             | ✅ Done     |
| fs.readlinkSync(path[, options])                             | ✅ Done     |
| fs.readSync(fd, buffer, offset, length[, position])          | 🚧 Partial  |
| fs.readSync(fd, buffer[, options])                           | 🚧 Partial  |
| fs.readvSync(fd, buffers[, position])                        |             |
| fs.realpathSync(path[, options])                             | ✅ Done     |
| fs.realpathSync.native(path[, options])                      |             |
| fs.renameSync(oldPath, newPath)                              | ✅ Done     |
| fs.rmdirSync(path[, options])                                | ✅ Done     |
| fs.rmSync(path[, options])                                   | ✅ Done     |
| fs.statSync(path[, options])                                 | ✅ Done     |
| fs.statfsSync(path[, options])                               |             |
| fs.symlinkSync(target, path[, type])                         | ✅ Done     |
| fs.truncateSync(path[, len])                                 | ✅ Done     |
| fs.unlinkSync(path)                                          | ✅ Done     |
| fs.utimesSync(path, atime, mtime)                            | ✅ Done     |
| fs.writeFileSync(file, data[, options])                      | ✅ Done     |
| fs.writeSync(fd, buffer, offset[, length[, position]])       | 🚧 Partial  |
| fs.writeSync(fd, buffer[, options])                          | 🚧 Partial  |
| fs.writeSync(fd, string[, position[, encoding]])             | 🚧 Partial  |
| fs.writevSync(fd, buffers[, position])                       |             |
