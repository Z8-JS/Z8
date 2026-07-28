// Regression test for the isPathSafe over-rejection (issue #18).
//
// Before fix: the path validator scanned the whole string for any
// pair of consecutive dots and rejected it. That refused legitimate
// filenames like `file..txt`, `v1..0-release`, or `a...b`.
//
// After fix: only a path component that is *exactly* `..` is rejected,
// which is the only sequence that genuinely escapes the directory.

import { writeFileSync, readFileSync, existsSync, unlinkSync } from 'node:fs';

let totalChecks = 0;
let failures = [];

// Paths that should be ALLOWED now (these were wrongly rejected before).
const ALLOWED = [
    'file..txt',
    'v1..0-release',
    'a...b',
    '...so-on',
    'foo..bar..baz',
    'test..',
    '..test',  // contains '..' but is not exactly '..'
    'normal.txt',
    'foo/bar..baz',
    'foo/file..txt',
];

// Paths that should still be REJECTED.
const REJECTED = [
    '..',
    '../etc/passwd',
    'foo/../../etc/passwd',
    'foo/..',
    'foo/../bar',
    'foo/bar/..',
    '/etc/passwd',
    'C:\\Windows\\System32\\config\\SAM',
    'C:/Windows',
    'CON',
    'CON.txt',
    'PRN',
    'COM1',
    'LPT1',
];

for (const p of ALLOWED) {
    totalChecks++;
    try {
        // Just calling writeFileSync with the path; we don't care about
        // content success — only that isPathSafe lets the path through.
        writeFileSync(p, 'test');
        if (existsSync(p)) unlinkSync(p);
    } catch (err) {
        const msg = (err && err.message) || String(err);
        if (/SecurityError|Path validation failed|traversal/i.test(msg)) {
            failures.push(`'${p}' should be allowed but was rejected: ${msg}`);
        }
        // Other errors (e.g. permission denied) are fine — we just want
        // isPathSafe to not block.
    }
}

for (const p of REJECTED) {
    totalChecks++;
    try {
        writeFileSync(p, 'test');
        // If we got here, isPathSafe let the path through.
        failures.push(`'${p}' should be rejected but was accepted`);
        if (existsSync(p)) unlinkSync(p);
    } catch (err) {
        const msg = (err && err.message) || String(err);
        if (!/SecurityError|Path validation failed|traversal/i.test(msg)) {
            // Different error (e.g. EINVAL on the path syntax). Fail.
            failures.push(`'${p}' threw wrong error: ${msg}`);
        }
    }
}

if (failures.length > 0) {
    console.error(`[isPathSafe] FAIL — ${failures.length}/${totalChecks} checks failed`);
    for (const f of failures.slice(0, 20)) {
        console.error('  ' + f);
    }
    process.exit(1);
}

console.log(`[isPathSafe] PASS — ${totalChecks} checks; ${ALLOWED.length} legitimate paths allowed, ${REJECTED.length} dangerous paths rejected`);
process.exit(0);
