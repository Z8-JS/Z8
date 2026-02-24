import events, { EventEmitter, captureRejectionSymbol } from 'node:events';

async function testCompatibility() {
    console.log('Testing Symbol.dispose support...');
    const ee = new EventEmitter();
    ee.on('foo', () => {});
    if (ee.listenerCount('foo') !== 1) throw new Error('Listener not added');
    
    // Explicit resource management (if supported by JS engine)
    const dispose = Symbol.dispose || Symbol.for('Symbol.dispose');
    if (typeof ee[dispose] === 'function') {
        ee[dispose]();
        if (ee.listenerCount('foo') !== 0) throw new Error('Dispose failed to remove listeners');
        console.log('OK: Symbol.dispose functional.');
    } else {
        console.log('WARN: Symbol.dispose not found on instance (check V8 version).');
    }

    console.log('Testing captureRejectionSymbol (nodejs.rejection)...');
    let rejectionArgs = null;
    class MyEmitter extends EventEmitter {
        constructor() {
            super({ captureRejections: true });
        }
        [captureRejectionSymbol](err, event, ...args) {
            rejectionArgs = { err, event, args };
        }
    }

    const myEe = new MyEmitter();
    myEe.on('error', (err) => {
        console.log('Error event emitted (should NOT happen if custom handler works)');
    });

    const promise = new Promise((_, reject) => setTimeout(() => reject(new Error('async fail')), 10));
    myEe.on('test', () => promise);
    
    console.log('Emitting test event...');
    myEe.emit('test');

    // Wait for rejection to be handled
    await new Promise(r => setTimeout(r, 100));

    if (rejectionArgs && rejectionArgs.event === 'test' && rejectionArgs.err.message === 'async fail') {
        console.log('OK: captureRejectionSymbol handler called correctly.');
    } else {
        throw new Error('captureRejectionSymbol handler NOT called or called with wrong args: ' + JSON.stringify(rejectionArgs));
    }

    console.log('Testing global captureRejections...');
    const oldDefault = events.captureRejections;
    events.captureRejections = true;
    const eeAuto = new EventEmitter();
    if (eeAuto.captureRejections !== true) throw new Error('Auto captureRejections failed');
    events.captureRejections = oldDefault;
    console.log('OK: Global captureRejections functional.');
}

testCompatibility().catch(err => {
    console.error(err);
    process.exit(1);
});
