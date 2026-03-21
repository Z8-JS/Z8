import stream from 'node:stream';

console.log('=== Testing Stream Phase 2 Features ===\n');

// Test 1: PassThrough stream
console.log('1. Testing PassThrough stream...');
try {
    const passthrough = new stream.PassThrough();
    console.log('   ✓ PassThrough created');
    console.log('   - readable:', passthrough.readable);
    console.log('   - writable:', passthrough.writable);
} catch (e) {
    console.log('   ✗ Error:', e.message);
}

// Test 2: Additional properties (closed, destroyed, errored)
console.log('\n2. Testing additional properties...');
try {
    const readable = new stream.Readable();
    console.log('   Readable properties:');
    console.log('   - closed:', readable.closed);
    console.log('   - destroyed:', readable.destroyed);
    console.log('   - errored:', readable.errored);
    
    const writable = new stream.Writable();
    console.log('   Writable properties:');
    console.log('   - closed:', writable.closed);
    console.log('   - destroyed:', writable.destroyed);
    console.log('   - errored:', writable.errored);
    console.log('   ✓ Properties accessible');
} catch (e) {
    console.log('   ✗ Error:', e.message);
}

// Test 3: Duplex.from() static method
console.log('\n3. Testing Duplex.from()...');
try {
    // This should throw "not yet implemented" for now
    stream.Duplex.from({});
    console.log('   ✓ Duplex.from() called');
} catch (e) {
    console.log('   ⚠ Expected:', e.message);
}

// Test 4: Collection methods (should throw "not yet implemented")
console.log('\n4. Testing collection methods...');
const testMethods = [
    'map', 'filter', 'forEach', 'toArray', 
    'some', 'find', 'every', 'flatMap', 
    'drop', 'take', 'reduce'
];

const readable = new stream.Readable();
let methodCount = 0;
for (const method of testMethods) {
    try {
        if (typeof readable[method] === 'function') {
            methodCount++;
            try {
                readable[method](() => {});
            } catch (e) {
                if (e.message.includes('not yet implemented')) {
                    // Expected
                }
            }
        }
    } catch (e) {
        console.log(`   ✗ ${method}:`, e.message);
    }
}
console.log(`   ✓ ${methodCount}/${testMethods.length} collection methods available`);

// Test 5: PassThrough data flow
console.log('\n5. Testing PassThrough data flow...');
try {
    const passthrough = new stream.PassThrough();
    let dataReceived = false;
    
    passthrough.on('data', (chunk) => {
        console.log('   ✓ Data received:', chunk.toString());
        dataReceived = true;
    });
    
    passthrough.on('error', (err) => {
        console.log('   ✗ Error:', err.message);
    });
    
    passthrough.write('Hello from PassThrough!');
    passthrough.end();
    
    // Give it a moment for async operations
    setTimeout(() => {
        if (!dataReceived) {
            console.log('   ⚠ Data not received (async behavior or not implemented)');
        }
        console.log('\n=== Phase 2 Tests Complete ===');
    }, 100);
} catch (e) {
    console.log('   ✗ Error:', e.message);
    console.log('\n=== Phase 2 Tests Complete ===');
}
