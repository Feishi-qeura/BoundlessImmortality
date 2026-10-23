#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "LiveWeaponTestWorld.h"
#include "LiveWeapons/LiveWeaponSwarm.h"
#include "LiveWeapons/LiveMonster.h"
#include "GameFramework/Pawn.h"
#include "UObject/UnrealType.h"

namespace
{
bool SetRangeField(FAutomationTestBase& Test,ALiveWeaponSwarm* Swarm,const TCHAR* Name,float Value)
{
    auto* Field=FindFProperty<FFloatProperty>(Swarm->GetClass(),Name);
    if(!Test.TestNotNull(Name,Field))return false;
    Field->SetPropertyValue_InContainer(Swarm,Value);return true;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveSpatialRangeTest,"BoundlessImmortality.LiveWeapons.SpatialRanges.IndependentMotion",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveSpatialRangeTest::RunTest(const FString&)
{
    FLiveWeaponTestWorld W;auto* Swarm=W.World->SpawnActor<ALiveWeaponSwarm>();
    bool Ready=true;
    for(const auto& Field: {TPair<const TCHAR*,float>(TEXT("AlertRadiusMin"),300),{TEXT("AlertRadiusMax"),500},{TEXT("AlertHeightMin"),220},{TEXT("AlertHeightMax"),380},
        {TEXT("IdleRadiusMin"),350},{TEXT("IdleRadiusMax"),550},{TEXT("IdleHeightMin"),220},{TEXT("IdleHeightMax"),380}})
        Ready=SetRangeField(*this,Swarm,Field.Key,Field.Value)&&Ready;
    if(!Ready)return false;
    auto* Owner=W.World->SpawnActor<APawn>();Swarm->SetAnchor(Owner);Swarm->bAutomaticAttacks=false;
    Swarm->CharacterAvoidanceExtent=FVector(1);Swarm->AlertOrbitSpeed=0;
    for(int I=0;I<1000;++I)
    {
        const FString Id=FString::FromInt(I);Swarm->JoinParticipant({Id,TEXT("viewer"),TEXT("")});
        const FVector P=Swarm->GetWeaponPosition(Id);
        if(P.Size2D()<349.9||P.Size2D()>550.1||P.Z<219.9||P.Z>380.1){AddError(TEXT("Initial idle positions must respect explicit radius/height intervals"));return false;}
    }
    auto* Enemy=W.World->SpawnActor<ALiveMonster>(FVector(1200,0,45),FRotator::ZeroRotator);Enemy->Health=100000;
    for(int I=0;I<600;++I)Swarm->Tick(1.f/60);
    TArray<FVector> Before;float MinR=10000,MaxR=0,MinZ=10000,MaxZ=-10000;
    for(int I=0;I<1000;++I)
    {
        const FVector P=Swarm->GetWeaponPosition(FString::FromInt(I));Before.Add(P);
        MinR=FMath::Min(MinR,float(P.Size2D()));MaxR=FMath::Max(MaxR,float(P.Size2D()));MinZ=FMath::Min(MinZ,float(P.Z));MaxZ=FMath::Max(MaxZ,float(P.Z));
    }
    TestTrue(TEXT("Thousand spirits respect alert radius interval"),MinR>=299&&MaxR<=501);
    TestTrue(TEXT("Thousand spirits respect alert height interval"),MinZ>=219&&MaxZ<=381);
    TestTrue(TEXT("Independent radial distribution is wide"),MaxR-MinR>130);
    TestTrue(TEXT("Independent vertical distribution is wide"),MaxZ-MinZ>100);
    for(int I=0;I<120;++I)Swarm->Tick(1.f/60);
    int32 ChangedRadius=0,ChangedHeight=0;
    for(int I=0;I<1000;++I){const auto P=Swarm->GetWeaponPosition(FString::FromInt(I));ChangedRadius+=FMath::Abs(P.Size2D()-Before[I].Size2D())>3;ChangedHeight+=FMath::Abs(P.Z-Before[I].Z)>3;}
    TestTrue(TEXT("Radius drifts independently over time"),ChangedRadius>800);
    TestTrue(TEXT("Height drifts independently over time"),ChangedHeight>800);
    SetRangeField(*this,Swarm,TEXT("AlertRadiusMin"),420);SetRangeField(*this,Swarm,TEXT("AlertRadiusMax"),420);
    SetRangeField(*this,Swarm,TEXT("AlertHeightMin"),260);SetRangeField(*this,Swarm,TEXT("AlertHeightMax"),260);
    for(int I=0;I<300;++I)Swarm->Tick(1.f/60);
    float Error=0;for(int I=0;I<1000;++I){const auto P=Swarm->GetWeaponPosition(FString::FromInt(I));Error=FMath::Max(Error,float(FMath::Max(FMath::Abs(P.Size2D()-420),FMath::Abs(P.Z-260))));}
    TestTrue(TEXT("Equal bounds disable radial and vertical randomness"),Error<1);
    // Reversed fields must normalize into a usable interval.
    SetRangeField(*this,Swarm,TEXT("AlertRadiusMin"),500);SetRangeField(*this,Swarm,TEXT("AlertRadiusMax"),300);
    for(int I=0;I<300;++I)Swarm->Tick(1.f/60);
    bool Bounded=true;for(int I=0;I<1000;++I){const double R=Swarm->GetWeaponPosition(FString::FromInt(I)).Size2D();Bounded&=R>=299&&R<=501;}
    TestTrue(TEXT("Reversed bounds are normalized safely"),Bounded);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveSpatialMigrationTest,"BoundlessImmortality.LiveWeapons.SpatialRanges.LegacyInheritance",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveSpatialMigrationTest::RunTest(const FString&)
{
    FLiveWeaponTestWorld W;auto* Parent=W.World->SpawnActor<ALiveWeaponSwarm>();auto* Child=W.World->SpawnActor<ALiveWeaponSwarm>();
    // An old child/placed instance receives the parent's newly edited ranges before PostLoad.
    Parent->AlertRadiusMin=Child->AlertRadiusMin=700;Parent->AlertRadiusMax=Child->AlertRadiusMax=900;
    Parent->IdleHeightMin=Child->IdleHeightMin=300;Parent->IdleHeightMax=Child->IdleHeightMax=500;
    Child->MigrateLegacySpatialRanges(Parent);
    TestEqual(TEXT("Inherited migrated radius is not replaced by stale legacy data"),Child->AlertRadiusMin,700.f);
    TestEqual(TEXT("Inherited migrated height is not replaced by stale legacy data"),Child->IdleHeightMax,500.f);
    Child->AlertRingWidth=100;Child->AlertRingRadius=400;Child->FloatHeight=200;Child->IdleHeightSpread=120;
    Child->MigrateLegacySpatialRanges(Parent);
    TestEqual(TEXT("Locally overridden legacy radius is converted"),Child->AlertRadiusMin,350.f);
    TestEqual(TEXT("Locally overridden legacy radius keeps full width"),Child->AlertRadiusMax,450.f);
    TestEqual(TEXT("Locally overridden legacy idle lower height is converted"),Child->IdleHeightMin,120.f);
    TestEqual(TEXT("Locally overridden legacy idle upper height is converted"),Child->IdleHeightMax,250.f);
    return true;
}
#endif
