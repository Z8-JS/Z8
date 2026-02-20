import zlib from 'node:zlib';

console.log('Testing zlib utility functions...');

const stringToUint8 = (str) => {
    const arr = new Uint8Array(str.length);
    for (let i = 0; i < str.length; i++) arr[i] = str.charCodeAt(i);
    return arr;
};

const uint8ToString = (arr) => {
    let str = '';
    for (let i = 0; i < arr.length; i++) str += String.fromCharCode(arr[i]);
    return str;
};

const data = new Uint8Array([1, 2, 3, 4, 5]);

// Test crc32
const crc = zlib.crc32(data);
console.log('CRC32:', crc);
if (typeof crc !== 'number') throw new Error('CRC32 should return a number');

const crc2 = zlib.crc32(data, crc);
console.log('CRC32 (with initial):', crc2);

// Test adler32
const adler = zlib.adler32(data);
console.log('Adler32:', adler);
if (typeof adler !== 'number') throw new Error('Adler32 should return a number');

const adler2 = zlib.adler32(data, adler);
console.log('Adler32 (with initial):', adler2);

console.log('Testing Stream APIs...');

// Test Gzip stream
const gzip = zlib.createGzip();
const compressed = gzip.write(stringToUint8('Hello World!'));
const finished = gzip.end();

const final_compressed = new Uint8Array(compressed.length + finished.length);
final_compressed.set(compressed);
final_compressed.set(finished, compressed.length);

console.log('Compressed (Gzip stream):', final_compressed.length, 'bytes');

// Test Gunzip stream
const gunzip = zlib.createGunzip();
const decompressed = gunzip.write(final_compressed);
const finished_decomp = gunzip.end();

const final_decompressed = new Uint8Array(decompressed.length + finished_decomp.length);
final_decompressed.set(decompressed);
final_decompressed.set(finished_decomp, decompressed.length);

const result = uint8ToString(final_decompressed);
console.log('Decompressed:', result);
if (result !== 'Hello World!') throw new Error('Gzip stream result mismatch');

console.log('Testing Brotli Stream...');
const brotli = zlib.createBrotliCompress();
const b_comp = brotli.write(stringToUint8('Brotli Stream Test'));
const b_fin = brotli.end();

const b_total = new Uint8Array(b_comp.length + b_fin.length);
b_total.set(b_comp);
b_total.set(b_fin, b_comp.length);

const b_dec = zlib.createBrotliDecompress();
const b_out = b_dec.write(b_total);
const b_out_fin = b_dec.end();

const b_final = new Uint8Array(b_out.length + b_out_fin.length);
b_final.set(b_out);
b_final.set(b_out_fin, b_out.length);

const b_result = uint8ToString(b_final);
console.log('Brotli Decompressed:', b_result);
if (b_result !== 'Brotli Stream Test') throw new Error('Brotli stream result mismatch');

console.log('All tests passed!');
