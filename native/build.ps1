# Build the OpenZone frequency proxy (hid.dll) with the Visual Studio toolchain.
#
#   .\build.ps1                          # build into .\build
#   .\build.ps1 -Deploy -TargetDir <dir> # build, then install beside a server
#   .\build.ps1 -Undeploy -TargetDir <dir>   # remove it again
#
# cl.exe is not on PATH by default, so this finds the installation with vswhere
# and imports the x64 environment itself.
#
# On deployment: the proxy belongs beside a DayZ SERVER executable. Deploying it
# into a directory that also holds the retail client is refused, because the
# client loads it too -- it patches nothing there thanks to the -server gate, but
# BattlEye blocks the load and says so in the launcher, and an unsigned DLL has
# no business sitting in a directory someone plays retail DayZ from. Diag stands
# share one directory for both, so -Force exists for exactly that case; run
# -Undeploy when the test session ends.

param(
    [switch]$Deploy,
    [switch]$Undeploy,
    [switch]$Force,
    [string]$TargetDir = 'E:\Programs\Steam\steamapps\common\DayZ'
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$out  = Join-Path $root 'build'

$deployed = @('hid.dll', 'oz_frequencies.json', 'oz_frequencies.log')

# Only a process running FROM the target directory can hold our file open.
# Checking for "any DayZ anywhere" would block on someone else's server, which
# is not ours to stop and cannot be locking anything here.
function Assert-NothingRunning([string]$dir) {
    $full = (Resolve-Path $dir -ErrorAction SilentlyContinue).Path
    if (-not $full) { return }

    $here = Get-Process -Name 'DayZDiag_x64', 'DayZServer_x64', 'DayZ_x64' -ErrorAction SilentlyContinue |
        Where-Object {
            $p = $null
            try { $p = $_.Path } catch { }        # access can be denied; then it is not ours to judge
            $p -and ([System.IO.Path]::GetDirectoryName($p) -eq $full)
        }

    if ($here) {
        throw "DayZ is running from $full (pid $($here.Id -join ', ')) - stop it first, a loaded DLL cannot be replaced or removed"
    }
}

# --- undeploy runs on its own and never needs the toolchain ----------------

if ($Undeploy) {
    Assert-NothingRunning $TargetDir
    $removed = @()
    foreach ($name in $deployed) {
        $p = Join-Path $TargetDir $name
        if (Test-Path $p) { Remove-Item $p -Force; $removed += $name }
    }
    if ($removed) { Write-Output "removed from ${TargetDir}: $($removed -join ', ')" }
    else          { Write-Output "nothing of ours was in $TargetDir" }
    return
}

# --- locate the toolchain -------------------------------------------------

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) {
    throw "vswhere.exe not found at $vswhere - is Visual Studio installed?"
}

$vsPath = & $vswhere -latest -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath
if (-not $vsPath) {
    throw 'No Visual Studio installation with the C++ x64 tools was found.'
}

$vcvars = Join-Path $vsPath 'VC\Auxiliary\Build\vcvars64.bat'
if (-not (Test-Path $vcvars)) {
    throw "vcvars64.bat not found at $vcvars"
}

# --- import its environment into this session -----------------------------

# cmd sets dozens of variables; capture them all rather than guessing which
# ones cl.exe needs.
& "${env:COMSPEC}" /c "`"$vcvars`" >nul 2>&1 && set" | ForEach-Object {
    if ($_ -match '^([^=]+)=(.*)$') {
        Set-Item -Path "env:$($matches[1])" -Value $matches[2] -ErrorAction SilentlyContinue
    }
}

# --- build ----------------------------------------------------------------

New-Item -ItemType Directory -Force -Path $out | Out-Null
Push-Location $out
try {
    $sources = @(
        (Join-Path $root 'src\dllmain.cpp'),
        (Join-Path $root 'src\patch.cpp')
    )

    # /MT so the DLL carries its own CRT: it loads before anything else in the
    # process and must not depend on a redistributable being present.
    & cl.exe /nologo /std:c++17 /O2 /MT /W4 /EHsc /LD `
        "/I$(Join-Path $root 'src')" `
        $sources `
        /Fe:hid.dll `
        /link /OUT:hid.dll
    if ($LASTEXITCODE -ne 0) { throw "cl.exe failed with exit code $LASTEXITCODE" }
}
finally {
    Pop-Location
}

$dll = Join-Path $out 'hid.dll'
Write-Output "built: $dll ($((Get-Item $dll).Length) bytes)"

# --- deploy ---------------------------------------------------------------

if ($Deploy) {
    if (-not (Test-Path $TargetDir)) { throw "target directory not found: $TargetDir" }
    Assert-NothingRunning $TargetDir

    # A retail client in the same directory is the one case worth refusing.
    $client = @('DayZ_x64.exe', 'DayZ_BE.exe') |
        Where-Object { Test-Path (Join-Path $TargetDir $_) }
    if ($client -and -not $Force) {
        throw @"
$TargetDir holds a retail client ($($client -join ', ')).

The client loads this DLL too. The -server gate stops it patching anything
there, but BattlEye blocks the load and reports it in the launcher, and an
unsigned DLL does not belong in a directory someone plays retail DayZ from.

A DayZ Server install has no client in it and is the right target. If this IS
a Diag stand where both share one directory, pass -Force -- and run
    .\build.ps1 -Undeploy -TargetDir '$TargetDir'
when the test session ends.
"@
    }

    Copy-Item $dll (Join-Path $TargetDir 'hid.dll') -Force
    Write-Output "deployed: $(Join-Path $TargetDir 'hid.dll')"

    $cfgSrc = Join-Path $root 'oz_frequencies.json'
    $cfgDst = Join-Path $TargetDir 'oz_frequencies.json'
    if (Test-Path $cfgDst) {
        Write-Output "config already present, left alone: $cfgDst"
    } else {
        Copy-Item $cfgSrc $cfgDst
        Write-Output "config installed: $cfgDst"
    }

    Write-Output ''
    Write-Output 'The proxy patches only a process launched with -server.'
    Write-Output "It reports what it did in $(Join-Path $TargetDir 'oz_frequencies.log')."
    if ($client) {
        Write-Output ''
        Write-Warning "a retail client shares this directory - run -Undeploy when you are done testing"
    }
}
