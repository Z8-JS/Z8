import stream from 'node:stream';

async function testMethods() {
    try {
        // Test 1: toArray
        console.log('1. Testing toArray()...');
        const arr = await stream.Readable.from([1, 2, 3]).toArray();
        console.log('   ✓ Result:', arr);
        
        // Test 2: forEach
        console.log('2. Testing forEach()...');
        let count = 0;
        await stream.Readable.from(['a', 'b', 'c']).forEach(x => {
            count++;
            console.log('   - Item:', x);
        });
        console.log('   ✓ Processed', count, 'items');
        
        // Test 3: some
        console.log('3. Testing some()...');
        const someResult = await stream.Readable.from([1, 2, 3, 4]).some(x => x > 2);
        console.log('   ✓ Result:', someResult);
        
        // Test 4: find
        console.log('4. Testing find()...');
        const findResult = await stream.Readable.from([10, 20, 30]).find(x => x > 15);
        console.log('   ✓ Result:', findResult);
        
        // Test 5: every
        console.log('5. Testing every()...');
        const everyResult = await stream.Readable.from([2, 4, 6]).every(x => x % 2 === 0);
        console.log('   ✓ Result:', everyResult);
        
        // Test 6: reduce
        console.log('6. Testing reduce()...');
        const reduceResult = await stream.Readable.from([1, 2, 3, 4]).reduce((a, b) => a + b, 0);
        console.log('   ✓ Result:', reduceResult);
        
        console.log('\n=== All Tests Passed ===');
        process.exit(0);
    } catch (err) {
        console.error('Error:', err.message);
        console.error('Stack:', err.stack);
        process.exit(1);
    }
}

testMethods();
