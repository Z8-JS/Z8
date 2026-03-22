import stream from 'node:stream';

console.log('Testing Readable.from()...');

const readable = stream.Readable.from([1, 2, 3]);

readable.on('data', (chunk) => {
    console.log('Data:', chunk);
});

readable.on('end', () => {
    console.log('End event');
    process.exit(0);
});

setTimeout(() => {
    console.log('Timeout - events not fired');
    process.exit(1);
}, 1000);
