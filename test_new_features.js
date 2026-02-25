import { EventEmitter, errorMonitor } from 'node:events';

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

async function runTests() {
    try {
        await testErrorMonitor();
        await testRemoveAllListenersEmit();
        await testListenerCountOverload();
        console.log('\nAll new feature tests passed successfully!');
    } catch (err) {
        console.error('\nTest failed!');
        console.error(err);
        process.exit(1);
    }
}

runTests();
