import stream from 'node:stream';

console.log('=== Testing Transform Collection Methods ===\n');

let allResults = {
    map: [],
    filter: [],
    drop: [],
    take: [],
    flatMap: []
};

// Test 1: map()
console.log('1. Testing map()...');
const readable1 = stream.Readable.from([1, 2, 3, 4, 5]);
const mapped = readable1.map(x => x * 2);

mapped.on('data', chunk => {
    allResults.map.push(chunk);
});

mapped.on('end', () => {
    console.log('   ✓ map() results:', allResults.map);
});

// Test 2: filter()
console.log('\n2. Testing filter()...');
const readable2 = stream.Readable.from([1, 2, 3, 4, 5, 6]);
const filtered = readable2.filter(x => x % 2 === 0);

filtered.on('data', chunk => {
    allResults.filter.push(chunk);
});

filtered.on('end', () => {
    console.log('   ✓ filter() results:', allResults.filter);
});

// Test 3: drop()
console.log('\n3. Testing drop()...');
const readable3 = stream.Readable.from(['a', 'b', 'c', 'd', 'e']);
const dropped = readable3.drop(2);

dropped.on('data', chunk => {
    allResults.drop.push(chunk);
});

dropped.on('end', () => {
    console.log('   ✓ drop(2) results:', allResults.drop);
});

// Test 4: take()
console.log('\n4. Testing take()...');
const readable4 = stream.Readable.from(['A', 'B', 'C', 'D', 'E']);
const taken = readable4.take(3);

taken.on('data', chunk => {
    allResults.take.push(chunk);
});

taken.on('end', () => {
    console.log('   ✓ take(3) results:', allResults.take);
});

// Test 5: flatMap()
console.log('\n5. Testing flatMap()...');
const readable5 = stream.Readable.from([1, 2, 3]);
const flatMapped = readable5.flatMap(x => [x, x * 2]);

flatMapped.on('data', chunk => {
    allResults.flatMap.push(chunk);
});

flatMapped.on('end', () => {
    console.log('   ✓ flatMap() results:', allResults.flatMap);
});

// Wait for all async operations
setTimeout(() => {
    console.log('\n=== Tests Complete ===');
    process.exit(0);
}, 500);
