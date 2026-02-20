import zlib from 'node:zlib';
import { promisify } from 'node:util';

const deflate = promisify(zlib.deflate);
const inflate = promisify(zlib.inflate);
const gzip = promisify(zlib.gzip);
const gunzip = promisify(zlib.gunzip);
const deflateRaw = promisify(zlib.deflateRaw);
const inflateRaw = promisify(zlib.inflateRaw);
const unzip = promisify(zlib.unzip);

const input = new Uint8Array(2000);
for (let i = 0; i < input.length; i++) {
    input[i] = (i % 256);
}

console.log('--- Zlib Promisify Test ---');

const checkPass = (a, b) => {
    if (a.length !== b.length) return false;
    for (let i = 0; i < a.length; i++) if (a[i] !== b[i]) return false;
    return true;
};

async function test() {
    try {
        const deflated = await deflate(input);
        const inflated = await inflate(deflated);
        console.log('Deflate/Inflate:', checkPass(inflated, input) ? 'PASS' : 'FAIL');

        const gzipped = await gzip(input);
        const gunzipped = await gunzip(gzipped);
        console.log('Gzip/Gunzip:', checkPass(gunzipped, input) ? 'PASS' : 'FAIL');

        const raw = await deflateRaw(input);
        const rawInflated = await inflateRaw(raw);
        console.log('DeflateRaw/InflateRaw:', checkPass(rawInflated, input) ? 'PASS' : 'FAIL');

        const unzipped = await unzip(gzipped);
        console.log('Unzip:', checkPass(unzipped, input) ? 'PASS' : 'FAIL');

    } catch (e) {
        console.error('Test failed with error:', e);
    }
}

test();
