"""Temporarily disable package aliases so AssetTools can inspect actual redirectors."""
from pathlib import Path
import re,subprocess
ROOT=Path(__file__).resolve().parents[4];REV=Path(__file__).parent;OUT=ROOT/'Saved/Codex/PortraitUI20261023'
config=ROOT/'Config/DefaultEngine.ini';original=config.read_bytes()
(OUT/'DefaultEngine-before-redirect-cleanup.ini').write_bytes(original)
try:
 text=original.decode('utf-8-sig')
 text,count=re.subn(r'; BEGIN PortraitUI20261023 legacy UI package aliases.*?; END PortraitUI20261023 legacy UI package aliases\n?','',text,flags=re.S)
 assert count==1
 config.write_text(text,encoding='utf-8')
 with (OUT/'migration-cleanup-console.log').open('wb') as log:
  result=subprocess.run(['F:/UNREAL/UE_5.8/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe',str(ROOT/'BoundlessImmortality.uproject'),'-run=pythonscript','-script='+str(REV/'reconcile_meter_copy_ue.py'),'-PortraitCleanupOnly','-PortraitAuditOnly','-unattended','-nop4','-nosplash','-nullrhi','-abslog='+str(OUT/'migration-cleanup.log')],stdout=log,stderr=subprocess.STDOUT)
finally:
 config.write_bytes(original)
assert result.returncode==0,result.returncode
