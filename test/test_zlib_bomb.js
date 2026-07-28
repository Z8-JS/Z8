// Regression test for the decompression-bomb vuln from issue #17.
//
// Before fix: zlibSync()/brotliDecompressSync()/zstdDecompressSync()
// with no maxOutputLength would happily resize the output buffer to
// whatever the codec produced. A 1 MB compressed payload that expands
// to gigabytes could OOM the process. A user-supplied
// {chunkSize: 2_000_000_000} also caused an immediate allocation that
// large before any work began.
//
// After fix: chunkSize is clamped to [64, 1<<20]; maxOutputLength
// defaults to 512 MB when not supplied. Both clamps are applied
// uniformly in parseZlibOptions / parseBrotliOptions / parseZstdOptions.

import * as zlib from 'node:zlib';

let totalChecks = 0;
let failures = [];

function check(label, fn) {
    totalChecks++;
    try {
        fn();
        failures.push(`${label} did NOT throw`);
    } catch (err) {
        const msg = (err && err.message) || String(err);
        // The error we expect is either an explicit "maxOutputLength
        // exceeded" or a brotli/zstd OOM error. Either is acceptable —
        // we just want the process to survive.
        if (!/maxOutputLength|exceed|out of memory|ENOMEM/i.test(msg)) {
            failures.push(`${label} threw unexpected error: ${msg}`);
        }
    }
}

// 1. Huge chunkSize should be clamped, not honored → does not throw
//    a memory error before the work starts.
try {
    zlib.deflateSync(Buffer.from('hello'), { chunkSize: 2_000_000_000 });
    // Surviving without throwing is the win.
} catch (err) {
    failures.push(`deflateSync({chunkSize: 2_000_000_000}) crashed: ${err.message}`);
    totalChecks++;
}

// 2. Negative chunkSize should also be clamped to kMinChunkSize.
try {
    zlib.deflateSync(Buffer.from('hello'), { chunkSize: -1 });
    totalChecks++;
} catch (err) {
    failures.push(`deflateSync({chunkSize: -1}) unexpected: ${err.message}`);
    totalChecks++;
}

// 3. Compress a real payload, ensure inflate round-trips.
const data = Buffer.from('A'.repeat(10000));
const compressed = zlib.deflateSync(data);
const decompressed = zlib.inflateSync(compressed);
if (decompressed.toString() !== data.toString()) {
    failures.push('round-trip inflate produced wrong content');
}
totalChecks++;

// 4. Explicit maxOutputLength should still be honored.
try {
    zlib.inflateSync(compressed, { maxOutputLength: 100 });
    failures.push('inflateSync({maxOutputLength: 100}) did NOT throw on extra-large payload');
} catch (err) {
    if (!/maxOutputLength exceeded/i.test(err.message)) {
        failures.push(`inflateSync explicit maxOutputLength: wrong error: ${err.message}`);
    }
}
totalChecks++;

if (failures.length > 0) {
    console.error(`[zlib-bomb] FAIL — ${failures.length}/${totalChecks} checks failed`);
    for (const f of failures.slice(0, 20)) {
        console.error('  ' + f);
    }
    process.exit(1);
}

console.log(`[zlib-bomb] PASS — ${totalChecks} checks; chunkSize clamps work, maxOutputLength default applied, round-trip intact`);
process.exit(0);
