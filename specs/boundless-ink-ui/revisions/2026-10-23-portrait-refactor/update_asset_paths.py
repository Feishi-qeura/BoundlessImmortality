"""Run after successful UE migration; update native loads and legacy package aliases."""
from pathlib import Path
import json,re
ROOT=Path(__file__).resolve().parents[4]
REV=Path(__file__).parent
result=json.loads((ROOT/'Saved/Codex/PortraitUI20261023/migration-result.json').read_text('utf-8'))
assert result['passed']
rows=json.loads((REV/'migration-manifest.json').read_text('utf-8'))['assets']
mapping={r['old_package']:r['proposed_package'] for r in rows if r['old_package']!=r['proposed_package']}
ui=ROOT/'Source/BoundlessImmortality/Frontend/UI'
(ui/'InkUiAssetPaths.h').write_text('''#pragma once
#include "CoreMinimal.h"
#include "UObject/UObjectGlobals.h"

// Legacy dynamically assembled UI paths resolve to the canonical typed folders.
namespace InkUiAssets
{
    BOUNDLESSIMMORTALITY_API FString Resolve(const TCHAR* Path);
    template<class T,class... Args> T* Load(UObject* Outer,const TCHAR* Path,Args&&... Arguments)
    {const FString Canonical=Resolve(Path);return LoadObject<T>(Outer,*Canonical,Forward<Args>(Arguments)...);}
}
''',encoding='utf-8')
entries='\n'.join('        {TEXT("'+old+'"),TEXT("'+new+'")},' for old,new in sorted(mapping.items()))
(ui/'InkUiAssetPaths.cpp').write_text('''#include "InkUiAssetPaths.h"
FString InkUiAssets::Resolve(const TCHAR* Path)
{
    FString Value(Path?Path:TEXT(""));
    if(!Value.StartsWith(TEXT("/Game/UI/")))return Value;
    static const TMap<FString,FString> Paths={
'''+entries+'''
    };
    int32 Dot=INDEX_NONE;Value.FindChar(TEXT('.'),Dot);
    const FString Package=Dot==INDEX_NONE?Value:Value.Left(Dot);
    if(const FString* Target=Paths.Find(Package))return *Target+(Dot==INDEX_NONE?FString():Value.Mid(Dot));
    return Value;
}
''',encoding='utf-8')
changed=[]
for base in ['Source','Config']:
 for p in (ROOT/base).rglob('*'):
  if not p.is_file() or p.suffix not in ['.h','.cpp','.ini','.cs']:continue
  if p.name in ['InkUiAssetPaths.cpp','InkUiAssetPaths.h']:continue
  s=p.read_text('utf-8-sig');old=s
  for before,after in sorted(mapping.items(),key=lambda pair:len(pair[0]),reverse=True):
   s=re.sub(re.escape(before)+r'(?=[.\"\s\)])',lambda _:after,s)
  if '/Frontend/' in p.as_posix() and 'LoadObject<' in s:
   s=s.replace('LoadObject<','InkUiAssets::Load<')
   s=re.sub(r'(^#include[^\n]*\n)',r'\1#include "Frontend/UI/InkUiAssetPaths.h"\n',s,count=1,flags=re.MULTILINE)
  if s!=old:p.write_text(s,encoding='utf-8');changed.append(p.relative_to(ROOT).as_posix())
config=ROOT/'Config/DefaultEngine.ini'
s=config.read_text('utf-8-sig')
begin='; BEGIN PortraitUI20261023 legacy UI package aliases'
end='; END PortraitUI20261023 legacy UI package aliases'
s=re.sub(re.escape(begin)+r'.*?'+re.escape(end)+r'\n?','',s,flags=re.S)
aliases='\n'.join('+PackageRedirects=(OldName="'+old+'",NewName="'+new+'")' for old,new in sorted(mapping.items()))
s+='\n'+begin+'\n[CoreRedirects]\n'+aliases+'\n'+end+'\n'
config.write_text(s,encoding='utf-8')
(ROOT/'Tools/InkFrontend/ui_asset_paths.json').write_text(json.dumps(mapping,ensure_ascii=False,indent=2),encoding='utf-8')
(ROOT/'Saved/Codex/PortraitUI20261023/path-update.json').write_text(json.dumps(dict(changed=changed,aliases=len(mapping)),indent=2),encoding='utf-8')
print('Updated',len(changed),'files;',len(mapping),'legacy aliases')
