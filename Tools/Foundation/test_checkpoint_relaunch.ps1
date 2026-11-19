param(
    [Parameter(Mandatory)][string]$GameBinary,
    [Parameter(Mandatory)][string]$EvidenceDirectory
)
$ErrorActionPreference = 'Stop'
$exe = (Resolve-Path -LiteralPath $GameBinary).Path
$dest = [IO.Path]::GetFullPath($EvidenceDirectory)
if (Test-Path -LiteralPath $dest) { throw 'Choose a fresh test profile directory.' }
$null = New-Item -ItemType Directory -Path $dest
foreach ($mode in 'Write','Read') {
    $log = Join-Path $dest "$mode.log"
    $argsList = @('-windowed','-FoundationPlaytest','-nosplash',"-UserDir=`"$dest`"","-abslog=`"$log`"",
        "-ExecCmds=Foundation.${mode}RelaunchProbe")
    if ($mode -eq 'Write') { $argsList += '-ResX=1280','-ResY=720' }
    else { $argsList += '-ResX=1920','-ResY=1080' }
    $game = Start-Process -FilePath $exe -ArgumentList $argsList -WindowStyle Normal -PassThru
    $deadline = (Get-Date).AddSeconds(100)
    $marker = "FoundationRelaunch $mode (PASS|FAIL)"
    do {
        Start-Sleep -Milliseconds 400
        if (Test-Path -LiteralPath $log) { $result = Select-String -LiteralPath $log -Pattern $marker }
    } while (!$result -and !$game.HasExited -and (Get-Date) -lt $deadline)
    $game.Refresh()
    $timer = [Diagnostics.Stopwatch]::StartNew()
    if (!$game.CloseMainWindow() -or !$game.WaitForExit(90000)) { throw "$mode window close failed. PID $($game.Id)" }
    if (!$result -or $result.Line -notmatch 'PASS' -or $game.ExitCode -ne 0) { throw "$mode checkpoint check failed. See $log" }
    [pscustomobject]@{Operation=$mode; ExitSeconds=$timer.Elapsed.TotalSeconds; ExitCode=$game.ExitCode; Log=$log}
    $result = $null
}
