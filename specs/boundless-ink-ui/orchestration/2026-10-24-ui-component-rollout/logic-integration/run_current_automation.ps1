param([string]$RunName='automation-03')
$ErrorActionPreference='Stop'
if($RunName -notmatch '^automation-[0-9]+$'){throw 'Invalid bounded report name'}
$taskDir=$PSScriptRoot
$taskLease=Get-Content (Join-Path (Split-Path $taskDir) 'engine-lease.json') -Raw | ConvertFrom-Json
if($taskLease.owner_team -ne 'logic-integration' -or $taskLease.state -ne 'granted' -or [DateTimeOffset]::Parse($taskLease.expires_at) -le [DateTimeOffset]::Now){throw 'No active engine lease'}
$taskEditors=@(Get-Process -Name UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue)
if($taskEditors.Count){throw 'Another Editor is present; serial batch required'}
$taskArgs=@('D:/demo/BoundlessImmortality/BoundlessImmortality.uproject',
 '-ExecCmds=Automation RunTests Boundless.InkUI.Rollout20261024.Journey+BoundlessImmortality.Frontend.Rollout20261024.Core+BoundlessImmortality.Frontend.Inventory+BoundlessImmortality.Frontend.SignIn+BoundlessImmortality.Frontend.Leaderboard+Boundless.InkUI.Launch+BoundlessImmortality.Frontend.Refactor.SafeLayout',
 '-TestExit=Automation Test Queue Empty',"-ReportExportPath=$taskDir/$RunName",'-unattended','-NullRHI','-nosplash','-nop4','-NoSound','-NoLiveCoding',
 "-UserDir=$taskDir/engine-user-$RunName",'-SaveToUserDir',"-abslog=$taskDir/$RunName-engine.log",'-UTF8Output')
& 'F:/UNREAL/UE_5.8/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' @taskArgs 2>&1 | Tee-Object -FilePath "$taskDir/$RunName-console.log"
$taskCode=$LASTEXITCODE
@{exit_code=$taskCode;completed_at=(Get-Date -Format o);report="$RunName/index.json";scope='Current binary affected native suites plus existing responsive safe-layout test';user_dir="engine-user-$RunName"} | ConvertTo-Json | Set-Content "$taskDir/$RunName-result.json"
exit $taskCode
