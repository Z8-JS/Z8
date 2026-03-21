import stream from 'node:stream';

const { pipeline, finished } = stream.promises;

class MyReadable extends stream.Readable {
  constructor(data, options) {
    super(options);
    this.data = data;
    this.index = 0;
    console.log('MyReadable constructor called.');
  }

  _read(size) {
    console.log('MyReadable _read called with size:', size);
    while (this.index < this.data.length) {
      const chunk = this.data[this.index];
      const canPush = this.push(chunk);
      console.log(`MyReadable pushing: ${chunk}, canPush: ${canPush}`);
      this.index++;
      if (!canPush) {
        // If push returns false, the internal buffer is full, stop pushing for now
        console.log('MyReadable internal buffer full, stopping _read for now.');
        break;
      }
    }
    if (this.index >= this.data.length) {
      this.push(null); // No more data
      console.log('MyReadable pushing null, end of data.');
    }
  }
}

class MyWritable extends stream.Writable {
  constructor(options) {
    super(options);
    this.receivedData = [];
    console.log('MyWritable constructor called.');
  }

  _write(chunk, encoding, callback) {
    console.log(`MyWritable _write called with chunk: ${chunk.toString()}`);
    this.receivedData.push(chunk.toString());
    console.log(`Writable received: ${chunk.toString()}`);
    callback();
  }
}

async function runTests() {
  console.log('--- Testing Readable.from ---');
  const readableFromArray = stream.Readable.from(['item1', 'item2', 'item3']);
  let fromData = [];
  readableFromArray.on('data', (chunk) => {
    fromData.push(chunk.toString());
  });
  await finished(readableFromArray);
  console.log('Readable.from collected:', fromData);
  if (fromData.join(',') === 'item1,item2,item3') {
    console.log('Readable.from test PASSED');
  } else {
    console.error('Readable.from test FAILED');
  }

  console.log('\n--- Testing stream.promises.pipeline ---');
  const readable = new MyReadable(['hello', 'world', 'z8']);
  const writable = new MyWritable();

  try {
    await pipeline(readable, writable);
    console.log('Pipeline succeeded.');
    console.log('Writable collected:', writable.receivedData);
    if (writable.receivedData.join(',') === 'hello,world,z8') {
      console.log('Pipeline test PASSED');
    } else {
      console.error('Pipeline test FAILED');
    }
  } catch (err) {
    console.error('Pipeline failed:', err);
    console.error('Pipeline test FAILED');
  }

  console.log('\n--- Testing stream.promises.finished ---');
  const finishedStream = new MyReadable(['final', 'data']);
  let finishedData = [];
  finishedStream.on('data', (chunk) => {
    finishedData.push(chunk.toString());
  });

  try {
    await finished(finishedStream);
    console.log('Finished stream completed.');
    console.log('Finished stream collected:', finishedData);
    if (finishedData.join(',') === 'final,data') {
      console.log('Finished test PASSED');
    } else {
      console.error('Finished test FAILED');
    }
  } catch (err) {
    console.error('Finished stream failed:', err);
    console.error('Finished test FAILED');
  }
}

runTests();

// Keep the process alive for a short period to allow async operations to complete
setTimeout(() => {
  console.log('Test script finished execution.');
}, 5000); // 5 second delay
