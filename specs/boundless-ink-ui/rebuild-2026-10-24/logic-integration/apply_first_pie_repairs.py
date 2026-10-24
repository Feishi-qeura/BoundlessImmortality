"""Repair concrete native Slate findings from actual default PIE attempt01."""
from pathlib import Path
import runpy,re
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
rec=runpy.run_path(str(ROOT/'specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-integration/visible-source-integration/record_source_change.py'),run_name='first_pie_repairs_recorder')
def put(relative,transform):
    p=ROOT/relative;before=p.read_text(encoding='utf-8-sig');after=transform(before);assert before!=after
    rec['begin'](relative,'Actual default PIE01 repair: fixed corner radius, filled/centered button content, bounded HUD geometry, explicit finite JSON metrics')
    p.write_text(after,encoding='utf-8');rec['finish'](relative)
def one(text,old,new):assert text.count(old)==1,(old,text.count(old));return text.replace(old,new)
base='Source/BoundlessImmortality/Frontend/UIRebuild20261024/'
def player(text):
    text=one(text,'Brush.DrawAs = ESlateBrushDrawType::RoundedBox;','Brush.DrawAs = ESlateBrushDrawType::RoundedBox;\n\t\tBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;\n\t\tBrush.OutlineSettings.CornerRadii = FVector4(4.f);')
    content='Sizing' if 'Button->SetContent(Sizing);' in text else 'Size'
    text=one(text,'Button->SetContent('+content+');','Button->SetContent('+content+');\n\t\tif(auto* Slot=Cast<UButtonSlot>('+content+'->Slot))\n\t\t{Slot->SetHorizontalAlignment(HAlign_Fill);Slot->SetVerticalAlignment(VAlign_Center);Slot->SetPadding(FMargin(12.f,4.f));}')
    return text
put(base+'Player/InkPlayerMenuPageWidgets.cpp',player)
put(base+'Player/InkPlayerCharacterInventoryWidgets.cpp',player)
def visible(text):
    text=player(text)
    text=one(text,'auto* Caption = MakeLabel(Tree, Label, 17, NAME_None, PaperLight);\n\t\tCaption->SetJustification(ETextJustify::Left);\n\t\tBody->AddChildToVerticalBox(Caption)->SetPadding(FMargin(2.f, 2.f, 2.f, 0.f));','// The live value already includes the semantic label; avoid a duplicate line.')
    text=one(text,'Body->AddChildToVerticalBox(Meter)->SetPadding(FMargin(2.f, 0.f, 2.f, 4.f));','auto* MeterSize=Tree->ConstructWidget<USizeBox>();MeterSize->SetHeightOverride(10.f);MeterSize->SetContent(Meter);\n\t\tBody->AddChildToVerticalBox(MeterSize)->SetPadding(FMargin(2.f, 2.f, 2.f, 6.f));')
    text=one(text,'IdentitySlot->SetOffsets(FMargin(24.f, 24.f, 344.f, 0.f));\n\tIdentitySlot->SetAutoSize(true);','IdentitySlot->SetOffsets(FMargin(24.f, 24.f, 344.f, 220.f));\n\tIdentitySlot->SetAutoSize(false);')
    return text
put(base+'Player/InkPlayerVisibleEntryWidgets.cpp',visible)
def root(text):
    assert text.count('请求处理中，保留已确认的数据')==2
    return text.replace('请求处理中，保留已确认的数据','请求处理中')
put(base+'InkRebuildFrontendWidget.cpp',root)
def editor(text):
    start=text.index('FString UInkFrontendEditorLibrary::DescribeWidget(');end=text.index('\nbool UInkFrontendEditorLibrary::DragInventoryScrollThumb',start)
    block=text[start:end]
    block,count=re.subn(r'(\w+)->SetNumberField\(',r'Number(\1,',block);assert count>25
    brace=block.index('{')+1
    block=block[:brace]+'''\n    // Cached geometry of collapsed or newly reflowed widgets may be nonfinite.
    // Preserve that diagnostic explicitly as null rather than emitting invalid JSON.
    auto Number=[](const TSharedRef<FJsonObject>& Object,const FString& Key,double Value)
    {
        if(FMath::IsFinite(Value))Object->SetNumberField(Key,Value);
        else{Object->SetField(Key,MakeShared<FJsonValueNull>());Object->SetBoolField(TEXT("has_nonfinite_metrics"),true);}
    };
''' +block[brace:]
    return text[:start]+block+text[end:]
put('Source/BoundlessImmortalityEditor/Private/InkFrontendEditorLibrary.cpp',editor)
# Existing HUD target has full presentation authorization; close the remaining
# canonical loader gap found while inspecting the separate Slate GM fallback.
def hud(text):
    mapping=__import__('json').loads((HERE.parent/'migration/reference-replacement-plan.json').read_text(encoding='utf-8-sig'))['resolved_package_replacements']
    def replace(m):
        old=m.group(1);package=old.split('.',1)[0];assert package in mapping
        return '"'+mapping[package]+old[len(package):]+'"'
    return re.sub(r'"(/Game/UI/[^"\r\n]+)"',replace,text)
put('Source/BoundlessImmortality/LiveWeapons/LiveWeaponHUD.cpp',hud)
print('Recorded6 source repairs from actual PIE findings')
