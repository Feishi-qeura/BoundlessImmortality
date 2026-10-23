#if WITH_DEV_AUTOMATION_TESTS
#include "Frontend/InkProgressState.h"
#include "Misc/AutomationTest.h"
#include "LiveWeapons/LiveGMTestSession.h"
#include "Cultivation/CultivationHealthComponent.h"
#include "Frontend/InkDamageFeedbackComponent.h"
#include "Camera/CameraTypes.h"
#include "Frontend/InkPresentationStyle.h"
#include "Frontend/InkAnchorStatusWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkBufferedProgressTest,"BoundlessImmortality.Frontend.Feedback.BufferedProgress",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkBufferedProgressTest::RunTest(const FString&)
{
    FInkProgressSettings Settings;
    FInkProgressState Small,Large;
    Small.Reset(1);Large.Reset(1);Small.SetTarget(.8f);Large.SetTarget(.2f);
    TestEqual(TEXT("Foreground is immediate"),Large.Value,.2f);
    TestEqual(TEXT("Trail initially preserves old value"),Large.Trail,1.f);
    Large.Tick(.06f,Settings);TestEqual(TEXT("Trail holds before delay"),Large.Trail,1.f);
    Small.Tick(.2f,Settings);Large.Tick(.14f,Settings);
    TestTrue(TEXT("Larger gap moves faster in same active time"),1-Large.Trail>1-Small.Trail);
    FInkProgressState SmallGap,LargeGap;SmallGap.Reset(1);LargeGap.Reset(1);SmallGap.SetTarget(.8f);LargeGap.SetTarget(.2f);
    int32 SmallFrames=0,LargeFrames=0;
    for(int32 I=1;I<=600;++I){SmallGap.Tick(1.f/60,Settings);LargeGap.Tick(1.f/60,Settings);if(!SmallFrames&&SmallGap.Trail==SmallGap.Value)SmallFrames=I;if(!LargeFrames&&LargeGap.Trail==LargeGap.Value)LargeFrames=I;}
    TestTrue(TEXT("Larger initial damage finishes catchup sooner"),LargeFrames>0&&LargeFrames<SmallFrames);
    for(int32 I=0;I<300;++I)Large.Tick(1.f/60,Settings);
    TestEqual(TEXT("Trail converges exactly"),Large.Trail,Large.Value);
    Large.SetTarget(.9f);TestEqual(TEXT("Healing foreground is immediate"),Large.Value,.9f);
    TestTrue(TEXT("Growth retains trailing layer"),Large.Trail<Large.Value);
    Large.Reset(.3f);TestEqual(TEXT("Reset clears previous threshold"),Large.Trail,.3f);
    FInkProgressContext Context;
    TestTrue(TEXT("Initial identity starts a new scale"),Context.Update(TEXT("a"),0,100,100));
    TestFalse(TEXT("Damage on the same scale keeps the trail"),Context.Update(TEXT("a"),0,100,100));
    TestTrue(TEXT("Realm reset even with same denominator"),Context.Update(TEXT("a"),1,100,100));
    TestTrue(TEXT("Threshold change resets scale"),Context.Update(TEXT("a"),1,100,200));
    TestTrue(TEXT("Identity change resets scale"),Context.Update(TEXT("b"),1,100,200));
    Large.SetTarget(-2);TestEqual(TEXT("Fractions clamp"),Large.Value,0.f);
    Large.SetTarget(std::numeric_limits<float>::quiet_NaN());Large.Tick(-1,Settings);
    TestTrue(TEXT("Invalid inputs cannot poison state"),FMath::IsFinite(Large.Value)&&FMath::IsFinite(Large.Trail));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkDamageTierTest,"BoundlessImmortality.Frontend.Feedback.DamageTiers",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkDamageTierTest::RunTest(const FString&)
{
    const double Values[]={.199,.2,.255,.26,.419,.42,.6,.601};
    const int32 Expected[]={0,1,1,2,2,3,3,4};
    for(int32 I=0;I<8;++I)TestEqual(*FString::Printf(TEXT("Damage ratio %.3f"),Values[I]),InkFeedback::DamageTier(Values[I]),Expected[I]);
    TestEqual(TEXT("Invalid ratio has no shake"),InkFeedback::DamageTier(std::numeric_limits<double>::quiet_NaN()),0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkGMIsolationTest,"BoundlessImmortality.Frontend.Feedback.GMIsolation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkGMIsolationTest::RunTest(const FString&)
{
    TestFalse(TEXT("Ordinary local demo does not require a QA storage label"),ULiveGMTestSession::IsIsolatedCommandLine(TEXT("-GWLocalTest")));
    TestTrue(TEXT("Dedicated Saved directory permits QA"),ULiveGMTestSession::IsIsolatedCommandLine(TEXT("-saveddirsuffix=CohesionUIQA")));
    TestFalse(TEXT("Empty suffix is not isolation"),ULiveGMTestSession::IsIsolatedCommandLine(TEXT("-saveddirsuffix=")));
    TestFalse(TEXT("Path traversal is not an isolated suffix"),ULiveGMTestSession::IsIsolatedCommandLine(TEXT("-saveddirsuffix=CohesionUI/../../Saved")));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkSingleDamageEventTest,"BoundlessImmortality.Frontend.Feedback.SingleDamageEvent",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkSingleDamageEventTest::RunTest(const FString&)
{
    auto* Health=NewObject<UCultivationHealthComponent>();auto* Feedback=NewObject<UInkDamageFeedbackComponent>();Feedback->Observe(Health);
    Health->ApplyIncomingDamage(26);TestEqual(TEXT("One actual damage event"),Feedback->DamageEventCount,1);TestEqual(TEXT("Actual loss determines tier"),Feedback->LastDamageTier,2);
    Health->Heal(20);TestEqual(TEXT("Healing cannot become another damage event"),Feedback->DamageEventCount,1);
    Health->ApplyIncomingDamage(5);Health->ApplyIncomingDamage(5);TestEqual(TEXT("Small hits are independent"),Feedback->LastDamageTier,0);
    Feedback->Observe(nullptr);Health->ApplyIncomingDamage(5);TestEqual(TEXT("Unbind stops callbacks"),Feedback->DamageEventCount,3);
    auto* Status=NewObject<UInkAnchorStatusWidget>();Status->ObserveDamage(Health);
    Health->ApplyIncomingDamage(5);Health->Heal(Health->MaxHealth);
    TestTrue(TEXT("A hit followed by healing retains its event flash"),Status->DamageFlash>0);
    Status->ObserveDamage(nullptr);Status->ObserveDamage(Health);Health->MaxHealth=50;Health->Health=50;
    TestEqual(TEXT("Changing maximum health does not create a hit flash"),Status->DamageFlash,0.f);
    Status->ObserveDamage(nullptr);Health->ApplyIncomingDamage(5);
    TestEqual(TEXT("Retired widget no longer receives hits"),Status->DamageFlash,0.f);
    auto* Modifier=NewObject<UInkDamageCameraModifier>();FInkHitShakeSettings Settings;Modifier->Pulse(Settings,1);
    FMinimalViewInfo POV;POV.Location=FVector(123,456,789);POV.Rotation=FRotator(10,20,0);const auto Baseline=POV;
    Modifier->ModifyCamera(.03f,POV);TestFalse(TEXT("Impulse changes current view"),POV.Rotation.Equals(Baseline.Rotation));
    POV=Baseline;Modifier->ModifyCamera(1.f,POV);TestTrue(TEXT("Feedback ends at exact base view"),POV.Location.Equals(Baseline.Location)&&POV.Rotation.Equals(Baseline.Rotation));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkPreservedBarsTest,"BoundlessImmortality.Frontend.Feedback.RegeneratedMeterArtwork",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkPreservedBarsTest::RunTest(const FString&)
{
    using namespace InkPresentationStyle;
    for(const auto Part:{ESurface::BarTrack,ESurface::BarFill})
    {
        const auto Brush=Surface(Part);
        TestTrue(TEXT("Requested regenerated meter art is loaded from its canonical folder"),Brush.GetResourceObject()&&Brush.GetResourceObject()->GetPathName().Contains(TEXT("/Textures/HUD/T_XianxiaMeters")));
    }
    return true;
}
#endif
