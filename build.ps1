# Zane Build Wrapper (CMake)
#
# All build logic now lives in CMakeLists.txt. This script:
#   1. loads the MSVC environment if needed (cl.exe / nmake.exe / rc.exe)
#   2. configures the project with CMake (NMake Makefiles on first run)
#   3. builds it (deps are compiled in-tree or auto-downloaded, see CMakeLists.txt)
#
# Usage: .\build.ps1 [-Config Release|Debug] [-Clean]

param(
    [Parameter(Mandatory=$false)]
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Release",

    [Parameter(Mandatory=$false)]
    [switch]$Clean,

    # Kept for backwards compatibility (was already a no-op in the old
    # script): IOCP is the default I/O strategy used by Zane.
    [Parameter(Mandatory=$false)]
    [switch]$UseIOCP
)

# NOTE: keep ErrorActionPreference at its default ("Continue"): CMake prints
# its status/info messages to stderr, which PowerShell 5.1 would otherwise
# treat as terminating errors and abort the script. Failures are caught via
# $LASTEXITCODE after each native command instead.
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir = Join-Path $root "build"

# --- Step 1: Ensure CMake is available ---
if (-not (Get-Command "cmake.exe" -ErrorAction SilentlyContinue)) {
    Write-Error "cmake.exe not found in PATH. Install CMake >= 3.24 (https://cmake.org/download/) and retry."
    exit 1
}

# --- Step 2: Ensure MSVC environment is loaded ---
if (-not $env:INCLUDE) {
    Write-Host "[build] MSVC environment not detected. Locating Visual Studio..."
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $installPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($installPath) {
            $vcvars = Join-Path $installPath "VC\Auxiliary\Build\vcvars64.bat"
            if (Test-Path $vcvars) {
                Write-Host "[build] Loading environment from $vcvars..."
                $envVars = cmd /c "`"$vcvars`" x64 && set"
                foreach ($line in $envVars) {
                    if ($line -match "^([^=]+)=(.*)$") {
                        Set-Item -Path "Env:\$($matches[1])" -Value $matches[2]
                    }
                }
            }
        }
    }
}

foreach ($tool in @("cl.exe", "nmake.exe", "rc.exe")) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
        Write-Error "$tool not found in PATH. Run this script from a Developer Command Prompt, or ensure Visual Studio with the 'Desktop development with C++' workload is installed."
        exit 1
    }
}

# --- Step 3: Clean (optional) ---
if ($Clean -and (Test-Path $buildDir)) {
    Write-Host "[build] Cleaning $buildDir..."
    Remove-Item -Path $buildDir -Recurse -Force
}

# --- Step 4: Configure (reuses the existing cache/generator afterwards) ---
$genArgs = @()
if (-not (Test-Path (Join-Path $buildDir "CMakeCache.txt"))) {
    $genArgs = @("-G", "NMake Makefiles")
}
Write-Host "[build] Configuring Zane ($Config)..."
& cmake -S $root -B $buildDir @genArgs "-DCMAKE_BUILD_TYPE=$Config"
if ($LASTEXITCODE -ne 0) { exit 1 }

# --- Step 5: Build (style check runs automatically, deps are handled by CMake) ---
Write-Host "[build] Building Zane ($Config)..."
& cmake --build $buildDir --config $Config
if ($LASTEXITCODE -ne 0) { exit 1 }

$exe = Join-Path $buildDir "bin\zane.exe"
Write-Host ""
Write-Host "Zane is ready ($Config): $exe"
Write-Host "Run it with: .\build\bin\zane.exe test\test.js"