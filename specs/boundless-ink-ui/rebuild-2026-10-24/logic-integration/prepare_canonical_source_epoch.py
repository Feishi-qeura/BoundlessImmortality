"""Stage exact authorized canonical paths; production edits use the hash ledger."""
from pathlib import Path
import json,re,hashlib,difflib
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3];TEAM=HERE.parent/'migration'
plan=json.loads((TEAM/'reference-replacement-plan.json').read_text(encoding='utf-8-sig'))
mapping=plan['resolved_package_replacements']
stage=HERE/'canonical-source-stage';stage.mkdir(exist_ok=True)
paths=list((ROOT/'Source/BoundlessImmortality/Frontend').rglob('*.cpp'))+list((ROOT/'Source/BoundlessImmortalityEditor').rglob('*.cpp'))
paths += [ROOT/'Source/BoundlessImmortality/LiveWeapons'/name for name in ('LiveGMControlPanel.cpp','LiveGMOverlay.cpp','LiveTitleEffectsComponent.cpp')]
excluded={'NewUiMigrationExactFixup.cpp','ScopedKnownRenameWarning.cpp','LegacyUiRedirectorFixupCommandlet.cpp'}
report={'files':[],'unresolved':[],'mapped_packages':len(mapping),'exceptions':sorted(excluded)}
for file in paths:
    if file.name in excluded or file.name=='InkUiAssetPaths.cpp':continue
    before=file.read_text(encoding='utf-8-sig');after=before
    # Runtime names resolve through the finite canonical582 index, rather than
    # reconstructing legacy folders which now fan out across typed folders.
    after=re.sub(r'(?:FString\(TEXT\("/Game/UI[^"\r\n]+/"\)\)|TEXT\("/Game/UI[^"\r\n]+/"\))\+([A-Za-z_][A-Za-z0-9_>.-]*)\+TEXT\("\."\)\+\1',lambda m:'InkUiAssets::ResolveNamed('+m.group(1)+')',after)
    if file.name in ('InkNiagaraTitleWidget.cpp','InkNiagaraTitleTests.cpp'):
        old='const FString Path=InkUiAssets::ResolveNamed(Name);'
        assert after.count(old)==1
        after=after.replace(old,'const FString Path=InkUiAssets::TitleParticlesPath(Name);')
    # All gifts are finite; RareChest belongs to Rewards, not Gifts.
    if file.name=='LiveGMControlPanel.cpp':
        old='const TCHAR* Icons[]={TEXT("Wand"),TEXT("LoveExplosion"),TEXT("RareChest"),TEXT("SuperJet"),TEXT("MysteryAirdrop"),TEXT("StrawberryDessert"),TEXT("SuperAirdrop"),TEXT("LifePotion")};'
        names=('Wand','LoveExplosion','RareChest','SuperJet','MysteryAirdrop','StrawberryDessert','SuperAirdrop','LifePotion')
        targets=[mapping['/Game/UI/Ink/Gifts/T_Gift_'+n] for n in names]
        new='const TCHAR* IconPaths[]={'+','.join('TEXT("'+p+'.'+p.rsplit('/',1)[1]+'")' for p in targets)+'};'
        assert old in after;after=after.replace(old,new)
        old='const FString Path=FString::Printf(TEXT("/Game/UI/Ink/Gifts/T_Gift_%s.T_Gift_%s"),Icons[I],Icons[I]);'
        assert old in after;after=after.replace(old,'const FString Path=IconPaths[I];')
    def literal(match):
        old=match.group(1);package=old.split('.',1)[0];suffix=old[len(package):]
        if package in mapping:return '"'+mapping[package]+suffix+'"'
        # Verify a dynamic folder only if every mapped descendant preserves its suffix.
        if old.endswith('/'):
            children=[(k,v) for k,v in mapping.items() if k.startswith(old)]
            prefixes={v[:-len(k[len(old):])] for k,v in children if k[len(old):] and v.endswith(k[len(old):])}
            if children and len(prefixes)==1 and all(v==next(iter(prefixes))+k[len(old):] for k,v in children):return '"'+next(iter(prefixes))+'"'
        if old=='/Game/UI/Ink/CharacterRating20261022/T_Rating_%s.T_Rating_%s':
            for grade in ('D','C','B','A','S','SS','SSS'):assert mapping['/Game/UI/Ink/CharacterRating20261022/T_Rating_'+grade]=='/Game/newUI/Ink/Textures/Character/T_Rating_'+grade
            return '"/Game/newUI/Ink/Textures/Character/T_Rating_%s.T_Rating_%s"'
        if old=='/Game/UI/Ink/ComponentRollout20261024/Journey/%s.%s':return '"/Game/newUI/Ink/ComponentRollout20261024/Journey/%s.%s"'
        if file.parts[-3]=='BoundlessImmortalityEditor' or 'BoundlessImmortalityEditor' in file.parts:
            # Editor validation/output scopes cannot create anything in the retired root.
            if old in ('/Game/UI','/Game/UI/','/Game/UI/Ink/') or old.endswith('/'):
                return '"'+old.replace('/Game/UI','/Game/newUI',1)+'"'
        report['unresolved'].append({'file':file.relative_to(ROOT).as_posix(),'literal':old});return match.group(0)
    after=re.sub(r'"(/Game/UI[^"\r\n]*)"',literal,after)
    if file.name=='InkFrontendEditorLibrary.cpp':
        start=after.index('UObject* UInkFrontendEditorLibrary::CreateFrontendWidgetAsset(')
        end=after.index('\nUObject* UInkFrontendEditorLibrary::CreateRebuiltFrontendWidgetAsset(',start)
        after=after[:start]+'''UObject* UInkFrontendEditorLibrary::CreateFrontendWidgetAsset(const FString& Path,FName PreviewPage)
{
    // Retired legacy factory delegates only to the independent new runtime root.
    return CreateRebuiltFrontendWidgetAsset(Path);
}
''' + after[end:]
    if after!=before:
        relative=file.relative_to(ROOT).as_posix();dest=stage/relative;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_text(after,encoding='utf-8')
        report['files'].append({'path':relative,'before_sha256':hashlib.sha256(file.read_bytes()).hexdigest(),'staged':str(dest),'staged_sha256':hashlib.sha256(dest.read_bytes()).hexdigest()})
        (dest.with_suffix('.diff')).write_text(''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile=relative,tofile=relative)),encoding='utf-8')
