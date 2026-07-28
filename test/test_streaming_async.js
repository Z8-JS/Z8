// Self-contained test for streaming response + async fetch path.
//
// Verifies the new res.writeHead() / res.write() / res.end() API works in
// combination with async fetch handlers (Promise). This exercises:
//   - writeHead replaces headers collected via setHeader()
//   - write() auto-flushes a chunked head when writeHead() was not called
//   - empty write() is a no-op (not a malformed zero-length chunk)
//   - the legacy res.send() path still produces Content-Length, NOT chunked
//
// Usage: ./zane.exe test/test_streaming_async.js
// Then in another shell:
//   curl -s -i http://127.0.0.1:8744/async    # expect chunked, "ASYNC-3-2-1"
//   curl -s -i http://127.0.0.1:8744/legacy   # expect Content-Length: 11
//
// Server is kept alive so the assertions can be checked externally; kill
// the process when done.

const PORT = 8744;

console.log("[stream-async] server binding to :8744...");

Zane.serve({
    port: PORT,
    hostname: "127.0.0.1",
    fetch(req, res) {
        const url = req.url;

        // Async handler returning a Promise that resolves AFTER calling
        // writeHead + multiple write + end. This is the most common
        // real-world pattern (read from DB, stream rows as you go).
        if (url === "/async") {
            Promise.resolve()
                .then(() => {
                    res.writeHead(200, { "Content-Type": "text/plain" });
                    res.write("ASYNC-3\n");
                })
                .then(() => {
                    res.write("ASYNC-2\n");
                })
                .then(() => {
                    res.write("ASYNC-1\n");
                    res.end();
                });
            return;
        }

        // Empty write should be a no-op and not corrupt the chunked stream.
        if (url === "/empty-write") {
            res.writeHead(200);
            res.write("first");
            res.write(""); // no-op
            res.write("second");
            res.end();
            return;
        }

        // writeHead with no headers arg — should keep previously-set
        // headers collected via setHeader() (Node.js semantics).
        if (url === "/merge-headers") {
            res.setHeader("X-Foo", "1");
            res.setHeader("X-Bar", "2");
            res.writeHead(200); // no headers arg — keep X-Foo, X-Bar
            res.write("merged");
            res.end();
            return;
        }

        // writeHead with headers arg — should REPLACE collected headers.
        if (url === "/replace-headers") {
            res.setHeader("X-Foo", "1");
            res.writeHead(200, { "X-Replaced": "yes" });
            res.write("replaced");
            res.end();
            return;
        }

        // Legacy path: res.send() with no write/writeHead should still
        // produce Content-Length framing, NOT chunked.
        if (url === "/legacy") {
            res.send("legacy-ok");
            return;
        }

        // Once-per-server smoke assertion that the V8 wiring still loads.
        if (!asserted) {
            asserted = true;
            console.log("[stream-async] server up, endpoints:");
            console.log("  GET /async            -> chunked (3 ticks of write)");
            console.log("  GET /empty-write      -> chunked (empty write no-op)");
            console.log("  GET /merge-headers    -> chunked (X-Foo + X-Bar preserved)");
            console.log("  GET /replace-headers  -> chunked (only X-Replaced)");
            console.log("  GET /legacy           -> Content-Length: 9");
        }

        res.status = 404;
        res.send("not found");
    },
});
