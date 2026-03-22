import stream from 'node:stream';

console.log('Testing map()...');

const source = stream.Readable.from([1, 2, 3]);
console.log('Source created');

const mapped = source.map(x => {
    console.log('Mapping:', x);
    return x * 2;
});
console.log('Mapped stream created');

mapped.on('data', (chunk) => {
    console.log('Mapped data:', chunk);
});

mapped.on('end', () => {
    console.log('End event');
    process.exit(0);
});

setTimeout(() => {
    console.log('Timeout');
    process.exit(1);
}, 1000);
