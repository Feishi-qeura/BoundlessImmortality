#include "InkFrontendEditorLibrary.h"
#include "LiveWeapons/LiveAnchorController.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "LiveWeapons/LiveWeaponGameMode.h"
#include "LiveWeapons/LiveWeaponSwarm.h"
#include "LiveWeapons/LiveAudienceAdapterComponent.h"
#include "LiveWeapons/LiveMonster.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Cultivation/CultivationSubsystem.h"
#include "Engine/GameInstance.h"
#include "Misc/ScopeExit.h"
#include "Frontend/InkFrontendWidget.h"
#include "Frontend/UIRebuild20261024/InkRebuildFrontendWidget.h"
#include "Frontend/UIRebuild20261024/InkRebuildFeatureHost.h"
#include "Frontend/InkExtensionWidget.h"
#include "Frontend/InkAnchorStatusWidget.h"
#include "Frontend/InkNiagaraTitleWidget.h"
#include "LiveWeapons/LiveTitleEffectsComponent.h"
#include "Cultivation/CultivationNameplate.h"
#include "NiagaraSystemWidget.h"
#include "LiveInteractionSubsystem.h"
#include "LiveInteractionTestAdapter.h"
#include "Frontend/InkSignInChestWidget.h"
#include "Frontend/InkBufferedProgressWidget.h"
#include "LiveWeapons/LiveGMOverlay.h"
#include "LiveWeapons/LiveGMControlPanel.h"
#include "Frontend/InkInventoryModel.h"
#include "Frontend/InkInventoryScrollBar.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Animation/WidgetAnimation.h"
#include "WidgetBlueprint.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "Components/TextBlock.h"
#include "Components/SafeZoneSlot.h"
#include "Components/SizeBox.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/Border.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/EditableTextBox.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstance.h"
#include "Components/WidgetSwitcher.h"
#include "Framework/Application/SlateApplication.h"
#include "Layout/WidgetPath.h"
#include "Layout/ArrangedChildren.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "JsonObjectConverter.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Slate/SceneViewport.h"
#include "Editor.h"
#include "PlayInEditorDataTypes.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Widgets/SWindow.h"
#include "Widgets/SViewport.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"

BOUNDLESSIMMORTALITY_API bool InkNewUiSuppressesLegacyWeaponHud(UWorld* World,APlayerController* Player);
BOUNDLESSIMMORTALITY_API bool InkNewUiSuppressesLegacyGMAnchor(UWorld* World,APlayerController* Player);
namespace InkInventoryEditor
{
    int32 CountDescendantTexts(const UWidget* Widget)
    {
        int32 Count=0;
        if(const auto* Panel=Cast<UPanelWidget>(Widget))for(const auto* Child:Panel->GetAllChildren())
        {Count+=Cast<UTextBlock>(Child)?1:0;Count+=CountDescendantTexts(Child);}
        return Count;
    }

    void DescribeMaterial(const UObject* Resource,const TSharedRef<FJsonObject>& Entry)
    {
        const auto* Material=Cast<UMaterialInterface>(Resource);if(!Material)return;
        if(const auto* Instance=Cast<UMaterialInstance>(Material))if(Instance->Parent)Entry->SetStringField(TEXT("material_parent"),Instance->Parent->GetPathName());
        UTexture* Tier=nullptr;if(Material->GetTextureParameterValue(FMaterialParameterInfo(TEXT("TierTexture")),Tier)&&Tier)Entry->SetStringField(TEXT("tier_texture"),Tier->GetPathName());
        UTexture* Atlas=nullptr;
        if(Material->GetTextureParameterValue(FMaterialParameterInfo(TEXT("AtlasTexture")),Atlas)&&Atlas)
            Entry->SetStringField(TEXT("material_atlas"),Atlas->GetPathName());
        const TCHAR* Parameters[]={TEXT("KeyPaper"),TEXT("ClearErasedAlpha"),TEXT("Opacity"),TEXT("Mode")};
        const TCHAR* Fields[]={TEXT("material_key_paper"),TEXT("material_clear_erased_alpha"),TEXT("material_opacity"),TEXT("material_mode")};
        for(int32 Index=0;Index<UE_ARRAY_COUNT(Parameters);++Index)
        {
            float Value=0;if(Material->GetScalarParameterValue(FMaterialParameterInfo(Parameters[Index]),Value))Entry->SetNumberField(Fields[Index],Value);
        }
        FLinearColor Interior;
        if(Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("OpaqueInterior")),Interior))
        {
            TArray<TSharedPtr<FJsonValue>> Values;
            for(float Value:{Interior.R,Interior.G,Interior.B,Interior.A})Values.Add(MakeShared<FJsonValueNumber>(Value));
            Entry->SetArrayField(TEXT("material_opaque_interior"),Values);
        }
    }

    bool IsEffectivelyVisible(const UWidget* Widget)
    {
        if(!Widget||!Widget->IsVisible())return false;
        const UWidget* Ancestor=Widget;TSet<const UWidget*> Seen;
        while(Ancestor&&!Seen.Contains(Ancestor))
        {
            Seen.Add(Ancestor);const UWidget* Parent=Ancestor->GetParent();
            if(!Parent){const auto* Owner=Ancestor->GetTypedOuter<UUserWidget>();if(Owner&&Owner!=Ancestor){Ancestor=Owner;if(!Owner->IsVisible())return false;continue;}break;}
            if(!Parent->IsVisible())return false;
            if(const auto* Switcher=Cast<UWidgetSwitcher>(Parent))if(Switcher->GetActiveWidget()!=Ancestor)return false;
            Ancestor=Parent;
        }
        return true;
    }

    UWidget* FindCurrentNamedWidget(UInkFrontendWidget* Root,FName Name)
    {
        TArray<UWidget*> Matches;TSet<const UUserWidget*> Seen;
        TFunction<void(UUserWidget*)> Visit=[&](UUserWidget* User)
        {if(!User||!User->WidgetTree||Seen.Contains(User))return;Seen.Add(User);User->WidgetTree->ForEachWidget([&](UWidget* Child){if(Child->GetFName()==Name&&IsEffectivelyVisible(Child))Matches.AddUnique(Child);if(auto* Nested=Cast<UUserWidget>(Child);Nested&&Nested!=User)Visit(Nested);});};
        Visit(Root);if(Root)Visit(Root->ExtensionPages.FindRef(Root->CurrentPage));return Matches.Num()==1?Matches[0]:nullptr;
    }

    bool MoveTo(UWidget* Widget)
    {
        if(!IsEffectivelyVisible(Widget)||!FSlateApplication::IsInitialized()||!Widget->GetCachedWidget())return false;
        const auto& Geometry=Widget->GetCachedGeometry();if(Geometry.GetLocalSize().IsNearlyZero())return false;
        const FVector2D Position=Geometry.LocalToAbsolute(Geometry.GetLocalSize()*.5f);
        auto& Slate=FSlateApplication::Get();const FVector2D Previous=Slate.GetCursorPos();Slate.SetCursorPos(Position);
        Slate.ProcessMouseMoveEvent(FPointerEvent(FSlateApplication::CursorPointerIndex,Position,Previous,TSet<FKey>{},EKeys::Invalid,0,FModifierKeysState()));
        return Slate.LocateWindowUnderMouse(Position,Slate.GetInteractiveTopLevelWindows()).ContainsWidget(Widget->GetCachedWidget().Get());
    }
}

UObject* UInkFrontendEditorLibrary::CreateFrontendWidgetAsset(const FString& Path,FName PreviewPage)
{
    // Retired legacy factory delegates only to the independent new runtime root.
    return CreateRebuiltFrontendWidgetAsset(Path);
}

UObject* UInkFrontendEditorLibrary::CreateRebuiltFrontendWidgetAsset(const FString& Path)
{
    if(Path!=TEXT("/Game/newUI/Runtime/WBP_NewUIFrontend")||!GEditor||GEditor->PlayWorld)return nullptr;
    const FString Name=FPackageName::GetLongPackageAssetName(Path);
    if(auto* Existing=LoadObject<UWidgetBlueprint>(nullptr,*(Path+TEXT(".")+Name),nullptr,LOAD_NoWarn|LOAD_Quiet))
        return Existing->ParentClass&&Existing->ParentClass->IsChildOf(UInkRebuildFrontendWidget::StaticClass())?Existing:nullptr;
    auto* Blueprint=Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(UInkRebuildFrontendWidget::StaticClass(),CreatePackage(*Path),*Name,BPTYPE_Normal,UWidgetBlueprint::StaticClass(),UWidgetBlueprintGeneratedClass::StaticClass()));
    if(!Blueprint)return nullptr;
    if(!Blueprint->WidgetTree)Blueprint->WidgetTree=NewObject<UWidgetTree>(Blueprint,TEXT("WidgetTree"),RF_Transactional);
    auto* Template=NewObject<UInkRebuildFrontendWidget>();Template->BuildDefaultTree(Blueprint->WidgetTree);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);FKismetEditorUtilities::CompileBlueprint(Blueprint);
    if(!Blueprint->GeneratedClass||!Blueprint->GeneratedClass->IsChildOf(UInkRebuildFrontendWidget::StaticClass())||Blueprint->Status==BS_Error)return nullptr;
    FAssetRegistryModule::AssetCreated(Blueprint);Blueprint->MarkPackageDirty();return Blueprint;
}

