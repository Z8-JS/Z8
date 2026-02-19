console.log("Process version:", process.version);
console.log("Process versions:", process.versions);
console.log("Platform:", process.platform);
console.log("Arch:", process.arch);
console.log("PID:", process.pid);
console.log("CWD:", process.cwd());

console.log("Setting title to 'Z8 Test'...");
process.title = "Z8 Test";
console.log("Current title:", process.title);

console.log("Memory Usage:", process.memoryUsage());
console.log("Resource Usage:", process.resourceUsage());
console.log("CPU Usage:", process.cpuUsage());
console.log("Uptime:", process.uptime());
console.log("HRTime BigInt:", process.hrtime.bigint());

console.log("Stdout isTTY:", process.stdout.isTTY);
console.log("Stderr isTTY:", process.stderr.isTTY);
console.log("Stdin isTTY:", process.stdin.isTTY);

console.log("Current umask:", process.umask().toString(8));

console.log("Testing Event Emitter stubs...");
process.on('exit', (code) => {
    console.log("Process exit with code:", code);
});
process.emit('test_event', 1, 2, 3);
console.log("Event emitter stubs called successfully.");

console.log("Testing process.kill with SIGHUP (self-signal)...");
process.kill(process.pid, 'SIGHUP');
console.log("DONE"); // This might not be reached if killed immediately
