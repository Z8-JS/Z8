// Simple benchmark test file
const start = Date.now();
let sum = 0;
for (let i = 0; i < 1000000; i++) {
    sum += i;
}
console.log('Sum:', sum);
console.log('Time:', Date.now() - start, 'ms');
