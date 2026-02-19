console.log('--- Process Argv ---');
console.log('Argv:', process.argv);

console.log('--- Process PID ---');
console.log('PID:', process.pid);

console.log('--- Process Memory Usage ---');
const mem = process.memoryUsage();
console.log('Heap Total:', (Number(mem.heapTotal) / 1024 / 1024).toFixed(2), 'MB');
console.log('Heap Used:', (Number(mem.heapUsed) / 1024 / 1024).toFixed(2), 'MB');

console.log('--- Process nextTick ---');
process.nextTick(() => {
    console.log('nextTick callback executed (Microtask)');
});
console.log('Main script execution finished');