bool UInkFrontendEditorLibrary::ClickGMAction(ULiveGMOverlayWidget* Widget,FName Action)
{
    if(!Widget||!Widget->Controls||!Widget->bPanelOpen||!FSlateApplication::IsInitialized())return false;
    for(auto Button:Widget->Controls->Buttons)if(Button&&Button->Action==Action&&Button->GetIsEnabled())
    {
        if(!InkInventoryEditor::MoveTo(Button))continue;const auto& G=Button->GetCachedGeometry();const FVector2D P=G.LocalToAbsolute(G.GetLocalSize()*.5f);auto& Slate=FSlateApplication::Get();
        const auto Hit=Slate.LocateWindowUnderMouse(P,Slate.GetInteractiveTopLevelWindows());if(!Hit.ContainsWidget(Button->GetCachedWidget().Get()))continue;
        const bool Down=Slate.ProcessMouseButtonDownEvent(nullptr,FPointerEvent(FSlateApplication::CursorPointerIndex,P,P,TSet<FKey>{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0,FModifierKeysState()));Slate.ProcessMouseButtonUpEvent(FPointerEvent(FSlateApplication::CursorPointerIndex,P,P,TSet<FKey>{},EKeys::LeftMouseButton,0,FModifierKeysState()));return Down;
    }
    return false;
}

bool UInkFrontendEditorLibrary::ClickAction(UInkFrontendWidget* Widget,FName Action)
{
    if(!Widget||!FSlateApplication::IsInitialized())return false;
    for(auto Button:Widget->ActionButtons)if(Button&&Button->Action==Action&&Button->GetIsEnabled()&&InkInventoryEditor::IsEffectivelyVisible(Button))
    {
        const auto& Geometry=Button->GetCachedGeometry();if(Geometry.GetLocalSize().IsNearlyZero())continue;
        const FVector2D Position=Geometry.LocalToAbsolute(Geometry.GetLocalSize()*.5f);
        auto& Slate=FSlateApplication::Get();Slate.SetCursorPos(Position);
        Slate.ProcessMouseMoveEvent(FPointerEvent(FSlateApplication::CursorPointerIndex,Position,Position,TSet<FKey>{},EKeys::Invalid,0,FModifierKeysState()));
        const FWidgetPath Hit=Slate.LocateWindowUnderMouse(Position,Slate.GetInteractiveTopLevelWindows());
        if(!Button->GetCachedWidget()||!Hit.ContainsWidget(Button->GetCachedWidget().Get()))continue;
        const bool Handled=Slate.ProcessMouseButtonDownEvent(nullptr,FPointerEvent(FSlateApplication::CursorPointerIndex,Position,Position,TSet<FKey>{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0,FModifierKeysState()));
        Slate.ProcessMouseButtonUpEvent(FPointerEvent(FSlateApplication::CursorPointerIndex,Position,Position,TSet<FKey>{},EKeys::LeftMouseButton,0,FModifierKeysState()));return Handled;
    }
    return false;
}

bool UInkFrontendEditorLibrary::ClickRolloutButton(UInkFrontendWidget* Widget,UButton* Button)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("GWLocalTest"))||!Widget||!Button||!Widget->GetWorld()||!Widget->GetWorld()->IsGameWorld()||Button->GetWorld()!=Widget->GetWorld()||!FSlateApplication::IsInitialized()||!Button->GetIsEnabled()||!InkInventoryEditor::IsEffectivelyVisible(Button))return false;
    const auto& Geometry=Button->GetCachedGeometry();if(Geometry.GetLocalSize().IsNearlyZero())return false;
    const FVector2D Position=Geometry.LocalToAbsolute(Geometry.GetLocalSize()*.5f);
    auto& Slate=FSlateApplication::Get();Slate.SetCursorPos(Position);
    Slate.ProcessMouseMoveEvent(FPointerEvent(FSlateApplication::CursorPointerIndex,Position,Position,TSet<FKey>{},EKeys::Invalid,0,FModifierKeysState()));
    const FWidgetPath Hit=Slate.LocateWindowUnderMouse(Position,Slate.GetInteractiveTopLevelWindows());
    if(!Button->GetCachedWidget()||!Hit.ContainsWidget(Button->GetCachedWidget().Get()))return false;
    const bool Handled=Slate.ProcessMouseButtonDownEvent(nullptr,FPointerEvent(FSlateApplication::CursorPointerIndex,Position,Position,TSet<FKey>{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0,FModifierKeysState()));
    Slate.ProcessMouseButtonUpEvent(FPointerEvent(FSlateApplication::CursorPointerIndex,Position,Position,TSet<FKey>{},EKeys::LeftMouseButton,0,FModifierKeysState()));return Handled;
}

bool UInkFrontendEditorLibrary::RefreshGeneratedFrontendAsset(const FString& Path,FName PreviewPage)
{
    if(!Path.StartsWith(TEXT("/Game/newUI/Ink/Widgets/")))return false;
    auto* Blueprint=LoadObject<UWidgetBlueprint>(nullptr,*(Path+TEXT(".")+FPackageName::GetLongPackageAssetName(Path)));
    if(!Blueprint||Blueprint->ParentClass!=UInkFrontendWidget::StaticClass())return false;
    if(auto* OldTree=Blueprint->WidgetTree.Get())OldTree->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors|REN_NonTransactional);
    Blueprint->WidgetTree=NewObject<UWidgetTree>(Blueprint,TEXT("WidgetTree"),RF_Transactional);
    auto* Template=NewObject<UInkFrontendWidget>();Template->BuildDefaultTree(Blueprint->WidgetTree);
    // This explicit rebuild replaces the generated tree; regenerate its variable GUID table too.
    Blueprint->WidgetVariableNameToGuidMap.Reset();
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);FKismetEditorUtilities::CompileBlueprint(Blueprint);
    if(auto* Defaults=Cast<UInkFrontendWidget>(Blueprint->GeneratedClass?Blueprint->GeneratedClass->GetDefaultObject():nullptr))
    {Defaults->DesignerPreviewPage=PreviewPage;Defaults->CurrentPage=PreviewPage;}
    Blueprint->MarkPackageDirty();return Blueprint->Status!=BS_Error;
}

bool UInkFrontendEditorLibrary::PressFrontendKey(UInkFrontendWidget* Widget,FKey Key)
{
    if(!Widget||!FSlateApplication::IsInitialized()||!Widget->GetCachedWidget())return false;
    auto& Slate=FSlateApplication::Get();Slate.SetKeyboardFocus(Widget->GetCachedWidget(),EFocusCause::SetDirectly);
    const FKeyEvent Event(Key,FModifierKeysState(),0,false,0,0);
    const bool Handled=Slate.ProcessKeyDownEvent(Event);Slate.ProcessKeyUpEvent(Event);return Handled;
}

bool UInkFrontendEditorLibrary::PressFocusedFrontendKey(FKey Key)
{
    if(!FSlateApplication::IsInitialized())return false;
    auto& Slate=FSlateApplication::Get();const FKeyEvent Event(Key,FModifierKeysState(),0,false,0,0);
    const bool Handled=Slate.ProcessKeyDownEvent(Event);Slate.ProcessKeyUpEvent(Event);return Handled;
}

