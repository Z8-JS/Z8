import stream from 'node:stream';

console.log('Testing toArray()...');
const readable = stream.Readable.from(['A', 'B', 'C']);

readable.toArray()
    .then(arr => {
        console.log('Success! Result:', arr);
        process.exit(0);
    })
    .catch(err => {
        console.log('Error:', err.message);
        console.log('Stack:', err.stack);
        process.exit(1);
    });

// Timeout in case promise never resolves
setTimeout(() => {
    console.log('Timeout - promise did not resolve');
    process.exit(1);
}, 2000);
