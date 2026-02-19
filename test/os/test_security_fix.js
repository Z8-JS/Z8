// Test security fix with ES modules
import os from 'node:os';

console.log('Platform:', os.platform());
console.log('Type:', os.type());
console.log('Arch:', os.arch());
console.log('✅ Security fix working - file loaded successfully');
