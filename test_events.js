// Basic integration tests for Z8 events implementation (ESM only, no CJS).
// Run with: .\\z8.exe test_events.js

import * as events from 'node:events';

const {
    EventEmitter,
    EventTarget,
    NodeEventTarget,
    Event,
    CustomEvent,
    once,
    on,
    getEventListeners,
    setMaxListeners,
    getMaxListeners,
    addAbortListener
} = events;

function log(title, value) {
    // Simple, consistent output so we can visually verify behavior.
    console.log(`=== ${title} ===`);
    console.log(value);
}

async function testEventEmitterBasic() {
    const emitter = new EventEmitter();

    const calls = [];
    emitter.on('ping', (v) => {
        calls.push(`first:${v}`);
    });
    emitter.prependListener('ping', (v) => {
        calls.push(`prepended:${v}`);
    });

    emitter.emit('ping', 42);

    log('EventEmitter basic', calls);

    // listenerCount / eventNames / rawListeners
    log('EventEmitter listenerCount("ping")', emitter.listenerCount('ping'));
    log('EventEmitter eventNames()', emitter.eventNames());
    log('EventEmitter rawListeners("ping") length', emitter.rawListeners('ping').length);
}

async function testEventsOncePromise() {
    const emitter = new EventEmitter();

    const p = once(emitter, 'ready');
    emitter.emit('ready', 'a', 'b', 'c');

    const args = await p;
    log('events.once result', args);

    // Abort signal support (optional)
    if (typeof AbortController !== 'undefined') {
        const emitter2 = new EventEmitter();
        const ac = new AbortController();
        const pAbort = once(emitter2, 'never', { signal: ac.signal }).catch((err) => err.name);
        ac.abort();
        const abortedName = await pAbort;
        log('events.once with AbortController name', abortedName);
    }
}

async function testEventsOnAsyncIterator() {
    const emitter = new EventEmitter();

    // Async iterator that collects first 3 "data" events then stops.
    const collected = [];
    const iter = on(emitter, 'data');

    (async () => {
        for await (const args of iter) {
            collected.push(args);
            if (collected.length === 3) {
                break;
            }
        }
    })();

    emitter.emit('data', 1);
    emitter.emit('data', 2);
    emitter.emit('data', 3);
    emitter.emit('data', 4); // should be ignored by consumer

    // Give the iterator a tick to consume events.
    await new Promise((resolve) => setTimeout(resolve, 0));

    log('events.on async iterator collected', collected);
}

function testEventAndEventTarget() {
    const target = new EventTarget();

    const received = [];

    target.addEventListener('custom', (ev) => {
        received.push(`L1:${ev.type}:${ev.bubbles}:${ev.cancelable}`);
        ev.stopImmediatePropagation();
    });

    target.addEventListener('custom', () => {
        // Should not be called because of stopImmediatePropagation
        received.push('L2:SHOULD_NOT_RUN');
    });

    const ev = new Event('custom', { bubbles: true, cancelable: true });
    target.dispatchEvent(ev);

    log('EventTarget + Event.stopImmediatePropagation', received);
}

function testCustomEvent() {
    const target = new EventTarget();
    const details = [];

    target.addEventListener('foo', (ev) => {
        details.push(ev.detail);
    });

    const ev1 = new CustomEvent('foo', { detail: 123 });
    const ev2 = new CustomEvent('foo', { detail: { a: 1 } });

    target.dispatchEvent(ev1);
    target.dispatchEvent(ev2);

    log('CustomEvent detail', details);
}

function testNodeEventTargetBasic() {
    if (typeof NodeEventTarget !== 'function') {
        log('NodeEventTarget availability', 'NodeEventTarget is not defined');
        return;
    }

    const target = new NodeEventTarget();
    const values = [];

    target.on('x', (v) => values.push(`on:${v}`));
    target.once('x', (v) => values.push(`once:${v}`));

    target.emit('x', 10);
    target.emit('x', 20);

    log('NodeEventTarget values', values);
    log('NodeEventTarget eventNames()', target.eventNames());
    log('NodeEventTarget listenerCount("x")', target.listenerCount('x'));

    const listeners = getEventListeners(target, 'x');
    log('events.getEventListeners(NodeEventTarget, "x") length', listeners.length);
}

function testSetGetMaxListeners() {
    const emitter = new EventEmitter();

    log('events.getMaxListeners default', getMaxListeners(emitter));

    setMaxListeners(5, emitter);
    log('events.getMaxListeners after set', getMaxListeners(emitter));
}

async function testAddAbortListener() {
    if (typeof AbortController === 'undefined') {
        log('addAbortListener / AbortController', 'AbortController not available, skipping');
        return;
    }

    const ac = new AbortController();
    let called = 0;

    const disposable = addAbortListener(ac.signal, () => {
        called++;
    });

    ac.abort();

    // Give listeners a tick to run
    await new Promise((resolve) => setTimeout(resolve, 0));

    log('addAbortListener called count', called);

    // Ensure disposable[Symbol.dispose] works and does not throw
    const disposeFn = disposable[Symbol.dispose];
    if (typeof disposeFn === 'function') {
        disposeFn.call(disposable);
    }
}

async function main() {
    await testEventEmitterBasic();
    await testEventsOncePromise();
    await testEventsOnAsyncIterator();
    testEventAndEventTarget();
    testCustomEvent();
    testNodeEventTargetBasic();
    testSetGetMaxListeners();
    await testAddAbortListener();
}

main().catch((err) => {
    console.error('Test suite failed:', err);
});