TArray<int32> UInkFrontendEditorLibrary::GetUniformGridColumns(UInkFrontendWidget* Widget,FName Name)
{
    TArray<int32> Result;
    if(auto* Grid=Widget?Cast<UUniformGridPanel>(Widget->GetWidgetFromName(Name)):nullptr)
        for(auto* Child:Grid->GetAllChildren())if(auto* Slot=Cast<UUniformGridSlot>(Child->Slot))Result.Add(Slot->GetColumn());
    return Result;
}
FString UInkFrontendEditorLibrary::DescribeWidget(UInkFrontendWidget* Widget)
{
    // Cached geometry of collapsed or newly reflowed widgets may be nonfinite.
    // Preserve that diagnostic explicitly as null rather than emitting invalid JSON.
    auto Number=[](const TSharedRef<FJsonObject>& Object,const FString& Key,double Value)
    {
        if(FMath::IsFinite(Value))Object->SetNumberField(Key,Value);
        else{Object->SetField(Key,MakeShared<FJsonValueNull>());Object->SetBoolField(TEXT("has_nonfinite_metrics"),true);}
    };

    auto Root=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Buttons,Texts,Images,Borders,Scrolls,Cells,Chests,Progress,Inputs;
    if(Widget&&Widget->WidgetTree)
    {
        Root->SetStringField(TEXT("page"),Widget->CurrentPage.ToString());
        Root->SetStringField(TEXT("runtime_class"),Widget->GetClass()->GetPathName());
        Root->SetStringField(TEXT("board_category"),Widget->GetPresentedLeaderboardCategory().ToString());
        Root->SetStringField(TEXT("board_scope"),Widget->LeaderboardScope.ToString());
        Root->SetStringField(TEXT("runtime_object"),Widget->GetPathName());
        Root->SetStringField(TEXT("runtime_layout_version"),Widget->GetRuntimeLayoutVersion().ToString());
        Root->SetStringField(TEXT("page_layout_version"),Widget->GetRuntimePageLayoutVersion(Widget->CurrentPage).ToString());
        Root->SetStringField(TEXT("root_widget"),Widget->WidgetTree->RootWidget?Widget->WidgetTree->RootWidget->GetName():TEXT(""));
        if(const auto* Extension=Widget->ExtensionPages.FindRef(Widget->CurrentPage).Get())
        {
            Root->SetStringField(TEXT("extension_runtime_class"),Extension->GetClass()->GetPathName());
            Root->SetStringField(TEXT("extension_root_widget"),Extension->WidgetTree&&Extension->WidgetTree->RootWidget?Extension->WidgetTree->RootWidget->GetName():TEXT(""));
            if(const auto* Host=Cast<UInkRebuildFeatureHost>(Extension))Root->SetStringField(TEXT("feature_composition"),Host->GetCompositionName());
        }
        Root->SetBoolField(TEXT("legacy_weapon_hud_suppression_condition"),InkNewUiSuppressesLegacyWeaponHud(Widget->GetWorld(),Widget->GetOwningPlayer()));
        Root->SetBoolField(TEXT("legacy_gm_anchor_suppression_condition"),InkNewUiSuppressesLegacyGMAnchor(Widget->GetWorld(),Widget->GetOwningPlayer()));
        Root->SetBoolField(TEXT("legacy_status_adapter_collapsed"),!Widget->AnchorStatusWidget||Widget->AnchorStatusWidget->GetVisibility()==ESlateVisibility::Collapsed);
        Root->SetStringField(TEXT("active_leaderboard"),Widget->ActiveGameplayLeaderboard.ToString());
        Number(Root,TEXT("leaderboard_opacity"),Widget->GameplayLeaderboardOpacity);
        Number(Root,TEXT("width"),Widget->GetCachedGeometry().GetLocalSize().X);Number(Root,TEXT("height"),Widget->GetCachedGeometry().GetLocalSize().Y);
        TArray<TSharedPtr<FJsonValue>> Nameplates;
        for(TActorIterator<ALiveWeaponSwarm> It(Widget->GetWorld());It;++It)if(It->TitleEffects)
        for(auto* Plate:It->TitleEffects->GetActiveNameplates())if(Plate&&Plate->IsInViewport()&&Plate->IsVisible())
        {
            auto Entry=MakeShared<FJsonObject>();Entry->SetStringField(TEXT("participant_id"),Plate->GetParticipantId());
            const auto* Avatar=Plate->GetAvatarImage();const auto* Frame=Plate->GetAvatarEffect();const auto* Particles=Frame?Frame->GetEffectWidget():nullptr;
            auto Point=[](const UWidget* W,FVector2D Fraction){const auto& G=W->GetCachedGeometry();return G.LocalToAbsolute(G.GetLocalSize()*Fraction);};
            const bool Owned=Avatar&&Frame&&Particles&&Plate->GetIdentityText();Entry->SetBoolField(TEXT("owned"),Owned);
            if(Owned)
            {
                Number(Entry,TEXT("center_error_pixels"),FVector2D::Distance(Point(Avatar,FVector2D(.5)),Point(Frame,FVector2D(.5))));
                Number(Entry,TEXT("particle_center_error_pixels"),FVector2D::Distance(Point(Avatar,FVector2D(.5)),Point(Particles,FVector2D(.5))));
                Number(Entry,TEXT("bounds_error_pixels"),FMath::Max(FVector2D::Distance(Point(Avatar,FVector2D::ZeroVector),Point(Frame,FVector2D::ZeroVector)),FVector2D::Distance(Point(Avatar,FVector2D(1)),Point(Frame,FVector2D(1)))));
                Number(Entry,TEXT("avatar_diagonal_pixels"),FVector2D::Distance(Point(Avatar,FVector2D::ZeroVector),Point(Avatar,FVector2D(1))));
                if(auto* PC=Widget->GetOwningPlayer())if(auto* View=PC->GetWorld()->GetGameViewport())if(auto Viewport=View->GetGameViewportWidget())
                {
                    FVector2D Projected;PC->ProjectWorldLocationToScreen(It->GetWeaponPosition(Plate->GetParticipantId())+FVector(0,0,45),Projected,true);
                    const auto Anchor=Point(Plate,FVector2D(.5,1))-Viewport->GetCachedGeometry().LocalToAbsolute(FVector2D::ZeroVector);
                    Number(Entry,TEXT("projection_error_pixels"),FVector2D::Distance(Anchor,Projected));
                }
            }
            Nameplates.Add(MakeShared<FJsonValueObject>(Entry));
        }
        Root->SetArrayField(TEXT("nameplates"),Nameplates);
        if(const auto* Anchor=Widget->AnchorStatusWidget.Get())
        {
            const auto& Geometry=Anchor->GetCachedGeometry();
            const auto Position=Widget->GetCachedGeometry().AbsoluteToLocal(Geometry.LocalToAbsolute(FVector2D::ZeroVector));
            auto Entry=MakeShared<FJsonObject>();
            Entry->SetStringField(TEXT("object"),Anchor->GetPathName());
            Number(Entry,TEXT("x"),Position.X);Number(Entry,TEXT("y"),Position.Y);
            Number(Entry,TEXT("width"),Geometry.GetLocalSize().X);Number(Entry,TEXT("height"),Geometry.GetLocalSize().Y);
            Number(Entry,TEXT("paint_width"),Anchor->GetPaintSpaceGeometry().GetLocalSize().X);
            Number(Entry,TEXT("paint_height"),Anchor->GetPaintSpaceGeometry().GetLocalSize().Y);
            Entry->SetStringField(TEXT("avatar_url"),Anchor->GetIdentityAvatarUrl());
            Entry->SetStringField(TEXT("avatar_texture"),Anchor->AvatarTexture?Anchor->AvatarTexture->GetPathName():TEXT(""));
            Entry->SetBoolField(TEXT("avatar_placeholder"),Anchor->bAvatarPlaceholder);
            Number(Entry,TEXT("health_fraction"),Anchor->DisplayedHealthFraction);
            Number(Entry,TEXT("cultivation_fraction"),Anchor->DisplayedCultivationFraction);
            Root->SetObjectField(TEXT("anchor_status"),Entry);
        }
        // Re-arrange the current Slate tree from its valid root, independently of each
        // UWidget cached geometry. A finite ancestor outside its finite culling rect
        // proves descendants are not painted; missing/invalid evidence stays Unknown.
        struct FPaintEvidence { bool bKnown=false,bCulled=false;FString CulledAncestor;FSlateRect Bounds,Clip,CulledBounds,CulledClip; };
        TMap<const SWidget*,FPaintEvidence> PaintEvidence;
        auto FiniteRect=[](const FSlateRect& R){return FMath::IsFinite(R.Left)&&FMath::IsFinite(R.Top)&&FMath::IsFinite(R.Right)&&FMath::IsFinite(R.Bottom);};
        TSet<const SWidget*> ArrangedSeen;
        TFunction<void(const TSharedRef<SWidget>&,const FGeometry&,const FSlateRect&,bool,const FString&,const FSlateRect&,const FSlateRect&,int32)> Arrange;
        Arrange=[&](const TSharedRef<SWidget>& Slate,const FGeometry& G,const FSlateRect& Clip,bool AncestorCulled,const FString& Proof,const FSlateRect& ProofBounds,const FSlateRect& ProofClip,int32 Depth)
        {
            if(Depth>256||ArrangedSeen.Contains(&Slate.Get()))return;ArrangedSeen.Add(&Slate.Get());
            const FSlateRect Bounds=G.GetRenderBoundingRect();
            const bool Valid=FiniteRect(Bounds)&&FiniteRect(Clip)&&G.GetLocalSize().X>0&&G.GetLocalSize().Y>0;
            const bool Outside=Valid&&!FSlateRect::DoRectanglesIntersect(Bounds,Clip);
            FPaintEvidence Evidence;Evidence.bKnown=AncestorCulled||Valid;Evidence.bCulled=AncestorCulled||Outside;
            Evidence.CulledAncestor=AncestorCulled?Proof:(Outside?Slate->GetTypeAsString():FString());Evidence.Bounds=Bounds;Evidence.Clip=Clip;Evidence.CulledBounds=AncestorCulled?ProofBounds:Bounds;Evidence.CulledClip=AncestorCulled?ProofClip:Clip;
            PaintEvidence.Add(&Slate.Get(),Evidence);
            bool ClipBounds=false,AlwaysClip=false,Intersect=true;
            const FSlateRect ChildClip=Slate->CalculateCullingAndClippingRules(G,Clip,ClipBounds,AlwaysClip,Intersect);
            FArrangedChildren Children(EVisibility::Visible);Slate->ArrangeChildren(G,Children);
            for(int32 Index=0;Index<Children.Num();++Index)
                Arrange(Children[Index].Widget,Children[Index].Geometry,ChildClip,Evidence.bCulled,Evidence.CulledAncestor,Evidence.CulledBounds,Evidence.CulledClip,Depth+1);
        };
        if(auto Slate=Widget->GetCachedWidget())
        {
            const auto& RootGeometry=Slate->GetPaintSpaceGeometry();const auto RootClip=RootGeometry.GetRenderBoundingRect();
            if(FiniteRect(RootClip)&&RootGeometry.GetLocalSize().X>0&&RootGeometry.GetLocalSize().Y>0)
                Arrange(Slate.ToSharedRef(),RootGeometry,RootClip,false,FString(),FSlateRect(),FSlateRect(),0);
        }
        auto DescribeChild=[&](UWidget* Child)
        {
            bool Visible=InkInventoryEditor::IsEffectivelyVisible(Child),bInConfirmation=false;UWidget* Ancestor=Child;
            for(UWidget* Parent=Child->GetParent();Parent;Ancestor=Parent,Parent=Parent->GetParent())
            {
                Visible&=Parent->IsVisible();if(auto* Switcher=Cast<UWidgetSwitcher>(Parent))Visible&=Switcher->GetActiveWidget()==Ancestor;
                bInConfirmation|=Parent->GetName()==TEXT("ConfirmationPaper");
            }
            auto Entry=MakeShared<FJsonObject>();Entry->SetStringField(TEXT("name"),Child->GetName());Entry->SetBoolField(TEXT("visible"),Visible);
            if(Visible)if(const auto* Hub=Cast<URebuildActivitiesHubWidget>(Child))Root->SetStringField(TEXT("activity_cycle"),Hub->GetPresentedCycle().ToString());
            Entry->SetStringField(TEXT("parent_object"),Child->GetParent()?Child->GetParent()->GetPathName():TEXT(""));
            Entry->SetStringField(TEXT("object"),Child->GetPathName());Number(Entry,TEXT("object_id"),Child->GetUniqueID());
            if(auto* Chest=Cast<UInkSignInChestWidget>(Child)){Entry->SetBoolField(TEXT("open"),Chest->IsChestOpen());Entry->SetBoolField(TEXT("glow_active"),Chest->IsGlowActive());Chests.Add(MakeShared<FJsonValueObject>(Entry));}
            if(auto* Bar=Cast<UInkBufferedProgressWidget>(Child)){Number(Entry,TEXT("fraction"),Bar->Fraction);Progress.Add(MakeShared<FJsonValueObject>(Entry));}
            Entry->SetBoolField(TEXT("keyboard_focused"),Child->HasKeyboardFocus());
            Entry->SetStringField(TEXT("clipping"),StaticEnum<EWidgetClipping>()->GetNameStringByValue(int64(Child->GetClipping())));
            if(const auto* CanvasSlot=Cast<UCanvasPanelSlot>(Child->Slot))Number(Entry,TEXT("z_order"),CanvasSlot->GetZOrder());
            Entry->SetBoolField(TEXT("in_confirmation"),bInConfirmation);
            const auto& Geometry=Child->GetCachedGeometry();const auto Size=Geometry.GetLocalSize();const auto Position=Widget->GetCachedGeometry().AbsoluteToLocal(Geometry.LocalToAbsolute(FVector2D::ZeroVector));
            Number(Entry,TEXT("x"),Position.X);Number(Entry,TEXT("y"),Position.Y);Number(Entry,TEXT("width"),Size.X);Number(Entry,TEXT("height"),Size.Y);
            const auto PaintedEnd=Widget->GetCachedGeometry().AbsoluteToLocal(Geometry.LocalToAbsolute(Size));
            Entry->SetBoolField(TEXT("geometry_arranged"),Size.X>0&&Size.Y>0);
            Entry->SetStringField(TEXT("paint_expectation"),TEXT("unknown"));
            if(auto Slate=Child->GetCachedWidget())if(const auto* Evidence=PaintEvidence.Find(Slate.Get()))
            {
                Entry->SetStringField(TEXT("paint_expectation"),Evidence->bKnown?(Evidence->bCulled?TEXT("culled"):TEXT("expected")):TEXT("unknown"));
                Entry->SetStringField(TEXT("culling_proof_ancestor"),Evidence->CulledAncestor);
                if(Evidence->bCulled)
                {
                    Number(Entry,TEXT("culled_left"),Evidence->CulledBounds.Left);Number(Entry,TEXT("culled_top"),Evidence->CulledBounds.Top);
                    Number(Entry,TEXT("culled_right"),Evidence->CulledBounds.Right);Number(Entry,TEXT("culled_bottom"),Evidence->CulledBounds.Bottom);
                    Number(Entry,TEXT("culled_clip_left"),Evidence->CulledClip.Left);Number(Entry,TEXT("culled_clip_top"),Evidence->CulledClip.Top);
                    Number(Entry,TEXT("culled_clip_right"),Evidence->CulledClip.Right);Number(Entry,TEXT("culled_clip_bottom"),Evidence->CulledClip.Bottom);
                }
                if(Evidence->bKnown)
                {
                    Number(Entry,TEXT("independent_left"),Evidence->Bounds.Left);Number(Entry,TEXT("independent_top"),Evidence->Bounds.Top);
                    Number(Entry,TEXT("independent_right"),Evidence->Bounds.Right);Number(Entry,TEXT("independent_bottom"),Evidence->Bounds.Bottom);
                    Number(Entry,TEXT("independent_clip_left"),Evidence->Clip.Left);Number(Entry,TEXT("independent_clip_top"),Evidence->Clip.Top);
                    Number(Entry,TEXT("independent_clip_right"),Evidence->Clip.Right);Number(Entry,TEXT("independent_clip_bottom"),Evidence->Clip.Bottom);
                }
            }
            Number(Entry,TEXT("painted_width"),PaintedEnd.X-Position.X);Number(Entry,TEXT("painted_height"),PaintedEnd.Y-Position.Y);
            if(auto* Button=Cast<UInkFrontendActionButton>(Child))
            {
                Entry->SetStringField(TEXT("action"),Button->Action.ToString());Entry->SetBoolField(TEXT("enabled"),Button->GetIsEnabled());
                Number(Entry,TEXT("descendant_text_count"),InkInventoryEditor::CountDescendantTexts(Button));
                const auto* Art=Button->GetStyle().Normal.GetResourceObject();Entry->SetStringField(TEXT("resource"),Art?Art->GetPathName():TEXT(""));
                InkInventoryEditor::DescribeMaterial(Art,Entry);
                if(const auto* Material=Cast<UMaterialInterface>(Art))
                {
                    float Mode=0;FLinearColor SourceSize;
                    if(Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Mode")),Mode))Number(Entry,TEXT("art_mode"),Mode);
                    if(Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("SourceSize")),SourceSize))
                    {
                        const float Scale=FMath::Min(Size.X/FMath::Max(1.f,SourceSize.R),Size.Y/FMath::Max(1.f,SourceSize.G));
                        Number(Entry,TEXT("paint_width"),SourceSize.R*Scale);Number(Entry,TEXT("paint_height"),SourceSize.G*Scale);
                    }
                }
                if(auto* Sizing=Cast<USizeBox>(Button->GetContent()))if(auto* Label=Cast<UTextBlock>(Sizing->GetContent()))
                {
                    Entry->SetStringField(TEXT("label"),Label->GetText().ToString());
                    Number(Entry,TEXT("label_desired_width"),Label->GetDesiredSize().X);
                    Number(Entry,TEXT("label_width"),Label->GetCachedGeometry().GetLocalSize().X);
                    Entry->SetBoolField(TEXT("label_auto_wrap"),Label->GetAutoWrapText());
                }
                Buttons.Add(MakeShared<FJsonValueObject>(Entry));
            }
            if(auto* Button=Cast<UButton>(Child);Button&&!Cast<UInkFrontendActionButton>(Child))
            {
                Entry->SetStringField(TEXT("widget_class"),Button->GetClass()->GetPathName());Entry->SetBoolField(TEXT("enabled"),Button->GetIsEnabled());
                FString Caption;TFunction<void(UWidget*)> Gather=[&](UWidget* W){if(auto* Label=Cast<UTextBlock>(W)){if(!Caption.IsEmpty())Caption+=TEXT("\n");Caption+=Label->GetText().ToString();}if(auto* Panel=Cast<UPanelWidget>(W))for(auto* Node:Panel->GetAllChildren())Gather(Node);};Gather(Button);Entry->SetStringField(TEXT("label"),Caption);
                if(const auto* Intent=Cast<URebuildActivityIntentButton>(Button)){Entry->SetStringField(TEXT("intent_action"),Intent->Request.Action.ToString());Entry->SetStringField(TEXT("intent_entry"),Intent->Request.EntryId.ToString());Entry->SetStringField(TEXT("intent_cycle"),Intent->Request.Cycle.ToString());Entry->SetBoolField(TEXT("intent_confirmed"),Intent->Request.bConfirmed);}
                Buttons.Add(MakeShared<FJsonValueObject>(Entry));
            }
            if(auto* Input=Cast<UEditableTextBox>(Child)){Entry->SetStringField(TEXT("text"),Input->GetText().ToString());Entry->SetStringField(TEXT("hint"),Input->GetHintText().ToString());Entry->SetBoolField(TEXT("enabled"),Input->GetIsEnabled());Inputs.Add(MakeShared<FJsonValueObject>(Entry));}
            if(auto* Text=Cast<UTextBlock>(Child)){Entry->SetStringField(TEXT("text"),Text->GetText().ToString());Number(Entry,TEXT("desired_width"),Text->GetDesiredSize().X);Number(Entry,TEXT("desired_height"),Text->GetDesiredSize().Y);Entry->SetBoolField(TEXT("auto_wrap"),Text->GetAutoWrapText());const auto Font=Text->GetFont();Entry->SetStringField(TEXT("font"),Font.FontObject?Font.FontObject->GetPathName():TEXT(""));Number(Entry,TEXT("font_size"),Font.Size);Texts.Add(MakeShared<FJsonValueObject>(Entry));}
            if(auto* Picture=Cast<UImage>(Child))
            {
                const auto* Resource=Picture->GetBrush().GetResourceObject();Entry->SetStringField(TEXT("resource"),Resource?Resource->GetPathName():TEXT(""));
                InkInventoryEditor::DescribeMaterial(Resource,Entry);
                Images.Add(MakeShared<FJsonValueObject>(Entry));
            }
            if(auto* Border=Cast<UBorder>(Child))
            {const auto* Resource=Border->Background.GetResourceObject();Entry->SetStringField(TEXT("resource"),Resource?Resource->GetPathName():TEXT(""));InkInventoryEditor::DescribeMaterial(Resource,Entry);Borders.Add(MakeShared<FJsonValueObject>(Entry));}
            if(auto* Handle=Cast<UInkInventoryScrollBar>(Child))
            {
                Entry->SetBoolField(TEXT("independent_handle"),true);
                Entry->SetBoolField(TEXT("enabled"),Handle->GetIsEnabled());
                Entry->SetStringField(TEXT("target"),Handle->TargetScrollBox?Handle->TargetScrollBox->GetName():TEXT(""));
                const auto& Style=Handle->GetWidgetStyle();TArray<TSharedPtr<FJsonValue>> Resources;
                for(const auto* Brush:{&Style.NormalThumbImage,&Style.HoveredThumbImage,&Style.DraggedThumbImage})
                    if(const auto* Resource=Brush->GetResourceObject())Resources.Add(MakeShared<FJsonValueString>(Resource->GetPathName()));
                Entry->SetArrayField(TEXT("resources"),Resources);Scrolls.Add(MakeShared<FJsonValueObject>(Entry));
            }
            if(auto* Scroll=Cast<UScrollBox>(Child))
            {
                Number(Entry,TEXT("offset"),Scroll->GetScrollOffset());
                Number(Entry,TEXT("end_offset"),Scroll->GetScrollOffsetOfEnd());
                const auto& Style=Scroll->GetWidgetBarStyle();TArray<TSharedPtr<FJsonValue>> Resources;
                for(const auto* Brush:{&Style.VerticalBackgroundImage,&Style.NormalThumbImage,&Style.HoveredThumbImage,&Style.DraggedThumbImage})
                    if(const auto* Resource=Brush->GetResourceObject())Resources.Add(MakeShared<FJsonValueString>(Resource->GetPathName()));
                Entry->SetArrayField(TEXT("resources"),Resources);Scrolls.Add(MakeShared<FJsonValueObject>(Entry));
            }
            if(Child->GetParent()&&Child->GetParent()->GetName()==TEXT("InventoryItemList"))Cells.Add(MakeShared<FJsonValueObject>(Entry));
        };
        TSet<const UUserWidget*> Visited;
        TFunction<void(const UUserWidget*)> Visit=[&](const UUserWidget* User)
        {
            if(!User||!User->WidgetTree||Visited.Contains(User))return;Visited.Add(User);
            User->WidgetTree->ForEachWidget([&](UWidget* Child){DescribeChild(Child);if(const auto* Nested=Cast<UUserWidget>(Child);Nested&&Nested!=User)Visit(Nested);});
        };
        Visit(Widget);Visit(Widget->AnchorStatusWidget.Get());
        Visit(Widget->ExtensionPages.FindRef(Widget->CurrentPage).Get());

    }
    Root->SetArrayField(TEXT("inputs"),Inputs);
    Root->SetArrayField(TEXT("buttons"),Buttons);Root->SetArrayField(TEXT("texts"),Texts);
    Root->SetArrayField(TEXT("images"),Images);
    Root->SetArrayField(TEXT("chests"),Chests);Root->SetArrayField(TEXT("progress"),Progress);
    Root->SetArrayField(TEXT("borders"),Borders);
    Root->SetArrayField(TEXT("scrolls"),Scrolls);Root->SetArrayField(TEXT("inventory_cells"),Cells);
    FString Result;FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Result));return Result;
}

