#if WITH_DEV_AUTOMATION_TESTS
#include "Frontend/InkAttributeRating.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkAttributeRatingTest,"BoundlessImmortality.Frontend.AttributeRating.MajorRealmAndBoundaries",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkAttributeRatingTest::RunTest(const FString&)
{
    const int32 Starts[]={0,9,13,16,22,23,24,26,31,32,33},Ends[]={8,12,15,21,22,23,25,30,31,32,33};
    for(bool V14:{false,true})for(int32 Major=0;Major<11;++Major)
    {
        FInkAttributeRatingContext C;C.bV14Rules=V14;C.RealmIndex=Starts[Major];
        for(const FName Id:{TEXT("Health"),TEXT("Attack"),TEXT("Defense"),TEXT("SpiritDamage"),TEXT("Critical"),TEXT("Evasion"),TEXT("MoveSpeed"),TEXT("CultivationSpeed")})
        {
            const auto Bounds=UInkAttributeRatingLibrary::Thresholds(Id,C);TestEqual(TEXT("Seven thresholds"),Bounds.Num(),7);
            for(int32 Grade=0;Grade<7;++Grade)
            {
                TestEqual(TEXT("Boundary is inclusive"),int32(UInkAttributeRatingLibrary::Evaluate(Id,Bounds[Grade],C)),Grade);
                if(Grade)TestEqual(TEXT("Below boundary remains lower grade"),int32(UInkAttributeRatingLibrary::Evaluate(Id,Bounds[Grade]-1.e-7,C)),Grade-1);
            }
            for(int32 Minor=Starts[Major];Minor<=Ends[Major];++Minor)
            {auto S=C;S.RealmIndex=Minor;TestEqual(TEXT("Major identity"),UInkAttributeRatingLibrary::MajorIndex(Minor),Major);TestTrue(TEXT("Minor stages keep same thresholds"),UInkAttributeRatingLibrary::Thresholds(Id,S)==Bounds);}
        }
    }
    FInkAttributeRatingContext C;C.bV14Rules=true;
    TestEqual(TEXT("Qi health baseline from design"),UInkAttributeRatingLibrary::Thresholds(TEXT("Health"),C)[2],1000.);
    TestEqual(TEXT("Qi attainable high gear anchor"),UInkAttributeRatingLibrary::Thresholds(TEXT("Health"),C)[6],1000.+.95*(2200.*1.03-1000.));
    for(double Meditation:{.5,2.,4.})
    {auto Legacy=C;Legacy.bV14Rules=false;Legacy.LegacyMeditationPerSecond=Meditation;TestEqual(TEXT("Custom meditation speed remains baseline B"),int32(UInkAttributeRatingLibrary::Evaluate(TEXT("CultivationSpeed"),Meditation*Legacy.BaseAttributes.CultivationSpeed,Legacy)),int32(EInkAttributeRating::B));}
    for(const FName Id:{TEXT("Luck"),TEXT("Swordsmanship"),TEXT("Cultivation"),TEXT("RealmStone"),TEXT("Unknown")})TestEqual(TEXT("Nonformal fields ungraded"),int32(UInkAttributeRatingLibrary::Evaluate(Id,100,C)),int32(EInkAttributeRating::Unrated));
    for(double Invalid:{-1.,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()})TestEqual(TEXT("Invalid input ungraded"),int32(UInkAttributeRatingLibrary::Evaluate(TEXT("Health"),Invalid,C)),int32(EInkAttributeRating::Unrated));
    return true;
}
#endif
