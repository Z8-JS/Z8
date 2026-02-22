const assert = (condition, message) => {
    if (!condition) {
        console.error('❌ Failed:', message);
        process.exit(1);
    }
};

console.log('--- Testing base64url ---');
const original = 'Hello? World!'; // Base64 is SGVsbG8/IFdvcmxkIQ== -> SGVsbG8_IFdvcmxkIQ (base64url)
const b64url = 'SGVsbG8_IFdvcmxkIQ';
const buf = Buffer.from(b64url, 'base64url');
assert(buf.toString() === 'Hello? World!', 'base64url decoding failed');

console.log('--- Testing Buffer.from(ArrayBuffer, offset, length) ---');
const ab = new ArrayBuffer(10);
const view = new Uint8Array(ab);
for(let i=0; i<10; i++) view[i] = i;
const sliced = Buffer.from(ab, 2, 4);
assert(sliced.length === 4, 'length should be 4');
assert(sliced[0] === 2 && sliced[3] === 5, 'content mismatch');

console.log('--- Testing Buffer.from(Object) ---');
const obj = { toString: () => 'hello' };
const objBuf = Buffer.from(obj);
assert(objBuf.toString() === 'hello', 'Buffer.from(object) failed');

console.log('✅ Final Polish Tests Passed!');
