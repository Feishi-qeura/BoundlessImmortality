#include "InkUiStyleEditorLibrary.h"
#include "Frontend/UI/InkUiTheme.h"
#include "Frontend/InkFrontendWidget.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "Factories/FontFileImportFactory.h"
#include "Fonts/CompositeFont.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "WidgetBlueprint.h"
#include "ObjectTools.h"
#include "AssetToolsModule.h"
#include "UObject/ObjectRedirector.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"

bool UInkUiStyleEditorLibrary::RelocateUiAsset(UObject* Asset,const FString& DestinationPackage)
{
    if(!Asset||!Asset->GetOutermost()->GetName().StartsWith(TEXT("/Game/newUI/"))||!DestinationPackage.StartsWith(TEXT("/Game/newUI/Ink/"))||!FPackageName::IsValidLongPackageName(DestinationPackage))return false;
    UPackage* Old=Asset->GetOutermost();Old->FullyLoad();
    ObjectTools::FPackageGroupName Target;Target.PackageName=DestinationPackage;Target.ObjectName=Asset->GetName();
    TSet<UPackage*> Refused;FText Error;
    if(!ObjectTools::RenameSingleObject(Asset,Target,Refused,Error,nullptr,true))
    {UE_LOG(LogTemp,Error,TEXT("UI relocation failed: %s"),*Error.ToString());return false;}
    FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;
    const FString OldFile=FPackageName::LongPackageNameToFilename(Old->GetName(),FPackageName::GetAssetPackageExtension());
    const FString NewFile=FPackageName::LongPackageNameToFilename(DestinationPackage,FPackageName::GetAssetPackageExtension());
    return UPackage::SavePackage(Asset->GetOutermost(),Asset,*NewFile,Args)&&UPackage::SavePackage(Old,nullptr,*OldFile,Args);
}

void UInkUiStyleEditorLibrary::FixUiRedirectors()
{
    auto& Registry=FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    TArray<FAssetData> Assets;Registry.GetAssetsByPath(FName(TEXT("/Game/newUI")),Assets,true);
    TArray<UObjectRedirector*> Redirectors;
    for(const auto& Data:Assets)if(Data.AssetClassPath==UObjectRedirector::StaticClass()->GetClassPathName())
        if(auto* Redirector=Cast<UObjectRedirector>(Data.GetAsset()))Redirectors.Add(Redirector);
    FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().FixupReferencers(Redirectors,false);
}

namespace
{
    const FString Folder=TEXT("/Game/newUI/Ink/Refactor20261021/");
    UFontFace* ImportFace(const FString& Name,const FString& Relative)
    {
        const FString Path=Folder+TEXT("Fonts/")+Name;
        if(auto* Existing=LoadObject<UFontFace>(nullptr,*(Path+TEXT(".")+Name),nullptr,LOAD_NoWarn|LOAD_Quiet))return Existing;
        const FString Source=FPaths::ProjectDir()/TEXT("docs/design/2026-10-21-ink-ui-standard-v1/fonts")/Relative;
        TArray<uint8> Bytes;if(!FFileHelper::LoadFileToArray(Bytes,*Source)||Bytes.IsEmpty())return nullptr;
        auto* Factory=NewObject<UFontFileImportFactory>();Factory->BatchCreateFontAsset=EBatchCreateFontAsset::No;
        const uint8* Buffer=Bytes.GetData();
        auto* Face=Cast<UFontFace>(Factory->FactoryCreateBinary(UFontFace::StaticClass(),CreatePackage(*Path),FName(Name),RF_Public|RF_Standalone,nullptr,*FPaths::GetExtension(Source),Buffer,Buffer+Bytes.Num(),GWarn));
        if(Face){Face->SourceFilename=Source;Face->LoadingPolicy=EFontLoadingPolicy::LazyLoad;FAssetRegistryModule::AssetCreated(Face);Face->MarkPackageDirty();}
        return Face;
    }
    UFont* MakeFont(const FString& Name,UFontFace* Main,UFontFace* Fallback)
    {
        const FString Path=Folder+TEXT("Fonts/")+Name;
        if(auto* Existing=LoadObject<UFont>(nullptr,*(Path+TEXT(".")+Name),nullptr,LOAD_NoWarn|LOAD_Quiet))return Existing;
        if(!Main||!Fallback)return nullptr;
        auto* Font=NewObject<UFont>(CreatePackage(*Path),FName(Name),RF_Public|RF_Standalone);
        Font->FontCacheType=EFontCacheType::Runtime;
        auto& Composite=Font->GetMutableInternalCompositeFont();
        FTypefaceEntry Entry(TEXT("Regular"));Entry.Font=FFontData(Main);Composite.DefaultTypeface.Fonts.Add(Entry);
        Entry.Font=FFontData(Fallback);Composite.FallbackTypeface.Typeface.Fonts.Add(Entry);
        Composite.MakeDirty();FAssetRegistryModule::AssetCreated(Font);Font->MarkPackageDirty();return Font;
    }
}