bool UInkFrontendEditorLibrary::DragInventoryScrollThumb(UInkFrontendWidget* Widget,float FromFraction,float ToFraction)
{return DragFrontendScrollThumb(Widget,TEXT("InventoryScrollThumb"),FromFraction,ToFraction);}

bool UInkFrontendEditorLibrary::DragFrontendScrollThumb(UInkFrontendWidget* Widget,FName ThumbName,float FromFraction,float ToFraction)
{
    if(!Widget||!Widget->WidgetTree||!FSlateApplication::IsInitialized())return false;
    auto* Handle=Cast<UInkInventoryScrollBar>(Widget->WidgetTree->FindWidget(ThumbName));
    if(!Handle||!Handle->GetIsEnabled()||!Handle->GetCachedWidget())return false;
    const auto& Geometry=Handle->GetCachedGeometry();const FVector2D Size=Geometry.GetLocalSize();
    const FVector2D Start=Geometry.LocalToAbsolute(FVector2D(Size.X*.5,Size.Y*FromFraction));
    const FVector2D End=Geometry.LocalToAbsolute(FVector2D(Size.X*.5,Size.Y*ToFraction));
    auto& Slate=FSlateApplication::Get();Slate.SetCursorPos(Start);
    Slate.ProcessMouseMoveEvent(FPointerEvent(FSlateApplication::CursorPointerIndex,Start,Start,TSet<FKey>{},EKeys::Invalid,0,FModifierKeysState()));
    const auto Hit=Slate.LocateWindowUnderMouse(Start,Slate.GetInteractiveTopLevelWindows());
    if(!Hit.ContainsWidget(Handle->GetCachedWidget().Get()))return false;
    const bool Down=Slate.ProcessMouseButtonDownEvent(nullptr,FPointerEvent(FSlateApplication::CursorPointerIndex,Start,Start,TSet<FKey>{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0,FModifierKeysState()));
    for(int32 Step=1;Step<=12;++Step)
    {
        const FVector2D Position=FMath::Lerp(Start,End,Step/12.f);
        const FVector2D Previous=FMath::Lerp(Start,End,(Step-1)/12.f);Slate.SetCursorPos(Position);
        Slate.ProcessMouseMoveEvent(FPointerEvent(FSlateApplication::CursorPointerIndex,Position,Previous,TSet<FKey>{EKeys::LeftMouseButton},EKeys::Invalid,0,FModifierKeysState()));
    }
    Slate.ProcessMouseButtonUpEvent(FPointerEvent(FSlateApplication::CursorPointerIndex,End,End,TSet<FKey>{},EKeys::LeftMouseButton,0,FModifierKeysState()));
    return Down;
}

bool UInkFrontendEditorLibrary::SetPIEViewportSize(int32 Width,int32 Height)
{
    if(!GEditor||!GEditor->GameViewport||Width<320||Height<320)return false;
    auto* Viewport=GEditor->GameViewport->GetGameViewport();if(!Viewport)return false;
    const FIntPoint Current=Viewport->GetSizeXY();
    auto Window=Viewport->FindWindow();if(!Window)return false;
    // PIE windows inherit desktop-sized limits. QA must also exercise phone
    // resolutions taller than this monitor without changing editor preferences.
    auto Limits=Window->GetSizeLimits();
    Limits.SetMaxWidth(FMath::Max(Limits.GetMaxWidth().Get(0.f),float(Width+128)));
    Limits.SetMaxHeight(FMath::Max(Limits.GetMaxHeight().Get(0.f),float(Height+128)));
    Window->SetSizeLimits(Limits);
    // PIE adds a small Slate border; resize by the measured viewport delta instead of
    // treating the outer client area as the rendered game area.
    Window->Resize(Window->GetClientSizeInScreen()+FVector2D(Width-Current.X,Height-Current.Y));return true;
}

bool UInkFrontendEditorLibrary::BeginFrontendPIE(int32 Width,int32 Height)
{
    if(!GEditor||GEditor->PlayWorld||Width<320||Height<320)return false;
    auto* Settings=DuplicateObject<ULevelEditorPlaySettings>(GetDefault<ULevelEditorPlaySettings>(),GetTransientPackage());
    Settings->NewWindowWidth=Width;Settings->NewWindowHeight=Height;Settings->CenterNewWindow=true;Settings->GameGetsMouseControl=true;
    FRequestPlaySessionParams Params;Params.WorldType=EPlaySessionWorldType::PlayInEditor;Params.EditorPlaySettings=Settings;Params.bAllowOnlineSubsystem=false;
    GEditor->RequestPlaySession(Params);return true;
}

bool UInkFrontendEditorLibrary::CaptureFrontendViewport(const FString& Filename)
{
    if(!GEditor||!GEditor->GameViewport||!FSlateApplication::IsInitialized())return false;
    const auto ViewportWidget=GEditor->GameViewport->GetGameViewportWidget();if(!ViewportWidget)return false;
    TArray<FColor> Pixels;FIntVector Size;
    const auto Window=FSlateApplication::Get().FindWidgetWindow(ViewportWidget.ToSharedRef());if(!Window)return false;
    // Repaint from the Slate window root. Subtree screenshots reuse cached child
    // layers without their parent clipping state and can omit parts of glyphs.
    if(!FSlateApplication::Get().TakeScreenshot(Window.ToSharedRef(),Pixels,Size)||Size.X<=0||Size.Y<=0)return false;
    TArray64<uint8> WindowPng;FImageUtils::PNGCompressImageArray(Size.X,Size.Y,Pixels,WindowPng);
    FFileHelper::SaveArrayToFile(WindowPng,*Filename.Replace(TEXT(".png"),TEXT("-window.png")));
    const auto& Geometry=ViewportWidget->GetCachedGeometry();
    const auto Origin=Geometry.GetAbsolutePosition()-Window->GetCachedGeometry().GetAbsolutePosition();
    const auto Extent=Geometry.GetAbsoluteSize();
    const int32 X=FMath::Clamp(FMath::RoundToInt(Origin.X),0,Size.X-1),Y=FMath::Clamp(FMath::RoundToInt(Origin.Y),0,Size.Y-1);
    const int32 W=FMath::Clamp(FMath::RoundToInt(Extent.X),1,Size.X-X),H=FMath::Clamp(FMath::RoundToInt(Extent.Y),1,Size.Y-Y);
    TArray<FColor> Cropped;Cropped.SetNumUninitialized(W*H);
    for(int32 Row=0;Row<H;++Row)FMemory::Memcpy(Cropped.GetData()+Row*W,Pixels.GetData()+(Y+Row)*Size.X+X,W*sizeof(FColor));
    Pixels=MoveTemp(Cropped);Size=FIntVector(W,H,0);
    TArray64<uint8> Png;FImageUtils::PNGCompressImageArray(Size.X,Size.Y,Pixels,Png);
    return FFileHelper::SaveArrayToFile(Png,*Filename);
}

bool UInkFrontendEditorLibrary::SetFrontendSafePadding(UInkFrontendWidget* Widget,FMargin Padding)
{
    auto* Canvas=Widget?Widget->GetWidgetFromName(TEXT("SafeCanvas")):nullptr;
    auto* Slot=Canvas?Cast<USafeZoneSlot>(Canvas->Slot):nullptr;
    if(!Slot)return false;
    Slot->SetPadding(Padding);return true;
}

bool UInkFrontendEditorLibrary::SetFrontendTestAnchorName(UInkFrontendWidget* Widget,const FString& Name)
{
    if(!Widget||!FParse::Param(FCommandLine::Get(),TEXT("GWLocalTest"))||!FParse::Param(FCommandLine::Get(),TEXT("WulingPrototype")))return false;
    auto* State=Widget->GetGameInstance()?Widget->GetGameInstance()->GetSubsystem<UCultivationSubsystem>():nullptr;
    if(!State)return false;
    const auto S=State->GetParticipantSnapshot(State->GetAnchorId());
    return State->RegisterParticipant(State->GetAnchorId(),Name,S.AvatarUrl,true);
}

bool UInkFrontendEditorLibrary::SetFrontendTestAnchorAvatar(UInkFrontendWidget* Widget,const FString& Url,UTexture2D* Texture)
{
#if WITH_DEV_AUTOMATION_TESTS
    if(!Widget||!Texture||!Url.StartsWith(TEXT("https://fixture.invalid/"))||!FParse::Param(FCommandLine::Get(),TEXT("GWLocalTest"))||!FParse::Param(FCommandLine::Get(),TEXT("WulingPrototype")))return false;
    auto* Instance=Widget->GetGameInstance();auto* State=Instance?Instance->GetSubsystem<UCultivationSubsystem>():nullptr;
    auto* Host=Instance?Instance->GetSubsystem<ULiveInteractionSubsystem>():nullptr;
    if(!State||!Host||!Host->IsDevelopmentSimulation())return false;
    FLiveInteractionTestAdapter::CacheAvatar(*Host,Url,Texture);
    const auto Profile=State->GetParticipantSnapshot(State->GetAnchorId());
    return State->RegisterParticipant(State->GetAnchorId(),Profile.DisplayName,Url,true);
#else
    return false;
#endif
}

bool UInkFrontendEditorLibrary::ScrollToFrontendWidget(UInkFrontendWidget* Widget,FName Name)
{
    auto* Target=InkInventoryEditor::FindCurrentNamedWidget(Widget,Name);if(!Target)return false;
    TSet<const UWidget*> Seen;UWidget* Current=Target;bool Scrolled=false;
    while(Current&&!Seen.Contains(Current))
    {
        Seen.Add(Current);UWidget* Parent=Current->GetParent();
        if(!Parent){auto* Owner=Current->GetTypedOuter<UUserWidget>();if(!Owner||Owner==Current)break;Parent=Owner;}
        if(auto* Scroll=Cast<UScrollBox>(Parent)){Scroll->ScrollWidgetIntoView(Target,false,EDescendantScrollDestination::TopOrLeft);Scrolled=true;}Current=Parent;
    }
    return Scrolled;
}

bool UInkFrontendEditorLibrary::RefreshInventoryFrontendAsset(const FString& Path)
{
    if(!Path.StartsWith(TEXT("/Game/newUI/Ink/Widgets/")))return false;
    auto* Blueprint=LoadObject<UWidgetBlueprint>(nullptr,*(Path+TEXT(".")+FPackageName::GetLongPackageAssetName(Path)));
    if(!Blueprint||!Blueprint->WidgetTree||Blueprint->ParentClass!=UInkFrontendWidget::StaticClass())return false;
    TMap<FName,TWeakObjectPtr<UWidget>> PreservedWidgets;
    Blueprint->WidgetTree->ForEachWidget([&](UWidget* Child)
    {
        for(const UWidget* Ancestor=Child;Ancestor;Ancestor=Ancestor->GetParent())if(Ancestor->GetFName()==TEXT("Inventory"))return;
        PreservedWidgets.Add(Child->GetFName(),Child);
    });
    // RebuildInventoryPage replaces only its named page root in the existing switcher.
    auto* Template=NewObject<UInkFrontendWidget>();Template->RebuildInventoryPage(Blueprint->WidgetTree);
    for(const auto& Pair:PreservedWidgets)if(Blueprint->WidgetTree->FindWidget(Pair.Key)!=Pair.Value.Get())
    {
        UE_LOG(LogTemp,Error,TEXT("Inventory migration replaced unrelated authored widget %s"),*Pair.Key.ToString());return false;
    }
    // Match UE's ValidateAndFixUpVariableGuids source enumeration exactly. A
    // partially populated map triggers ensures before compilation can repair it.
    // Retain valid existing identities, add deterministic identities for new
    // inventory widgets, and discard entries no longer in the Blueprint source.
    TMap<FName,FGuid> ReconciledGuids;TSet<FGuid> SeenGuids;
    auto TrackSourceObject=[&](const UObject* Object)
    {
        if(!Object)return;
        const FName Name=Object->GetFName();if(ReconciledGuids.Contains(Name))return;
        const FGuid* Existing=Blueprint->WidgetVariableNameToGuidMap.Find(Name);
        FGuid Guid=Existing&&Existing->IsValid()&&!SeenGuids.Contains(*Existing)?*Existing:FGuid::NewDeterministicGuid(Object->GetPathName());
        for(int64 Seed=1;!Guid.IsValid()||SeenGuids.Contains(Guid);++Seed)Guid=FGuid::NewDeterministicGuid(Object->GetPathName(),Seed);
        ReconciledGuids.Add(Name,Guid);SeenGuids.Add(Guid);
    };
    Blueprint->ForEachSourceWidget([&](UWidget* Child){TrackSourceObject(Child);});
    for(auto Animation:Blueprint->Animations)TrackSourceObject(Animation);
    Blueprint->WidgetVariableNameToGuidMap=MoveTemp(ReconciledGuids);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);FKismetEditorUtilities::CompileBlueprint(Blueprint);
    Blueprint->MarkPackageDirty();return Blueprint->Status!=BS_Error;
}

