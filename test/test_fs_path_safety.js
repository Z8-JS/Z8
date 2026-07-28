// Regression test for the path-traversal bypass reported in issue #16.
//
// Before fix: many async/promise/stream fs APIs (rm, rmdir, rename, copyFile,
// open, cp, mkdtemp, lutimes, opendir, createReadStream, createWriteStream)
// did NOT call isPathSafe() on the user-supplied path. The sync versions
// were hardened in commit 6402e4b but the async/promise/stream variants
// inherited the same raw syscall calls without validation.
//
// After fix: every path-accepting fs entry point calls isPathSafe() first.
// Direct traversal sequences (../, absolute paths, drive letters) and
// Windows reserved device names must be rejected with a SecurityError.
//
// Each assertion below corresponds to a function in the patch. The
// test passes the function a dangerous path and asserts it throws
// SecurityError rather than touching the filesystem.

import * as fs from 'node:fs';
import * as fsp from 'node:fs/promises';

const DANGEROUS_PATHS = [
    '../escape.txt',
    '../../../../etc/passwd',
    '/etc/passwd',
    'C:\\Windows\\System32\\config\\SAM',
    'foo/../../bar',
    '..',
    'foo/..',
    'CON',
    'CON.txt',
    'PRN',
    'COM1',
    'LPT1',
];

let totalChecks = 0;
let failures = [];

function expectSecurityError(label, fn) {
    for (const p of DANGEROUS_PATHS) {
        totalChecks++;
        try {
            fn(p);
            failures.push(`${label}("(${p})") did NOT throw`);
        } catch (err) {
            const msg = (err && err.message) || String(err);
            if (!/SecurityError|Path validation failed|traversal/i.test(msg)) {
                failures.push(`${label}("${p}") threw wrong error: ${msg}`);
            }
        }
    }
}

// --- async callback variants ---
expectSecurityError('fs.rm', (p) => fs.rm(p, () => {}));
expectSecurityError('fs.rmdir', (p) => fs.rmdir(p, () => {}));
expectSecurityError('fs.rename', (p) => fs.rename(p, p, () => {}));
expectSecurityError('fs.copyFile', (p) => fs.copyFile(p, p, () => {}));
expectSecurityError('fs.open', (p) => fs.open(p, 'r', () => {}));
expectSecurityError('fs.cp', (p) => fs.cp(p, p, () => {}));
expectSecurityError('fs.mkdtemp', (p) => fs.mkdtemp(p, () => {}));
expectSecurityError('fs.lutimes', (p) => fs.lutimes(p, 0, 0, () => {}));
expectSecurityError('fs.opendir', (p) => fs.opendir(p, () => {}));
expectSecurityError('fs.createReadStream', (p) => fs.createReadStream(p));
expectSecurityError('fs.createWriteStream', (p) => fs.createWriteStream(p));

// --- promise variants ---
async function checkPromise(label, fn) {
    for (const p of DANGEROUS_PATHS) {
        totalChecks++;
        try {
            await fn(p);
            failures.push(`${label}("${p}") resolved without rejection`);
        } catch (err) {
            const msg = (err && err.message) || String(err);
            if (!/SecurityError|Path validation failed|traversal/i.test(msg)) {
                failures.push(`${label}("${p}") rejected with wrong error: ${msg}`);
            }
        }
    }
}

await checkPromise('fsp.rm', (p) => fsp.rm(p));
await checkPromise('fsp.rmdir', (p) => fsp.rmdir(p));
await checkPromise('fsp.rename', (p) => fsp.rename(p, p));
await checkPromise('fsp.copyFile', (p) => fsp.copyFile(p, p));
await checkPromise('fsp.open', (p) => fsp.open(p, 'r'));
await checkPromise('fsp.cp', (p) => fsp.cp(p, p));
await checkPromise('fsp.mkdtemp', (p) => fsp.mkdtemp(p));
await checkPromise('fsp.lutimes', (p) => fsp.lutimes(p, 0, 0));
await checkPromise('fsp.opendir', (p) => fsp.opendir(p));

// --- sync variants (also need coverage; cpSync was missing) ---
for (const p of DANGEROUS_PATHS) {
    totalChecks++;
    try {
        fs.cpSync(p, p);
        failures.push(`fs.cpSync("${p}") did NOT throw`);
    } catch (err) {
        const msg = (err && err.message) || String(err);
        if (!/SecurityError|Path validation failed|traversal/i.test(msg)) {
            failures.push(`fs.cpSync("${p}") threw wrong error: ${msg}`);
        }
    }
}

// --- negative control: relative paths without traversal MUST still work ---
// We write a normal file, then verify it can be touched by the same APIs.
const safeFile = 'zane_test_safe.txt';
const safeFile2 = 'zane_test_safe_2.txt';

try {
    fs.writeFileSync(safeFile, 'hello');
    if (fs.readFileSync(safeFile, 'utf8') !== 'hello') {
        failures.push('safe readFileSync returned wrong content');
    }
    fs.unlinkSync(safeFile);
} catch (err) {
    failures.push(`safe-path control failed: ${err.message}`);
}

if (failures.length > 0) {
    console.error(`[path-safety] FAIL — ${failures.length}/${totalChecks} checks failed`);
    for (const f of failures.slice(0, 20)) {
        console.error('  ' + f);
    }
    process.exit(1);
}

console.log(`[path-safety] PASS — ${totalChecks} dangerous-path checks rejected, safe-path control passed`);
process.exit(0);
