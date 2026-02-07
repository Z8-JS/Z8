// Performance test for path validation
import { readFileSync } from 'node:fs';

const start = Date.now();

// Read this file multiple times to test I/O performance
for (let i = 0; i < 100; i++) {
    const content = readFileSync('bench_security.js');
}

console.log('Total time:', Date.now() - start, 'ms');
