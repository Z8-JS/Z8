import zlib from 'node:zlib';
import { promisify } from 'node:util';

const brotliCompress = promisify(zlib.brotliCompress);
const brotliDecompress = promisify(zlib.brotliDecompress);

const input = new Uint8Array(2000);
for (let i = 0; i < input.length; i++) {
    input[i] = (i % 256);
}

console.log('--- Brotli Test ---');

const checkPass = (a, b) => {
    if (a.length !== b.length) return false;
    for (let i = 0; i < a.length; i++) if (a[i] !== b[i]) return false;
    return true;
};

async function test() {
    try {
        console.log('Testing Brotli Sync...');
        const compressedSync = zlib.brotliCompressSync(input);
        console.log('Compressed Length (Sync):', compressedSync.length);
        const decompressedSync = zlib.brotliDecompressSync(compressedSync);
        console.log('Brotli Sync Pass:', checkPass(decompressedSync, input) ? 'PASS' : 'FAIL');
        console.log('Compression Ratio:', (compressedSync.length / input.length).toFixed(2));

        console.log('Testing Brotli Async (promisified)...');
        const compressedAsync = await brotliCompress(input);
        const decompressedAsync = await brotliDecompress(compressedAsync);
        console.log('Brotli Async Pass:', checkPass(decompressedAsync, input) ? 'PASS' : 'FAIL');

    } catch (e) {
        console.error('Test failed with error:', e);
    }
}

test();
