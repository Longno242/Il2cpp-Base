$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$Vendor = Join-Path $Root "vendor"

New-Item -ItemType Directory -Force -Path $Vendor | Out-Null

function Ensure-Repo($Name, $Url, $Branch = "master") {
    $Path = Join-Path $Vendor $Name
    if (Test-Path (Join-Path $Path ".git")) {
        Write-Host "[ok] $Name already cloned"
        return
    }
    Write-Host "[..] Cloning $Name..."
    git clone --depth 1 --branch $Branch $Url $Path
}

Ensure-Repo "minhook" "https://github.com/TsudaKageyu/minhook.git" "master"
Ensure-Repo "imgui"  "https://github.com/ocornut/imgui.git" "master"

Write-Host ""
Write-Host "Dependencies ready in vendor/"
Write-Host "Build: cmake -B build -G `"Visual Studio 18 2026`" -A x64; cmake --build build --config Release"
