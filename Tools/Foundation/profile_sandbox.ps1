param(
    [Parameter(Mandatory=$true)][string]$GameBinary,
    [Parameter(Mandatory=$true)][string]$EvidenceDirectory,
    [switch]$FullPopulation
)
$ErrorActionPreference = 'Stop'
$exe = (Resolve-Path -LiteralPath $GameBinary).Path
$dest = [IO.Path]::GetFullPath($EvidenceDirectory)
if (Test-Path -LiteralPath $dest) { throw 'Choose a new evidence directory.' }
if (Get-Process UnrealEditor,UnrealEditor-Cmd,AfghanistanFPS,AfghanistanFPS-Win64-Development -ErrorAction SilentlyContinue) {
    throw 'Close other Unreal processes before measuring.'
}
$null = New-Item -ItemType Directory -Path $dest
$result = @()
foreach ($height in 720,1080) {
    $width = if ($height -eq 720) { 1280 } else { 1920 }
    $profile = Join-Path $dest "profile-${height}p"
    $log = Join-Path $dest "${height}p.log"
    $cmds = if ($FullPopulation) { '-ExecCmds="Foundation.FillPopulation,csv.ForceExit 1"' } else { '-ExecCmds="csv.ForceExit 1"' }
    $process = Start-Process -FilePath $exe -ArgumentList @(
        '-d3d12', '-FoundationPlaytest', '-windowed', "-ResX=$width", "-ResY=$height",
        "-UserDir=`"$profile`"", "-abslog=`"$log`"", '-csvCaptureFrames=720',
        '-csvCompression=0', '-ExitAfterCsvProfiling', $cmds, '-NoSplash', '-unattended'
    ) -PassThru -WindowStyle Normal
    try {
        $deadline = (Get-Date).AddSeconds(90)
        $peakSampleMiB = 0
        $freeGpu = @()
        $gpuTool = Get-Command nvidia-smi -ErrorAction SilentlyContinue
        $csv = $null
        do {
            Start-Sleep -Milliseconds 500
            $process.Refresh()
            if (-not $process.HasExited) {
                if ($FullPopulation -and $gpuTool) {
                    $reading = & $gpuTool.Source --id=0 --query-gpu=memory.free --format=csv,noheader,nounits
                    if ($LASTEXITCODE -eq 0 -and $reading -match '^\s*\d+\s*$') { $freeGpu += [int]$reading }
                }
                $peakSampleMiB = [Math]::Max($peakSampleMiB, $process.WorkingSet64 / 1MB)
            }
            $csv = Get-ChildItem -LiteralPath $profile -Recurse -Filter '*.csv' -File -ErrorAction SilentlyContinue |
                Where-Object { $_.DirectoryName -like '*Profiling*' } | Select-Object -First 1
            $done = $csv -and (Select-String -Path $log -SimpleMatch 'CSV finalize time' -Quiet)
            if ($process.HasExited -and -not $done) { throw "Game exited before capture: $log" }
        } until ($done -or (Get-Date) -ge $deadline)
        if (-not $done) { throw "Timed out waiting for CSV capture: $log" }
        $launch = Get-Content -LiteralPath $log -Raw
        if ($launch -notmatch 'Bringing World /Game/AfghanFPS/TestMaps/L_Test_PlayerFoundation') {
            throw "Sandbox map load was not recorded: $log"
        }
        if ($launch -notmatch '(?i)LogD3D12RHI.*(feature level|created|adapter)') {
            throw "D3D12 startup was not recorded: $log"
        }
        if ($FullPopulation -and $launch -notmatch 'FoundationPopulation NPCs=10 possessed=10') { throw "Population fixture not confirmed: $log" }
        $result += [pscustomobject]@{
            Width = $width; Height = $height
            PeakSampledWorkingSetMiB = [Math]::Round($peakSampleMiB)
            MinimumSampledFreeVramMiB = ($freeGpu | Measure-Object -Minimum).Minimum
            Log = $log; FrameCapture = $csv.FullName
        }
        $result | ConvertTo-Json -AsArray | Set-Content -Encoding utf8 (Join-Path $dest 'captures.json')
    } finally {
        if (-not $process.WaitForExit(15000)) {
            throw "Close the test instance (PID $($process.Id)) before retrying."
        }
    }
}
