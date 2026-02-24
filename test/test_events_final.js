import events, { EventEmitter, Event, EventTarget } from 'node:events';

async function testFinal() {
    console.log('Testing events.once with signal...');
    const ac = new AbortController();
    const ee = new EventEmitter();
    
    setTimeout(() => ac.abort(), 20);
    try {
        await events.once(ee, 'foo', { signal: ac.signal });
        throw new Error('Should have aborted');
    } catch (err) {
        // Log error for debugging
        console.log('Caught error name:', err.name, 'message:', err.message);
        if (err.name !== 'AbortError' && err.message !== 'Aborted') throw err;
        console.log('OK: AbortError status caught.');
    }

    console.log('Testing events.on with close...');
    const ee2 = new EventEmitter();
    const iterator = events.on(ee2, 'data', { close: ['end'] });
    
    ee2.emit('data', 1);
    ee2.emit('end');
    
    let count = 0;
    for await (const [val] of iterator) {
        count++;
        if (val !== 1) throw new Error('Wrong value: ' + val);
    }
    if (count !== 1) throw new Error('Iterator should have stopped after end, got count: ' + count);
    console.log('OK: Iterator closed correctly.');

    console.log('Testing Event and EventTarget...');
    const target = new EventTarget();
    let called = false;
    target.addEventListener('foo', (ev) => {
        called = true;
        if (ev.type !== 'foo') throw new Error('Wrong event type');
    });
    target.dispatchEvent(new Event('foo'));
    if (!called) throw new Error('EventListener not called');
    console.log('OK: EventTarget works.');

    console.log('Testing events.bubbles...');
    const ev = new Event('bar');
    if (events.bubbles(ev) !== false) throw new Error('Should not bubble');
    console.log('OK: events.bubbles works.');
}

// AbortController shim
if (typeof AbortController === 'undefined') {
    globalThis.AbortController = class {
        constructor() {
            this.signal = new EventTarget();
            this.signal.aborted = false;
            this.signal.reason = undefined;
        }
        abort(reason) {
            if (this.signal.aborted) return;
            this.signal.aborted = true;
            const err = new Error('Aborted');
            err.name = 'AbortError';
            this.signal.reason = reason || err;
            const ev = new Event('abort');
            this.signal.dispatchEvent(ev);
        }
    };
}

testFinal().catch(err => {
    console.error('Test failed:');
    console.error(err);
    process.exit(1);
});
