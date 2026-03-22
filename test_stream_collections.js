import stream from 'node:stream';

console.log('=== Testing Stream Collection Methods ===\n');

// Test 1: toArray()
console.log('1. Testing toArray()...');
try {
    const readable = stream.Readable.from(['A', 'B', 'C']);
    readable.toArray().then(arr => {
        console.log('   ✓ toArray() result:', arr);
    }).catch(err => {
        console.log('   ✗ Error:', err.message);
    });
} catch (e) {
    console.log('   ✗ Error:', e.message);
}

// Test 2: map()
console.log('\n2. Testing map()...');
try {
    const readable = stream.Readable.from([1, 2, 3]);
    const mapped = readable.map(x => x * 2);
    console.log('   ✓ map() created stream');
    
    mapped.on('data', chunk => {
        console.log('   - Mapped data:', chunk);
    });
} catch (e) {
    console.log('   ✗ Error:', e.message);
}

// Test 3: filter()
console.log('\n3. Testing filter()...');
try {
    const readable = stream.Readable.from([1, 2, 3, 4, 5]);
    const filtered = readable.filter(x => x % 2 === 0);
    console.log('   ✓ filter() created stream');
    
    filtered.on('data', chunk => {
        console.log('   - Filtered data:', chunk);
    });
} catch (e) {
    console.log('   ✗ Error:', e.message);
}

// Test 4: forEach()
console.log('\n4. Testing forEach()...');
try {
    const readable = stream.Readable.from(['X', 'Y', 'Z']);
    readable.forEach(chunk => {
        console.log('   - forEach chunk:', chunk);
    }).then(() => {
        console.log('   ✓ forEach() completed');
    }).catch(err => {
        console.log('   ✗ Error:', err.message);
    });
} catch (e) {
    console.log('   ✗ Error:', e.message);
}

// Test 5: some()
console.log('\n5. Testing some()...');
try {
    const readable = stream.Readable.from([1, 2, 3, 4, 5]);
    readable.some(x => x > 3).then(result => {
        console.log('   ✓ some() result:', result);
    }).catch(err => {
        console.log('   ✗ Error:', err.message);
    });
} catch (e) {
    console.log('   ✗ Error:', e.message);
}

// Test 6: find()
console.log('\n6. Testing find()...');
try {
    const readable = stream.Readable.from([10, 20, 30, 40]);
    readable.find(x => x > 25).then(result => {
        console.log('   ✓ find() result:', result);
    }).catch(err => {
        console.log('   ✗ Error:', err.message);
    });
} catch (e) {
    console.log('   ✗ Error:', e.message);
}

// Test 7: every()
console.log('\n7. Testing every()...');
try {
    const readable = stream.Readable.from([2, 4, 6, 8]);
    readable.every(x => x % 2 === 0).then(result => {
        console.log('   ✓ every() result:', result);
    }).catch(err => {
        console.log('   ✗ Error:', err.message);
    });
} catch (e) {
    console.log('   ✗ Error:', e.message);
}

// Test 8: reduce()
console.log('\n8. Testing reduce()...');
try {
    const readable = stream.Readable.from([1, 2, 3, 4]);
    readable.reduce((acc, x) => acc + x, 0).then(result => {
        console.log('   ✓ reduce() result:', result);
    }).catch(err => {
        console.log('   ✗ Error:', err.message);
    });
} catch (e) {
    console.log('   ✗ Error:', e.message);
}

// Test 9: drop()
console.log('\n9. Testing drop()...');
try {
    const readable = stream.Readable.from(['a', 'b', 'c', 'd', 'e']);
    const dropped = readable.drop(2);
    console.log('   ✓ drop() created stream');
    
    dropped.on('data', chunk => {
        console.log('   - After drop(2):', chunk);
    });
} catch (e) {
    console.log('   ✗ Error:', e.message);
}

// Test 10: take()
console.log('\n10. Testing take()...');
try {
    const readable = stream.Readable.from(['A', 'B', 'C', 'D', 'E']);
    const taken = readable.take(3);
    console.log('   ✓ take() created stream');
    
    taken.on('data', chunk => {
        console.log('   - After take(3):', chunk);
    });
} catch (e) {
    console.log('   ✗ Error:', e.message);
}

// Wait for async operations
setTimeout(() => {
    console.log('\n=== Collection Methods Tests Complete ===');
    process.exit(0);
}, 500);
