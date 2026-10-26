param([Parameter(Mandatory=$true)][string]$Attempt)
$ErrorActionPreference='Stop'
$taskHere=$PSScriptRoot
$taskProcesses=Get-CimInstance Win32_Process | Where-Object { $_.Name -match '^(UnrealEditor.*|LiveCodingConsole|UnrealBuildTool|cl|link)\.exe$' }
if($taskProcesses){$taskProcesses | Select-Object ProcessId,ParentProcessId,Name,CommandLine | ConvertTo-Json -Depth 3;throw 'Engine/compiler owner conflict'}
& 'C:/Users/zxxuh/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' "$taskHere/verify_source_epoch.py"
if($LASTEXITCODE -ne 0){throw 'Source epoch failed'}
$taskEpoch=Get-Content "$taskHere/source-epoch.json" -Raw | ConvertFrom-Json
$taskLedger=Get-Content 'D:/demo/BoundlessImmortality/specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-integration/visible-source-integration/source-change-ledger.json' -Raw | ConvertFrom-Json
$taskStarted=Get-Date
& 'F:/UNREAL/UE_5.8/UE_5.8/Engine/Build/BatchFiles/Build.bat' BoundlessImmortalityEditor Win64 Development '-Project=D:/demo/BoundlessImmortality/BoundlessImmortality.uproject' -WaitMutex -NoHotReloadFromIDE -NoXGE -NoUBA -DisableUnity -MaxParallelActions=1 *> "$taskHere/build-$Attempt.log"
$taskBuildExit=$LASTEXITCODE
[ordered]@{scope='Current bounded functional repair; explicit DisableUnity due pre-existing merged-helper collisions; default unity failure retained';source_files=$taskEpoch.files.Count;source_ledger_records=$taskLedger.changes.Count;started_at=$taskStarted.ToString('o');finished_at=(Get-Date).ToString('o');exit_code=$taskBuildExit} | ConvertTo-Json | Set-Content "$taskHere/build-$Attempt-result.json" -Encoding UTF8
Get-Content "$taskHere/build-$Attempt.log" -Tail 38
exit $taskBuildExit
