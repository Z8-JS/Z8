import zlib from 'node:zlib';

function myAssert(condition, message) {
    if (!condition) {
        throw new Error('Assertion failed: ' + message);
    }
}

function myAssertStrictEqual(actual, expected, message) {
    if (actual !== expected) {
        throw new Error(`Assertion failed: ${message} (expected: ${expected}, actual: ${actual})`);
    }
}

async function testBrotliParams() {
    console.log('Testing Brotli params...');
    const input = Buffer.from('hello world'.repeat(100));
    const compressed = zlib.brotliCompressSync(input, {
        params: {
            [zlib.constants.BROTLI_PARAM_QUALITY]: 5,
            [zlib.constants.BROTLI_PARAM_LGWIN]: 10
        }
    });
    const decompressed = zlib.brotliDecompressSync(compressed);
    myAssert(input.equals(decompressed), 'Brotli params roundtrip failed');
    console.log('Brotli params test passed.');
}

async function testMaxOutputLength() {
    console.log('Testing maxOutputLength...');
    const input = Buffer.from('a'.repeat(1000));
    const compressed = zlib.deflateSync(input);
    
    // This should fail
    try {
        zlib.inflateSync(compressed, { maxOutputLength: 100 });
        myAssert(false, 'Should have thrown for maxOutputLength');
    } catch (e) {
        console.log('Caught expected error:', e.message);
        myAssert(e.message.includes('maxOutputLength'), 'Error message should mention maxOutputLength');
    }
    console.log('maxOutputLength test passed.');
}

async function testStreamBytesCount() {
    console.log('Testing stream bytesRead/bytesWritten...');
    const input = Buffer.from('modern web design is awesome');
    const stream = zlib.createGzip();
    const output = stream.write(input);
    const end = stream.end();
    
    myAssert(stream.bytesRead === input.length, `bytesRead mismatch: ${stream.bytesRead} !== ${input.length}`);
    myAssert(stream.bytesWritten > 0, 'bytesWritten should be > 0');
    console.log(`Stream bytesRead: ${stream.bytesRead}, bytesWritten: ${stream.bytesWritten}`);
    console.log('Stream bytes count test passed.');
}

async function testBrotliConstants() {
    console.log('Testing Brotli constants...');
    myAssert(zlib.constants.BROTLI_PARAM_QUALITY !== undefined, 'BROTLI_PARAM_QUALITY missing');
    myAssert(zlib.constants.BROTLI_MODE_GENERIC !== undefined, 'BROTLI_MODE_GENERIC missing');
    myAssert(zlib.constants.BROTLI_OPERATION_PROCESS !== undefined, 'BROTLI_OPERATION_PROCESS missing');
    console.log('Brotli constants test passed.');
}

async function testDictionary() {
    console.log('Testing Zlib dictionary...');
    const dict = Buffer.from('the quick brown fox');
    const input = Buffer.from('the quick brown fox jumps over the lazy dog');
    const compressed = zlib.deflateSync(input, { dictionary: dict });
    const decompressed = zlib.inflateSync(compressed, { dictionary: dict });
    myAssert(input.equals(decompressed), 'Zlib dictionary roundtrip failed');
    console.log('Zlib dictionary test passed.');
}

async function testBrotliDictionary() {
    console.log('Testing Brotli dictionary...');
    const dict = Buffer.from('shared dictionary contents');
    const input = Buffer.from('shared dictionary contents plus some unique data');
    const compressed = zlib.brotliCompressSync(input, { dictionary: dict });
    const decompressed = zlib.brotliDecompressSync(compressed, { dictionary: dict });
    myAssert(input.equals(decompressed), 'Brotli dictionary roundtrip failed');
    console.log('Brotli dictionary test passed.');
}

async function testZstdDictionary() {
    console.log('Testing Zstd dictionary...');
    const dict = Buffer.from('zstd shared dict');
    const input = Buffer.from('zstd shared dict more data');
    // @ts-ignore
    const compressed = zlib.zstdCompressSync(input, { dictionary: dict });
    // @ts-ignore
    const decompressed = zlib.zstdDecompressSync(compressed, { dictionary: dict });
    myAssert(input.equals(decompressed), 'Zstd dictionary roundtrip failed');
    console.log('Zstd dictionary test passed.');
}

async function testGzipHeader() {
    console.log('Testing Gzip header (gzname, gzcomment)...');
    const input = Buffer.from('gzip header test');
    const options = {
        gzname: 'test.txt',
        gzcomment: 'this is a comment',
        gzmtime: 123456789
    };
    const compressed = zlib.gzipSync(input, options);
    // Identification: Gzip magic 1F 8B
    myAssert(compressed[0] === 0x1F && compressed[1] === 0x8B, 'Not a gzip stream');
    
    const decompressed = zlib.gunzipSync(compressed);
    myAssert(input.equals(decompressed), 'Gzip with headers roundtrip failed');
    console.log('Gzip header test passed.');
}

async function testUnzipAuto() {
    console.log('Testing zlib.unzip automatic detection...');
    const input = Buffer.from('auto detection test');
    
    const gz = zlib.gzipSync(input);
    const unzippedGz = zlib.unzipSync(gz);
    myAssert(input.equals(unzippedGz), 'Unzip failed for gzip');
    
    const df = zlib.deflateSync(input);
    const unzippedDf = zlib.unzipSync(df);
    myAssert(input.equals(unzippedDf), 'Unzip failed for deflate');
    
    console.log('Unzip automatic detection test passed.');
}

async function runTests() {
    try {
        await testBrotliParams();
        await testMaxOutputLength();
        await testStreamBytesCount();
        await testBrotliConstants();
        await testDictionary();
        await testBrotliDictionary();
        await testZstdDictionary();
        await testGzipHeader();
        await testUnzipAuto();
        console.log('\nALL TESTS PASSED SUCCESSFULLY!');
    } catch (e) {
        console.error('\nTEST FAILED!');
        console.error(e);
        process.exit(1);
    }
}

runTests();
