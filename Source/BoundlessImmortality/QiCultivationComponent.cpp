#include "QiCultivationComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraSystemInstanceController.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraEmitterInstance.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "UObject/ObjectSaveContext.h"
namespace {
const TCHAR* Types[]={TEXT("White"),TEXT("Fire"),TEXT("Wind"),TEXT("Earth"),TEXT("Water"),TEXT("Ice"),TEXT("Lightning")};
float Safe(float V,float Low,float High,float Default){return FMath::IsFinite(V)?FMath::Clamp(V,Low,High):Default;}
}
UQiCultivationComponent::UQiCultivationComponent()
{
    PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.TickGroup=TG_PostUpdateWork;
    bTickInEditor=true;ToggleKey=EKeys::C;LiveCounts.Init(0,7);
}
FQiOrbSettings* UQiCultivationComponent::Settings(int32 Type)
{
    switch(Type){case 0:return &WhiteQi;case 1:return &FireQi;case 2:return &WindQi;case 3:return &EarthQi;case 4:return &WaterQi;case 5:return &IceQi;case 6:return &LightningQi;default:return nullptr;}
}
const FQiOrbSettings* UQiCultivationComponent::Settings(int32 Type)const{return const_cast<UQiCultivationComponent*>(this)->Settings(Type);}
bool UQiCultivationComponent::IsPreviewWorld()const
{return GetWorld()&&(GetWorld()->WorldType==EWorldType::Editor||GetWorld()->WorldType==EWorldType::EditorPreview);}
void UQiCultivationComponent::EnsureEffect()
{
    if(IsTemplate()||!GetOwner()||!GetOwner()->GetRootComponent()||!GetWorld())return;
    if(!QiSystem){ReleaseEffect();return;}
    if(!IsValid(Effect))
    {
        Effect=NewObject<UNiagaraComponent>(GetOwner(),NAME_None,RF_Transient|RF_DuplicateTransient|RF_TextExportTransient);
        Effect->SetAutoActivate(false);Effect->SetupAttachment(GetOwner()->GetRootComponent());
        Effect->bTickInEditor=true;Effect->SetAsset(QiSystem);Effect->RegisterComponent();
    }
    else if(Effect->GetAsset()!=QiSystem){CancelCultivation();Effect->SetAsset(QiSystem);}
    if((!AuraMaterial&&Aura)||(AuraMaterial&&(!Aura||Aura->Parent!=AuraMaterial)))
    {
        for(int32 I=0;I<AuraMeshes.Num();++I)
            if(IsValid(AuraMeshes[I])&&AuraMeshes[I]->GetOverlayMaterial()==Aura)AuraMeshes[I]->SetOverlayMaterial(PreviousOverlays[I]);
        AuraMeshes.Empty();PreviousOverlays.Empty();Aura=AuraMaterial?UMaterialInstanceDynamic::Create(AuraMaterial,this):nullptr;
    }
}
void UQiCultivationComponent::OnRegister()
{
    Super::OnRegister();if(IsTemplate())return;EnsureEffect();ApplySettings();
#if WITH_EDITOR
    // 编辑器角色没有游戏生命周期；使用弱引用的编辑器预览更新，蓝图重建时注销。
    if(IsPreviewWorld()&&!PreviewTicker.IsValid())
        PreviewTicker=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak=TWeakObjectPtr<UQiCultivationComponent>(this)](float Delta)
        {if(auto* C=Weak.Get()){C->TickEditorPreview(Delta);return true;}return false;}));
