$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$globalOutput = Join-Path $root "output"
$summaryFile = Join-Path $globalOutput "summary_$timestamp.txt"

$modules = @(
    @{ Id = "01"; Name = "CPU Scheduling" },
    @{ Id = "02"; Name = "Memory Management" },
    @{ Id = "03"; Name = "Process Sync" },
    @{ Id = "04"; Name = "File System" }
)

New-Item -ItemType Directory -Force -Path $globalOutput | Out-Null

function Write-Summary {
    param([string]$Text)
    $Text | Tee-Object -FilePath $summaryFile -Append
}

function Get-Compiler {
    $candidates = @("gcc", "clang", "cl")
    foreach ($name in $candidates) {
        $cmd = Get-Command $name -ErrorAction SilentlyContinue
        if ($cmd) {
            return $cmd.Name
        }
    }
    return $null
}

Write-Summary "OS course project bootstrap check"
Write-Summary "Time: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
Write-Summary ""

$compiler = Get-Compiler
if ($compiler) {
    Write-Summary "Compiler detected: $compiler"
} else {
    Write-Summary "No C compiler detected (gcc / clang / cl)."
    Write-Summary "This script will only validate directories at the current stage."
}

Write-Summary ""
Write-Summary "Module check:"

foreach ($module in $modules) {
    $moduleDir = Get-ChildItem -Path $root -Directory | Where-Object { $_.Name -like "$($module.Id)_*" } | Select-Object -First 1
    if (-not $moduleDir) {
        Write-Summary "[Missing] module $($module.Id) ($($module.Name))"
        continue
    }

    $moduleRoot = $moduleDir.FullName
    $moduleOutput = Join-Path $moduleRoot "output"
    $srcDir = Join-Path $moduleRoot "src"
    $readme = Join-Path $moduleRoot "README.md"

    New-Item -ItemType Directory -Force -Path $moduleOutput | Out-Null

    $status = @()
    if (Test-Path $srcDir) { $status += "src:OK" } else { $status += "src:Missing" }
    if (Test-Path $readme) { $status += "README:OK" } else { $status += "README:Missing" }

    Write-Summary "[OK] $($moduleDir.Name) -> $($status -join ', ')"
}

Write-Summary ""
Write-Summary "Notes:"
Write-Summary "1. The project skeleton is ready."
Write-Summary "2. Implement and test C programs inside WSL/Linux."
Write-Summary "3. Aggregated output files are stored in the root output directory."
Write-Summary ""
Write-Summary "Summary file: $summaryFile"
