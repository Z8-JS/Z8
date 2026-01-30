function first() {
    second();
}

function second() {
    third("Testing trace");
}

function third(msg) {
    console.trace(msg);
}

first();
