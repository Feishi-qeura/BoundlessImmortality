#include "LiveWeaponPerformanceComponent.h"
#include "LiveWeaponGameMode.h"
#include "LiveWeaponSwarm.h"
#include "LiveAudienceAdapterComponent.h"
#include "LiveMonster.h"
#include "LiveMonsterRegistrySubsystem.h"
#include "LiveWeaponHUD.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "UnrealClient.h"
#include "RenderTimer.h"
#include "DynamicRHI.h"
#include "HAL/PlatformMemory.h"
#include "HAL/PlatformMisc.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/DateTime.h"
ULiveWeaponPerformanceComponent::ULiveWeaponPerformanceComponent(){PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.TickGroup=TG_PostUpdateWork;}
void ULiveWeaponPerformanceComponent::BeginPlay()
{
    Super::BeginPlay();
#if !UE_BUILD_SHIPPING
    if(!FParse::Param(FCommandLine::Get(),TEXT("LiveStress"))){SetComponentTickEnabled(false);return;}
    // Measure available throughput rather than the project's fixed 60 Hz pacing.
    if(GEngine)GEngine->bUseFixedFrameRate=false;
    FParse::Value(FCommandLine::Get(),TEXT("LiveSeconds="),Seconds);Seconds=FMath::Clamp(Seconds,10,1800);
    FString Run;FParse::Value(FCommandLine::Get(),TEXT("LiveRun="),Run);if(Run.IsEmpty())Run=FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S"));
    for(TCHAR& C:Run)if(!FChar::IsAlnum(C)&&C!='-'&&C!='_')C='_';
    Output=FPaths::ProjectSavedDir()/TEXT("Codex/LiveWeapons/Stress")/Run;IFileManager::Get().MakeDirectory(*Output,true);
    Started=Previous=FPlatformTime::Seconds();Rows=TEXT("elapsed,frame_ms,game_ms,render_ms,gpu_ms,memory_mb,viewers,rendered,attacks,labels,avatars,primary_launched,followup_hits,max_combo,active_arrows\n");
#else
    SetComponentTickEnabled(false);
#endif
}
void ULiveWeaponPerformanceComponent::TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* ThisTick)
{
    Super::TickComponent(Delta,Type,ThisTick);
#if !UE_BUILD_SHIPPING
    if(Started<=0)return;const double Now=FPlatformTime::Seconds(),Elapsed=Now-Started,FrameMs=(Now-Previous)*1000;Previous=Now;
    auto* Mode=GetWorld()->GetAuthGameMode<ALiveWeaponGameMode>();if(!Mode||!Mode->Swarm)return;
    if(Elapsed>NextMemory)
    {
        MemoryBytes=FPlatformMemory::GetStats().UsedPhysical;NextMemory=Elapsed+1;
        if(Elapsed<5)
        {
            TArray<TWeakObjectPtr<ALiveMonster>> Found;GetWorld()->GetSubsystem<ULiveMonsterRegistrySubsystem>()->CollectInRange(FVector::ZeroVector,10000,Found);
            UE_LOG(LogTemp,Display,TEXT("LiveProbe time=%.2f swarmClock=%.2f swarmTick=%d begun=%d authority=%d registry=%d"),Elapsed,Mode->Swarm->GetSimulationTime(),Mode->Swarm->IsActorTickEnabled(),Mode->Swarm->HasActorBegunPlay(),Mode->Swarm->HasAuthority(),Found.Num());
            for(TActorIterator<ALiveMonster> It(GetWorld());It;++It){UE_LOG(LogTemp,Display,TEXT("LiveProbe monsterTick=%d canTick=%d begun=%d target=%d position=%s"),It->IsActorTickEnabled(),It->PrimaryActorTick.bCanEverTick,It->HasActorBegunPlay(),IsValid(It->FollowTarget),*It->GetActorLocation().ToString());break;}
        }
    }
    const double GpuMs=FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());
    const auto* PC=GetWorld()->GetFirstPlayerController();const auto* HUD=PC?Cast<ALiveWeaponHUD>(PC->GetHUD()):nullptr;
    Rows+=FString::Printf(TEXT("%.4f,%.4f,%.4f,%.4f,%.4f,%.2f,%d,%d,%lld,%d,%d,%lld,%lld,%d,%d\n"),Elapsed,FrameMs,FPlatformTime::ToMilliseconds(GGameThreadTime),FPlatformTime::ToMilliseconds(GRenderThreadTime),GpuMs,double(MemoryBytes)/1048576,Mode->Swarm->GetParticipantCount(),Mode->Swarm->GetRenderedCount(),Mode->Swarm->TotalAttacks,HUD?HUD->DrawnNameplates:0,Mode->Audience->GetReadyAvatarCount(),Mode->Swarm->TotalPrimaryLaunched,Mode->Swarm->TotalFollowUpHits,Mode->Swarm->MaxComboHitsObserved,Mode->Swarm->GetActiveArrowCount());
    if(Elapsed>NextSwitch&&Mode->Audience)
    {
        NextSwitch=Elapsed+.5;const int32 Count=Mode->Swarm->GetParticipantCount();for(int I=0;I<32&&Count>0;++I){const int32 Viewer=(SwitchBatch*32+I)%Count;Mode->Audience->SimulateComment(FString::Printf(TEXT("viewer_%04d"),Viewer),FString::Printf(TEXT("观众%04d"),Viewer+1),FString::FromInt((SwitchBatch+I)%4+1));}++SwitchBatch;
    }
    if(!bCaptured&&Elapsed>15){FScreenshotRequest::RequestScreenshot(Output/TEXT("preview.png"),true,false);bCaptured=true;}
    if(Elapsed>=Seconds)
    {
        FFileHelper::SaveStringToFile(Rows,*(Output/TEXT("frames.csv")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        FString Hardware=FString::Printf(TEXT("CPU=%s\nGPU=%s\nDurationSeconds=%d\nHighHealthMonsters=true (1 million HP; 1-damage float subtraction remains exact)\nFixedFrameRate=false for throughput capture only\nAvatarSource=generated local thumbnails; real HTTP not load-tested\n"),*FPlatformMisc::GetCPUBrand(),*FPlatformMisc::GetPrimaryGPUBrand(),Seconds);
        FFileHelper::SaveStringToFile(Hardware,*(Output/TEXT("hardware.txt")));Started=0;SetComponentTickEnabled(false);FPlatformMisc::RequestExit(false);
    }
#endif
}
