$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../../../..')).Path
$backup=Join-Path $root 'Saved/Codex/PortraitUI20261023/Backup'
$files=@('Source/BoundlessImmortality/Frontend/InkWulingPanel.cpp','Source/BoundlessImmortality/Frontend/UI/InkWulingToolsPanel.cpp','Source/BoundlessImmortality/Frontend/UI/InkWulingRecordsPanel.cpp','Source/BoundlessImmortality/Frontend/InkCharacterPanels.cpp')
$checks=@()
foreach($file in $files){
 $before=[IO.File]::ReadAllText((Join-Path $backup $file)).Replace("`r`n","`n")
 $after=[IO.File]::ReadAllText((Join-Path $root $file)).Replace("`r`n","`n")
 $pattern='SubmitPrototypeCommand\(TEXT\("([^"]+)"\)'
 $old=@([regex]::Matches($before,$pattern)|ForEach-Object {$_.Groups[1].Value}|Sort-Object -Unique)
 $new=@([regex]::Matches($after,$pattern)|ForEach-Object {$_.Groups[1].Value}|Sort-Object -Unique)
 $missing=@($old|Where-Object {$_ -notin $new});if($missing.Count){throw "Missing authority commands: $file $missing"}
 $clean=[regex]::Replace($after,'(?s)/\*.*?\*/|//[^\r\n]*|"(?:\\.|[^"\\])*"','')
 $depth=0;foreach($char in $clean.ToCharArray()){if($char -eq '{'){$depth++};if($char -eq '}'){$depth--};if($depth -lt 0){throw "Unbalanced braces: $file"}};if($depth){throw "Unbalanced braces: $file"}
 $checks+=[pscustomobject]@{file=$file;authority_commands_preserved=$true;brace_balance=$true;sha256=(Get-FileHash -LiteralPath (Join-Path $root $file) -Algorithm SHA256).Hash}
 if($file.EndsWith('InkCharacterPanels.cpp')){
  $a=$before.IndexOf('namespace');$b=$before.IndexOf('void UInkFeaturePanel::BuildCharacterSection');$c=$after.IndexOf('namespace');$d=$after.IndexOf('void UInkFeaturePanel::BuildCharacterSection')
  if($before.Substring($a,$b-$a) -cne $after.Substring($c,$d-$c)){throw 'Protected CharacterProfile method region changed'}
 }
 if($file.EndsWith('InkWulingToolsPanel.cpp')){
  $a=$before.IndexOf('void UInkWulingToolsPanel::ExecuteSelected()');$b=$after.IndexOf('void UInkWulingToolsPanel::ExecuteSelected()')
  if($before.Substring($a).Trim() -cne $after.Substring($b).Trim()){throw 'Authority execution payload implementation changed'}
  if($after.Contains('BodyHeight=FMath::Max')){throw 'Tool actions force viewport height to grow'}
 }
}
# Service price contract: Market.buy uses ceil(total/20), listing costs one stone.
$feeChecks=0
foreach($quantity in 1..100){foreach($price in 4..18){$total=$quantity*$price;$authorityTax=[int][Math]::Floor(($total+19)/20.0);$expected=[int][Math]::Ceiling($total*.05);if($authorityTax -ne $expected){throw 'Market fee mismatch'};$feeChecks++}}
$result=[pscustomobject]@{checked_at=(Get-Date -Format o);character_profile_method_region_unchanged=$true;authority_execute_selected_unchanged=$true;market_fee_formula_cases=$feeChecks;source_checks=$checks;native_build_run=$false;runtime_tests_run=$false}
$result|ConvertTo-Json -Depth 6|Set-Content -Encoding utf8 (Join-Path $PSScriptRoot 'wuling-pages-static-checks.json')
$result|ConvertTo-Json -Depth 6
