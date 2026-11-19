param(
    [Parameter(Mandatory=$true)][string]$GameBinary,
    [Parameter(Mandatory=$true)][string]$EvidenceDirectory,
    [switch]$NoAudio,
    [switch]$DisableRuntimeTelemetry,
    [switch]$CloseWindow,
    [switch]$SingleThreadDiagnostic,
    [switch]$NoRenderThreadDiagnostic,
    [switch]$VerifyNavigation,
    [switch]$MixedSoak,
    [switch]$CaptureSoakFrames,
    [switch]$VerifyTitle,
    [switch]$TraceCpu,
    [ValidateSet(720,1080)][int]$Height=720,
    [ValidateRange(5,90)][int]$ExitTimeoutSeconds = 35,
    [ValidateSet('d3d12','d3d11','nullrhi')][string]$Renderer = 'd3d12'
)
$ErrorActionPreference = 'Stop'
$exe = (Resolve-Path -LiteralPath $GameBinary).Path
$dest = [IO.Path]::GetFullPath($EvidenceDirectory)
$probes = @($MixedSoak,$VerifyNavigation,$VerifyTitle) | Where-Object { $_ }
if ($probes -and !$CloseWindow) { throw 'Gameplay probes require -CloseWindow.' }
if (@($probes).Count -gt 1) { throw 'Select one gameplay probe per run.' }
if ($CaptureSoakFrames -and !$MixedSoak) { throw '-CaptureSoakFrames requires -MixedSoak.' }
if (Test-Path -LiteralPath $dest) { throw 'Choose a new evidence directory.' }
if (Get-Process UnrealEditor,UnrealEditor-Cmd,AfghanistanFPS -ErrorAction SilentlyContinue) {
    throw 'Close other Unreal processes before testing.'
}
$null = New-Item -ItemType Directory -Path $dest
$log = Join-Path $dest 'normal-exit.log'
$launch = @(
    "-$Renderer", '-windowed', "-ResX=$(if ($Height -eq 720) {1280} else {1920})", "-ResY=$Height",
    "-UserDir=`"$dest`"", "-abslog=`"$log`"", '-NoSplash'
)
if (!$VerifyTitle) { $launch += '-FoundationPlaytest' }
if ($TraceCpu) { $launch += '-trace=cpu,bookmark','-statnamedevents',"-tracefile=`"$dest/game.utrace`"" }
if (!$CloseWindow) { $launch += @('-csvCaptureFrames=600', '-ExitAfterCsvProfiling') }
if ($VerifyNavigation -and !$CloseWindow) { throw '-VerifyNavigation requires -CloseWindow to observe the full patrol sample.' }
if ($NoAudio) { $launch += '-nosound' }
if ($DisableRuntimeTelemetry) { $launch += '-DisablePlugins=RuntimeTelemetry' }
if ($SingleThreadDiagnostic) { $launch += '-onethread' }
if ($NoRenderThreadDiagnostic) { $launch += '-norenderthread' }
if ($VerifyNavigation) { $launch += '-ExecCmds=Foundation.VerifyNavigation' }
if ($MixedSoak) { $launch += '-ExecCmds=Foundation.MixedSoak' }
if ($CaptureSoakFrames) { $launch += '-csvCaptureFrames=3600','-csvCompression=0' }
if ($VerifyTitle) { $launch += '-ExecCmds=Foundation.VerifyTitleFlow' }
$game = Start-Process -FilePath $exe -ArgumentList $launch -WindowStyle Normal -PassThru
# Give the sandbox time to load and capture separately from the exit timeout.
$startupDeadline = (Get-Date).AddSeconds(90)
$sawExitRequest = $false
$navigation = $null
$soak = $null
$titleFlow = $null
$memorySamples = [Collections.Generic.List[object]]::new()
$exitClock = $null
$marker = if ($CloseWindow) {
    'Bringing World /Game/AfghanFPS/TestMaps/L_Test_PlayerFoundation'
} else {
    'CsvProfiler.ExitAfterCsvProfiling'
}
while (!$game.HasExited -and (Get-Date) -lt $startupDeadline) {
    if ((Test-Path -LiteralPath $log) -and (Select-String -LiteralPath $log -SimpleMatch $marker -Quiet)) {
        if ($CloseWindow) {
            if ($VerifyNavigation) {
                $navDeadline = (Get-Date).AddSeconds(25)
                while (!$game.HasExited -and (Get-Date) -lt $navDeadline) {
                    $navLine = Select-String -LiteralPath $log -Pattern 'FoundationNavSmoke (PASS|FAIL):' | Select-Object -Last 1
                    if ($navLine) {
                        $navigation = $navLine.Line
                        break
                    }
                    Start-Sleep -Milliseconds 400
                }
            } elseif ($MixedSoak) {
                $soakDeadline = (Get-Date).AddMinutes(13)
                while (!$game.HasExited -and (Get-Date) -lt $soakDeadline) {
                    $game.Refresh()
                    if (!$game.HasExited) {
                        $memorySamples.Add([pscustomobject]@{
                            Seconds=[Math]::Round(((Get-Date)-$game.StartTime).TotalSeconds,2)
                            WorkingSetMiB=[Math]::Round($game.WorkingSet64/1MB,1)
                        })
                    }
                    $line = Select-String -LiteralPath $log -Pattern 'FoundationMixedSoak (PASS|FAIL)' | Select-Object -Last 1
                    if ($line) { $soak = $line.Line; break }
                    Start-Sleep -Seconds 1
                }
            } elseif ($VerifyTitle) {
                $titleDeadline = (Get-Date).AddSeconds(40)
                while (!$game.HasExited -and (Get-Date) -lt $titleDeadline) {
                    $line = Select-String -LiteralPath $log -Pattern 'FoundationTitleFlow (PASS|FAIL)' | Select-Object -Last 1
                    if ($line) { $titleFlow = $line.Line; break }
                    Start-Sleep -Milliseconds 400
                }
            } else { Start-Sleep -Seconds 8 }
            $game.Refresh()
            $exitClock = [Diagnostics.Stopwatch]::StartNew()
            $sawExitRequest = !$game.HasExited -and $game.CloseMainWindow()
        } else {
            $exitClock = [Diagnostics.Stopwatch]::StartNew()
            $sawExitRequest = $true
        }
        break
    }
    Start-Sleep -Milliseconds 250
}
$exited = $game.HasExited -or ($sawExitRequest -and $game.WaitForExit($ExitTimeoutSeconds * 1000))
if ($exitClock) { $exitClock.Stop() }
$result = [pscustomobject]@{
    Renderer = $Renderer; RequestedExit = $sawExitRequest; Exited = $exited; Log = $log; PID = $game.Id;
    ExitCode = if ($exited) { $game.ExitCode } else { $null }
    ExitSeconds = if ($exitClock) { [Math]::Round($exitClock.Elapsed.TotalSeconds, 3) } else { $null }
    NavigationVerified = if ($VerifyNavigation) { $navigation -match 'FoundationNavSmoke PASS:' } else { $null }
    NavigationLog = $navigation
    MixedSoakLog = $soak
    TitleFlowLog = $titleFlow
}
$result | ConvertTo-Json | Set-Content -Encoding utf8 (Join-Path $dest 'result.json')
if ($memorySamples.Count) {
    $memorySamples | Export-Csv -LiteralPath (Join-Path $dest 'process-memory.csv') -NoTypeInformation
}
$result
if (!$exited) {
    if (!$sawExitRequest) { throw "Sandbox startup and capture timed out (PID $($game.Id))." }
    throw "Normal exit timed out. Close the sandbox test process (PID $($game.Id))."
}
if ($result.ExitCode -ne 0 -or !$sawExitRequest) {
    throw "Packaged sandbox exited with code $($result.ExitCode). See $log."
}
if ($VerifyNavigation -and !$result.NavigationVerified) { throw "Packaged navigation smoke failed. See $log." }
if ($MixedSoak -and $soak -notmatch 'FoundationMixedSoak PASS') { throw "Packaged mixed soak failed. See $log." }
if ($VerifyTitle -and $titleFlow -notmatch 'FoundationTitleFlow PASS') { throw "Packaged title flow failed. See $log." }
