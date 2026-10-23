#if WITH_DEV_AUTOMATION_TESTS
#include "Cultivation/CultivationNameplate.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkCultivationPlateLayoutTest,"BoundlessImmortality.Frontend.CultivationNameplate.Geometry",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkCultivationPlateLayoutTest::RunTest(const FString&)
{
    for(float Scale:{.5f,1.f,2.f})
    {
        const auto G=CultivationNameplate::Layout(Scale);
        TestTrue(TEXT("Badge stays left of both title rows"),G.Badge.Max.X<=FMath::Min(G.Weapon.Min.X,G.Realm.Min.X));
        TestTrue(TEXT("Weapon title is strictly above realm title"),G.Weapon.GetCenter().Y<G.Realm.GetCenter().Y);
        TestTrue(TEXT("Badge center equals the midpoint between title centers"),FMath::IsNearlyEqual(G.Badge.GetCenter().Y,(G.Weapon.GetCenter().Y+G.Realm.GetCenter().Y)*.5));
        TestTrue(TEXT("Name sits below both title rows"),G.Name.Min.Y>=G.Realm.Max.Y);
    }
    TSet<FString> Regions;
    for(int32 I=0;I<34;++I)
    {
        const auto UV=CultivationNameplate::BannerUV(I);const auto Badge=CultivationNameplate::BadgeUV(I);
        TestTrue(TEXT("All realm art crops stay inside the supplied texture"),UV.Min.X>=0&&UV.Min.Y>=0&&UV.Max.X<=1&&UV.Max.Y<=1&&Badge.Min.X>=0&&Badge.Min.Y>=0&&Badge.Max.X<=1&&Badge.Max.Y<=1);
        TestTrue(TEXT("All realm crops have nonzero area"),UV.GetArea()>0&&Badge.GetArea()>0);
        Regions.Add(UV.ToString());
    }
    TestEqual(TEXT("All thirty-four realm stages retain distinct banners"),Regions.Num(),34);
    TestTrue(TEXT("Incorrect Cyberspace lettering uses exact runtime text"),CultivationNameplate::WeaponUV(TEXT("Cyberspace")).GetArea()==0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkCultivationPlateDistanceTest,"BoundlessImmortality.Frontend.CultivationNameplate.Distance",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkCultivationPlateDistanceTest::RunTest(const FString&)
{
    FCultivationNameplateDistanceSettings Settings;
    TestEqual(TEXT("Near sword uses normal size"),CultivationNameplate::DistanceScale(0,Settings),1.f);
    TestEqual(TEXT("Full size threshold"),CultivationNameplate::DistanceScale(250,Settings),1.f);
    TestEqual(TEXT("Far sword reaches minimum"),CultivationNameplate::DistanceScale(2500,Settings),.25f);
    TestEqual(TEXT("Farther swords stay at minimum"),CultivationNameplate::DistanceScale(10000,Settings),.25f);
    float Previous=1;
    for(int32 Distance=0;Distance<=3000;Distance+=25)
    {
        const float Scale=CultivationNameplate::DistanceScale(float(Distance),Settings);
        TestTrue(TEXT("Distance never enlarges or reverses scale"),Scale<=Previous&&Scale>=.25f&&Scale<=1.f);
        Previous=Scale;
    }
    TestEqual(TEXT("Quarter scale affects entire geometry"),CultivationNameplate::Layout(.25f).Size.X,CultivationNameplate::Layout(1).Size.X*.25);
    Settings.MinimumScale=4;Settings.MinSizeDistance=-1;
    TestTrue(TEXT("Invalid settings remain finite and capped"),FMath::IsFinite(CultivationNameplate::DistanceScale(100,Settings))&&CultivationNameplate::DistanceScale(100,Settings)<=1);
    return true;
}
#endif