# Complete compatibility dictionary: historical and formerly physical names resolve canonical.
file=ROOT/'Source/BoundlessImmortality/Frontend/UI/InkUiAssetPaths.cpp';before=file.read_text(encoding='utf-8-sig')
start=before.index('    static const TMap<FString,FString> Paths={');end=before.index('    };',start)+6
body='    static const TMap<FString,FString> Paths={\n'+''.join('        {TEXT("'+old+'"),TEXT("'+new+'")},\n' for old,new in sorted(mapping.items()))+'    };'
after=before[:start]+body+before[end:]
canonical=sorted(set(mapping.values()));by_name={}
for value in canonical:by_name.setdefault(value.rsplit('/',1)[1],set()).add(value)
ambiguous={k:sorted(v) for k,v in by_name.items() if len(v)>1}
report['ambiguous_canonical_names']=ambiguous
after+='\nFString InkUiAssets::ResolveNamed(const FString& Name)\n{\n    static const TMap<FString,FString> Names={\n'+''.join('        {TEXT("'+name+'"),TEXT("'+next(iter(values))+'.'+name+'")},\n' for name,values in sorted(by_name.items()) if len(values)==1)+'    };\n    if(const FString* Path=Names.Find(Name))return *Path;\n    return FString();\n}\n'
effects={name:mapping['/Game/UI/Ink/NiagaraRevision20261022/Flowing20261022/'+name] for name in ('NS_HonorParticles','NS_RealmParticles','NS_RealmIconParticles','NS_AvatarFrameParticles')}
after+='\nFString InkUiAssets::TitleParticlesPath(const FString& Name)\n{\n    static const TMap<FString,FString> Paths={\n'+''.join('        {TEXT("'+name+'"),TEXT("'+value+'.'+name+'")},\n' for name,value in sorted(effects.items()))+'    };\n    if(const FString* Path=Paths.Find(Name))return *Path;\n    return FString();\n}\n'
relative=file.relative_to(ROOT).as_posix();dest=stage/relative;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_text(after,encoding='utf-8')
report['files'].append({'path':relative,'before_sha256':hashlib.sha256(file.read_bytes()).hexdigest(),'staged':str(dest),'staged_sha256':hashlib.sha256(dest.read_bytes()).hexdigest()})
file=ROOT/'Source/BoundlessImmortality/Frontend/UI/InkUiAssetPaths.h';before=file.read_text(encoding='utf-8-sig')
after=before.replace('    BOUNDLESSIMMORTALITY_API FString Resolve(const TCHAR* Path);','    BOUNDLESSIMMORTALITY_API FString Resolve(const TCHAR* Path);\n    /** Exact finite canonical asset-name index; unknown/ambiguous names are empty. */\n    BOUNDLESSIMMORTALITY_API FString ResolveNamed(const FString& Name);')
after=after.replace('    template<class T,class... Args>', '    /** Preserve the exact reviewed flowing particle asset when names collide. */\n    BOUNDLESSIMMORTALITY_API FString TitleParticlesPath(const FString& Name);\n    template<class T,class... Args>')
relative=file.relative_to(ROOT).as_posix();dest=stage/relative;dest.write_text(after,encoding='utf-8')
report['files'].append({'path':relative,'before_sha256':hashlib.sha256(file.read_bytes()).hexdigest(),'staged':str(dest),'staged_sha256':hashlib.sha256(dest.read_bytes()).hexdigest()})
# Rewrite only NewName fields and add exact old canonical package redirects after migration.
for name in ('DefaultEngine.ini','DefaultGame.ini'):
    file=ROOT/'Config'/name;before=file.read_text(encoding='utf-8-sig');after=before
    if name=='DefaultEngine.ini':
        after=re.sub(r'NewName="([^"\r\n]+)"',lambda m:'NewName="'+mapping.get(m.group(1),m.group(1))+'"',after)
        keys=set(re.findall(r'\+PackageRedirects=\(OldName="([^"]+)"',after))
        extra=''.join('+PackageRedirects=(OldName="'+old+'",NewName="'+new+'")\n' for old,new in sorted(mapping.items()) if old not in keys)
        # Add under the existing CoreRedirects section before the next section.
        section=after.index('[CoreRedirects]');end=after.find('\n[',section+1);end=len(after) if end<0 else end
        after=after[:end].rstrip()+'\n'+extra+after[end:]
    else:after=after.replace('DirectoriesToAlwaysCook=(Path="/Game/UI/Ink")','DirectoriesToAlwaysCook=(Path="/Game/newUI")')
    if after!=before:
        relative=file.relative_to(ROOT).as_posix();dest=stage/relative;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_text(after,encoding='utf-8')
        report['files'].append({'path':relative,'before_sha256':hashlib.sha256(file.read_bytes()).hexdigest(),'staged':str(dest),'staged_sha256':hashlib.sha256(dest.read_bytes()).hexdigest()})
(HERE/'canonical-source-stage-review.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps({'files':len(report['files']),'unresolved':report['unresolved'],'mapped':len(mapping)},ensure_ascii=False))
