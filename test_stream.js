// Test Z8 Stream Module Implementation

import stream from 'node:stream';

console.log('=== Testing Stream Module ===\n');

// Test 1: Readable Stream
console.log('1. Testing Readable Stream:');
const { Readable } = stream;
const readable = new Readable({
    read() {
        this.push('Hello ');
        this.push('World!');
        this.push(null); // EOF
    }
});

readable.on('data', (chunk) => {
    console.log('  Data:', chunk.toString());
});

readable.on('end', () => {
    console.log('  Readable stream ended\n');
});

// Test 2: Writable Stream
console.log('2. Testing Writable Stream:');
const { Writable } = stream;
const writable = new Writable({
    write(chunk, encoding, callback) {
        console.log('  Writing:', chunk.toString());
        callback();
    }
});

writable.write('Test data');
writable.end();

writable.on('finish', () => {
    console.log('  Writable stream finished\n');
});

// Test 3: Properties
console.log('3. Testing Stream Properties:');
const testReadable = new Readable();
console.log('  readable.readable:', testReadable.readable);
console.log('  readable.readableFlowing:', testReadable.readableFlowing);
console.log('  readable.readableHighWaterMark:', testReadable.readableHighWaterMark);
console.log('  readable.isPaused():', testReadable.isPaused());

const testWritable = new Writable();
console.log('  writable.writable:', testWritable.writable);
console.log('  writable.writableHighWaterMark:', testWritable.writableHighWaterMark);
console.log('  writable.writableCorked:', testWritable.writableCorked);

// Test 4: Readable.from
console.log('\n4. Testing Readable.from:');
const fromReadable = Readable.from(['A', 'B', 'C']);
fromReadable.on('data', (chunk) => {
    console.log('  From data:', chunk.toString());
});

// Test 5: Pause/Resume
console.log('\n5. Testing Pause/Resume:');
const pauseTest = new Readable({
    read() {
        this.push('Data');
        this.push(null);
    }
});

pauseTest.pause();
console.log('  After pause, isPaused():', pauseTest.isPaused());
pauseTest.resume();
console.log('  After resume, isPaused():', pauseTest.isPaused());

// Test 6: Utility Functions
console.log('\n6. Testing Utility Functions:');
console.log('  getDefaultHighWaterMark(false):', stream.getDefaultHighWaterMark(false));
console.log('  getDefaultHighWaterMark(true):', stream.getDefaultHighWaterMark(true));

console.log('\n=== All Stream Tests Completed ===');
