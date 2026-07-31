# ⚡ Adaptive I/O Strategy & Control Guide

**Adaptive I/O** is a core performance optimization mechanism in Zane designed to balance **ultra-low latency** for interactive console outputs and **high throughput** for heavy I/O operations (logging bursts, high-frequency file writes).

---

## 1. 🎯 Purpose & Problem Statement

In high-performance runtimes, standard stream I/O often suffers from a trade-off:
- **Unbuffered / Line-Buffered I/O (`_IOLBF`)**: Provides instant feedback for `console.log()` in interactive shells, but incurs massive OS system call overhead (`write`/`WriteFile`) when logging thousands of lines per second.
- **Full Buffering (`_IOFBF`)**: Maximizes throughput for bulk operations, but causes latency lag in interactive debugging and can overwhelm OS pipe buffers on process exit, leading to `Broken Pipe` errors.

**Adaptive I/O** dynamically bridges this gap without requiring developers to manage buffer sizes manually.

---

## 2. ⚙️ How Adaptive I/O Works

Adaptive I/O monitors stream activity per time window:

| Parameter | Default Value | Description |
|---|---|---|
| `m_burst_threshold` | `20 calls` | Number of I/O operations within window to trigger Burst Mode |
| `m_window_ms` | `50 ms` | Rolling time window duration |
| Buffer Size | `64 KB` | Stream buffer size when active (`_IOFBF`) |

### Operational States

1. **Low-Frequency / Interactive Mode** (`calls < 20` in 50ms):
   - Every I/O operation is flushed **immediately** (`fflush()`) to ensure real-time terminal output.
2. **Burst Mode** (`calls >= 20` in 50ms):
   - Flushes are deferred and handled via Zane's **64 KB full buffer**.
   - Periodic flush occurs once every `50ms` window to prevent buffer bloat and pipe overflow.

---

## 3. 🛠️ Disabling & Controlling Adaptive I/O

If your application requires strict deterministic buffering or standard C/C++ stream rules (e.g., benchmarking, piping to external tools, or low-level file stdout redirect), Adaptive I/O can be disabled via **JavaScript API**, **CLI Flags**, or **Environment Variables**.

### A. JavaScript API (`Zane.*`)

Adaptive I/O is exposed under the global `Zane` namespace:

```js
// Check current status (returns true by default)
console.log(Zane.isAdaptiveIOEnabled()); // true

// Disable Adaptive I/O globally
Zane.setAdaptiveIO(false);
console.log(Zane.isAdaptiveIOEnabled()); // false

// Re-enable Adaptive I/O
Zane.setAdaptiveIO(true);
```

### B. Command-Line Flag (CLI)

Pass `--no-adaptive-io` when launching `zane`:

```powershell
zane --no-adaptive-io app.js
```

### C. Environment Variable

Set `ZANE_NO_ADAPTIVE_IO` in your terminal or OS environment:

```powershell
# Windows PowerShell
$env:ZANE_NO_ADAPTIVE_IO="1"
zane app.js

# Linux / macOS
ZANE_NO_ADAPTIVE_IO=1 zane app.js
```

> **Note**: Both `1` and `true` (case-insensitive) are accepted values.

---

## 4. 🔄 Standard Fallback Behavior (When Disabled)

When Adaptive I/O is disabled globally via any of the methods above, Zane immediately reverts standard streams (`stdout`, `stderr`) to standard **C/C++ stdio buffering rules**:

- **Terminal Output (`isatty` = true)**: Reverts to **Line-Buffered (`_IOLBF`)** mode. Flushes automatically on newline `\n`.
- **File / Pipe Output (`isatty` = false)**: Reverts to standard **Fully-Buffered (`_IOFBF`)** mode with a standard `8 KB` buffer.

---

## 5. 💻 C++ Implementation Details

The core implementation lives in `src/module/adaptive_io.hpp`:

- **Zero-overhead Inlining**: The `AdaptiveIO` class is header-only and template-based (`apply<FlushFunc>`), allowing MSVC `/GL /LTCG` or GCC/Clang `-flto` to inline the burst check directly into the call site.
- **Thread Safety**: Uses an `std::atomic<bool> g_adaptive_io_enabled` flag and per-stream `std::mutex` locks during burst transitions to ensure safe concurrent operations across worker threads.
