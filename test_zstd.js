import zlib from 'node:zlib';

const input = new Uint8Array(2000);
for (let i = 0; i < input.length; i++) {
    input[i] = (i % 256);
}

console.log('--- Zstd Test ---');

function arraysEqual(a, b) {
    if (a.length !== b.length) return false;
    for (let i = 0; i < a.length; i++) {
        if (a[i] !== b[i]) return false;
    }
    return true;
}

// Sync test
try {
    const compressed = zlib.zstdCompressSync(input, { level: 3 });
    console.log('Zstd Sync Compressed Size:', compressed.length);
    const decompressed = zlib.zstdDecompressSync(compressed);
    console.log('Zstd Sync Decompressed Correctly:', arraysEqual(decompressed, input));
} catch (e) {
    console.error('Zstd Sync Failed:', e);
}

// Async test
zlib.zstdCompress(input, (err, compressed) => {
    if (err) {
        console.error('Zstd Async Compress Failed:', err);
        return;
    }
    console.log('Zstd Async Compressed Size:', compressed.length);
    zlib.zstdDecompress(compressed, (err, decompressed) => {
        if (err) {
            console.error('Zstd Async Decompress Failed:', err);
            return;
        }
        console.log('Zstd Async Decompressed Correctly:', arraysEqual(decompressed, input));
    });
});

// Stream Test
const compressor = zlib.createZstdCompress({ level: 5 });
const decompressor = zlib.createZstdDecompress();

const chunk1 = input.slice(0, 500);
const chunk2 = input.slice(500);

const c1 = compressor.write(chunk1);
const c2 = compressor.write(chunk2);
const c3 = compressor.end();

const compressedStream = new Uint8Array(c1.length + c2.length + c3.length);
compressedStream.set(c1, 0);
compressedStream.set(c2, c1.length);
compressedStream.set(c3, c1.length + c2.length);

console.log('Zstd Stream Compressed Size:', compressedStream.length);

const d1 = decompressor.write(compressedStream);
const d2 = decompressor.end();

const decompressedStream = new Uint8Array(d1.length + d2.length);
decompressedStream.set(d1, 0);
decompressedStream.set(d2, d1.length);

console.log('Zstd Stream Decompressed Correctly:', arraysEqual(decompressedStream, input));
