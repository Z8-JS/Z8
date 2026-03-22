import stream from 'node:stream';

console.log('Testing PassThrough...');

const passthrough = new stream.PassThrough();

passthrough.on('data', (chunk) => {
    console.log('Data event:', chunk.toString());
});

passthrough.on('end', () => {
    console.log('End event');
    process.exit(0);
});

passthrough.on('error', (err) => {
    console.log('Error event:', err.message);
    process.exit(1);
});

console.log('Writing data...');
passthrough.write('Hello');
passthrough.write(' World');
passthrough.end();

setTimeout(() => {
    console.log('Timeout - events not fired');
    process.exit(1);
}, 1000);
