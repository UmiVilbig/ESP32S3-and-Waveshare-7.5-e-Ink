# Build and run the host-side unit tests on Windows.
# Requires a C++17 compiler on PATH (g++/MinGW, clang++, or "cl" from MSVC).
# Usage:  pwsh test/run_tests.ps1
$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
Push-Location $here
try {
    $sources = @(
        "main.cpp", "tests_layout.cpp", "tests_bmp.cpp", "tests_status.cpp",
        "../src/layout.cpp", "../src/bmp.cpp", "../src/status.cpp"
    )
    $cxx = $null
    foreach ($c in @("g++", "clang++")) {
        if (Get-Command $c -ErrorAction SilentlyContinue) { $cxx = $c; break }
    }
    if (-not $cxx) {
        Write-Error "No C++ compiler found (looked for g++, clang++). Install MinGW-w64 or LLVM."
    }
    & $cxx -std=c++17 -Wall -Wextra $sources -o run_tests.exe
    & ./run_tests.exe
    exit $LASTEXITCODE
}
finally {
    Pop-Location
}
