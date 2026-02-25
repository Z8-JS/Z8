import { EventEmitter, errorMonitor, EventTarget, getEventListeners, getMaxListeners } from 'node:events';

function assert(condition, message) {
    if (!condition) {
        throw new Error(message || 'Assertion failed');
    }
}

async function testErrorMonitor() {
    console.log('Testing errorMonitor...');
    const ee = new EventEmitter();
    let monitorCalled = false;
    let errorCalled = false;
    const err = new Error('test error');

    ee.on(errorMonitor, (e) => {
        monitorCalled = true;
        assert(e === err, 'Monitor should receive the correct error object');
    });

    ee.on('error', (e) => {
        errorCalled = true;
        assert(e === err, 'Error handler should receive the correct error object');
    });

    ee.emit('error', err);

    assert(monitorCalled, 'errorMonitor listener should have been called');
    assert(errorCalled, 'error handler should have been called');
    console.log('OK: errorMonitor functional.');
}

async function testRemoveAllListenersEmit() {
    console.log('Testing removeAllListeners emit behavior...');
    const ee = new EventEmitter();
    let removeCount = 0;

    ee.on('removeListener', (event, listener) => {
        removeCount++;
    });

    ee.on('foo', () => {});
    ee.on('foo', () => {});
    ee.on('bar', () => {});

    console.log('Calling removeAllListeners("foo")...');
    ee.removeAllListeners('foo');
    assert(removeCount === 2, `Expected 2 removeListener calls for "foo", got ${removeCount}`);

    removeCount = 0;
    ee.on('baz', () => {});
    console.log('Calling removeAllListeners()...');
    ee.removeAllListeners();
    // baz (1) + bar (1) = 2. Nếu có thêm thì có thể do 'removeListener' chính nó cũng bị xóa.
    assert(removeCount >= 2, `Expected at least 2 removeListener calls for all, got ${removeCount}`);

    console.log('OK: removeAllListeners emits removeListener correctly.');
}

async function testListenerCountOverload() {
    console.log('Testing listenerCount overload...');
    const ee = new EventEmitter();
    ee.on('foo', () => {});
    
    // Instance method (standard)
    assert(ee.listenerCount('foo') === 1, 'Standard listenerCount failed');
    
    // Overload (emitter as first arg)
    assert(ee.listenerCount(ee, 'foo') === 1, 'Overloaded listenerCount failed');
    
    console.log('OK: listenerCount overload functional.');
}

async function testEventTargetV24() {
    console.log('Testing EventTarget Node v24 features...');
    const target = new EventTarget();

    // getEventListeners on EventTarget should return [] instead of undefined
    const listeners = getEventListeners(target, 'foo');
    assert(Array.isArray(listeners), 'getEventListeners should return an array');
    assert(listeners.length === 0, 'getEventListeners should return an empty array for no listeners');

    // getMaxListeners on EventTarget should return Infinity
    const max = getMaxListeners(target);
    assert(max === Infinity, `Expected Infinity for EventTarget, got ${max}`);

    console.log('OK: EventTarget Node v24 features functional.');
}

async function testEEV24Deep() {
    console.log('Testing deep EventEmitter Node v24 features...');
    
    // Test EventEmitter.init()
    const ee = new EventEmitter();
    if (typeof EventEmitter.init === 'function') {
        EventEmitter.init.call(ee);
        console.log('OK: EventEmitter.init static exists.');
    }

    // Test EventEmitter.usingDomains
    assert(typeof EventEmitter.usingDomains === 'boolean', 'EventEmitter.usingDomains should be a boolean');
    console.log('OK: EventEmitter.usingDomains exists.');

    // Test internal _eventsCount
    assert(ee._eventsCount === 0, 'Initial _eventsCount should be 0');
    const l1 = () => {};
    ee.on('a', l1);
    assert(ee._eventsCount === 1, `_eventsCount should be 1, got ${ee._eventsCount}`);
    ee.on('a', () => {});
    assert(ee._eventsCount === 1, 'Adding second listener to same event should NOT increase _eventsCount');
    ee.on('b', () => {});
    assert(ee._eventsCount === 2, 'Adding different event should increase _eventsCount');
    ee.removeListener('a', l1);
    assert(ee._eventsCount === 2, 'Removing one listener (still has others) should NOT decrease _eventsCount');
    ee.removeAllListeners('a');
    assert(ee._eventsCount === 1, 'Removing all listeners for an event should decrease _eventsCount');
    ee.removeAllListeners();
    assert(ee._eventsCount === 0, 'removeAllListeners() should reset _eventsCount to 0');
    console.log('OK: _eventsCount internal property functional.');

    // Test Symbol.asyncDispose
    const asyncDispose = Symbol.asyncDispose || Symbol.for('Symbol.asyncDispose');
    if (typeof ee[asyncDispose] === 'function') {
        await ee[asyncDispose]();
        console.log('OK: EventEmitter[Symbol.asyncDispose] functional.');
    }

    // Test 'newListener' / 'removeListener' order (Node v24 behavior)
    let order = [];
    const ee2 = new EventEmitter();
    ee2.on('newListener', (event) => {
        if (event === 'test') {
            order.push('newListener');
            assert(ee2.listenerCount('test') === 0, 'newListener should be emitted BEFORE adding');
        }
    });
    const listener = () => {};
    ee2.on('test', listener);
    order.push('added');

    ee2.on('removeListener', (event) => {
        if (event === 'test') {
            order.push('removeListener');
            assert(ee2.listenerCount('test') === 0, 'removeListener should be emitted AFTER removal');
        }
    });
    ee2.removeListener('test', listener);
    order.push('removed');

    assert(JSON.stringify(order) === JSON.stringify(['newListener', 'added', 'removeListener', 'removed']), 'Wrong event order');
    console.log('OK: Event emission order correct.');
}

async function runTests() {
    try {
        await testErrorMonitor();
        await testRemoveAllListenersEmit();
        await testListenerCountOverload();
        await testEventTargetV24();
        await testEEV24Deep();
        console.log('\nAll new feature tests passed successfully!');
    } catch (err) {
        console.error('\nTest failed!');
        console.error(err);
        process.exit(1);
    }
}

runTests();
