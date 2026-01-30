import { stat, readdir, readFile, writeFile, mkdir, rm, existsSync } from 'node:fs';

async function testAsync() {
    console.log("🚀 Testing Async I/O...");
    const testDir = "./async_test_dir";
    
    try {
        if (existsSync(testDir)) {
            console.log("Cleaning up old test dir...");
            // rm is currently sync in our template, but exported as 'rm'
            rm(testDir, { recursive: true });
        }
        
        // Test mkdir (Note: using sync versions mapped to async names for now)
        mkdir(testDir);
        console.log("✅ mkdir successful");

        const filePath = `${testDir}/hello.txt`;
        await writeFile(filePath, "Hello from Gravity Bot and Z8 Async!");
        console.log("✅ writeFile successful");

        const content = await readFile(filePath);
        console.log("✅ readFile successful:", content);

        const stats = await stat(filePath);
        console.log("✅ stat successful - Size:", stats.size, "isFile:", stats.isFile());

        const files = await readdir(testDir);
        console.log("✅ readdir successful - Files:", files);

        console.log("\n🎉 All Async tests passed!");

    } catch (e) {
        console.error("❌ Async test failed:", e);
    }
}

testAsync();
