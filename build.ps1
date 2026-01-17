# Zane V8 (Z8) Master Build Script
# Run this in VS 2026 Developer PowerShell

Write-Host "🚀 Building Zane V8 (Z8)..." -ForegroundColor Cyan
Write-Host "=" * 40

# Step 0: Extract Shims
if (Test-Path "libs/v8_monolith.lib") {
    Write-Host "[0/3] Regenerating Temporal shims..." -ForegroundColor Yellow
    python tools/extract_shims.py
} else {
    Write-Host "⚠️  libs/v8_monolith.lib not found. Skipping shim regeneration." -ForegroundColor Gray
}

# Step 1: Compile
Write-Host "[1/3] Compiling C++ source files..." -ForegroundColor Yellow
cl.exe /c src/main.cpp src/temporal_shims.cpp `
    /I"include" /I"src" `
    /std:c++latest /Zc:__cplusplus /EHsc /O2 /MT /DNDEBUG `
    /DV8_COMPRESS_POINTERS /DV8_ENABLE_SANDBOX `
    /nologo

if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ Compilation failed!" -ForegroundColor Red
    exit 1
}

# Step 2: Link
Write-Host "[2/3] Linking z8.exe..." -ForegroundColor Yellow
link.exe /OUT:z8.exe `
    main.obj temporal_shims.obj `
    libs/v8_monolith.lib `
    libcmt.lib libcpmt.lib `
    winmm.lib dbghelp.lib shlwapi.lib user32.lib iphlpapi.lib `
    advapi32.lib shell32.lib ole32.lib uuid.lib rpcrt4.lib ntdll.lib `
    /SUBSYSTEM:CONSOLE `
    /MACHINE:X64 `
    /NOLOGO

if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ Linking failed!" -ForegroundColor Red
    exit 1
}

Write-Host "=" * 40
Write-Host "✅ Zane V8 (Z8) is ready!" -ForegroundColor Green
Write-Host "Run it with: .\z8.exe" -ForegroundColor Cyan
