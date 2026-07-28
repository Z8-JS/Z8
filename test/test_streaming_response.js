// Test streaming response API (chunked transfer encoding).
//
// Exercises three modes:
//   1. Legacy: res.send("...") — single-shot, Content-Length framed.
//   2. writeHead + write + end — explicit chunked streaming.
//   3. Auto-chunked: write("...") without prior writeHead() — should still
//      produce valid chunked bytes.
//
// Usage: ./zane.exe test/test_streaming_response.js
// Then, in another terminal: curl http://127.0.0.1:8743/<path>

const PORT = 8743;

Zane.serve({
    port: PORT,
    hostname: "127.0.0.1",
    fetch(req, res) {
        const url = req.url;

        if (url === "/legacy") {
            // Mode 1: legacy one-shot. Wire should contain Content-Length.
            res.send("legacy-body");
            return;
        }

        if (url === "/stream") {
            // Mode 2: explicit streaming.
            res.writeHead(200, { "Content-Type": "text/plain" });
            res.write("chunk-1 ");
            res.write("chunk-2 ");
            res.write("chunk-3");
            res.end();
            return;
        }

        if (url === "/auto-chunk") {
            // Mode 3: no writeHead, just write. Head should be auto-flushed
            // with Transfer-Encoding: chunked.
            res.setHeader("X-Test", "auto");
            res.write("hello ");
            res.write("world");
            res.end();
            return;
        }

        if (url === "/sse") {
            // SSE-style: write head with text/event-stream, then stream events.
            res.writeHead(200, {
                "Content-Type": "text/event-stream",
                "Cache-Control": "no-cache",
            });
            res.write("event: ping\ndata: 1\n\n");
            res.write("event: ping\ndata: 2\n\n");
            res.end();
            return;
        }

        if (url === "/binary") {
            // Binary streaming via Uint8Array.
            res.writeHead(200, { "Content-Type": "application/octet-stream" });
            const bytes = new Uint8Array([0x00, 0x01, 0x02, 0xff, 0xfe]);
            res.write(bytes);
            res.end();
            return;
        }

        res.status = 404;
        res.send("not found");
    },
});

console.log(`[stream-test] server on http://127.0.0.1:${PORT}`);
console.log(`[stream-test] try: /legacy, /stream, /auto-chunk, /sse, /binary`);
