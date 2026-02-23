# Zane V8 (Z8) Master Build Script

param(
    [Parameter(Mandatory=$false)]
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Release"
)

# Step 0: Check coding style
Write-Host "[Step 0/4] Checking coding style..."
if (Test-Path "tools/check_style.py") {
    python tools/check_style.py ./src/
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Coding style check failed! Please fix the errors before building."
        exit 1
    }
} else {
    Write-Warning "Style checker (tools/check_style.py) not found. Skipping..."
}

# Step 1: Ensure MSVC environment is loaded
Write-Host "[Step 1/4] Ensuring MSVC environment is loaded..."
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

# Ensure build directory exists
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

# Clean up old object files in root to avoid confusion
Remove-Item -Path "*.obj" -ErrorAction SilentlyContinue

Write-Host "Building Zane V8 (Z8)..."
Write-Host "Configuration: $Config"

# Step 2: Extract Shims
Write-Host "[Step 2/4] Extracting Temporal shims..."
if (Test-Path "v8/libs/v8_monolith.lib") {
    python tools/extract_shims.py
}

# Always use /MT /O2 /DNDEBUG for V8 compatibility
$cppFlags = @(
    "/std:c++latest", "/Zc:__cplusplus", "/EHsc", 
    "/O2", "/Oi", "/Ot", "/MT", "/DNDEBUG",
    "/DV8_COMPRESS_POINTERS",
    "/nologo", "/c",
    "/Fo`"build\\`"",  # Redirect object files to build directory
    "/Iv8/include", "/Isrc", "/Ideps/zlib", "/Ideps/brotli/c/include", "/Ideps/zstd/lib"
)

# Variant without /Fo directory flag, for compiling individual files with explicit /Fo output path
$cppFlagsNoFo = $cppFlags | Where-Object { $_ -notlike '/Fo*' }

$linkFlags = @(
    "/OUT:z8.exe", "/SUBSYSTEM:CONSOLE", "/MACHINE:X64", "/NOLOGO",
    "build\main.obj", "build\temporal_shims.obj", "build\console.obj", "build\fs.obj", "build\path.obj", "build\os.obj", "build\process.obj", "build\util.obj", "build\timer.obj", "build\buffer.obj", "build\zlib.obj", "build\events.obj",
    "build\adler32.obj", "build\compress.obj", "build\crc32.obj", "build\deflate.obj", "build\infback.obj", "build\inffast.obj", "build\inflate.obj", "build\inftrees.obj", "build\trees.obj", "build\uncompr.obj", "build\zutil.obj",
    "build\constants.obj", "build\context.obj", "build\dictionary.obj", "build\platform.obj", "build\shared_dictionary.obj", "build\transform.obj",
    "build\bit_reader.obj", "build\decode.obj", "build\huffman.obj", "build\prefix.obj", "build\state.obj", "build\static_init_dec.obj",
    "build\backward_references.obj", "build\backward_references_hq.obj", "build\bit_cost.obj", "build\block_splitter.obj", "build\brotli_bit_stream.obj", "build\cluster.obj", "build\command.obj", "build\compound_dictionary.obj", "build\compress_fragment.obj", "build\compress_fragment_two_pass.obj", "build\dictionary_hash.obj", "build\encode.obj", "build\encoder_dict.obj", "build\entropy_encode.obj", "build\fast_log.obj", "build\histogram.obj", "build\literal_cost.obj", "build\memory.obj", "build\metablock.obj", "build\static_dict.obj", "build\static_dict_lut.obj", "build\static_init_enc.obj", "build\utf8_util.obj",
    "build\debug.obj", "build\entropy_common.obj", "build\error_private.obj", "build\fse_decompress.obj", "build\pool.obj", "build\threading.obj", "build\xxhash.obj", "build\zstd_common.obj",
    "build\fse_compress.obj", "build\hist.obj", "build\huf_compress.obj", "build\zstd_compress.obj", "build\zstd_compress_literals.obj", "build\zstd_compress_sequences.obj", "build\zstd_compress_superblock.obj", "build\zstd_double_fast.obj", "build\zstd_fast.obj", "build\zstd_lazy.obj", "build\zstd_ldm.obj", "build\zstd_opt.obj", "build\zstd_preSplit.obj", "build\zstdmt_compress.obj",
    "build\huf_decompress.obj", "build\zstd_ddict.obj", "build\zstd_decompress.obj", "build\zstd_decompress_block.obj",
    "V8\out.gn\x64.release\obj\v8_monolith.lib",
    "libcmt.lib", "libcpmt.lib",
    "winmm.lib", "dbghelp.lib", "shlwapi.lib", "user32.lib", "iphlpapi.lib",
    "advapi32.lib", "shell32.lib", "ole32.lib", "uuid.lib", "rpcrt4.lib", "ntdll.lib", "userenv.lib"
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

# Step 3: Compile
Write-Host "[3/4] Compiling C++ source files..."
$sources = @("src/main.cpp", "src/temporal_shims.cpp", "src/module/console.cpp", "src/module/node/fs/fs.cpp", "src/module/node/path/path.cpp", "src/module/node/os/os.cpp", "src/module/node/process/process.cpp",    "src/module/node/util/util.cpp",
    "src/module/node/buffer/buffer.cpp",
    "src/module/node/zlib/zlib.cpp",
    "src/module/node/events/events.cpp",
 "src/module/timer.cpp")
$sources += @("deps/zlib/adler32.c", "deps/zlib/compress.c", "deps/zlib/crc32.c", "deps/zlib/deflate.c", "deps/zlib/infback.c", "deps/zlib/inffast.c", "deps/zlib/inflate.c", "deps/zlib/inftrees.c", "deps/zlib/trees.c", "deps/zlib/uncompr.c", "deps/zlib/zutil.c")
$sources += @("deps/brotli/c/common/constants.c", "deps/brotli/c/common/context.c", "deps/brotli/c/common/dictionary.c", "deps/brotli/c/common/platform.c", "deps/brotli/c/common/shared_dictionary.c", "deps/brotli/c/common/transform.c")
# dec/static_init.c and enc/static_init.c share the same basename so they must
# be compiled individually with /Fo to avoid object-name collisions.
$sources += @("deps/brotli/c/dec/bit_reader.c", "deps/brotli/c/dec/decode.c", "deps/brotli/c/dec/huffman.c", "deps/brotli/c/dec/prefix.c", "deps/brotli/c/dec/state.c")
$sources += @("deps/brotli/c/enc/backward_references.c", "deps/brotli/c/enc/backward_references_hq.c", "deps/brotli/c/enc/bit_cost.c", "deps/brotli/c/enc/block_splitter.c", "deps/brotli/c/enc/brotli_bit_stream.c", "deps/brotli/c/enc/cluster.c", "deps/brotli/c/enc/command.c", "deps/brotli/c/enc/compound_dictionary.c", "deps/brotli/c/enc/compress_fragment.c", "deps/brotli/c/enc/compress_fragment_two_pass.c", "deps/brotli/c/enc/dictionary_hash.c", "deps/brotli/c/enc/encode.c", "deps/brotli/c/enc/encoder_dict.c", "deps/brotli/c/enc/entropy_encode.c", "deps/brotli/c/enc/fast_log.c", "deps/brotli/c/enc/histogram.c", "deps/brotli/c/enc/literal_cost.c", "deps/brotli/c/enc/memory.c", "deps/brotli/c/enc/metablock.c", "deps/brotli/c/enc/static_dict.c", "deps/brotli/c/enc/static_dict_lut.c", "deps/brotli/c/enc/utf8_util.c")
$sources += @("deps/zstd/lib/common/debug.c", "deps/zstd/lib/common/entropy_common.c", "deps/zstd/lib/common/error_private.c", "deps/zstd/lib/common/fse_decompress.c", "deps/zstd/lib/common/pool.c", "deps/zstd/lib/common/threading.c", "deps/zstd/lib/common/xxhash.c", "deps/zstd/lib/common/zstd_common.c")
$sources += @("deps/zstd/lib/compress/fse_compress.c", "deps/zstd/lib/compress/hist.c", "deps/zstd/lib/compress/huf_compress.c", "deps/zstd/lib/compress/zstd_compress.c", "deps/zstd/lib/compress/zstd_compress_literals.c", "deps/zstd/lib/compress/zstd_compress_sequences.c", "deps/zstd/lib/compress/zstd_compress_superblock.c", "deps/zstd/lib/compress/zstd_double_fast.c", "deps/zstd/lib/compress/zstd_fast.c", "deps/zstd/lib/compress/zstd_lazy.c", "deps/zstd/lib/compress/zstd_ldm.c", "deps/zstd/lib/compress/zstd_opt.c", "deps/zstd/lib/compress/zstdmt_compress.c")
$sources += @("deps/zstd/lib/decompress/huf_decompress.c", "deps/zstd/lib/decompress/zstd_ddict.c", "deps/zstd/lib/decompress/zstd_decompress.c", "deps/zstd/lib/decompress/zstd_decompress_block.c")
& cl.exe $cppFlags $sources
if ($LASTEXITCODE -ne 0) { Write-Host "Compilation failed!"; exit 1 }

# Compile the two static_init.c files individually with distinct output names
Write-Host "Compiling brotli static_init files..."
& cl.exe $cppFlagsNoFo "/Fobuild\static_init_dec.obj" "deps/brotli/c/dec/static_init.c"
if ($LASTEXITCODE -ne 0) { Write-Host "Compilation failed (static_init_dec)!"; exit 1 }
& cl.exe $cppFlagsNoFo "/Fobuild\static_init_enc.obj" "deps/brotli/c/enc/static_init.c"
if ($LASTEXITCODE -ne 0) { Write-Host "Compilation failed (static_init_enc)!"; exit 1 }
# Compile zstd_preSplit.c separately to guarantee ZSTD_splitBlock is available
Write-Host "Compiling zstd_preSplit.c..."
& cl.exe $cppFlagsNoFo "/Fobuild\zstd_preSplit.obj" "deps/zstd/lib/compress/zstd_preSplit.c"
if ($LASTEXITCODE -ne 0) { Write-Host "Compilation failed (zstd_preSplit)!"; exit 1 }


# Step 4: Link
Write-Host "[4/4] Linking z8.exe..."
& link.exe $linkFlags

if ($LASTEXITCODE -ne 0) {
    Write-Host "Linking failed!"
    exit 1
}

Write-Host "Zane V8 (Z8) is ready ($Config)!"
Write-Host "Run it with: .\z8.exe test.js"