bool UInkFrontendEditorLibrary::RefreshLeaderboardFrontendAsset(const FString& Path)
{
    if(!Path.StartsWith(TEXT("/Game/newUI/Ink/Widgets/")))return false;
    auto* Blueprint=LoadObject<UWidgetBlueprint>(nullptr,*(Path+TEXT(".")+FPackageName::GetLongPackageAssetName(Path)));
    if(!Blueprint||!Blueprint->WidgetTree||Blueprint->ParentClass!=UInkFrontendWidget::StaticClass())return false;
    TMap<FName,TWeakObjectPtr<UWidget>> PreservedWidgets;
    Blueprint->WidgetTree->ForEachWidget([&](UWidget* Child)
    {
        for(const UWidget* Ancestor=Child;Ancestor;Ancestor=Ancestor->GetParent())if((Ancestor->GetFName()==TEXT("Leaderboard")||Ancestor->GetFName()==TEXT("RankHUDScale")))return;
        PreservedWidgets.Add(Child->GetFName(),Child);
    });
    // RebuildLeaderboardSurfaces replaces only its named page root in the existing switcher.
    auto* Template=NewObject<UInkFrontendWidget>();Template->RebuildLeaderboardSurfaces(Blueprint->WidgetTree);
    for(const auto& Pair:PreservedWidgets)if(Blueprint->WidgetTree->FindWidget(Pair.Key)!=Pair.Value.Get())
    {
        UE_LOG(LogTemp,Error,TEXT("Leaderboard migration replaced unrelated authored widget %s"),*Pair.Key.ToString());return false;
    }
    // Match UE's ValidateAndFixUpVariableGuids source enumeration exactly. A
    // partially populated map triggers ensures before compilation can repair it.
    // Retain valid existing identities, add deterministic identities for new
    // inventory widgets, and discard entries no longer in the Blueprint source.
    TMap<FName,FGuid> ReconciledGuids;TSet<FGuid> SeenGuids;
    auto TrackSourceObject=[&](const UObject* Object)
    {
        if(!Object)return;
        const FName Name=Object->GetFName();if(ReconciledGuids.Contains(Name))return;
        const FGuid* Existing=Blueprint->WidgetVariableNameToGuidMap.Find(Name);
        FGuid Guid=Existing&&Existing->IsValid()&&!SeenGuids.Contains(*Existing)?*Existing:FGuid::NewDeterministicGuid(Object->GetPathName());
        for(int64 Seed=1;!Guid.IsValid()||SeenGuids.Contains(Guid);++Seed)Guid=FGuid::NewDeterministicGuid(Object->GetPathName(),Seed);
        ReconciledGuids.Add(Name,Guid);SeenGuids.Add(Guid);
    };
    Blueprint->ForEachSourceWidget([&](UWidget* Child){TrackSourceObject(Child);});
    for(auto Animation:Blueprint->Animations)TrackSourceObject(Animation);
    Blueprint->WidgetVariableNameToGuidMap=MoveTemp(ReconciledGuids);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);FKismetEditorUtilities::CompileBlueprint(Blueprint);
    Blueprint->MarkPackageDirty();return Blueprint->Status!=BS_Error;
}

