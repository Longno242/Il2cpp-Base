$ErrorActionPreference = "Continue"
$Root = Split-Path -Parent $PSScriptRoot
Set-Location $Root

$generators = @(
    "Visual Studio 18 2026",
    "Visual Studio 17 2022"
)

foreach ($gen in $generators) {
    Write-Host "[..] Trying $gen..."
    & cmake -B build -G $gen -A x64
    if ($LASTEXITCODE -eq 0) {
        Write-Host "[ok] Configured with $gen"
        exit 0
    }
    Write-Host "[!!] $gen not available"
    if (Test-Path (Join-Path $Root "build")) {
        Remove-Item -Recurse -Force (Join-Path $Root "build")
    }
}

Write-Host "[..] Falling back to default CMake generator..."
& cmake -B build -A x64
if ($LASTEXITCODE -ne 0) {
    throw "CMake configure failed. Install Visual Studio 2022+ with the C++ desktop workload."
}
Write-Host "[ok] Configured with default generator"
