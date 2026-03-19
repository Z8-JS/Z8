import fs from 'node:fs';
import zlib from 'node:zlib';
import stream from 'node:stream';

console.log('--- Testing node:stream, node:fs, and node:zlib ---');

const testFile = 'test_input.txt';
const compressedFile = 'test_output.gz';
const decompressedFile = 'test_decompressed.txt';
const content = 'Hello Z8! This is a test for node:stream, node:fs, and node:zlib integration.\n'.repeat(10);

// 1. Setup: Create test input file
console.log('Step 1: Creating test file...');
fs.writeFileSync(testFile, content);

// 2. Test Stream Pipeline (Read -> Gzip -> Write)
console.log('Step 2: Compressing (Pipeline)...');
const source = fs.createReadStream(testFile);
const gzip = zlib.createGzip();
const destination = fs.createWriteStream(compressedFile);

stream.pipeline(source, gzip, destination, (err) => {
    if (err) {
        console.error('Compression failed:', err);
        return;
    }
    console.log('Compression finished.');

    // 3. Test Decompression
    console.log('Step 3: Decompressing (Pipeline)...');
    const source2 = fs.createReadStream(compressedFile);
    const gunzip = zlib.createGunzip();
    const destination2 = fs.createWriteStream(decompressedFile);

    stream.pipeline(source2, gunzip, destination2, (err) => {
        if (err) {
            console.error('Decompression failed:', err);
            return;
        }
        console.log('Decompression finished.');

        // 4. Verify content
        const result = fs.readFileSync(decompressedFile, 'utf8');
        if (result === content) {
            console.log('SUCCESS: Decompressed content matches original!');
        } else {
            console.error('FAILURE: Content mismatch!');
            console.log('Expected length:', content.length);
            console.log('Actual length:', result.length);
        }
    });
});
