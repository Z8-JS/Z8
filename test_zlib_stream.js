import { createGzip, createGunzip } from 'node:zlib';
import { Buffer } from 'node:buffer';

const gzip = createGzip();
const gunzip = createGunzip();

console.log('Action: Starting pipeline');

gzip.on('data', (c) => console.log('Event: gzip data', c.length));
gunzip.on('data', (c) => console.log('Event: gunzip data', c.length));

let output = '';
gunzip.on('data', (chunk) => {
    output += chunk.toString();
});

gunzip.on('end', () => {
    console.log('Final output:', output);
});

gzip.pipe(gunzip);

console.log('Action: Writing to gzip');
gzip.write(Buffer.from('Hello '));
console.log('Action: Ending gzip');
gzip.end(Buffer.from('World!'));