#endif
}
void UQiCultivationComponent::OnUnregister()
{
#if WITH_EDITOR
    FTSTicker::GetCoreTicker().RemoveTicker(PreviewTicker);PreviewTicker.Reset();
#endif
    ReleaseEffect();Super::OnUnregister();
}
void UQiCultivationComponent::PreSave(FObjectPreSaveContext SaveContext)
{
    // 预览的临时 MID 不写进角色网格，避免覆盖原资产材质。下一次编辑器 tick 自动恢复预览。
    if(IsPreviewWorld())CancelCultivation();Super::PreSave(SaveContext);
}
void UQiCultivationComponent::BeginPlay(){CancelCultivation();EnsureEffect();ApplySettings();Super::BeginPlay();}
void UQiCultivationComponent::AttachAura()
{
    if(!Aura||!GetOwner())return;
    TArray<USkeletalMeshComponent*> Meshes;GetOwner()->GetComponents(Meshes);
    for(auto* Mesh:Meshes)
    {
        if(Mesh->bHiddenInGame||!Mesh->IsVisible()||AuraMeshes.Contains(Mesh))continue;
        AuraMeshes.Add(Mesh);PreviousOverlays.Add(Mesh->GetOverlayMaterial());Mesh->SetOverlayMaterial(Aura);
    }
}
void UQiCultivationComponent::StartCultivation()
{
    EnsureEffect();if(bCultivating||!Effect)return;
    const bool Resume=bDraining;
    bDraining=false;AuraFadeRemaining=0;bCultivating=true;ApplySettings();
    if(!Resume||!Effect->IsActive())Effect->Activate(true);
    AttachAura();UpdateAura();
}
void UQiCultivationComponent::StopCultivation()
{
    if(bDraining)return;
    bCultivating=false;
    if(!IsValid(Effect)){CancelCultivation();return;}
    bDraining=true;AuraFadeRemaining=0;
    // Keep the CPU simulation alive; deactivation can kill particles through emitter settings.
    for(const TCHAR* Type:Types)Effect->SetVariableFloat(FName(FString::Printf(TEXT("User.%sCount"),Type)),0);
    UpdateAura();
}
void UQiCultivationComponent::CancelCultivation()
{
    bCultivating=false;bDraining=false;AuraFadeRemaining=0;LastParticleTotal=0;
    if(IsValid(Effect))Effect->DeactivateImmediate();
    for(int32 I=0;I<AuraMeshes.Num();++I)
        if(IsValid(AuraMeshes[I])&&AuraMeshes[I]->GetOverlayMaterial()==Aura)AuraMeshes[I]->SetOverlayMaterial(PreviousOverlays[I]);
    AuraMeshes.Empty();PreviousOverlays.Empty();LiveCounts.Init(0,7);
    if(Aura)Aura->SetScalarParameterValue(TEXT("Strength"),0);
}
void UQiCultivationComponent::ReleaseEffect(){CancelCultivation();if(IsValid(Effect))Effect->DestroyComponent();Effect=nullptr;Aura=nullptr;}
void UQiCultivationComponent::ToggleCultivation(){if(bCultivating)StopCultivation();else StartCultivation();}
void UQiCultivationComponent::ConfigureQi(int32 Type,int32 Count,float Range)
{if(auto* S=Settings(Type)){S->Count=FMath::Clamp(Count,0,64);S->Range=Safe(Range,20,2000,240);ApplySettings();}}
void UQiCultivationComponent::SetQiCount(EQiElement Element,int32 Count)
{if(auto* S=Settings(int32(Element)))ConfigureQi(int32(Element),Count,S->Range);}
void UQiCultivationComponent::SetQiRange(EQiElement Element,float Range)
{if(auto* S=Settings(int32(Element)))ConfigureQi(int32(Element),S->Count,Range);}
int32 UQiCultivationComponent::GetQiCount(EQiElement Element)const
{const auto* S=Settings(int32(Element));return S?FMath::Clamp(S->Count,0,64):0;}
void UQiCultivationComponent::SetAuraSettings(float Size,float Width,float Intensity,float Speed)
{AuraSize=Safe(Size,0,12,1.5);AuraWidth=Safe(Width,.05,3,1);AuraIntensity=Safe(Intensity,0,5,1);AuraSpeed=Safe(Speed,0,5,1);ApplySettings();}
void UQiCultivationComponent::ApplySettings()
{
    if(IsRegistered())EnsureEffect();
    Duration=Safe(Duration,1,20,5);SpawnHeight=Safe(SpawnHeight,0,1000,100);OrbSize=Safe(OrbSize,8,120,44);
    if(Effect)
    {
        Effect->SetRelativeLocation(ConvergenceOffset.ContainsNaN()?FVector(0,0,15):ConvergenceOffset);
        Effect->SetVariableFloat(TEXT("User.Duration"),Duration);Effect->SetVariableFloat(TEXT("User.SpawnHeight"),SpawnHeight);Effect->SetVariableFloat(TEXT("User.OrbSize"),OrbSize);
    }
    for(int32 I=0;I<7;++I)
    {
        auto* S=Settings(I);S->Count=FMath::Clamp(S->Count,0,64);S->Range=Safe(S->Range,20,2000,240);
        if(Effect){Effect->SetVariableFloat(FName(FString::Printf(TEXT("User.%sCount"),Types[I])),bDraining?0:S->Count);Effect->SetVariableFloat(FName(FString::Printf(TEXT("User.%sRange"),Types[I])),S->Range);}
    }
    if(Aura)
    {
        Aura->SetScalarParameterValue(TEXT("AuraSize"),Safe(AuraSize,0,12,1.5));Aura->SetScalarParameterValue(TEXT("AuraWidth"),Safe(AuraWidth,.05,3,1));
        Aura->SetScalarParameterValue(TEXT("AuraSpeed"),Safe(AuraSpeed,0,5,1));
    }
}
void UQiCultivationComponent::RestartPreview()
{if(!IsPreviewWorld()||IsTemplate())return;CancelCultivation();if(bPreviewInEditor)StartCultivation();}
void UQiCultivationComponent::UpdateAura()
{
    LiveCounts.Init(0,7);
    if(Effect)if(auto Controller=Effect->GetSystemInstanceController())
    {
        Controller->WaitForConcurrentTickAndFinalize();
        if(auto* Instance=Controller->GetSystemInstance_Unsafe())
        {
            const auto& Emitters=Instance->GetEmitters();
            for(int32 I=0;I<FMath::Min(7,Emitters.Num());++I)LiveCounts[I]=Emitters[I]->GetNumParticles();
        }
    }
    int32 ParticleTotal=0;for(int32 Count:LiveCounts)ParticleTotal+=Count;
    if(bDraining&&ParticleTotal==0)return; // Preserve the last aura weights through its final fade.
    LastParticleTotal=ParticleTotal;
    if(!Aura)return;
    int32 Total=0;
    for(int32 I=0;I<7;++I){Total+=LiveCounts[I];Aura->SetScalarParameterValue(FName(FString::Printf(TEXT("Weight%d"),I)),LiveCounts[I]);}
    Aura->SetScalarParameterValue(TEXT("Strength"),(Total>0?FMath::Clamp(Total/12.f,.25f,1.f):0.f)*Safe(AuraIntensity,0,5,1));
}
void UQiCultivationComponent::AdvanceDrain(float Delta)
{
    if(!bDraining)return;
    for(int32 Count:LiveCounts)if(Count>0)return;
    if(AuraFadeRemaining<=0)AuraFadeRemaining=.35f;
    AuraFadeRemaining-=Delta;
    if(Aura)Aura->SetScalarParameterValue(TEXT("Strength"),FMath::Clamp(LastParticleTotal/12.f,.25f,1.f)*Safe(AuraIntensity,0,5,1)*FMath::Max(0.f,AuraFadeRemaining/.35f));
    if(AuraFadeRemaining<=0)CancelCultivation();
}
void UQiCultivationComponent::TickComponent(float Delta,ELevelTick TickType,FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta,TickType,Function);
    if(IsPreviewWorld())return;
    if(bEnableKeyboard)
        if(const auto* Pawn=Cast<APawn>(GetOwner());Pawn&&Pawn->IsLocallyControlled())
            if(auto* PC=Cast<APlayerController>(Pawn->GetController());PC&&PC->WasInputKeyJustPressed(ToggleKey))ToggleCultivation();
    if((!bCultivating&&!bDraining)||!Effect)return;AttachAura();UpdateAura();AdvanceDrain(Delta);
}
void UQiCultivationComponent::EndPlay(const EEndPlayReason::Type Reason){ReleaseEffect();Super::EndPlay(Reason);}
#if WITH_EDITOR
void UQiCultivationComponent::TickEditorPreview(float Delta)
{
    if(!IsRegistered()||!IsPreviewWorld())return;
    if(!bPreviewInEditor){if(bCultivating||bDraining)CancelCultivation();return;}
    EnsureEffect();if(!Effect)return;
    Effect->SetForceSolo(true);Effect->SetComponentTickEnabled(false);
    ApplySettings();if(!bCultivating)StartCultivation();
    // 手动推进一次，禁用 Niagara 自动 tick，避免同时被两个视口推进而加速。
    Effect->AdvanceSimulation(1,FMath::Clamp(Delta,0.001f,0.1f));AttachAura();UpdateAura();
}
void UQiCultivationComponent::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
    // Super 可能重建蓝图组件；先清理，不在 Super 之后访问旧实例。
    CancelCultivation();Super::PostEditChangeProperty(Event);
}
#endif
