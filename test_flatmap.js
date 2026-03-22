import stream from 'node:stream';

console.log('=== Testing flatMap() ===\n');

// Test 1: flatMap with arrays
console.log('1. Testing flatMap() with arrays...');
const source1 = stream.Readable.from([1, 2, 3]);
const flatMapped1 = source1.flatMap(x => [x, x * 2]);

let results1 = [];
flatMapped1.on('data', chunk => {
    results1.push(chunk);
});

flatMapped1.on('end', () => {
    console.log('   ✓ Result:', results1);
    console.log('   Expected: [1, 2, 2, 4, 3, 6]');
});

// Test 2: flatMap with single values
console.log('\n2. Testing flatMap() with single values...');
const source2 = stream.Readable.from(['a', 'b', 'c']);
const flatMapped2 = source2.flatMap(x => x.toUpperCase());

let results2 = [];
flatMapped2.on('data', chunk => {
    results2.push(chunk);
});

flatMapped2.on('end', () => {
    console.log('   ✓ Result:', results2);
    console.log('   Expected: [A, B, C]');
});

// Test 3: flatMap with nested arrays
console.log('\n3. Testing flatMap() with nested arrays...');
const source3 = stream.Readable.from([[1, 2], [3, 4], [5]]);
const flatMapped3 = source3.flatMap(x => x);

let results3 = [];
flatMapped3.on('data', chunk => {
    results3.push(chunk);
});

flatMapped3.on('end', () => {
    console.log('   ✓ Result:', results3);
    console.log('   Expected: [1, 2, 3, 4, 5]');
});

// Test 4: flatMap with empty arrays
console.log('\n4. Testing flatMap() with empty arrays...');
const source4 = stream.Readable.from([1, 2, 3, 4]);
const flatMapped4 = source4.flatMap(x => x % 2 === 0 ? [x] : []);

let results4 = [];
flatMapped4.on('data', chunk => {
    results4.push(chunk);
});

flatMapped4.on('end', () => {
    console.log('   ✓ Result:', results4);
    console.log('   Expected: [2, 4]');
});

setTimeout(() => {
    console.log('\n=== flatMap() Tests Complete ===');
    process.exit(0);
}, 500);