TArray<UObject*> UInkUiStyleEditorLibrary::ImportRefactorFonts()
{
    auto* Body=ImportFace(TEXT("FF_InkWenKai"),TEXT("LXGWWenKai/LXGWWenKai-Regular.ttf"));
    auto* Display=ImportFace(TEXT("FF_InkMaShanZheng"),TEXT("MaShanZheng/MaShanZheng-Regular.ttf"));
    auto* Fallback=ImportFace(TEXT("FF_InkNotoFallback"),TEXT("NotoSansSC/NotoSansCJKsc-Regular.otf"));
    auto* BodyFont=MakeFont(TEXT("F_InkBody"),Body,Fallback);
    auto* DisplayFont=MakeFont(TEXT("F_InkDisplay"),Display,Body);
    auto* FallbackFont=MakeFont(TEXT("F_InkFallback"),Fallback,Fallback);
    if(!BodyFont||!DisplayFont||!FallbackFont)return {};
    const FString ThemePath=Folder+TEXT("Data/DA_InkUITheme_v1");
    auto* Theme=LoadObject<UInkUiThemeData>(nullptr,*(ThemePath+TEXT(".DA_InkUITheme_v1")),nullptr,LOAD_NoWarn|LOAD_Quiet);
    if(!Theme)
    {
        Theme=NewObject<UInkUiThemeData>(CreatePackage(*ThemePath),TEXT("DA_InkUITheme_v1"),RF_Public|RF_Standalone);
        Theme->DisplayFont=DisplayFont;Theme->BodyFont=BodyFont;Theme->DataFont=BodyFont;Theme->FallbackFont=FallbackFont;
        FAssetRegistryModule::AssetCreated(Theme);Theme->MarkPackageDirty();
    }
    return {Body,Display,Fallback,BodyFont,DisplayFont,FallbackFont,Theme};
}

bool UInkUiStyleEditorLibrary::SetMissingGlyphRanges(UFont* Font,UFontFace* Face,const TArray<int32>& Pairs)
{
    if(!Font||!Face||!Font->GetPathName().StartsWith(Folder)||!Face->GetPathName().StartsWith(Folder)||Pairs.Num()%2)return false;
    auto& Composite=Font->GetMutableInternalCompositeFont();
    Composite.SubTypefaces.Reset();
    if(!Pairs.IsEmpty())
    {
        FCompositeSubFont Sub;
        FTypefaceEntry Entry(TEXT("Regular"));Entry.Font=FFontData(Face);Sub.Typeface.Fonts.Add(Entry);
        for(int32 I=0;I<Pairs.Num();I+=2)
        {
            if(Pairs[I]<0||Pairs[I+1]<Pairs[I]||Pairs[I+1]>0x10ffff)return false;
            Sub.CharacterRanges.Add(FInt32Range::Inclusive(Pairs[I],Pairs[I+1]));
        }
        Composite.SubTypefaces.Add(Sub);
    }
    Composite.MakeDirty();Font->MarkPackageDirty();return true;
}

FString UInkUiStyleEditorLibrary::MigrateRefactorTypography()
{
    const auto* Theme=InkUiTypography::Theme();if(!Theme->BodyFont||!Theme->DisplayFont)return TEXT("Missing theme fonts");
    int32 Assets=0,Texts=0;
    for(const TCHAR* Suffix:{TEXT("Frontend"),TEXT("MainMenu"),TEXT("Character"),TEXT("Inventory"),TEXT("Leaderboard"),TEXT("Saves"),TEXT("Settings")})
    {
        const FString Name=TEXT("WBP_Ink")+FString(Suffix),Path=TEXT("/Game/newUI/Ink/Widgets/")+Name;
        auto* B=LoadObject<UWidgetBlueprint>(nullptr,*(Path+TEXT(".")+Name));if(!B||!B->GeneratedClass||!B->WidgetTree)return TEXT("Missing WBP: ")+Path;
        auto* Defaults=Cast<UInkFrontendWidget>(B->GeneratedClass->GetDefaultObject());if(!Defaults)return TEXT("Unexpected parent: ")+Path;
        Defaults->TextFont=Theme->BodyFont.Get();
        B->WidgetTree->ForEachWidget([&](UWidget* W)
        {
            if(auto* T=Cast<UTextBlock>(W))
            {
                auto Font=T->GetFont();Font.FontObject=Font.Size>=40?Theme->DisplayFont.Get():Theme->BodyFont.Get();Font.TypefaceFontName=TEXT("Regular");T->SetFont(Font);++Texts;
            }
            if(auto* E=Cast<UEditableTextBox>(W)){auto Style=E->GetWidgetStyle();auto Font=Style.TextStyle.Font;Font.FontObject=Theme->BodyFont.Get();Font.TypefaceFontName=TEXT("Regular");Style.SetFont(Font);E->SetWidgetStyle(Style);}
        });
        FBlueprintEditorUtils::MarkBlueprintAsModified(B);FKismetEditorUtilities::CompileBlueprint(B);
        Defaults=Cast<UInkFrontendWidget>(B->GeneratedClass->GetDefaultObject());Defaults->TextFont=Theme->BodyFont.Get();
        if(B->Status==BS_Error)return TEXT("Compile error: ")+Path;
        B->MarkPackageDirty();++Assets;
    }
    return FString::Printf(TEXT("Migrated %d WBP font references; %d TextBlocks; trees retained"),Assets,Texts);
}
