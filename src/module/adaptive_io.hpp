#ifndef ZANE_ADAPTIVE_IO_H
#define ZANE_ADAPTIVE_IO_H

#include <chrono>
#include <cstdio>
#include <mutex>
#include <functional>
#include <atomic>

#ifdef _WIN32
#include <io.h>
#define ZANE_ISATTY _isatty
#define ZANE_FILENO _fileno
#else
#include <unistd.h>
#define ZANE_ISATTY isatty
#define ZANE_FILENO fileno
#endif

namespace zane {
namespace module {

inline std::atomic<bool> g_adaptive_io_enabled{true};

/**
 * AdaptiveIO provides a mechanism to balance between low-latency and high-throughput.
 * It automatically detects high-frequency I/O bursts and switches from immediate
 * flushing to buffered I/O to maximize performance.
 */
class AdaptiveIO {
public:
    AdaptiveIO(int32_t burst_threshold = 20, int32_t window_ms = 50)
        : m_burst_threshold(burst_threshold), m_window_ms(window_ms),
          m_calls_in_burst(0), m_last_flush(std::chrono::steady_clock::now()),
          m_last_burst_flush(std::chrono::steady_clock::now()) {}

    /**
     * Enables or disables Adaptive I/O globally.
     * When disabled, reverts standard streams to default C/C++ buffering rules:
     * - Line-buffered (_IOLBF) if connected to a terminal (isatty).
     * - Full-buffered (_IOFBF, 8 KB) if connected to a file/pipe.
     */
    static void setEnabled(bool enabled) {
        g_adaptive_io_enabled.store(enabled, std::memory_order_relaxed);
        setupBuffer(stdout);
        setupBuffer(stderr);
    }

    static bool isEnabled() {
        return g_adaptive_io_enabled.load(std::memory_order_relaxed);
    }

    /**
     * Decisions whether the I/O should be flushed based on current burst frequency.
     * If Adaptive I/O is disabled globally, always flushes immediately.
     * @param flush A callback function that performs the actual flush/syscall.
     */
    template<typename FlushFunc>
    void apply(FlushFunc&& flush) {
        if (!g_adaptive_io_enabled.load(std::memory_order_relaxed)) {
            flush();
            return;
        }

        std::lock_guard<std::mutex> lock(m_mutex);
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_flush).count();

        if (elapsed < m_window_ms) {
            m_calls_in_burst++;
        } else {
            m_calls_in_burst = 0;
            m_last_flush = now;
        }

        // Logic:
        // 1. If we are NOT in a burst, flush immediately for low latency.
        // 2. If we ARE in a burst, still flush periodically (once per window_ms)
        //    to prevent excessive buffer accumulation. Without this, all data
        //    accumulated during the burst gets bulk-flushed at process exit,
        //    overwhelming the Windows console pipe and causing Broken Pipe errors.
        if (m_calls_in_burst < m_burst_threshold) {
            flush();
        } else {
            auto since_burst_flush = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - m_last_burst_flush).count();
            if (since_burst_flush >= m_window_ms) {
                m_last_burst_flush = now;
                flush();
            }
        }
    }

    /**
     * Specialization for standard FILE* streams.
     */
    void flushIfNeeded(FILE* p_stream) {
        if (!p_stream) return;
        apply([p_stream]() {
            if (p_stream == stderr) {
                std::fflush(stdout);
            }
            std::fflush(p_stream);
        });
    }

    /**
     * Configures a stream's buffering based on global Adaptive I/O setting:
     * - Enabled: Zane's optimized 64KB full buffering.
     * - Disabled: Standard C/C++ buffering (line-buffered if tty, 8KB full-buffered if file/pipe).
     */
    static void setupBuffer(FILE* p_stream, size_t size = 64 * 1024) {
        if (!p_stream) return;

        if (g_adaptive_io_enabled.load(std::memory_order_relaxed)) {
            std::setvbuf(p_stream, nullptr, _IOFBF, static_cast<int>(size));
        } else {
            int32_t fd = ZANE_FILENO(p_stream);
            if (fd >= 0 && ZANE_ISATTY(fd)) {
                std::setvbuf(p_stream, nullptr, _IOLBF, BUFSIZ);
            } else {
                std::setvbuf(p_stream, nullptr, _IOFBF, 8 * 1024);
            }
        }
    }

private:
    int32_t m_burst_threshold;
    int32_t m_window_ms;
    int32_t m_calls_in_burst;
    std::chrono::steady_clock::time_point m_last_flush;
    std::chrono::steady_clock::time_point m_last_burst_flush;
    std::mutex m_mutex;
};

// Global shared instances for standard streams to ensure consistent 
// adaptive behavior across modules (console, fs, etc.)
inline AdaptiveIO g_stdout_io;
inline AdaptiveIO g_stderr_io;

} // namespace module
} // namespace zane

#endif // ZANE_ADAPTIVE_IO_H
