| API                                                          | Tiến độ |
| ------------------------------------------------------------ | ------- |
| fsPromises.stat(path[, options])                             | ✅ Done |
| fsPromises.unlink(path)                                      | ✅ Done |
| fsPromises.appendFile(path, data[, options])                 | ✅ Done |
| fsPromises.writeFile(file, data[, options])                  | ✅ Done |
| fsPromises.constants                                         | ✅ Done |
| fsPromises.chmod(path, mode)                                 | ✅ Done |
| fsPromises.chown(path, uid, gid)                             | ✅ Done |
| fsPromises.copyFile(src, dest[, mode])                       | ✅ Done |
| fsPromises.cp(src, dest[, options])                          | ✅ Done |
| fsPromises.glob(pattern[, options])                          |         |
| fsPromises.lchown(path, uid, gid)                            | ✅ Done |
| fsPromises.lutimes(path, atime, mtime)                       | ✅ Done |
| fsPromises.link(existingPath, newPath)                       | ✅ Done |
| fsPromises.lstat(path[, options])                            | ✅ Done |
| fsPromises.mkdir(path[, options])                            | ✅ Done |
| fsPromises.mkdtemp(prefix[, options])                        | ✅ Done |
| fsPromises.mkdtempDisposable(prefix[, options])              |         |
| fsPromises.open(path, flags[, mode])                         | ✅ Done |
| fsPromises.opendir(path[, options])                          | ✅ Done |
| fsPromises.readdir(path[, options])                          | ✅ Done |
| fsPromises.readFile(path[, options])                         | ✅ Done |
| fsPromises.readlink(path[, options])                         | ✅ Done |
| fsPromises.readv(fd, buffers[, position])                    | ✅ Done |
| fsPromises.realpath(path[, options])                         | ✅ Done |
| fsPromises.rename(oldPath, newPath)                          | ✅ Done |
| fsPromises.rmdir(path[, options])                            | ✅ Done |
| fsPromises.rm(path[, options])                               | ✅ Done |
| fsPromises.stat(path[, options])                             | ✅ Done |
| fsPromises.statfs(path[, options])                           | ✅ Done |
| fsPromises.symlink(target, path[, type])                     | ✅ Done |
| fsPromises.truncate(path[, len])                             | ✅ Done |
| fsPromises.unlink(path)                                      | ✅ Done |
| fsPromises.utimes(path, atime, mtime)                        | ✅ Done |
| fsPromises.watch(filename[, options])                        |         |
| fsPromises.writeFile(file, data[, options])                  | ✅ Done |
| fsPromises.writev(fd, buffers[, position])                   | ✅ Done |
| fsPromises.constants                                         | ✅ Done |
| fs.access(path[, mode], callback)                            | ✅ Done |
| fs.appendFile(path, data[, options], callback)               | ✅ Done |
| fs.chmod(path, mode, callback)                               | ✅ Done |
| fs.chown(path, uid, gid, callback)                           | ✅ Done |
| fs.close(fd[, callback])                                     | ✅ Done |
| fs.copyFile(src, dest[, mode], callback)                     | ✅ Done |
| fs.cp(src, dest[, options], callback)                        | ✅ Done |
| fs.createReadStream(path[, options])                         |         |
| fs.createWriteStream(path[, options])                        |         |
| fs.fchmod(fd, mode, callback)                                | ✅ Done |
| fs.fchown(fd, uid, gid, callback)                            | ✅ Done |
| fs.fdatasync(fd, callback)                                   | ✅ Done |
| fs.fstat(fd[, options], callback)                            | ✅ Done |
| fs.fsync(fd, callback)                                       | ✅ Done |
| fs.ftruncate(fd[, len], callback)                            | ✅ Done |
| fs.futimes(fd, atime, mtime, callback)                       | ✅ Done |
| fs.glob(pattern[, options], callback)                        |         |
| fs.lchown(path, uid, gid, callback)                          | ✅ Done |
| fs.lutimes(path, atime, mtime, callback)                     | ✅ Done |
| fs.link(existingPath, newPath, callback)                     | ✅ Done |
| fs.lstat(path[, options], callback)                          | ✅ Done |
| fs.mkdir(path[, options], callback)                          | ✅ Done |
| fs.mkdtemp(prefix[, options], callback)                      | ✅ Done |
| fs.open(path, flags[, mode], callback)                       | ✅ Done |
| fs.openAsBlob(path[, options])                               |         |
| fs.opendir(path[, options], callback)                        | ✅ Done |
| fs.read(fd, buffer, offset, length, position, callback)      | ✅ Done |
| fs.read(fd[, options], callback)                             |         |
| fs.read(fd, buffer[, options], callback)                     |         |
| fs.readdir(path[, options], callback)                        | ✅ Done |
| fs.readFile(path[, options], callback)                       | ✅ Done |
| fs.readlink(path[, options], callback)                       | ✅ Done |
| fs.readv(fd, buffers[, position], callback)                  | ✅ Done |
| fs.realpath(path[, options], callback)                       | ✅ Done |
| fs.realpath.native(path[, options], callback)                |         |
| fs.rename(oldPath, newPath, callback)                        | ✅ Done |
| fs.rmdir(path[, options], callback)                          | ✅ Done |
| fs.rm(path[, options], callback)                             | ✅ Done |
| fs.stat(path[, options], callback)                           | ✅ Done |
| fs.statfs(path[, options], callback)                         | ✅ Done |
| fs.symlink(target, path[, type], callback)                   | ✅ Done |
| fs.truncate(path[, len], callback)                           | ✅ Done |
| fs.unlink(path, callback)                                    | ✅ Done |
| fs.unwatchFile(filename[, listener])                         |         |
| fs.utimes(path, atime, mtime, callback)                      | ✅ Done |
| fs.watch(filename[, options][, listener])                    |         |
| fs.watchFile(filename[, options], listener)                  |         |
| fs.write(fd, buffer, offset[, length[, position]], callback) | ✅ Done |
| fs.write(fd, buffer[, options], callback)                    |         |
| fs.write(fd, string[, position[, encoding]], callback)       | ✅ Done |
| fs.writeFile(file, data[, options], callback)                | ✅ Done |
| fs.writev(fd, buffers[, position], callback)                 | ✅ Done |
| fs.accessSync(path[, mode])                                  | ✅ Done |
| fs.appendFileSync(path, data[, options])                     | ✅ Done |
| fs.chmodSync(path, mode)                                     | ✅ Done |
| fs.chownSync(path, uid, gid)                                 | ✅ Done |
| fs.closeSync(fd)                                             | ✅ Done |
| fs.copyFileSync(src, dest[, mode])                           | ✅ Done |
| fs.cpSync(src, dest[, options])                              | ✅ Done |
| fs.existsSync(path)                                          | ✅ Done |
| fs.fchmodSync(fd, mode)                                      | ✅ Done |
| fs.fchownSync(fd, uid, gid)                                  | ✅ Done |
| fs.fdatasyncSync(fd)                                         | ✅ Done |
| fs.fstatSync(fd[, options])                                  | ✅ Done |
| fs.fsyncSync(fd)                                             | ✅ Done |
| fs.ftruncateSync(fd[, len])                                  | ✅ Done |
| fs.futimesSync(fd, atime, mtime)                             | ✅ Done |
| fs.globSync(pattern[, options])                              |         |
| fs.lchownSync(path, uid, gid)                                | ✅ Done |
| fs.lutimesSync(path, atime, mtime)                           | ✅ Done |
| fs.linkSync(existingPath, newPath)                           | ✅ Done |
| fs.lstatSync(path[, options])                                | ✅ Done |
| fs.mkdirSync(path[, options])                                | ✅ Done |
| fs.mkdtempSync(prefix[, options])                            | ✅ Done |
| fs.mkdtempDisposableSync(prefix[, options])                  |         |
| fs.opendirSync(path[, options])                              | ✅ Done |
| fs.openSync(path[, flags[, mode]])                           | ✅ Done |
| fs.readdirSync(path[, options])                              | ✅ Done |
| fs.readFileSync(path[, options])                             | ✅ Done |
| fs.readlinkSync(path[, options])                             | ✅ Done |
| fs.readSync(fd, buffer, offset, length[, position])          | ✅ Done |
| fs.readSync(fd, buffer[, options])                           |         |
| fs.readvSync(fd, buffers[, position])                        | ✅ Done |
| fs.realpathSync(path[, options])                             | ✅ Done |
| fs.realpathSync.native(path[, options])                      |         |
| fs.renameSync(oldPath, newPath)                              | ✅ Done |
| fs.rmdirSync(path[, options])                                | ✅ Done |
| fs.rmSync(path[, options])                                   | ✅ Done |
| fs.statSync(path[, options])                                 | ✅ Done |
| fs.statfsSync(path[, options])                               | ✅ Done |
| fs.symlinkSync(target, path[, type])                         | ✅ Done |
| fs.truncateSync(path[, len])                                 | ✅ Done |
| fs.unlinkSync(path)                                          | ✅ Done |
| fs.utimesSync(path, atime, mtime)                            | ✅ Done |
| fs.writeFileSync(file, data[, options])                      | ✅ Done |
| fs.writeSync(fd, buffer, offset[, length[, position]])       | ✅ Done |
| fs.writeSync(fd, buffer[, options])                          |         |
| fs.writeSync(fd, string[, position[, encoding]])             | ✅ Done |
| fs.writevSync(fd, buffers[, position])                       | ✅ Done |
