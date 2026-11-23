# Fetches dependencies (first run only), configures and builds.
#   .\build.ps1           Release build
#   .\build.ps1 -Debug    Debug build
#   .\build.ps1 -Clean    wipe build/ first
param(
    [switch]$Debug,
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot

$deps = @(
    @{ Name = 'imgui';   Url = 'https://github.com/ocornut/imgui.git';      Tag = 'v1.92.9b' },
    @{ Name = 'minhook'; Url = 'https://github.com/TsudaKageyu/minhook.git'; Tag = 'v1.3.4' }
)

foreach ($dep in $deps) {
    $path = Join-Path 'vendor' $dep.Name
    if (-not (Test-Path $path)) {
        Write-Host "fetching $($dep.Name) $($dep.Tag)"
        git -c advice.detachedHead=false clone --quiet --depth 1 --branch $dep.Tag $dep.Url $path
        if ($LASTEXITCODE -ne 0) { throw "git clone $($dep.Name) failed" }
    }
}

if ($Clean -and (Test-Path build)) {
    Remove-Item -Recurse -Force build
}

if (-not (Test-Path build/CMakeCache.txt)) {
    $configured = $false
    foreach ($generator in 'Visual Studio 18 2026', 'Visual Studio 17 2022') {
        cmake -S . -B build -G $generator -A x64 2>$null | Out-Null
        if ($LASTEXITCODE -eq 0) {
            Write-Host "configured with $generator"
            $configured = $true
            break
        }
        if (Test-Path build) { Remove-Item -Recurse -Force build }
    }
    if (-not $configured) { throw 'no Visual Studio 2022/2026 with the C++ workload found' }
}

$config = if ($Debug) { 'Debug' } else { 'Release' }
cmake --build build --config $config
if ($LASTEXITCODE -ne 0) { throw 'build failed' }

Write-Host ''
Write-Host "built build\bin\$config\Il2cppBase.dll"
