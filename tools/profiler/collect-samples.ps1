# Sampling collector for a running DayZServer_x64.exe.
#
# It answers one question and nothing else: where is the server's time going,
# per thread. It suspends each thread for a few microseconds, reads RIP, and
# writes the raw counts. It does NOT try to name anything -- names come from
# the executable's .pdata, which is resolved off-box by resolve-samples.py.
#
# Why raw counts: the machine running the server does not need Python, a
# symbol server, or a debugger attached, and nothing about the game process is
# modified. Take the CSV it writes and hand it back for resolving.
#
#   .\collect-samples.ps1                          # 30 s at 200 Hz -> von-samples.csv
#   .\collect-samples.ps1 -Seconds 60 -Hz 200
#   .\collect-samples.ps1 -ProcessIdToSample 1234 -Out C:\temp\lag.csv
#
# Run it WHILE the lag is happening. Thirty seconds of a bad moment is worth
# more than ten minutes of a good one.

[CmdletBinding()]
param(
    [int]$ProcessIdToSample = 0,
    [double]$Seconds = 30,
    [double]$Hz = 200,
    [string]$Out = "von-samples.csv"
)

$ErrorActionPreference = 'Stop'

Add-Type -Namespace Win -Name Api -MemberDefinition @'
[DllImport("kernel32.dll", SetLastError=true)]
public static extern IntPtr OpenThread(uint access, bool inherit, uint tid);
[DllImport("kernel32.dll", SetLastError=true)]
public static extern uint SuspendThread(IntPtr h);
[DllImport("kernel32.dll", SetLastError=true)]
public static extern int ResumeThread(IntPtr h);
[DllImport("kernel32.dll", SetLastError=true)]
public static extern bool GetThreadContext(IntPtr h, IntPtr ctx);
[DllImport("kernel32.dll", SetLastError=true)]
public static extern bool CloseHandle(IntPtr h);
'@

# THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION
$ACCESS = 0x0002 -bor 0x0008 -bor 0x0040
# CONTEXT_AMD64 | CONTEXT_CONTROL
$CONTEXT_CONTROL = 0x00100001
$CONTEXT_SIZE = 1232
$RIP_OFFSET = 0xF8

if ($ProcessIdToSample -eq 0) {
    $p = Get-CimInstance Win32_Process -Filter "Name='DayZServer_x64.exe'" | Select-Object -First 1
    if (-not $p) { throw "DayZServer_x64.exe is not running; pass -ProcessIdToSample" }
    $ProcessIdToSample = [int]$p.ProcessId
}

$proc = Get-Process -Id $ProcessIdToSample
$mainModule = $proc.Modules | Where-Object { $_.ModuleName -eq 'DayZServer_x64.exe' } | Select-Object -First 1
if (-not $mainModule) { throw "cannot read the modules of pid $ProcessIdToSample (run as the same user, or elevated)" }
$exeBase = [uint64]$mainModule.BaseAddress.ToInt64()

# Every loaded module, so a sample inside ntdll or hid.dll is attributed
# instead of being counted as the executable.
$mods = @()
foreach ($m in $proc.Modules) {
    $mods += [pscustomobject]@{
        Name  = $m.ModuleName
        Start = [uint64]$m.BaseAddress.ToInt64()
        End   = [uint64]$m.BaseAddress.ToInt64() + [uint64]$m.ModuleMemorySize
    }
}
$mods = $mods | Sort-Object Start

Write-Host ("pid {0}, DayZServer_x64.exe at 0x{1:X}, {2} modules" -f $ProcessIdToSample, $exeBase, $mods.Count)

# CONTEXT must be 16-byte aligned.
$raw = [System.Runtime.InteropServices.Marshal]::AllocHGlobal($CONTEXT_SIZE + 16)
$ctx = [IntPtr](([int64]$raw + 15) -band -16)

$handles = @{}
$counts = @{}
$deadline = (Get-Date).AddSeconds($Seconds)
$period = 1.0 / $Hz
$taken = 0
$missed = 0

while ((Get-Date) -lt $deadline) {
    $tick = Get-Date
    $proc.Refresh()
    foreach ($t in $proc.Threads) {
        $tid = [uint32]$t.Id
        if (-not $handles.ContainsKey($tid)) {
            $handles[$tid] = [Win.Api]::OpenThread($ACCESS, $false, $tid)
        }
        $h = $handles[$tid]
        if ($h -eq [IntPtr]::Zero) { continue }

        if ([Win.Api]::SuspendThread($h) -eq [uint32]::MaxValue) { $missed++; continue }
        try {
            [System.Runtime.InteropServices.Marshal]::WriteInt32($ctx, 0x30, $CONTEXT_CONTROL)
            if (-not [Win.Api]::GetThreadContext($h, $ctx)) { $missed++; continue }
            $rip = [uint64][System.Runtime.InteropServices.Marshal]::ReadInt64($ctx, $RIP_OFFSET)
        }
        finally { [void][Win.Api]::ResumeThread($h) }

        $where = '?'
        foreach ($m in $mods) {
            if ($rip -ge $m.Start -and $rip -lt $m.End) { $where = $m.Name; break }
        }
        if ($where -eq 'DayZServer_x64.exe') {
            $key = "{0},exe,0x{1:X}" -f $tid, ($rip - $exeBase)
        } else {
            $key = "{0},module,{1}" -f $tid, $where
        }
        if ($counts.ContainsKey($key)) { $counts[$key]++ } else { $counts[$key] = 1 }
        $taken++
    }
    $spent = ((Get-Date) - $tick).TotalSeconds
    if ($spent -lt $period) { Start-Sleep -Milliseconds ([int](($period - $spent) * 1000)) }
}

foreach ($h in $handles.Values) { if ($h -ne [IntPtr]::Zero) { [void][Win.Api]::CloseHandle($h) } }
[System.Runtime.InteropServices.Marshal]::FreeHGlobal($raw)

$lines = @()
$lines += "# dayz von sampler"
$lines += "# exe_base=0x{0:X} pid={1} seconds={2} hz={3} samples={4} missed={5}" -f $exeBase, $ProcessIdToSample, $Seconds, $Hz, $taken, $missed
$lines += "tid,kind,where,count"
foreach ($k in ($counts.Keys | Sort-Object { -$counts[$_] })) {
    $lines += "{0},{1}" -f $k, $counts[$k]
}
$lines | Set-Content -Path $Out -Encoding UTF8

Write-Host ("{0} samples ({1} missed) -> {2}" -f $taken, $missed, $Out)