bool UInkFrontendEditorLibrary::HoverAction(UInkFrontendWidget* Widget,FName Action)
{
    if(!Widget)return false;
    for(auto Button:Widget->ActionButtons)if(Button&&Button->Action==Action&&Button->GetIsEnabled()&&InkInventoryEditor::MoveTo(Button))return true;
    return false;
}

bool UInkFrontendEditorLibrary::MoveFrontendPointerOutside(UInkFrontendWidget* Widget)
{
    if(!Widget||!FSlateApplication::IsInitialized())return false;
    auto& Slate=FSlateApplication::Get();const auto Previous=Slate.GetCursorPos();
    const FVector2D Position=Widget->GetCachedGeometry().LocalToAbsolute(FVector2D(-64,-64));Slate.SetCursorPos(Position);
    Slate.ProcessMouseMoveEvent(FPointerEvent(FSlateApplication::CursorPointerIndex,Position,Previous,TSet<FKey>{},EKeys::Invalid,0,FModifierKeysState()));return true;
}

bool UInkFrontendEditorLibrary::TypeInventorySearch(UInkFrontendWidget* Widget,const FString& Text)
{
    auto* Search=Widget?Cast<UEditableTextBox>(Widget->GetWidgetFromName(Widget->UsesRebuiltPresentation()?TEXT("NewUIInventorySearch"):TEXT("InventorySearch"))):nullptr;
    if(!Search||!InkInventoryEditor::MoveTo(Search))return false;
    auto& Slate=FSlateApplication::Get();Slate.SetKeyboardFocus(Search->GetCachedWidget(),EFocusCause::SetDirectly);
    const FKeyEvent SelectAll(EKeys::A,FModifierKeysState(false,false,true,false,false,false,false,false,false),0,false,0,0);
    Slate.ProcessKeyDownEvent(SelectAll);Slate.ProcessKeyUpEvent(SelectAll);
    const FKeyEvent Backspace(EKeys::BackSpace,FModifierKeysState(),0,false,0,0);
    Slate.ProcessKeyDownEvent(Backspace);Slate.ProcessKeyUpEvent(Backspace);
    for(const TCHAR Character:Text)Slate.ProcessKeyCharEvent(FCharacterEvent(Character,FModifierKeysState(),0,false));
    return Search->GetText().ToString()==Text;
}

