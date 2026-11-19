param(
    [Parameter(Mandatory=$true)][string]$ArchiveDirectory,
    [string]$EngineRoot = 'C:/Program Files/Epic Games/UE_5.8',
    [switch]$UseExistingCook
)
$ErrorActionPreference = 'Stop'
$project = (Resolve-Path (Join-Path $PSScriptRoot '../../AfghanistanFPS.uproject')).Path
$runUAT = Join-Path $EngineRoot 'Engine/Build/BatchFiles/RunUAT.bat'
if (Get-Process UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue) {
    throw 'Close Unreal Editor before starting the cook on this workstation.'
}
if (Test-Path -LiteralPath $ArchiveDirectory) {
    throw 'Choose a new archive directory to keep existing artifacts intact.'
}
$cook = if ($UseExistingCook) { '-skipcook' } else { '-cook' }
& $runUAT BuildCookRun "-project=$project" -noP4 -unattended -platform=Win64 -clientconfig=Development -build $cook '-map=/Game/AfghanFPS/TestMaps/L_Test_PlayerFoundation' -stage -pak -iostore -package -CrashReporter -archive "-archivedirectory=$ArchiveDirectory" -utf8output
$result = $LASTEXITCODE
if ($result -eq 0) {
    $notices = Join-Path (Split-Path -Parent $project) 'ThirdPartyNotices'
    Copy-Item -LiteralPath $notices -Destination (Join-Path $ArchiveDirectory 'ThirdPartyNotices') -Recurse
}
exit $result
