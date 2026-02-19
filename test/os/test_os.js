const os = require('node:os');
console.log('Platform:', os.platform());
console.log('Type:', os.type());
console.log('Release:', os.release());
console.log('Arch:', os.arch());
console.log('CPUs:', os.cpus().length);
console.log('Network Interfaces:', Object.keys(os.networkInterfaces()));
