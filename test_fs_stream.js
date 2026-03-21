import { createReadStream, createWriteStream, readFileSync } from 'node:fs';

const inputFile = 'test_in.txt';
const outputFile = 'test_out.txt';

// Create a dummy input file
import { writeFileSync } from 'node:fs';
writeFileSync(inputFile, 'This is a test of fs streams in Z8.');

const readStream = createReadStream(inputFile);
const writeStream = createWriteStream(outputFile);

console.log('Starting fs stream pipe...');

readStream.on('data', (c) => console.log('data', c.length));
readStream.on('end', () => console.log('readStream end'));
writeStream.on('finish', () => {
    console.log('Pipe finished.');
    const result = readFileSync(outputFile, 'utf8');
    console.log('Result:', result);
    if (result === 'This is a test of fs streams in Z8.') {
        console.log('SUCCESS!');
    } else {
        console.log('FAILED!');
    }
});

readStream.pipe(writeStream);
