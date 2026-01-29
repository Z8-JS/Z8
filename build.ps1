# Zane V8 (Z8) Master Build Script

param(
    [Parameter(Mandatory=$false)]
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Release"
)

# Step -1: Ensure MSVC environment is loaded
if (-not $env:INCLUDE) {
    Write-Host "MSVC environment not detected. Attempting to locate Visual Studio..."
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $installPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($installPath) {
            $vcvars = Join-Path $installPath "VC\Auxiliary\Build\vcvars64.bat"
            if (Test-Path $vcvars) {
                Write-Host "Loading environment from $vcvars..."
                $envVars = cmd /c "`"$vcvars`" x64 && set"
                foreach ($line in $envVars) {
                    if ($line -match "^([^=]+)=(.*)$") {
                        $name = $matches[1]
                        $value = $matches[2]
                        Set-Item -Path "Env:\$name" -Value $value
                    }
                }
            }
        }
    }
}

if (-not (Get-Command "cl.exe" -ErrorAction SilentlyContinue)) {
    Write-Error "cl.exe not found in PATH. Please run this script from a Developer Command Prompt or ensure Visual Studio is installed."
    exit 1
}

Write-Host "Building Zane V8 (Z8)..."
Write-Host "Configuration: $Config"

# Step 0: Extract Shims
if (Test-Path "v8/libs/v8_monolith.lib") {
    Write-Host "[0/3] Regenerating Temporal shims..."
    python tools/extract_shims.py
}

# Always use /MT /O2 /DNDEBUG for V8 compatibility
$cppFlags = @(
    "/std:c++latest", "/Zc:__cplusplus", "/EHsc", 
    "/O2", "/Oi", "/Ot", "/MT", "/DNDEBUG",
    "/DV8_COMPRESS_POINTERS", "/DV8_ENABLE_SANDBOX",
    "/nologo", "/c",
    "/Iv8/include", "/Isrc"
)

$linkFlags = @(
    "/OUT:z8.exe", "/SUBSYSTEM:CONSOLE", "/MACHINE:X64", "/NOLOGO",
    "main.obj", "temporal_shims.obj", "console.obj", "fs.obj", "timer.obj",
    "V8\out.gn\x64.release\obj\v8_monolith.lib",
    "libcmt.lib", "libcpmt.lib",
    "winmm.lib", "dbghelp.lib", "shlwapi.lib", "user32.lib", "iphlpapi.lib",
    "advapi32.lib", "shell32.lib", "ole32.lib", "uuid.lib", "rpcrt4.lib", "ntdll.lib"
)

if ($Config -eq "Release") {
    Write-Host "Enabling Whole Program Optimization (/GL /LTCG)..."
    $cppFlags += "/GL"
    $linkFlags += "/LTCG"
}

if ($Config -eq "Debug") {
    Write-Host "Enabling Z8_DEBUG_VERBOSE macro..."
    $cppFlags += "/DZ8_DEBUG_VERBOSE"
    $linkFlags += "/DEBUG"
}

# Step 1: Compile
Write-Host "[1/3] Compiling C++ source files..."
$sources = @("src/main.cpp", "src/temporal_shims.cpp", "src/module/console.cpp", "src/module/node/fs/fs.cpp", "src/module/timer.cpp")
& cl.exe $cppFlags $sources

if ($LASTEXITCODE -ne 0) {
    Write-Host "Compilation failed!"
    exit 1
}

# Step 2: Link
Write-Host "[2/3] Linking z8.exe..."
& link.exe $linkFlags

if ($LASTEXITCODE -ne 0) {
    Write-Host "Linking failed!"
    exit 1
}

Write-Host "Zane V8 (Z8) is ready ($Config)!"
Write-Host "Run it with: .\z8.exe test.js"
