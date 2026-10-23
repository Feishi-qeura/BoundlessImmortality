#include "LiveWeaponSwarm.h"
#include "LiveTitleEffectsComponent.h"
#include "LiveWeaponDefinition.h"
#include "LiveMonster.h"
#include "LiveMonsterRegistrySubsystem.h"
#include "Cultivation/CultivationSubsystem.h"
#include "Cultivation/CultivationHealthComponent.h"
#include "Engine/GameInstance.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "SceneView.h"
#include "Serialization/CustomVersion.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "UObject/ConstructorHelpers.h"
namespace
{
const FGuid SpatialRangeVersionGuid(0x37C83D41,0x96AE4B75,0xB9F846C2,0xD24CB03A);
FCustomVersionRegistration SpatialRangeVersion(SpatialRangeVersionGuid,1,TEXT("LiveWeaponSpatialRanges"));
FVector2D OrderedRange(float A,float B,float Floor=-MAX_flt)
{
    A=FMath::IsFinite(A)?FMath::Max(A,Floor):Floor;B=FMath::IsFinite(B)?FMath::Max(B,Floor):A;
    return FVector2D(FMath::Min(A,B),FMath::Max(A,B));
}
float HitRadius(const ALiveMonster* M){return M->Collision?M->Collision->GetScaledSphereRadius():40.f;}
FVector HitPoint(const ALiveMonster* M){return M->GetActorLocation();}
}
void ALiveWeaponSwarm::Serialize(FArchive& Ar)
{
    Ar.UsingCustomVersion(SpatialRangeVersionGuid);Super::Serialize(Ar);
}
void ALiveWeaponSwarm::PostLoad()
{
    Super::PostLoad();
    if(GetLinkerCustomVersion(SpatialRangeVersionGuid)>=1)return;
    MigrateLegacySpatialRanges(Cast<ALiveWeaponSwarm>(GetArchetype()));
}
void ALiveWeaponSwarm::MigrateLegacySpatialRanges(const ALiveWeaponSwarm* Defaults)
{
    // Unmodified legacy groups inherit their archetype's new ranges, including edits made after its migration.
    if(!Defaults||AlertRingRadius!=Defaults->AlertRingRadius||AlertRingWidth!=Defaults->AlertRingWidth)
    {
        AlertRadiusMin=FMath::Max(1.f,AlertRingRadius-FMath::Max(0.f,AlertRingWidth)*.5f);
        AlertRadiusMax=FMath::Max(AlertRadiusMin,AlertRingRadius+FMath::Max(0.f,AlertRingWidth)*.5f);
    }
    if(!Defaults||AlertRingHeight!=Defaults->AlertRingHeight||AlertRingThickness!=Defaults->AlertRingThickness)
    {AlertHeightMin=AlertRingHeight-FMath::Max(0.f,AlertRingThickness)*.5f;AlertHeightMax=AlertRingHeight+FMath::Max(0.f,AlertRingThickness)*.5f;}
    if(!Defaults||FormationRadius!=Defaults->FormationRadius)
    {
        // Old idle radius grew with slot count; retain the thousand-spirit envelope for local overrides.
        IdleRadiusMin=FMath::Max(1.f,FormationRadius);
        IdleRadiusMax=FMath::Max(IdleRadiusMin,FMath::Max(200.f,FormationRadius)+FMath::Sqrt(999.f)*12+100);
    }
    if(!Defaults||FloatHeight!=Defaults->FloatHeight||IdleHeightSpread!=Defaults->IdleHeightSpread)
    {IdleHeightMin=FloatHeight-80;IdleHeightMax=FloatHeight+FMath::Max(0.f,IdleHeightSpread)-70;}
}
ALiveWeaponSwarm::ALiveWeaponSwarm()
{
    PrimaryActorTick.bCanEverTick=true;SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    TitleEffects=CreateDefaultSubobject<ULiveTitleEffectsComponent>(TEXT("TitleEffects"));
    SetActorEnableCollision(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    for(int I=0;I<4;++I)
    {
        auto* Mesh=CreateDefaultSubobject<UInstancedStaticMeshComponent>(*FString::Printf(TEXT("Weapon%d"),I));
        Mesh->SetupAttachment(RootComponent);Mesh->SetStaticMesh(I==3?Cylinder.Object:Cube.Object);Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetCanEverAffectNavigation(false);Mesh->SetCastShadow(true);Mesh->SetRemoveSwap();WeaponMeshes.Add(Mesh);
    }
    ArrowMesh=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ArrowPool"));ArrowMesh->SetupAttachment(RootComponent);ArrowMesh->SetStaticMesh(Cube.Object);
    ArrowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);ArrowMesh->SetCanEverAffectNavigation(false);ArrowMesh->SetCastShadow(true);
}
void ALiveWeaponSwarm::BeginPlay()
{
    Super::BeginPlay();
    Combat.bDynamicAttackFrequency=true;
    SetActorEnableCollision(false);
    for(auto Mesh:WeaponMeshes){Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mesh->SetGenerateOverlapEvents(false);Mesh->SetSimulatePhysics(false);Mesh->SetCastShadow(bWeaponCastShadows);}
    ArrowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);ArrowMesh->SetGenerateOverlapEvents(false);ArrowMesh->SetSimulatePhysics(false);
    ArrowMesh->SetCastShadow(bWeaponCastShadows);
    for(ULiveWeaponDefinition* Def:Definitions)
    {
        if(!Def||uint8(Def->WeaponType)>3)continue;auto Mesh=WeaponMeshes[uint8(Def->WeaponType)];
        Combat.AttackIntervals.Add(Def->WeaponType,Def->AttackInterval);
        if(Def->Mesh)Mesh->SetStaticMesh(Def->Mesh);if(Def->Material)Mesh->SetMaterial(0,Def->Material);
        for(const auto& Skill:Def->Skills)
        {
            FLiveSkillRule Rule;Rule.SkillId=Skill.SkillId;Rule.WeaponType=Def->WeaponType;Rule.TriggerChance=Skill.TriggerChance;Rule.Weight=Skill.Weight;Rule.Cooldown=Skill.Cooldown;Combat.SkillRules.Add(Rule);
            if(Skill.SkillClass)SkillObjects.Add(Skill.SkillId,NewObject<ULiveWeaponSkill>(this,Skill.SkillClass));
        }
    }
    Arrows.SetNum(2048);for(int I=0;I<Arrows.Num();++I){FreeArrows.Add(I);ArrowMesh->AddInstance(FTransform(FQuat::Identity,FVector::ZeroVector,FVector::ZeroVector));}
}
void ALiveWeaponSwarm::SetAnchor(APawn* Pawn)
{
    Anchor=Pawn;RefreshReadabilityView();
    if(Pawn&&GetWorld()->GetGameInstance()&&PresentationMode==ELiveWeaponPresentationMode::Gameplay)
    {
        auto* C=GetWorld()->GetGameInstance()->GetSubsystem<UCultivationSubsystem>();const auto S=C->GetParticipantSnapshot(C->GetAnchorId());C->SetAnchor(Pawn,C->GetAnchorId(),S.DisplayName.IsEmpty()?TEXT("主播"):S.DisplayName);EnsurePersonalSword();
    }
}
void ALiveWeaponSwarm::EndPlay(const EEndPlayReason::Type Reason)
{
    AlertLeases.Reset();
    if(PresentationMode==ELiveWeaponPresentationMode::Gameplay&&GetWorld()->GetGameInstance())
    {
        auto* C=GetWorld()->GetGameInstance()->GetSubsystem<UCultivationSubsystem>();
        for(const auto& S:Roster.GetStates())if(!S.bPersonalSword&&S.bParticipating)C->LeaveParticipant(S.Profile.ParticipantId);
        if(const auto* Health=C->GetAnchorHealth();Health&&Health->GetOwner()==Anchor.Get()){C->LeaveParticipant(C->GetAnchorId());C->SetAnchor(nullptr);}
    }
    Super::EndPlay(Reason);
}
void ALiveWeaponSwarm::EnsurePersonalSword()
{
    if(PresentationMode!=ELiveWeaponPresentationMode::Gameplay||!Anchor.IsValid()||!GetWorld()->GetGameInstance())return;
    auto* C=GetWorld()->GetGameInstance()->GetSubsystem<UCultivationSubsystem>();if(!C||!C->GetAnchorHealth())return;
    const FString Expected=TEXT("@anchor:")+C->GetAnchorId();
    if(PersonalSwordId!=Expected&&!PersonalSwordId.IsEmpty())RemoveParticipant(PersonalSwordId);
    PersonalSwordId=Expected;
    if(auto* Existing=Roster.Find(PersonalSwordId);Existing&&Existing->bParticipating)return;
    JoinParticipant({PersonalSwordId,TEXT("本命剑"),TEXT("")});if(auto* State=Roster.Find(PersonalSwordId))State->bPersonalSword=true;
}
FString ALiveWeaponSwarm::GetProgressionOwnerId(const FString& Id)const
{
    if(const auto* S=Roster.Find(Id);S&&S->bPersonalSword&&GetWorld()->GetGameInstance())return GetWorld()->GetGameInstance()->GetSubsystem<UCultivationSubsystem>()->GetAnchorId();return Id;
}
int32 ALiveWeaponSwarm::GetParticipantCount()const
{
    int32 Count=0;for(const auto& S:Roster.GetStates())if(S.bParticipating&&!S.bPersonalSword)++Count;return Count;
}
bool ALiveWeaponSwarm::RemoveParticipant(const FString& Id)
{
    AlertLeases.Remove(Id);
    auto* S=Roster.Find(Id);if(!S||!S->bParticipating)return false;S->bParticipating=false;++S->WeaponRevision;
    auto& V=Visuals[S->Handle.Index];++V.AttackToken;V.bPathValid=false;V.Target.Reset();V.bPendingArrow=false;
    for(int32 I=0;I<Arrows.Num();++I)if(Arrows[I].bActive&&Arrows[I].Owner==S->Handle)ReleaseArrow(I);
    WeaponMeshes[uint8(S->WeaponType)]->UpdateInstanceTransform(V.InstanceIndex,FTransform(FQuat::Identity,FVector::ZeroVector,FVector::ZeroVector),true,false,true);
    if(!S->bPersonalSword&&GetWorld()->GetGameInstance())GetWorld()->GetGameInstance()->GetSubsystem<UCultivationSubsystem>()->LeaveParticipant(Id);return true;
}
void ALiveWeaponSwarm::SetPresentationMode(ELiveWeaponPresentationMode Mode)
{
    if(PresentationMode==Mode)return;
    const bool bEnteringPresentation=PresentationMode==ELiveWeaponPresentationMode::Gameplay&&Mode!=ELiveWeaponPresentationMode::Gameplay;
    PresentationMode=Mode;
    if(Mode!=ELiveWeaponPresentationMode::Gameplay&&!PersonalSwordId.IsEmpty())RemoveParticipant(PersonalSwordId);
    bNearbyThreat=Mode==ELiveWeaponPresentationMode::Alert;
    if(!bEnteringPresentation)return;
    // A visual-only mode must also cancel a sortie already in flight, including arrows.
    for(int32 I=0;I<Arrows.Num();++I)if(Arrows[I].bActive)ReleaseArrow(I);
    for(int32 I=0;I<Visuals.Num();++I)
    {
        auto& V=Visuals[I];++V.AttackToken;V.Target.Reset();V.HitTargets.Reset();V.bPendingArrow=false;
        V.bContinueCombo=false;V.bNeedsRecovery=false;V.RecoveryRemaining=0;V.ComboHits=0;V.SkillId=NAME_None;
        if(bNearbyThreat)BeginReturn(I,false);else BeginWander(I);
    }
}
void ALiveWeaponSwarm::RefreshReadabilityView()
{
    ViewCenter=Anchor.IsValid()?Anchor->GetActorLocation():GetActorLocation();
    if(auto* Pawn=Anchor.Get())
    {
        const auto* Movement=Pawn->GetMovementComponent();
        const float ConfiguredSpeed=Movement?Movement->GetMaxSpeed():600.f;
        PlayerFollowSpeed=2*FMath::Max(ConfiguredSpeed,float(Pawn->GetVelocity().Size()));
    }
    FRotator Rotation(-35,-45,0);bPerspectiveView=false;
    if(auto* PC=GetWorld()?GetWorld()->GetFirstPlayerController():nullptr;PC&&PC->PlayerCameraManager)
    {
        Rotation=PC->PlayerCameraManager->GetCameraRotation();ViewLocation=PC->PlayerCameraManager->GetCameraLocation();bPerspectiveView=true;
        if(const auto* Target=PC->GetViewTarget())
        {
            TInlineComponentArray<UCameraComponent*> Cameras;Target->GetComponents(Cameras);
            for(auto* Camera:Cameras)if(Camera->IsActive()){bPerspectiveView=Camera->ProjectionMode==ECameraProjectionMode::Perspective;break;}
        }
    }
    const FRotationMatrix Basis(Rotation);ViewForward=Basis.GetUnitAxis(EAxis::X);ViewRight=Basis.GetUnitAxis(EAxis::Y);ViewUp=Basis.GetUnitAxis(EAxis::Z);
    ViewDepth=FMath::Max(1.f,float(FVector::DotProduct(ViewCenter-ViewLocation,ViewForward)));
    bHasIdleView=false;IdleScreenBounds=FBox2D(ForceInit);
    if(!bAvoidCharacterOnScreen)return;
    auto* PC=GetWorld()?GetWorld()->GetFirstPlayerController():nullptr;
    auto* Player=PC?PC->GetLocalPlayer():nullptr;
    if(!Player||!Player->ViewportClient||!Player->ViewportClient->Viewport)return;
    FSceneViewProjectionData Projection;
    if(!Player->GetProjectionData(Player->ViewportClient->Viewport,Projection))return;
    IdleViewRect=Projection.GetConstrainedViewRect();IdleViewProjection=Projection.ComputeViewProjectionMatrix();IdleInverseViewProjection=IdleViewProjection.Inverse();
    IdleScreenBounds=GetCharacterScreenBounds(PC);
    if(!IdleScreenBounds.bIsValid)return;
    const FVector2D ViewportOffset(IdleViewRect.Min.X,IdleViewRect.Min.Y);
    IdleScreenBounds=FBox2D(IdleScreenBounds.Min+ViewportOffset,IdleScreenBounds.Max+ViewportOffset);
    // GetCharacterScreenBounds already includes twelve pixels of padding.
    IdleScreenBounds=IdleScreenBounds.ExpandBy(FMath::Max(0.f,CharacterScreenPadding)-12);
    FVector2D CenterPixel,RightPixel;
    if(!FSceneView::ProjectWorldToScreen(ViewCenter,IdleViewRect,IdleViewProjection,CenterPixel)||
       !FSceneView::ProjectWorldToScreen(ViewCenter+ViewRight,IdleViewRect,IdleViewProjection,RightPixel))return;
    IdlePixelsPerUnit=FVector2D::Distance(CenterPixel,RightPixel);IdleWeaponRadius=1;
    for(int I=0;I<4;++I)IdleWeaponRadius=FMath::Max(IdleWeaponRadius,WeaponHeightMargin(ELiveWeaponType(I)));
    bHasIdleView=true;
}
FVector2D ALiveWeaponSwarm::ReadabilityOffset(const FVector& Position)const
{
    const float Depth=bPerspectiveView?FMath::Max(1.f,float(FVector::DotProduct(Position-ViewLocation,ViewForward)))/ViewDepth:1;
    return FVector2D(FVector::DotProduct(Position-ViewCenter,ViewRight),FVector::DotProduct(Position-ViewCenter,ViewUp))/Depth;
}
bool ALiveWeaponSwarm::IsInsideCharacterReadabilityArea(FVector Position,float Padding)const
{
    const FVector Extent=CharacterAvoidanceExtent.GetAbs().ComponentMax(FVector(1))*FMath::Max(.1f,Padding);
    return ((Position-ViewCenter)/Extent).SizeSquared()<1;
}
float ALiveWeaponSwarm::IdleScreenWeaponMargin(const FVector& Position)const
{
    const float DepthScale=bPerspectiveView?ViewDepth/FMath::Max(1.f,float(FVector::DotProduct(Position-ViewLocation,ViewForward))):1;
    return IdleWeaponRadius*IdlePixelsPerUnit*DepthScale;
}
bool ALiveWeaponSwarm::IsInsideCharacterScreenArea(FVector Position)const
{
    if(!bHasIdleView)return false;
    FVector2D Pixel;if(!FSceneView::ProjectWorldToScreen(Position,IdleViewRect,IdleViewProjection,Pixel))return false;
    return IdleScreenBounds.ExpandBy(IdleScreenWeaponMargin(Position)).IsInsideOrOn(Pixel);
}
bool ALiveWeaponSwarm::IsIdlePositionBlocked(const FVector& Position,float Padding)const
{
    return IsInsideCharacterReadabilityArea(Position,Padding)||IsInsideCharacterScreenArea(Position);
}
FVector ALiveWeaponSwarm::SafeIdlePosition(const FVector& Position,bool bForIdle)const
{
    const FVector Extent=CharacterAvoidanceExtent.GetAbs().ComponentMax(FVector(1));
    FVector Offset=Position-ViewCenter;const double Distance=(Offset/Extent).Size();
    if(Distance<1.05)Offset=Distance>.001?Offset*(1.05/Distance):FVector(Extent.X*1.05,0,0);
    FVector Result=ViewCenter+Offset;
    if(!bForIdle||!bHasIdleView)return Result;
    // Alternate the two constraints: leaving the screen rectangle can re-enter the world ellipsoid.
    for(int Attempt=0;Attempt<4;++Attempt)
    {
        FVector2D Pixel;
        if(!FSceneView::ProjectWorldToScreen(Result,IdleViewRect,IdleViewProjection,Pixel))break;
        // Reserve the full six-centimetre hover bob before choosing an idle origin.
        const float HoverPixels=IdleScreenWeaponMargin(Result)*8/FMath::Max(1.f,IdleWeaponRadius);
        const FBox2D Area=IdleScreenBounds.ExpandBy(IdleScreenWeaponMargin(Result)+HoverPixels+4);
        if(Area.IsInsideOrOn(Pixel))
        {
            const FVector2D Candidates[]={FVector2D(Area.Min.X-2,Pixel.Y),FVector2D(Area.Max.X+2,Pixel.Y),FVector2D(Pixel.X,Area.Min.Y-2),FVector2D(Pixel.X,Area.Max.Y+2)};
            FVector2D Desired=Candidates[0];for(const auto& P:Candidates)if(FVector2D::DistSquared(P,Pixel)<FVector2D::DistSquared(Desired,Pixel))Desired=P;
            FVector RayOrigin,RayDirection;FSceneView::DeprojectScreenToWorld(Desired,IdleViewRect,IdleInverseViewProjection,RayOrigin,RayDirection);
            const double Denominator=FVector::DotProduct(RayDirection,ViewForward);
            if(FMath::Abs(Denominator)>.0001)Result=RayOrigin+RayDirection*(FVector::DotProduct(Result-RayOrigin,ViewForward)/Denominator);
        }
        Offset=Result-ViewCenter;const double D=(Offset/Extent).Size();
        if(D<1.05){Offset=D>.001?Offset*(1.05/D):FVector(Extent.X*1.05,0,0);Result=ViewCenter+Offset;}
        if(!IsIdlePositionBlocked(Result))break;
    }
    return Result;
}
FBox2D ALiveWeaponSwarm::GetCharacterScreenBounds(APlayerController* PC)const
{
    FBox2D Bounds(ForceInit);if(!PC||!Anchor.IsValid())return Bounds;
    FVector Extent(45,45,95);if(auto* Character=Cast<ACharacter>(Anchor.Get()))Extent=FVector(Character->GetCapsuleComponent()->GetScaledCapsuleRadius(),Character->GetCapsuleComponent()->GetScaledCapsuleRadius(),Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
    const FVector Center=Anchor->GetActorLocation();
    for(int X:{-1,1})for(int Y:{-1,1})for(int Z:{-1,1}){FVector2D P;if(PC->ProjectWorldLocationToScreen(Center+Extent*FVector(X,Y,Z),P,true))Bounds+=P;}
    return Bounds.bIsValid?Bounds.ExpandBy(12):Bounds;
}
FVector2D ALiveWeaponSwarm::KeepNameplateOutside(const FVector2D& Position,const FVector2D& Size,const FBox2D& Protected)
{
    if(!Protected.bIsValid||!FBox2D(Position,Position+Size).Intersect(Protected))return Position;
    const FVector2D Candidates[]={FVector2D(Protected.Min.X-Size.X-4,Position.Y),FVector2D(Protected.Max.X+4,Position.Y),FVector2D(Position.X,Protected.Min.Y-Size.Y-4),FVector2D(Position.X,Protected.Max.Y+4)};
    FVector2D Best=Candidates[0];for(auto P:Candidates)if(FVector2D::DistSquared(P,Position)<FVector2D::DistSquared(Best,Position))Best=P;return Best;
}
FVector2D ALiveWeaponSwarm::PlaceNameplate(const FVector2D& Position,const FVector2D& Size,const FBox2D& Character,const FBox2D& Panel,const FVector2D& Viewport)
{
    auto Clamp=[&](FVector2D P){return FVector2D(FMath::Clamp(P.X,0.,FMath::Max(0.,Viewport.X-Size.X)),FMath::Clamp(P.Y,0.,FMath::Max(0.,Viewport.Y-Size.Y)));};
    const FVector2D Origin=Clamp(Position);auto Clear=[&](FVector2D P){const FBox2D Rect(P,P+Size);return (!Character.bIsValid||!Rect.Intersect(Character))&&(!Panel.bIsValid||!Rect.Intersect(Panel));};
    if(Clear(Origin))return Origin;
    FVector2D Best=Origin;double Distance=MAX_dbl;
    for(const FBox2D& Area:{Character,Panel})if(Area.bIsValid)
    {
        const FVector2D Candidates[]={FVector2D(Area.Min.X-Size.X-4,Origin.Y),FVector2D(Area.Max.X+4,Origin.Y),FVector2D(Origin.X,Area.Min.Y-Size.Y-4),FVector2D(Origin.X,Area.Max.Y+4)};
        for(auto P:Candidates){P=Clamp(P);const double D=FVector2D::DistSquared(P,Origin);if(Clear(P)&&D<Distance){Distance=D;Best=P;}}
    }
    return Best;
}
ULiveWeaponDefinition* ALiveWeaponSwarm::Definition(ELiveWeaponType Type)const{for(ULiveWeaponDefinition* Def:Definitions)if(Def&&Def->WeaponType==Type)return Def;return nullptr;}
bool ALiveWeaponSwarm::IsCurrent(FLiveWeaponHandle H)const{return Roster.GetStates().IsValidIndex(H.Index)&&Roster.GetStates()[H.Index].Handle==H&&Roster.GetStates()[H.Index].bParticipating;}
FVector ALiveWeaponSwarm::FormationPosition(int32 Index)const
{
    const auto R=OrderedRange(IdleRadiusMin,IdleRadiusMax,1);const auto H=OrderedRange(IdleHeightMin,IdleHeightMax);
    const double Angle=Index*2.399963229728653,Radius=FMath::Lerp(R.X,R.Y,FMath::Frac((Index+1)*.7548776662));
    const double Height=FMath::Lerp(H.X,H.Y,FMath::Frac((Index+1)*.5698402910));
    FVector Position=(Anchor.IsValid()?Anchor->GetActorLocation():GetActorLocation())+FVector(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius,Height);
    Position.Z=FMath::Max(double(AnchorFootZ()+WeaponHeightMargin(ELiveWeaponType::Sword)),Position.Z);return SafeIdlePosition(Position,true);
}
FLiveWeaponHandle ALiveWeaponSwarm::JoinParticipant(const FLiveParticipantProfile& Profile)
{
    const auto* BeforeState=Roster.Find(Profile.ParticipantId);const bool Reactivating=BeforeState&&!BeforeState->bParticipating;
    if((!BeforeState||!BeforeState->bParticipating)&&!Profile.ParticipantId.StartsWith(TEXT("@anchor:"))&&GetParticipantCount()>=1000)return {};
    if(!HasAuthority())return {};const int32 Before=Roster.Num();const auto Handle=Roster.Join(Profile);if(!Handle.IsValid())return Handle;
    auto* Joined=Roster.Find(Profile.ParticipantId);Joined->LastInteractionTime=Clock;Joined->bParticipating=true;
    const auto Notify=[&]()
    {
        if(!Profile.ParticipantId.StartsWith(TEXT("@anchor:"))&&PresentationMode==ELiveWeaponPresentationMode::Gameplay&&GetWorld()->GetGameInstance())GetWorld()->GetGameInstance()->GetSubsystem<UCultivationSubsystem>()->RegisterParticipant(Profile.ParticipantId,Profile.DisplayName,Profile.AvatarUrl);
    };
    if(Before==Roster.Num()){if(Reactivating)BeginHover(Handle.Index);Notify();return IsCurrent(Handle)?Handle:FLiveWeaponHandle();}
    FLiveWeaponVisualState V;V.Position=FormationPosition(Handle.Index);V.InstanceIndex=RenderSlots[0].Num();V.Random.Initialize(int32(GetTypeHash(Profile.ParticipantId)));V.OrbitAngle=float(Handle.Index*2.399963229728653);
    V.RingRadiusOffset=V.Random.FRandRange(-1,1);V.RingHeightOffset=V.Random.FRandRange(-1,1);
    V.RadiusDriftPhase=V.Random.FRandRange(0,2*PI);V.HeightDriftPhase=V.Random.FRandRange(0,2*PI);
    V.RadiusDriftRate=V.Random.FRandRange(.15f,.4f);V.HeightDriftRate=V.Random.FRandRange(.12f,.35f);
    V.TempoBias=V.Random.FRandRange(-.25f,.25f);V.TempoPhase=V.Random.FRandRange(0,2*PI);V.TempoFrequency=V.Random.FRandRange(.4f,.85f);
    V.IdleRotation=FRotator(V.Random.FRandRange(-12,12),V.Random.FRandRange(0,360),V.Random.FRandRange(-10,10)).Quaternion();V.RenderRotation=V.IdleRotation;
    V.ReferenceFloorZ=AnchorFootZ();Visuals.Add(V);BeginHover(Handle.Index);RenderSlots[0].Add(Handle.Index);const auto* Def=Definition(ELiveWeaponType::Sword);
    WeaponMeshes[0]->AddInstance(WeaponTransform(V.RenderRotation,Visuals[Handle.Index].Position,Def?Def->MeshScale:FVector(.1f,.1f,.65f)),true);Notify();return IsCurrent(Handle)?Handle:FLiveWeaponHandle();
}
bool ALiveWeaponSwarm::SwitchParticipantWeapon(const FString& Id,ELiveWeaponType Type)
{
    auto* S=Roster.Find(Id);if(!HasAuthority()||!S||uint8(Type)>3||(S->bPersonalSword&&Type!=ELiveWeaponType::Sword))return false;
    const int Old=uint8(S->WeaponType),New=uint8(Type);S->LastInteractionTime=Clock;if(Old==New)return true;
    const int Slot=S->Handle.Index,Instance=Visuals[Slot].InstanceIndex,Moved=RenderSlots[Old].Last();
    RenderSlots[Old].RemoveAtSwap(Instance);WeaponMeshes[Old]->RemoveInstance(Instance);if(Moved!=Slot)Visuals[Moved].InstanceIndex=Instance;
    auto& V=Visuals[Slot];V.InstanceIndex=RenderSlots[New].Num();RenderSlots[New].Add(Slot);
    // Preserve world position and velocity, but cancel the old attack/shot.
    ++V.AttackToken;V.Phase=FLiveWeaponVisualState::EPhase::Wander;V.bPathValid=false;V.Target.Reset();V.HitTargets.Reset();V.bPendingArrow=false;V.bContinueCombo=false;
    const auto* Def=Definition(Type);WeaponMeshes[New]->AddInstance(WeaponTransform(FQuat::Identity,V.Position,Def?Def->MeshScale:FVector(.25f)),true);
    for(int I=0;I<Arrows.Num();++I)if(Arrows[I].bActive&&Arrows[I].Owner==S->Handle)ReleaseArrow(I);
    return Roster.SwitchWeapon(Id,Type);
}
void ALiveWeaponSwarm::ResetParticipants()
{
    AlertLeases.Reset();
    if(PresentationMode==ELiveWeaponPresentationMode::Gameplay&&GetWorld()->GetGameInstance())for(const auto& S:Roster.GetStates())if(!S.bPersonalSword)GetWorld()->GetGameInstance()->GetSubsystem<UCultivationSubsystem>()->LeaveParticipant(S.Profile.ParticipantId);
    Roster.Reset();Visuals.Reset();for(int I=0;I<4;++I){RenderSlots[I].Reset();WeaponMeshes[I]->ClearInstances();}
    FreeArrows.Reset();for(int I=0;I<Arrows.Num();++I){Arrows[I]=FLiveArrowVisual();FreeArrows.Add(I);ArrowMesh->UpdateInstanceTransform(I,FTransform(FQuat::Identity,FVector::ZeroVector,FVector::ZeroVector),true,false,true);}
    ArrowMesh->MarkRenderStateDirty();TotalAttacks=TotalPrimaryLaunched=TotalFollowUpHits=0;MaxComboHitsObserved=0;
    EnsurePersonalSword();
}
int32 ALiveWeaponSwarm::GetRenderedCount()const{int32 Count=0;for(const auto& S:Roster.GetStates())if(S.bParticipating)++Count;return Count;}
bool ALiveWeaponSwarm::HasActiveAttacks()const
{
    if(PresentationMode!=ELiveWeaponPresentationMode::Gameplay)return false;
    if(GetActiveArrowCount()>0)return true;
    for(const auto& S:Roster.GetStates())
    {
        if(!S.bParticipating)continue;
        const auto Phase=Visuals[S.Handle.Index].Phase;
        if(Phase==ELiveWeaponFlightPhase::Strike||Phase==ELiveWeaponFlightPhase::Drift)return true;
    }
    return false;
}
FVector ALiveWeaponSwarm::GetWeaponPosition(const FString& Id)const{const auto* S=Roster.Find(Id);return S?Visuals[S->Handle.Index].Position:FVector::ZeroVector;}
FVector ALiveWeaponSwarm::GetWeaponVelocity(const FString& Id)const{const auto* S=Roster.Find(Id);return S?Visuals[S->Handle.Index].Velocity:FVector::ZeroVector;}
float ALiveWeaponSwarm::GetIdleHoldRemaining(const FString& Id)const{const auto* S=Roster.Find(Id);return S&&Visuals[S->Handle.Index].Phase==FLiveWeaponVisualState::EPhase::Hover?Visuals[S->Handle.Index].HoldRemaining:0;}
ELiveWeaponFlightPhase ALiveWeaponSwarm::GetWeaponPhase(const FString& Id)const{const auto* S=Roster.Find(Id);return S?Visuals[S->Handle.Index].Phase:ELiveWeaponFlightPhase::Hover;}
float ALiveWeaponSwarm::GetRecoveryRemaining(const FString& Id)const{const auto* S=Roster.Find(Id);return S?Visuals[S->Handle.Index].RecoveryRemaining:0;}
float ALiveWeaponSwarm::GetWeaponSpeedMultiplier(const FString& Id)const{const auto* S=Roster.Find(Id);return S?Visuals[S->Handle.Index].SpeedScale(Clock):1.f;}
float ALiveWeaponSwarm::GetAttackFrequencyMultiplier(const FString& Id)const{return Roster.Find(Id)?FLiveWeaponCombatModel::AttackFrequencyScale(Id,Clock):1.f;}
void ALiveWeaponSwarm::GetPresentationSamples(TArray<FLiveWeaponLabelSample>& Out)const
{
    if(PresentationMode!=ELiveWeaponPresentationMode::Gameplay){Out.Reset();return;}
    Out.Reset(Roster.Num());for(const auto& S:Roster.GetStates())if(S.bParticipating)Out.Add({S.Profile.ParticipantId,S.Profile.DisplayName,S.Profile.AvatarUrl,Visuals[S.Handle.Index].Position+FVector(0,0,45),S.LastInteractionTime});
}
ALiveMonster* ALiveWeaponSwarm::FindPrimaryTarget(ELiveWeaponType Type,const FVector& From,const TArray<TWeakObjectPtr<ALiveMonster>>& Candidates)const
{
    const auto* Def=Definition(Type);const float Range=Def?Def->AttackRange:DefaultAttackRange;
    ALiveMonster* Best=nullptr;double BestDistance=FMath::Square(2200.);
    for(auto Weak:Candidates)if(auto* M=Weak.Get();M&&M->IsAlive()&&FVector::DistSquared2D(Anchor->GetActorLocation(),M->GetActorLocation())<=FMath::Square(Range))
    {const double D=FVector::DistSquared(From,HitPoint(M));if(D<BestDistance){Best=M;BestDistance=D;}}
    return Best;
}
bool ALiveWeaponSwarm::ShouldContinueCombo(ELiveWeaponType Type,int32 Hits,float Chance,int32 Maximum,FRandomStream& Random)
{
    return (Type==ELiveWeaponType::Sword||Type==ELiveWeaponType::Spear)&&Hits<FMath::Clamp(Maximum,1,4)&&Random.FRand()<FMath::Clamp(Chance,0.f,1.f);
}
void ALiveWeaponSwarm::BeginHover(int32 Slot)
{
    auto& V=Visuals[Slot];V.Phase=FLiveWeaponVisualState::EPhase::Hover;V.HoverOrigin=V.Position;V.Age=0;V.Velocity=FVector::ZeroVector;V.bPathValid=false;
    V.bIdleEvacuating=false;
    V.HoldRemaining=V.Random.FRandRange(FMath::Clamp(IdleHoldMin,1.f,5.f),FMath::Clamp(FMath::Max(IdleHoldMin,IdleHoldMax),1.f,5.f));
    V.FollowReference=Anchor.IsValid()?Anchor->GetActorLocation():GetActorLocation();V.Target.Reset();V.bContinueCombo=false;
}
void ALiveWeaponSwarm::BeginWander(int32 Slot,bool bEvacuate)
{
    auto& V=Visuals[Slot];const FVector Center=Anchor.IsValid()?Anchor->GetActorLocation():GetActorLocation();
    V.OrbitAngle+=V.Random.FRandRange(.35f,.85f)*(Slot%2?1:-1);
    const auto R=OrderedRange(IdleRadiusMin,IdleRadiusMax,1);const auto H=OrderedRange(IdleHeightMin,IdleHeightMax);
    const float Radius=V.Random.FRandRange(float(R.X),float(R.Y));
    FVector Destination=Center+FVector(FMath::Cos(V.OrbitAngle)*Radius,FMath::Sin(V.OrbitAngle)*Radius,V.Random.FRandRange(float(H.X),float(H.Y)));
    Destination.Z=FMath::Max(Destination.Z,double(AnchorFootZ()+WeaponHeightMargin(Roster.GetStates()[Slot].WeaponType)));
    Destination=SafeIdlePosition(Destination,true);V.bIdleEvacuating=bEvacuate;
    if(bEvacuate)
    {
        FVector Away=(V.Position-Center).GetSafeNormal();if(Away.IsNearlyZero())Away=FVector::ForwardVector;
        Destination=SafeIdlePosition(V.Position+Away*80,true);
    }
    const bool CatchUp=FVector::DistSquared2D(Center,V.Position)>FMath::Square(900.f);
    const float Speed=CatchUp||bEvacuate?1400.f:V.Random.FRandRange(240,380);
    const float PathScale=PresentationMode==ELiveWeaponPresentationMode::Gameplay?1.f:FMath::Clamp(PresentationPathScale,0.f,1.f);
    FVector Arc=FVector::UpVector*V.Random.FRandRange(35,100)*PathScale;const FVector Weave=ViewRight*V.Random.FRandRange(-25,25)*PathScale;
    FVector Away=((V.Position+Destination)*.5f-Center).GetSafeNormal();if(Away.IsNearlyZero())Away=FVector::UpVector;
    if(bHasIdleView)
    {
        const auto ScreenOffset=ReadabilityOffset((V.Position+Destination)*.5f);
        Away=(ViewRight*ScreenOffset.X+ViewUp*ScreenOffset.Y).GetSafeNormal();if(Away.IsNearlyZero())Away=ViewUp;
    }
    const bool StartBlocked=IsIdlePositionBlocked(V.Position,1.02f);
    bool Clear=false;
    for(int Attempt=0;Attempt<7;++Attempt)
    {
        V.Path=FLiveBezierFlight::BuildSpatial(V.Position,Destination,V.Velocity/V.SpeedScale(Clock),FVector::ZeroVector,FMath::Max(bEvacuate?.25f:1.f,float(FVector::Dist(V.Position,Destination))/Speed),Arc,Weave,(bEvacuate?12000:7000)/2.5f,2600/1.5f);
        V.Path.KeepAboveLinearFloor(AnchorFootZ(),AnchorFootZ(),WeaponHeightMargin(Roster.GetStates()[Slot].WeaponType));
        bool LeftBlockedStart=!StartBlocked;Clear=true;
        for(int I=1;I<=32;++I)
        {
            const bool Blocked=IsIdlePositionBlocked(V.Path.Position(V.Path.Duration*I/32),1.02f);
            if(!Blocked)LeftBlockedStart=true;
            else if(LeftBlockedStart){Clear=false;break;}
        }
        Clear&=LeftBlockedStart;
        if(Clear)break;Arc+=Away*50;
    }
    if(!Clear&&!bEvacuate)
    {
        // A short outward move is preferable to retaining an unsafe long chord.
        FVector Outward=(V.Position-Center).GetSafeNormal();if(Outward.IsNearlyZero())Outward=FVector::UpVector;
        Destination=SafeIdlePosition(V.Position+Outward*80,true);
        V.Path=FLiveBezierFlight::BuildSpatial(V.Position,Destination,V.Velocity/V.SpeedScale(Clock),FVector::ZeroVector,1.2f,Outward*50,FVector::ZeroVector,7000/2.5f,2600/1.5f);
    }
    ShapeFlightHeight(Slot,AnchorFootZ());V.FollowReference=Center;
    V.Age=0;V.bPathValid=true;V.Phase=FLiveWeaponVisualState::EPhase::Wander;V.Target.Reset();V.bContinueCombo=false;
}
FVector ALiveWeaponSwarm::AlertPosition(int32 Slot)const
{
    const auto& V=Visuals[Slot];
    const auto R=OrderedRange(AlertRadiusMin,AlertRadiusMax,1);const auto H=OrderedRange(AlertHeightMin,AlertHeightMax);
    const double Radius=FMath::Lerp(R.X,R.Y,.5+.5*FMath::Sin(Clock*V.RadiusDriftRate+V.RadiusDriftPhase));
    const double Height=FMath::Lerp(H.X,H.Y,.5+.5*FMath::Sin(Clock*V.HeightDriftRate+V.HeightDriftPhase));
    FVector Position=SafeIdlePosition(ViewCenter+FVector(FMath::Cos(V.OrbitAngle)*Radius,FMath::Sin(V.OrbitAngle)*Radius,
        Height));
    Position.Z=FMath::Max(double(AnchorFootZ()+WeaponHeightMargin(Roster.GetStates()[Slot].WeaponType)),Position.Z);return Position;
}
void ALiveWeaponSwarm::BeginReturn(int32 Slot,bool bAfterAttack)
{
    auto& V=Visuals[Slot];
    if(bAfterAttack){V.bNeedsRecovery=true;V.RecoveryRemaining=-1;}
    // Keep this spirit's orbit phase. Re-deriving atan2 behind a running owner collapses the crowd.
    V.Phase=FLiveWeaponVisualState::EPhase::Returning;V.Age=0;V.bPathValid=false;V.FollowReference=ViewCenter;
    V.Target.Reset();V.bContinueCombo=false;
}
void ALiveWeaponSwarm::AdvanceReturn(int32 Slot,float DeltaSeconds)
{
    auto& V=Visuals[Slot];
    V.OrbitAngle=FMath::Fmod(V.OrbitAngle+FMath::DegreesToRadians(FMath::Max(0.f,AlertOrbitSpeed))*V.SpeedScale(Clock)*DeltaSeconds,2*PI);
    const FVector Desired=AlertPosition(Slot),Before=V.Position,Delta=Desired-Before;
    const float Speed=FMath::Max(700.f*V.SpeedScale(Clock),PlayerFollowSpeed);
    const FVector WantedVelocity=Delta.GetSafeNormal()*FMath::Min(double(Speed),Delta.Size()/DeltaSeconds);
    V.Velocity=FMath::VInterpTo(V.Velocity,WantedVelocity,DeltaSeconds,12);
    V.Position+=V.Velocity.GetClampedToMaxSize(Speed)*DeltaSeconds;
    V.ReferenceFloorZ=FMath::Lerp(V.ReferenceFloorZ,AnchorFootZ(),FMath::Clamp(DeltaSeconds*8,0.f,1.f));
    V.Position.Z=FMath::Max(V.Position.Z,double(V.ReferenceFloorZ+WeaponHeightMargin(Roster.GetStates()[Slot].WeaponType)));
    V.Velocity=(V.Position-Before)/DeltaSeconds;
    if(FVector::DistSquared(V.Position,Desired)<FMath::Square(60.f))CompleteReturn(Slot);
}
void ALiveWeaponSwarm::AdvanceAlert(int32 Slot,float DeltaSeconds)
{
    auto& V=Visuals[Slot];
    if(!WantsAlert(Slot)){BeginWander(Slot);return;}
    const auto RadiusRange=OrderedRange(AlertRadiusMin,AlertRadiusMax,1);
    if(FVector::DistSquared(V.Position,ViewCenter)>FMath::Square(FMath::Max(600.,RadiusRange.Y+250)))
    {BeginReturn(Slot,false);return;}
    V.OrbitAngle=FMath::Fmod(V.OrbitAngle+FMath::DegreesToRadians(FMath::Max(0.f,AlertOrbitSpeed))*V.SpeedScale(Clock)*DeltaSeconds,2*PI);
    const FVector Desired=AlertPosition(Slot),Before=V.Position;
    // Exponential convergence follows a moving owner without teleporting the world-space spirit.
    const FVector Approach=FMath::VInterpTo(V.Position,Desired,DeltaSeconds,12)-V.Position;
    V.Position+=Approach.GetClampedToMaxSize(FMath::Max(700.f*V.SpeedScale(Clock),PlayerFollowSpeed)*DeltaSeconds);
    V.Velocity=(V.Position-Before)/DeltaSeconds;
    V.ReferenceFloorZ=AnchorFootZ();
    
    // Orbit phase lag grows with radius/speed; being back near the owner is what
    // permits recovery, not catching an endlessly moving point on the circle.
    const double RecoveryRadius=RadiusRange.Y+60;
    if(FVector::DistSquared2D(V.Position,ViewCenter)<=FMath::Square(RecoveryRadius))
        V.RecoveryRemaining=FMath::Max(0.f,V.RecoveryRemaining-DeltaSeconds);
    if(V.RecoveryRemaining<=0)V.bNeedsRecovery=false;
}
void ALiveWeaponSwarm::CompleteReturn(int32 Slot)
{
    auto& V=Visuals[Slot];
    if(V.bNeedsRecovery&&V.RecoveryRemaining<0)V.RecoveryRemaining=V.Random.FRandRange(FMath::Max(0.f,PostReturnAlertMin),FMath::Max(FMath::Max(0.f,PostReturnAlertMin),PostReturnAlertMax));
    V.bPathValid=false;V.Age=0;
    if(WantsAlert(Slot))V.Phase=FLiveWeaponVisualState::EPhase::Alert;else BeginHover(Slot);
}
float ALiveWeaponSwarm::AnchorFootZ()const
{
    float Height=ViewCenter.Z;
    if(auto* Character=Cast<ACharacter>(Anchor.Get()))Height-=Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    return Height;
}
float ALiveWeaponSwarm::WeaponHeightMargin(ELiveWeaponType Type)const
{
    const auto* Def=Definition(Type);
    const FVector Scale=Def?Def->MeshScale:(Type==ELiveWeaponType::Sword?FVector(.1,.1,.65):FVector(.25));
    const UStaticMesh* Mesh=WeaponMeshes[uint8(Type)]->GetStaticMesh();
    const float Radius=Mesh?float((Mesh->GetBounds().BoxExtent*Scale.GetAbs()).Size()+(Mesh->GetBounds().Origin*Scale).Size()):40;
    return Radius+FMath::Max(0.f,FlightHeightMargin);
}
void ALiveWeaponSwarm::ShapeFlightHeight(int32 Slot,float EndFloorZ)
{
    auto& V=Visuals[Slot];V.PathFloorStartZ=V.ReferenceFloorZ;V.PathFloorEndZ=EndFloorZ;
    V.Path.KeepAboveLinearFloor(V.PathFloorStartZ,V.PathFloorEndZ,WeaponHeightMargin(Roster.GetStates()[Slot].WeaponType));
    V.Position=V.Path.Start;
}
void ALiveWeaponSwarm::BeginStrike(int32 Slot,ALiveMonster* Target,FName SkillId,bool bNewCombo)
{
    auto& V=Visuals[Slot];const auto Type=Roster.GetStates()[Slot].WeaponType;const auto* Def=Definition(Type);
    if(bNewCombo){++V.AttackToken;V.ComboHits=0;V.HitTargets.Reset();V.bNeedsRecovery=false;}V.SkillId=SkillId;V.Target=Target;
    const FVector End=HitPoint(Target);FVector Direction=(End-V.Position).GetSafeNormal();if(Direction.IsNearlyZero())Direction=FVector::ForwardVector;
    FVector Side=FVector::CrossProduct(Direction,FVector::UpVector).GetSafeNormal();
    const float Speed=Def?Def->FlightSpeed:1900,Acceleration=Def?Def->FlightAcceleration:12000;
    const FVector Exit=(Direction.GetSafeNormal2D()+Side*V.Random.FRandRange(-.3f,.3f)+FVector::UpVector*.12f).GetSafeNormal()*Speed;
    V.Path=FLiveBezierFlight::BuildSpatial(V.Position,End,V.Velocity/V.SpeedScale(Clock),Exit,FMath::Max(.25f,float(FVector::Dist(V.Position,End))/(Speed*.8f)),FVector::UpVector*V.Random.FRandRange(80,230)+Side*V.Random.FRandRange(-90,90),Side*V.Random.FRandRange(35,95),Acceleration/2.5f,2600/1.5f);
    ShapeFlightHeight(Slot,float(End.Z)-HitRadius(Target));
    V.Age=0;V.bPathValid=true;V.bContinueCombo=false;V.Phase=FLiveWeaponVisualState::EPhase::Strike;
}
void ALiveWeaponSwarm::BeginDrift(int32 Slot,bool bContinue)
{
    auto& V=Visuals[Slot];const float Duration=V.Random.FRandRange(.13f,.23f);
    FVector Side=FVector::CrossProduct(V.Velocity.GetSafeNormal(),FVector::UpVector).GetSafeNormal();if(Side.IsNearlyZero())Side=ViewRight;
    FVector EndVelocity=V.Velocity.RotateAngleAxis(V.Random.FRandRange(-28,28),FVector::UpVector)*.65;
    EndVelocity.Z=FMath::Max(180.,EndVelocity.Z);
    V.Path=FLiveBezierFlight::BuildSpatial(V.Position,V.Position+(V.Velocity+EndVelocity)*(.5f*Duration),V.Velocity/V.SpeedScale(Clock),EndVelocity/V.SpeedScale(Clock),Duration,FVector::UpVector*V.Random.FRandRange(5,20),Side*V.Random.FRandRange(-8,8),12000/2.5f,2600/1.5f);
    ShapeFlightHeight(Slot,V.ReferenceFloorZ);
    V.Age=0;V.Phase=FLiveWeaponVisualState::EPhase::Drift;V.bPathValid=true;V.bContinueCombo=bContinue;V.Target.Reset();V.SkillId=NAME_None;
}
bool ALiveWeaponSwarm::ApplyImpact(FLiveWeaponHandle ParticipantHandle,ALiveMonster* Target,FName SkillId,bool bFollowUp)
{
    if(!IsCurrent(ParticipantHandle)||!IsValid(Target)||!Target->IsAlive())return false;
    const auto S=Roster.GetStates()[ParticipantHandle.Index];if(!S.bParticipating)return false;const auto* Def=Definition(S.WeaponType);float Damage=Def?Def->Damage:1;
    const FString OwnerId=GetProgressionOwnerId(S.Profile.ParticipantId);
    if(GetWorld()->GetGameInstance())
    {
        const auto Stats=GetWorld()->GetGameInstance()->GetSubsystem<UCultivationSubsystem>()->GetParticipantSnapshot(OwnerId);
        if(!Stats.ParticipantId.IsEmpty())
        {
            Damage=float(FMath::Min(1.e30,FMath::Max(0.0,Stats.Attributes.Attack+Stats.Attributes.SpiritDamage)*Damage));
            if(DamageRandom.FRand()<Stats.Attributes.CriticalChance)Damage*=2;
        }
    }
    const uint32 Token=Visuals[ParticipantHandle.Index].AttackToken;
    const auto StillCurrent=[&](){return IsCurrent(ParticipantHandle)&&Roster.GetStates()[ParticipantHandle.Index].WeaponType==S.WeaponType&&Visuals[ParticipantHandle.Index].AttackToken==Token;};
    bool Applied=false;
    if(!bFollowUp)if(ULiveWeaponSkill* Skill=SkillObjects.FindRef(SkillId))
    {
        FLiveSkillContext Context;Context.ParticipantId=OwnerId;Context.Anchor=Anchor.Get();Context.Target=Target;Context.BaseDamage=Damage;
        const bool CanExecute=Skill->CanExecute(Context);if(!StillCurrent())return false;
        if(CanExecute)Applied=Skill->Execute(Context);
        if(!StillCurrent())return false;
        if(Applied)Combat.CommitSkill(ParticipantHandle,SkillId,Clock);
    }
    if(!StillCurrent())return false;
    if(!Applied&&IsValid(Target)&&Target->IsAlive())Applied=Target->ApplyWeaponDamage(Damage,OwnerId);
    if(Applied&&StillCurrent()){++TotalAttacks;if(bFollowUp)++TotalFollowUpHits;}
    return Applied;
}
void ALiveWeaponSwarm::AdvanceWeapon(int32 Slot,float DeltaSeconds)
{
    if(!Roster.GetStates()[Slot].bParticipating)return;
    const auto Handle=Roster.GetStates()[Slot].Handle;
    using EPhase=FLiveWeaponVisualState::EPhase;
    if(Visuals[Slot].Phase==EPhase::Alert){AdvanceAlert(Slot,DeltaSeconds);return;}
    if(Visuals[Slot].Phase==EPhase::Returning){AdvanceReturn(Slot,DeltaSeconds);return;}
    if(GetAlertLeaseCount(Roster.GetStates()[Slot].Profile.ParticipantId)>0)Visuals[Slot].bContinueCombo=false;
    if(WantsAlert(Slot)&&(Visuals[Slot].Phase==EPhase::Hover||Visuals[Slot].Phase==EPhase::Wander))BeginReturn(Slot,false);
    if(Visuals[Slot].Phase==EPhase::Returning){AdvanceReturn(Slot,DeltaSeconds);return;}
    if(Visuals[Slot].Phase==FLiveWeaponVisualState::EPhase::Hover)
    {
        auto& Idle=Visuals[Slot];Idle.Age+=DeltaSeconds*Idle.SpeedScale(Clock);Idle.HoldRemaining-=DeltaSeconds;
        const FVector HoverPosition=Idle.HoverOrigin+FVector(0,0,3*(1-FMath::Cos(Idle.Age*1.1f))*FMath::Sin(Idle.OrbitAngle));
        if(!IsIdlePositionBlocked(Idle.Position)&&IsIdlePositionBlocked(HoverPosition))
        {BeginWander(Slot,true);return;}
        Idle.Position=HoverPosition;
        Idle.Velocity=FVector(0,0,3.3f*FMath::Sin(Idle.Age*1.1f)*FMath::Sin(Idle.OrbitAngle)*Idle.SpeedScale(Clock));
        const FVector Center=Anchor.IsValid()?Anchor->GetActorLocation():GetActorLocation();
        
        Idle.RecoveryRemaining=FMath::Max(0.f,Idle.RecoveryRemaining-DeltaSeconds);
        if(Idle.RecoveryRemaining<=0)Idle.bNeedsRecovery=false;
        const bool OwnerApproaching=IsIdlePositionBlocked(Idle.Position,1.08f);
        if(OwnerApproaching)BeginWander(Slot,true);
        else if(Idle.HoldRemaining<=0||FVector::DistSquared2D(Center,Idle.FollowReference)>FMath::Square(FMath::Max(30.f,FollowRefreshDistance)))BeginWander(Slot);
        return;
    }
    if(!Visuals[Slot].bPathValid)BeginWander(Slot);
    if(Visuals[Slot].Phase==FLiveWeaponVisualState::EPhase::Wander&&!Visuals[Slot].bIdleEvacuating&&Visuals[Slot].Age>.08f&&IsIdlePositionBlocked(Visuals[Slot].Position,1.08f))BeginWander(Slot,true);
    auto& V=Visuals[Slot];const FVector Before=V.Position;const float Tempo=V.SpeedScale(Clock);
    const float NextAge=FMath::Min(V.Age+DeltaSeconds*Tempo,V.Path.Duration);
    if(V.Phase==EPhase::Wander&&!IsIdlePositionBlocked(V.Position)&&IsIdlePositionBlocked(V.Path.Position(NextAge)))
    {BeginWander(Slot,true);return;}
    V.Age=NextAge;
    V.Position=V.Path.Position(V.Age);V.Velocity=V.Path.Velocity(V.Age)*Tempo;
    V.ReferenceFloorZ=FMath::Lerp(V.PathFloorStartZ,V.PathFloorEndZ,FMath::Clamp(V.Age/V.Path.Duration,0.f,1.f));
    
    if(V.Phase==FLiveWeaponVisualState::EPhase::Strike)
    {
        const auto Type=Roster.GetStates()[Slot].WeaponType;const auto* Def=Definition(Type);auto* Target=V.Target.Get();
        if(!Target||!Target->IsAlive()){BeginDrift(Slot,false);return;}
        if(FLiveBezierFlight::SweptBoxHits(Before,V.Position,HitPoint(Target),HitRadius(Target),Def?Def->AttackBoxHalfExtent:FVector(14,14,32)))
        {
            const uint32 Token=V.AttackToken;const bool Hit=ApplyImpact(Handle,Target,V.SkillId,V.ComboHits>0);
            if(!IsCurrent(Handle)||Roster.GetStates()[Slot].WeaponType!=Type||Visuals[Slot].AttackToken!=Token)return;
            auto& AfterCallback=Visuals[Slot];
            if(Hit){++AfterCallback.ComboHits;MaxComboHitsObserved=FMath::Max(MaxComboHitsObserved,AfterCallback.ComboHits);AfterCallback.HitTargets.Add(Target);}
            const bool Continue=Hit&&ShouldContinueCombo(Type,AfterCallback.ComboHits,Def?Def->FollowUpChance:.5f,Def?Def->MaxComboHits:4,AfterCallback.Random);
            BeginDrift(Slot,Continue);return;
        }
    }
    if(V.Age+KINDA_SMALL_NUMBER<V.Path.Duration)return;
    if(V.Phase==FLiveWeaponVisualState::EPhase::Drift&&V.bContinueCombo)
    {
        const auto* Def=Definition(Roster.GetStates()[Slot].WeaponType);TArray<TWeakObjectPtr<ALiveMonster>> Targets;
        GetWorld()->GetSubsystem<ULiveMonsterRegistrySubsystem>()->CollectInRange(V.Position,Def?Def->ChainSearchRadius:900,Targets);
        ALiveMonster* Best=nullptr;double Nearest=MAX_dbl;
        for(auto Weak:Targets)if(auto* M=Weak.Get();M&&M->IsAlive()&&!V.HitTargets.Contains(Weak))
        {const double D=FVector::DistSquared(V.Position,M->GetActorLocation());if(D<Nearest){Nearest=D;Best=M;}}
        if(Best){BeginStrike(Slot,Best,NAME_None,false);return;}
    }
    if(V.Phase==EPhase::Wander)BeginHover(Slot);else BeginReturn(Slot,true);
}
bool ALiveWeaponSwarm::StartArrow(int32 Slot,ALiveMonster* Target,FName SkillId)
{
    if(FreeArrows.IsEmpty())return false;const int I=FreeArrows.Pop(EAllowShrinking::No);auto& A=Arrows[I];auto& V=Visuals[Slot];
    const auto* Def=Definition(ELiveWeaponType::Bow);const FVector End=HitPoint(Target),Direction=(End-V.Position).GetSafeNormal();
    A=FLiveArrowVisual();A.Owner=Roster.GetStates()[Slot].Handle;A.AttackToken=++V.AttackToken;A.bActive=true;A.Target=Target;A.SkillId=SkillId;A.Position=V.Position;A.Velocity=V.Velocity;
    A.Path=FLiveBezierFlight::BuildSpatial(A.Position,End,A.Velocity,Direction*(Def?Def->FlightSpeed:1900),FMath::Max(.25f,float(FVector::Dist(End,A.Position))/1900),FVector::UpVector*V.Random.FRandRange(90,200),ViewRight*V.Random.FRandRange(-60,60),Def?Def->FlightAcceleration:12000,3000);
    A.Path.KeepAboveLinearFloor(V.ReferenceFloorZ,float(End.Z)-HitRadius(Target),26+FMath::Max(0.f,FlightHeightMargin));
    V.bPendingArrow=true;bArrowsNeedRenderUpdate=true;if(V.Phase==FLiveWeaponVisualState::EPhase::Hover)BeginWander(Slot);return true;
}
void ALiveWeaponSwarm::ReleaseArrow(int32 I)
{
    auto& A=Arrows[I];if(!A.bActive)return;
    if(IsCurrent(A.Owner)&&!A.bHit&&Visuals[A.Owner.Index].AttackToken==A.AttackToken)
    {Visuals[A.Owner.Index].bPendingArrow=false;BeginReturn(A.Owner.Index,true);}
    A=FLiveArrowVisual();FreeArrows.Add(I);bArrowsNeedRenderUpdate=true;
}
void ALiveWeaponSwarm::AdvanceArrows(float DeltaSeconds)
{
    for(int I=0;I<Arrows.Num();++I)
    {
        auto& A=Arrows[I];if(!A.bActive)continue;
        if(!IsCurrent(A.Owner)||Roster.GetStates()[A.Owner.Index].WeaponType!=ELiveWeaponType::Bow||(!A.bHit&&Visuals[A.Owner.Index].AttackToken!=A.AttackToken)){ReleaseArrow(I);continue;}
        if(A.bHit){A.Position+=A.Velocity*DeltaSeconds;A.AfterHit+=DeltaSeconds;if(A.AfterHit>.16f)ReleaseArrow(I);continue;}
        const FVector Before=A.Position;A.Age=FMath::Min(A.Age+DeltaSeconds,A.Path.Duration);A.Position=A.Path.Position(A.Age);A.Velocity=A.Path.Velocity(A.Age);
        auto* M=A.Target.Get();if(!M||!M->IsAlive()){ReleaseArrow(I);continue;}
        const auto* Def=Definition(ELiveWeaponType::Bow);
        if(FLiveBezierFlight::SweptBoxHits(Before,A.Position,HitPoint(M),HitRadius(M),Def?Def->AttackBoxHalfExtent:FVector(10,10,25)))
        {
            const auto ParticipantHandle=A.Owner;ApplyImpact(ParticipantHandle,M,A.SkillId,false);
            if(!A.bActive||!IsCurrent(ParticipantHandle))continue;
            A.bHit=true;A.Velocity.Z=FMath::Max(60.,A.Velocity.Z);Visuals[ParticipantHandle.Index].bPendingArrow=false;BeginReturn(ParticipantHandle.Index,true);
        }
        else if(A.Age+KINDA_SMALL_NUMBER>=A.Path.Duration)ReleaseArrow(I);
    }
}
void ALiveWeaponSwarm::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);if(!Anchor.IsValid()||!HasAuthority()||!FMath::IsFinite(DeltaSeconds)||DeltaSeconds<=0)return;Clock+=DeltaSeconds;RefreshReadabilityView();
    EnsurePersonalSword();
    float MaxRange=FMath::Max(DefaultAttackRange,AlertDetectionRange);for(ULiveWeaponDefinition* D:Definitions)if(D)MaxRange=FMath::Max(MaxRange,D->AttackRange);
    TArray<TWeakObjectPtr<ALiveMonster>> Targets;
    if(PresentationMode==ELiveWeaponPresentationMode::Gameplay)
        GetWorld()->GetSubsystem<ULiveMonsterRegistrySubsystem>()->CollectInRange(Anchor->GetActorLocation(),MaxRange,Targets);
    bNearbyThreat=PresentationMode==ELiveWeaponPresentationMode::Alert||!Targets.IsEmpty();
    // Bounded substeps cover fast sweeps without replaying many seconds after a hitch.
    float Remaining=FMath::Min(DeltaSeconds,.1f);
    while(Remaining>SMALL_NUMBER)
    {
        const float Step=FMath::Min(Remaining,1.f/60);for(int Slot=0;Slot<Visuals.Num();++Slot)AdvanceWeapon(Slot,Step);
        AdvanceArrows(Step);Remaining-=Step;
    }
    if(bAutomaticAttacks&&PresentationMode==ELiveWeaponPresentationMode::Gameplay)
    {
        const auto Ready=[&](const FLiveWeaponState& S)
        {
            if(GetWorld()->GetGameInstance())if(auto* Health=GetWorld()->GetGameInstance()->GetSubsystem<UCultivationSubsystem>()->GetAnchorHealth();Health&&Health->bDead)return false;
            const auto& V=Visuals[S.Handle.Index];
            if(GetAlertLeaseCount(S.Profile.ParticipantId)>0)return false;
            if(V.bNeedsRecovery||V.Phase==FLiveWeaponVisualState::EPhase::Strike||V.Phase==FLiveWeaponVisualState::EPhase::Drift)return false;
            return (S.WeaponType!=ELiveWeaponType::Bow||(!V.bPendingArrow&&!FreeArrows.IsEmpty()))&&FindPrimaryTarget(S.WeaponType,V.Position,Targets);
        };
        TArray<FLiveAttackRequest> Requests;Combat.Advance(Clock,!Targets.IsEmpty(),Requests,Ready,[&](const FLiveWeaponState& S,FName Id)
        {
            ULiveWeaponSkill* Skill=SkillObjects.FindRef(Id);auto* Target=FindPrimaryTarget(S.WeaponType,Visuals[S.Handle.Index].Position,Targets);if(!Skill||!Target)return false;
            const auto* Def=Definition(S.WeaponType);FLiveSkillContext C;C.ParticipantId=GetProgressionOwnerId(S.Profile.ParticipantId);C.Anchor=Anchor.Get();C.Target=Target;C.BaseDamage=Def?Def->Damage:1;
            if(GetWorld()->GetGameInstance()){const auto Stats=GetWorld()->GetGameInstance()->GetSubsystem<UCultivationSubsystem>()->GetParticipantSnapshot(C.ParticipantId);if(!Stats.ParticipantId.IsEmpty())C.BaseDamage*=float(Stats.Attributes.Attack+Stats.Attributes.SpiritDamage);}
            return Skill->CanExecute(C);
        });
        for(const auto& R:Requests)if(IsCurrent(R.Participant)&&Roster.GetStates()[R.Participant.Index].WeaponType==R.WeaponType&&Roster.GetStates()[R.Participant.Index].WeaponRevision==R.WeaponRevision)
        {
            const auto& S=Roster.GetStates()[R.Participant.Index];auto* Target=FindPrimaryTarget(S.WeaponType,Visuals[R.Participant.Index].Position,Targets);if(!Target)continue;
            if(S.WeaponType==ELiveWeaponType::Bow){if(StartArrow(R.Participant.Index,Target,R.SkillId))++TotalPrimaryLaunched;}
            else {BeginStrike(R.Participant.Index,Target,R.SkillId,true);++TotalPrimaryLaunched;}
        }
    }
    UpdatePresentation(DeltaSeconds);
}
FTransform ALiveWeaponSwarm::WeaponTransform(const FQuat& Rotation,const FVector& Position,const FVector& Scale)const
{
    if(PresentationMode==ELiveWeaponPresentationMode::Gameplay)return FTransform(Rotation,Position,Scale);
    const FQuat MeshRotation=Rotation*PresentationMeshRotation.Quaternion();
    FVector Center=Position;
    if(bConstrainPresentationHeight)
    {
        const double AnchorZ=Anchor.IsValid()?Anchor->GetActorLocation().Z:GetActorLocation().Z;
        double VerticalExtent=0;
        if(const auto* Def=Definition(ELiveWeaponType::Sword);Def&&Def->Mesh)
        {
            const FVector Extent=Def->Mesh->GetBounds().BoxExtent*Scale.GetAbs();
            VerticalExtent=FMath::Abs(MeshRotation.RotateVector(FVector(Extent.X,0,0)).Z)+FMath::Abs(MeshRotation.RotateVector(FVector(0,Extent.Y,0)).Z)+FMath::Abs(MeshRotation.RotateVector(FVector(0,0,Extent.Z)).Z);
        }
        const double Low=AnchorZ+PresentationHeightRange.X+VerticalExtent,High=AnchorZ+PresentationHeightRange.Y-VerticalExtent;
        Center.Z=Low<=High?FMath::Clamp(Center.Z,Low,High):(Low+High)*.5;
    }
    return FTransform(MeshRotation,Center-MeshRotation.RotateVector(PresentationMeshCenter*Scale),Scale);
}
void ALiveWeaponSwarm::UpdatePresentation(float DeltaSeconds)
{
    for(int Type=0;Type<4;++Type)
    {
        auto& Transforms=PresentationTransforms[Type];Transforms.Reset(RenderSlots[Type].Num());const auto* Def=Definition(ELiveWeaponType(Type));
        for(int Slot:RenderSlots[Type])
        {
            if(!Roster.GetStates()[Slot].bParticipating){Transforms.Add(FTransform(FQuat::Identity,FVector::ZeroVector,FVector::ZeroVector));continue;}
            auto& V=Visuals[Slot];FQuat Desired=V.Velocity.SizeSquared()<225?V.IdleRotation:FRotationMatrix::MakeFromZ(V.Velocity.GetSafeNormal()).ToQuat();
            if(Type==1&&(V.Phase==FLiveWeaponVisualState::EPhase::Strike||V.Phase==FLiveWeaponVisualState::EPhase::Drift))Desired=Desired*FQuat(FVector::UpVector,float(Clock)*8);
            V.RenderRotation=FMath::QInterpTo(V.RenderRotation,Desired,DeltaSeconds,8);const FQuat Rotation=V.RenderRotation;
            Transforms.Add(WeaponTransform(Rotation,V.Position,Def?Def->MeshScale:(Type==0?FVector(.1f,.1f,.65f):FVector(.25f))));
            if(bDrawAttackHitboxes&&V.Phase==FLiveWeaponVisualState::EPhase::Strike)DrawDebugBox(GetWorld(),V.Position,Def?Def->AttackBoxHalfExtent:FVector(14,14,32),Rotation,FColor::Cyan,false,0,0,1);
        }
        if(!Transforms.IsEmpty())WeaponMeshes[Type]->BatchUpdateInstancesTransforms(0,Transforms,true,true,false);
    }
    if(GetActiveArrowCount()==0&&!bArrowsNeedRenderUpdate)return;
    auto& ArrowTransforms=ArrowPresentationTransforms;ArrowTransforms.Reset(Arrows.Num());
    for(const auto& A:Arrows)
    {
        const FQuat Rotation=A.Velocity.IsNearlyZero()?FQuat::Identity:FRotationMatrix::MakeFromZ(A.Velocity.GetSafeNormal()).ToQuat();
        ArrowTransforms.Add(FTransform(Rotation,A.Position,A.bActive?FVector(.03f,.03f,.5f):FVector::ZeroVector));
        const auto* Def=Definition(ELiveWeaponType::Bow);
        if(bDrawAttackHitboxes&&A.bActive&&!A.bHit)DrawDebugBox(GetWorld(),A.Position,Def?Def->AttackBoxHalfExtent:FVector(14,14,32),Rotation,FColor::Yellow,false,0,0,1);
    }
    ArrowMesh->BatchUpdateInstancesTransforms(0,ArrowTransforms,true,true,false);bArrowsNeedRenderUpdate=false;
}
