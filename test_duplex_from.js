import stream from 'node:stream';

console.log('Testing Duplex.from()...');

try {
    const duplex = stream.Duplex.from([1, 2, 3]);
    console.log('✓ Duplex.from() created instance');
    console.log('  - readable:', duplex.readable);
    console.log('  - writable:', duplex.writable);
    process.exit(0);
} catch (err) {
    console.log('✗ Error:', err.message);
    process.exit(1);
}
