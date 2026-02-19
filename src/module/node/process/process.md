# Process

The `process` module provides information about, and control over, the current Node.js process. While it is available as a global, it can also be accessed explicitly via `import` or `require`:

```js
import process from "node:process";
```

## Properties

### `process.env`

The `process.env` property returns an object containing the user environment.
Z8 extension: Z8 automatically loads environment variables from a `.env` file in the current working directory if it exists (dotenv functionality).

```js
console.log(process.env.PATH);
console.log(process.env.MY_VAR); // Loads from .env
```

### `process.platform`

Returns a string identifying the operating system platform.

- `'win32'`
- `'linux'`
- `'darwin'`

### `process.version`

The `process.version` property returns the Z8 version string.

## Methods

### `process.cwd()`

Returns the current working directory of the process.

```js
console.log(`Current directory: ${process.cwd()}`);
```

### `process.exit([code])`

Terminates the process synchronously with an exit status of `code`. If omitted, exit uses either the 'success' code `0`.

```js
process.exit(1);
```

### `process.uptime()`

Returns the number of seconds the current process has been running.

```js
console.log(`Uptime: ${process.uptime()}s`);
```