bool UInkFrontendEditorLibrary::TypeFrontendText(UInkFrontendWidget* Widget,FName Name,const FString& Text)
{
    auto* Input=Cast<UEditableTextBox>(InkInventoryEditor::FindCurrentNamedWidget(Widget,Name));if(!Input||!InkInventoryEditor::MoveTo(Input))return false;
    auto& Slate=FSlateApplication::Get();Slate.SetKeyboardFocus(Input->GetCachedWidget(),EFocusCause::SetDirectly);
    const FKeyEvent SelectAll(EKeys::A,FModifierKeysState(false,false,true,false,false,false,false,false,false),0,false,0,0);Slate.ProcessKeyDownEvent(SelectAll);Slate.ProcessKeyUpEvent(SelectAll);
    const FKeyEvent Backspace(EKeys::BackSpace,FModifierKeysState(),0,false,0,0);Slate.ProcessKeyDownEvent(Backspace);Slate.ProcessKeyUpEvent(Backspace);
    for(const TCHAR Character:Text)Slate.ProcessKeyCharEvent(FCharacterEvent(Character,FModifierKeysState(),0,false));return Input->GetText().ToString()==Text;
}

FString UInkFrontendEditorLibrary::DescribeInventory(UInkFrontendWidget* Widget)
{
    auto Root=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Entries,Controls;
    if(Widget)
    {
        Root->SetStringField(TEXT("page"),Widget->CurrentPage.ToString());
        Root->SetStringField(TEXT("selected_entry"),Widget->SelectedInventoryEntry.ToString());
        Root->SetStringField(TEXT("hovered_entry"),Widget->HoveredInventoryEntry.ToString());
        if(FSlateApplication::IsInitialized())
        {
            const FVector2D Cursor=FSlateApplication::Get().GetCursorPos();
            const FVector2D Local=Widget->GetCachedGeometry().AbsoluteToLocal(Cursor);
            Root->SetNumberField(TEXT("cursor_screen_x"),Cursor.X);Root->SetNumberField(TEXT("cursor_screen_y"),Cursor.Y);
            Root->SetNumberField(TEXT("cursor_widget_x"),Local.X);Root->SetNumberField(TEXT("cursor_widget_y"),Local.Y);
        }
        Root->SetStringField(TEXT("category"),Widget->InventoryCategory.ToString());
        Root->SetNumberField(TEXT("quantity"),Widget->InventoryActionQuantity);
        Root->SetStringField(TEXT("quantity_preset"),Widget->InventoryQuantityPreset.ToString());
        Root->SetBoolField(TEXT("multi_select"),Widget->bInventoryMultiSelect);
        TArray<TSharedPtr<FJsonValue>> BatchSelection;
        for(const auto& Id:Widget->InventoryBatchSelection)BatchSelection.Add(MakeShared<FJsonValueString>(Id.ToString()));
        Root->SetArrayField(TEXT("batch_selection"),BatchSelection);
        Root->SetNumberField(TEXT("page_index"),Widget->InventoryPageIndex);
        Root->SetBoolField(TEXT("list_view"),Widget->bInventoryListView);
        Root->SetBoolField(TEXT("sort_ascending"),Widget->bInventorySortAscending);
        Root->SetBoolField(TEXT("filter_usable"),Widget->bInventoryFilterUsable);
        Root->SetBoolField(TEXT("filter_stackable"),Widget->bInventoryFilterStackable);
        Root->SetStringField(TEXT("search"),Widget->InventorySearchText);
        Root->SetBoolField(TEXT("tooltip_exists"),Widget->GetWidgetFromName(TEXT("InventoryTooltip"))!=nullptr);
        Root->SetBoolField(TEXT("detail_exists"),Widget->GetWidgetFromName(TEXT("InventoryDetailPanel"))!=nullptr);
        Root->SetBoolField(TEXT("selection_hint_exists"),Widget->GetWidgetFromName(TEXT("InventorySelectionHint"))!=nullptr||Widget->GetWidgetFromName(TEXT("InventorySelectionHintFrame"))!=nullptr);
        Root->SetBoolField(TEXT("tooltip_visible"),InkInventoryEditor::IsEffectivelyVisible(Widget->GetWidgetFromName(TEXT("InventoryTooltip"))));
        Root->SetBoolField(TEXT("detail_visible"),InkInventoryEditor::IsEffectivelyVisible(Widget->GetWidgetFromName(TEXT("InventoryDetailPanel"))));
        if(auto* Model=Widget->GetInventoryModel())
        {
            Root->SetNumberField(TEXT("unlocked"),Model->GetUnlockedSlotCount());Root->SetNumberField(TEXT("occupied"),Model->GetOccupiedSlotCount());
            for(const auto& Entry:Model->GetEntries())
            {
                auto Value=MakeShared<FJsonObject>();Value->SetStringField(TEXT("id"),Entry.EntryId.ToString());
                Value->SetNumberField(TEXT("slot"),Entry.SlotIndex);Value->SetBoolField(TEXT("new"),Entry.bIsNew);Value->SetBoolField(TEXT("equipped"),Entry.bEquipped);
                Value->SetStringField(TEXT("item_id"),Entry.Item.ItemId.ToString());Value->SetStringField(TEXT("name"),Entry.Item.Name.ToString());
                Value->SetStringField(TEXT("category"),Entry.Item.Category.ToString());Value->SetNumberField(TEXT("count"),Entry.Item.Count);
                Value->SetBoolField(TEXT("can_use"),Model->CanUseEntry(Entry.EntryId));Value->SetBoolField(TEXT("can_equip"),Model->CanEquipEntry(Entry.EntryId));
                auto Item=MakeShared<FJsonObject>();FJsonObjectConverter::UStructToJsonObject(FInkInventoryItem::StaticStruct(),&Entry.Item,Item);
                Value->SetObjectField(TEXT("item"),Item);Entries.Add(MakeShared<FJsonValueObject>(Value));
            }
        }
        if(Widget->WidgetTree)Widget->WidgetTree->ForEachWidget([&](UWidget* Child)
        {
            auto* Button=Cast<UInkFrontendActionButton>(Child);if(!Button)return;
            const FString Action=Button->Action.ToString();if(!Action.StartsWith(TEXT("Inventory"))&&!Action.StartsWith(TEXT("Filter")))return;
            auto Value=MakeShared<FJsonObject>();Value->SetStringField(TEXT("name"),Button->GetName());Value->SetStringField(TEXT("action"),Action);
            Value->SetBoolField(TEXT("visible"),InkInventoryEditor::IsEffectivelyVisible(Button));Value->SetBoolField(TEXT("enabled"),Button->GetIsEnabled());
            Value->SetBoolField(TEXT("hovered"),Button->IsHovered());Value->SetNumberField(TEXT("opacity"),Button->GetRenderOpacity());
            const auto& Style=Button->GetStyle();const auto* Normal=Style.Normal.GetResourceObject();const auto* Disabled=Style.Disabled.GetResourceObject();
            Value->SetStringField(TEXT("normal_resource"),Normal?Normal->GetPathName():TEXT(""));Value->SetStringField(TEXT("disabled_resource"),Disabled?Disabled->GetPathName():TEXT(""));
            const auto* Hovered=Style.Hovered.GetResourceObject();const auto* Pressed=Style.Pressed.GetResourceObject();
            Value->SetStringField(TEXT("hovered_resource"),Hovered?Hovered->GetPathName():TEXT(""));Value->SetStringField(TEXT("pressed_resource"),Pressed?Pressed->GetPathName():TEXT(""));
            const FLinearColor NormalTint=Style.Normal.TintColor.GetSpecifiedColor(),DisabledTint=Style.Disabled.TintColor.GetSpecifiedColor();
            Value->SetNumberField(TEXT("normal_luminance"),NormalTint.GetLuminance());Value->SetNumberField(TEXT("disabled_luminance"),DisabledTint.GetLuminance());
            Controls.Add(MakeShared<FJsonValueObject>(Value));
        });
    }
    Root->SetArrayField(TEXT("entries"),Entries);Root->SetArrayField(TEXT("controls"),Controls);
    FString Result;FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Result));return Result;
}

