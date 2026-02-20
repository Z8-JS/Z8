import zlib from 'node:zlib';

const input = new Uint8Array(2000);
for (let i = 0; i < input.length; i++) {
    input[i] = (i % 256);
}

console.log('--- Zlib Async Test ---');

let completed = 0;
const total = 7;

function check(name, inflated) {
    let pass = inflated.length === input.length;
    for (let i = 0; i < input.length; i++) {
        if (inflated[i] !== input[i]) {
            pass = false;
            break;
        }
    }
    console.log(`${name}: ${pass ? 'PASS' : 'FAIL'}`);
    completed++;
}

zlib.deflate(input, (err, deflated) => {
    if (err) console.error(err);
    zlib.inflate(deflated, (err, inflated) => {
        check('Deflate/Inflate', inflated);
    });
});

zlib.gzip(input, (err, gzipped) => {
    zlib.gunzip(gzipped, (err, gunzipped) => {
        check('Gzip/Gunzip', gunzipped);
    });
});

zlib.deflateRaw(input, (err, raw) => {
    zlib.inflateRaw(raw, (err, inflated) => {
        check('DeflateRaw/InflateRaw', inflated);
    });
});

zlib.deflate(input, (err, deflated) => {
    zlib.unzip(deflated, (err, unzipped) => {
        check('Unzip (zlib)', unzipped);
    });
});

zlib.gzip(input, (err, gzipped) => {
    zlib.unzip(gzipped, (err, unzipped) => {
        check('Unzip (gzip)', unzipped);
    });
});

// Test some error cases
zlib.inflate(new Uint8Array([1, 2, 3]), (err, result) => {
    console.log('Error test (invalid data):', err ? 'PASS (caught error)' : 'FAIL');
    completed++;
});

// Test unzip error
zlib.unzip(new Uint8Array([1, 2, 3]), (err, result) => {
    console.log('Error test (unzip):', err ? 'PASS (caught error)' : 'FAIL');
    completed++;
});
