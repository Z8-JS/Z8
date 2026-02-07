const util = require('node:util');

// Test format
console.log(util.format('%s:%s', 'foo', 'bar', 'baz'));

// Test inspect
const obj = { a: 1, b: { c: 2 } };
console.log(util.inspect(obj, { colors: true, depth: null }));

// Test promisify
const fs = require('node:fs');
const readFile = util.promisify(fs.readFile);
readFile(__filename).then(data => {
    console.log('Promisified read length:', data.length);
}).catch(err => {
    console.error('Promisify error:', err);
});

// Test inherits
function Parent() { this.name = 'parent'; }
Parent.prototype.sayHi = function() { console.log('Hi from', this.name); };
function Child() { Parent.call(this); this.name = 'child'; }
util.inherits(Child, Parent);
const c = new Child();
c.sayHi();

// Test types
console.log('Is Date:', util.types.isDate(new Date()));
console.log('Is Promise:', util.types.isPromise(Promise.resolve()));