FString UInkFrontendEditorLibrary::VerifyDevelopmentMutationRefusal(UObject* Context)
{
    UWorld* World=GEngine->GetWorldFromContextObject(Context,EGetWorldErrorMode::ReturnNull);auto* Mode=World?World->GetAuthGameMode<ALiveWeaponGameMode>():nullptr;
    if(!Mode||!Mode->Audience||!Mode->Swarm)return TEXT("{}");
    auto* PC=Cast<ALiveAnchorController>(World->GetFirstPlayerController());if(!PC)return TEXT("{}");
    int32 BeforeMonsters=0;for(TActorIterator<ALiveMonster> It(World);It;++It)++BeforeMonsters;
    const int32 BeforeAudience=Mode->Swarm->GetParticipantCount();
    const FString SavedLine=FCommandLine::Get();ON_SCOPE_EXIT{FCommandLine::Set(*SavedLine);};FCommandLine::Set(TEXT("-GWLocalTest"));
    const bool ClearAccepted=Mode->Audience->ClearSimulatedAudience();PC->LiveSpawnMonsters(7);PC->LiveSkill(TEXT("viewer_0000"),TEXT("QA_Unauthorized"));PC->LiveAudience(10);PC->LiveComment(TEXT("bad_id"),TEXT("加入"));
    int32 AfterMonsters=0;for(TActorIterator<ALiveMonster> It(World);It;++It)++AfterMonsters;
    auto Root=MakeShared<FJsonObject>();Root->SetBoolField(TEXT("clear_refused"),!ClearAccepted);Root->SetBoolField(TEXT("f4_refused"),BeforeMonsters==AfterMonsters);Root->SetBoolField(TEXT("audience_refused"),BeforeAudience==Mode->Swarm->GetParticipantCount());
    FString Result;FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Result));return Result;
}

bool UInkFrontendEditorLibrary::ClickLabeledButton(UInkFrontendWidget* Widget,const FText& Caption)
{
    if(!Widget||!FSlateApplication::IsInitialized())return false;
    TArray<UButton*> Candidates;TSet<const UUserWidget*> Visited;
    TFunction<bool(UWidget*)> HasCaption=[&](UWidget* Child)
    {
        if(const auto* Text=Cast<UTextBlock>(Child))return Text->GetText().EqualTo(Caption);
        if(const auto* Panel=Cast<UPanelWidget>(Child))for(auto* Sub:Panel->GetAllChildren())if(HasCaption(Sub))return true;
        return false;
    };
    TFunction<void(UUserWidget*)> Visit=[&](UUserWidget* User)
    {
        if(!User||!User->WidgetTree||Visited.Contains(User))return;Visited.Add(User);
        User->WidgetTree->ForEachWidget([&](UWidget* Child)
        {
            if(auto* Button=Cast<UButton>(Child);Button&&Button->GetIsEnabled()&&InkInventoryEditor::IsEffectivelyVisible(Button)&&HasCaption(Button->GetContent()))Candidates.Add(Button);
            if(auto* Nested=Cast<UUserWidget>(Child);Nested&&Nested!=User)Visit(Nested);
        });
    };
    Visit(Widget);
    if(Candidates.Num()!=1)return false;
    auto* Button=Candidates[0];if(!InkInventoryEditor::MoveTo(Button))return false;
    const auto& Geometry=Button->GetCachedGeometry();const FVector2D P=Geometry.LocalToAbsolute(Geometry.GetLocalSize()*.5f);auto& Slate=FSlateApplication::Get();
    const auto Hit=Slate.LocateWindowUnderMouse(P,Slate.GetInteractiveTopLevelWindows());if(!Hit.ContainsWidget(Button->GetCachedWidget().Get()))return false;
    const bool Down=Slate.ProcessMouseButtonDownEvent(nullptr,FPointerEvent(FSlateApplication::CursorPointerIndex,P,P,TSet<FKey>{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0,FModifierKeysState()));
    Slate.ProcessMouseButtonUpEvent(FPointerEvent(FSlateApplication::CursorPointerIndex,P,P,TSet<FKey>{},EKeys::LeftMouseButton,0,FModifierKeysState()));return Down;
}
