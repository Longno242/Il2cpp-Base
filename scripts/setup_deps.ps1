$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$Vendor = Join-Path $Root "vendor"
New-Item -ItemType Directory -Force -Path $Vendor | Out-Null

# Pin releases so CI and local builds stay reproducible.
# Bump these when you want a dependency update.
$MinHookRef = "v1.3.4"
$ImGuiRef   = "v1.92.9b"

function Ensure-Repo($Name, $Url, $Ref) {
    $Path = Join-Path $Vendor $Name
    if (Test-Path (Join-Path $Path ".git")) {
        $Current = (git -C $Path describe --tags --always --dirty 2>$null)
        if ($Current -and ($Current -eq $Ref -or $Current.StartsWith("$Ref-"))) {
            Write-Host "[ok] $Name already at $Ref"
            return
        }
        Write-Host "[..] Updating $Name ($Current -> $Ref)..."
        Remove-Item -Recurse -Force $Path
    }
    Write-Host "[..] Cloning $Name ($Ref)..."
    git clone --depth 1 --branch $Ref $Url $Path
    Write-Host "[ok] $Name @ $Ref"
}

Ensure-Repo "minhook" "https://github.com/TsudaKageyu/minhook.git" $MinHookRef
Ensure-Repo "imgui"  "https://github.com/ocornut/imgui.git" $ImGuiRef

Write-Host ""
Write-Host "Dependencies ready in vendor/"
Write-Host "Configure: .\scripts\configure.ps1"
Write-Host "Build:     cmake --build build --config Release"
