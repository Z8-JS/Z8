import { addAbortListener, EventTarget, Event } from 'node:events';

async function testAbortListener() {
    console.log('Testing addAbortListener Disposable...');
    const signal = new EventTarget();
    signal.aborted = false;
    
    let called = 0;
    const disposable = addAbortListener(signal, () => {
        called++;
    });

    if (typeof disposable[Symbol.dispose] !== 'function') {
        throw new Error('addAbortListener should return a Disposable');
    }

    console.log('Explicitly disposing...');
    disposable[Symbol.dispose]();

    console.log('Emitting abort...');
    const ev = new Event('abort');
    signal.dispatchEvent(ev);

    if (called > 0) {
        throw new Error('Listener should not have been called after disposal');
    }
    console.log('OK: Abort listener removed correctly via Disposable.');

    console.log('Testing addAbortListener immediate call if already aborted...');
    signal.aborted = true;
    let immediate = false;
    const d2 = addAbortListener(signal, () => {
        immediate = true;
    });
    if (!immediate) throw new Error('Listener should be called immediately if signal is aborted');
    console.log('OK: Immediate call work.');
}

testAbortListener().catch(err => {
    console.error(err);
    process.exit(1);
});
