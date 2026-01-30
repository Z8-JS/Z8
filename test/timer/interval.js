let count = 0;
const id = setInterval(() => {
    count++;
    console.log(`⏱️ Interval blink ${count}`);
    if (count === 5) {
        console.log("🛑 Clearing interval...");
        clearInterval(id);
    }
}, 500);

setTimeout(() => {
    console.log("ℹ️ This timeout runs in parallel with the interval.");
}, 1000);
