# Zane V8 (Z8)

![Lines of Code](https://img.shields.io/endpoint?url=https://codetabs.com/count-loc/count-loc-by-url?url=https://github.com/Z8-JS/Z8&label=Lines%20of%20Code&color=blue)
**Zane V8 (Z8)** is a high-performance, lightweight JavaScript runtime built on top of Google's V8 engine. Written in pure C++ for maximum stability and performance on Windows.

## ✨ Features

- **Pure C++17/20**: Zero overhead, built with the latest MSVC toolchain.
- **Monolithic Build**: Easy to distribute, no complex DLL dependencies.
- **Zane Context**: Optimized default environment with essential APIs like `console.log`.
- **Temporal Enabled**: Ready for next-generation JavaScript date and time handling.

## 🏗 Project Structure

- `src/`: Core C++ source code.
- `include/`: V8 and project headers.
- `libs/`: Pre-built V8 monolithic libraries.
- `tools/`: Utility scripts for build maintenance.

## 🛠 Prerequisites

- **Visual Studio 2022/2026** with "Desktop development with C++" workload.
- **Python 3.x** (for shim extraction).
- **V8 Artifacts**: Get them from [v8-z8](https://github.com/Z8-JS/v8-z8).

## 🚀 Building Z8

1. Open **Developer PowerShell for Visual Studio**.
2. Run the build script:
   ```powershell
   .\build.ps1
   ```
3. The resulting `z8.exe` will be created in the root directory.

## 🏁 Running

Make sure `icudtl.dat` is in the same directory as `z8.exe`.

```powershell
.\z8.exe
```

## 📜 Example

Zane V8 runs C++ wrapped JavaScript:

```cpp
z8::Runtime rt;
rt.Run("console.log('Hello from Z8!');", "app.js");
```

---

**Z8: V8 - A Gift from God.**

