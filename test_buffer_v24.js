const assert = (condition, message) => {
    if (!condition) {
        console.error('❌ Failed:', message);
        process.exit(1);
    }
};

console.log('--- Testing Buffer.poolSize and constants ---');
assert(Buffer.poolSize === 8192, 'poolSize should be 8192');
assert(Buffer.constants.MAX_LENGTH === 4294967296.0, 'MAX_LENGTH should be 4GB');
assert(Buffer.constants.MAX_STRING_LENGTH === 1073741823.0, 'MAX_STRING_LENGTH should be ~1GB');

console.log('--- Testing Buffer utilities ---');
assert(Buffer.isBuffer(Buffer.alloc(10)), 'isBuffer(Buffer.alloc(10)) should be true');
assert(Buffer.isEncoding('utf8'), 'isEncoding("utf8") should be true');
assert(Buffer.isEncoding('hex'), 'isEncoding("hex") should be true');
assert(Buffer.isEncoding('base64'), 'isEncoding("base64") should be true');
assert(Buffer.isEncoding('invalid') === false, 'isEncoding("invalid") should be false');

console.log('--- Testing atob/btoa ---');
const original = 'Hello World';
const encoded = Buffer.btoa(original);
assert(encoded === 'SGVsbG8gV29ybGQ=', 'btoa("Hello World") should be SGVsbG8gV29ybGQ=');
const decoded = Buffer.atob(encoded);
assert(decoded === original, 'atob(btoa("Hello World")) should be "Hello World"');

console.log('--- Testing isAscii/isUtf8 ---');
const asciiBuf = Buffer.from('abc');
const utf8Buf = Buffer.from('Hòa Bình');
assert(Buffer.isAscii(asciiBuf), 'isAscii(asciiBuf) should be true');
assert(Buffer.isAscii(utf8Buf) === false, 'isAscii(utf8Buf) should be false');
assert(Buffer.isUtf8(utf8Buf), 'isUtf8(utf8Buf) should be true');

console.log('--- Testing read/write variable length ---');
const buf = Buffer.alloc(10);
buf.writeUIntBE(0x123456, 0, 3);
assert(buf.readUIntBE(0, 3) === 0x123456, 'write/read UIntBE 3 bytes');
buf.writeIntLE(-0x123456, 3, 3);
assert(buf.readIntLE(3, 3) === -0x123456, 'write/read IntLE 3 bytes');

console.log('--- Testing Buffer.alloc with fill ---');
const filledBuf = Buffer.alloc(10, 'abc');
assert(filledBuf.toString() === 'abcabcabca', 'alloc with string fill');

const filledBuf2 = Buffer.alloc(10, 0x41);
assert(filledBuf2.toString() === 'AAAAAAAAAA', 'alloc with byte fill');

console.log('--- Testing toString with start/end ---');
const strBuf = Buffer.from('0123456789');
assert(strBuf.toString('utf8', 2, 5) === '234', 'toString(utf8, 2, 5) should be "234"');

console.log('--- Testing indexOf/lastIndexOf/includes ---');
const searchBuf = Buffer.from('hello world hello');
assert(searchBuf.indexOf('hello') === 0, 'indexOf("hello") should be 0');
assert(searchBuf.lastIndexOf('hello') === 12, 'lastIndexOf("hello") should be 12');
assert(searchBuf.includes('world'), 'includes("world") should be true');

console.log('--- Testing swap ---');
const swapBuf = Buffer.from([0x1, 0x2, 0x3, 0x4]);
swapBuf.swap16();
assert(swapBuf[0] === 0x2 && swapBuf[1] === 0x1, 'swap16');
swapBuf.swap32();
assert(swapBuf[0] === 0x3 && swapBuf[3] === 0x2, 'swap32');

console.log('--- Testing case-insensitive aliases ---');
assert(typeof buf.readUint8 === 'function', 'readUint8 alias exists');
assert(typeof buf.writeUint16BE === 'function', 'writeUint16BE alias exists');

console.log('✅ All tests passed!');
