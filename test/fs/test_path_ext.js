import path from 'node:path';

console.log('--- Host Path ---');
console.log('sep:', path.sep);
console.log('delimiter:', path.delimiter);
console.log('join(a, b):', path.join('a', 'b'));

console.log('--- Posix Path ---');
console.log('posix.sep:', path.posix.sep);
console.log('posix.delimiter:', path.posix.delimiter);
console.log('posix.join(a, b):', path.posix.join('a', 'b'));
console.log('posix.resolve(/a, b):', path.posix.resolve('/a', 'b'));
console.log('posix.normalize(/a/../b):', path.posix.normalize('/a/../b'));
console.log('posix.isAbsolute(/abc):', path.posix.isAbsolute('/abc'));
console.log('posix.dirname(/a/b/c):', path.posix.dirname('/a/b/c'));
console.log('posix.basename(/a/b/c.js):', path.posix.basename('/a/b/c.js'));
console.log('posix.extname(/a/b/c.js):', path.posix.extname('/a/b/c.js'));
console.log('posix.parse(/a/b/c.js):', JSON.stringify(path.posix.parse('/a/b/c.js')));
console.log('posix.format({dir: "/a/b", base: "c.js"}):', path.posix.format({dir: '/a/b', base: 'c.js'}));

console.log('--- Win32 Path ---');
console.log('win32.sep:', path.win32.sep);
console.log('win32.delimiter:', path.win32.delimiter);
console.log('win32.join(a, b):', path.win32.join('a', 'b'));
