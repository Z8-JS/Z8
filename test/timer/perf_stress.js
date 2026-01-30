const total = 100;
let count = 0;

console.time("1M Timers");

function tick() {
    count++;
    if (count < total) {
        setTimeout(tick, 0);
    } else {
        console.timeEnd("1M Timers");
    }
}

tick();
