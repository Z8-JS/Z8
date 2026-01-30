// test/test_fs_extended.js

// This relies on the native module resolver in main.cpp handling 'node:fs'
// and 'node:console'
import fs from 'node:fs';
import console from 'node:console';

const TEST_DIR = './fs_test_dir';
const TEST_FILE = `${TEST_DIR}/test.txt`;

function runTest(name, testFn) {
    try {
        console.log(`[RUNNING] ${name}`);
        testFn();
        console.log(`[PASS] ${name}`);
        return true;
    } catch (e) {
        console.error(`[FAIL] ${name}`);
        console.error(`  Error: ${e.message}`);
        // In case of failure, try to clean up before exiting
        cleanup();
        return false;
    }
}

function cleanup() {
    if (fs.existsSync(TEST_DIR)) {
        console.log('Cleaning up test directory...');
        fs.rmSync(TEST_DIR, { recursive: true, force: true });
    }
}

function main() {
    let allPassed = true;

    // 1. Cleanup from any previous failed run
    cleanup();

    // 2. Test fs.constants
    allPassed &= runTest('fs.constants', () => {
        if (typeof fs.constants !== 'object' || fs.constants.F_OK !== 0) {
            throw new Error(`fs.constants is invalid. fs.constants.F_OK = ${fs.constants.F_OK}`);
        }
    });
    if (!allPassed) return 1;

    // 3. Test mkdirSync
    allPassed &= runTest('fs.mkdirSync', () => {
        fs.mkdirSync(TEST_DIR);
        if (!fs.existsSync(TEST_DIR)) {
            throw new Error('Directory was not created.');
        }
    });
    if (!allPassed) return 1;

    // 4. Test writeFileSync
    allPassed &= runTest('fs.writeFileSync', () => {
        fs.writeFileSync(TEST_FILE, 'hello');
        const content = fs.readFileSync(TEST_FILE);
        if (content !== 'hello') {
            throw new Error(`File content mismatch: expected "hello", got "${content}"`);
        }
    });
    if (!allPassed) return 1;

    // 5. Test appendFileSync
    allPassed &= runTest('fs.appendFileSync', () => {
        fs.appendFileSync(TEST_FILE, ' world');
        const content = fs.readFileSync(TEST_FILE);
        if (content !== 'hello world') {
            throw new Error(`File content mismatch after append: got "${content}"`);
        }
    });
    if (!allPassed) return 1;

    // 6. Test statSync
    allPassed &= runTest('fs.statSync', () => {
        const fileStats = fs.statSync(TEST_FILE);
        if (!fileStats.isFile || fileStats.isDirectory) {
            throw new Error('statSync identifies file incorrectly.');
        }
        if (fileStats.size !== 11) {
            throw new Error(`statSync reports incorrect size: ${fileStats.size}`);
        }
        console.log('  File mtime:', fileStats.mtime);
        if (!(fileStats.mtime instanceof Date)) {
             throw new Error(`statSync mtime is not a Date object.`);
        }

        const dirStats = fs.statSync(TEST_DIR);
        if (!dirStats.isDirectory || dirStats.isFile) {
            throw new Error('statSync identifies directory incorrectly.');
        }
    });
    if (!allPassed) return 1;

    // 7. Test rmSync
    allPassed &= runTest('fs.rmSync', () => {
        fs.rmSync(TEST_DIR);
        if (fs.existsSync(TEST_DIR)) {
            throw new Error('Directory was not removed.');
        }
    });
    if (!allPassed) return 1;

    console.log('\nAll fs_extended tests passed!');
    return 0;
}

main();
