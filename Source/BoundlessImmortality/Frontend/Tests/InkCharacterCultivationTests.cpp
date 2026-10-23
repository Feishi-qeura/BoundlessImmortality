#if WITH_DEV_AUTOMATION_TESTS
#include "Frontend/InkCharacterCultivationWidget.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkDantianStageTest,"BoundlessImmortality.Frontend.CharacterCultivation.RealmBoundaries",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkDantianStageTest::RunTest(const FString&)
{
    using namespace InkCharacterCultivation;
    for(int32 I=0;I<34;++I)
    {
        const auto Expected=I<9?EInkDantianStage::Qi:I<13?EInkDantianStage::Foundation:I<16?EInkDantianStage::Crystal:I<22?EInkDantianStage::GoldenCore:EInkDantianStage::NascentSoul;
        TestEqual(FString::Printf(TEXT("Authoritative realm %d uses its major stage"),I),int32(StageForRealm(I)),int32(Expected));
    }
    TestEqual(TEXT("Invalid negative realm remains at Qi"),int32(StageForRealm(-1)),int32(EInkDantianStage::Qi));
    const auto Qi=StageColor(EInkDantianStage::Qi),Gold=StageColor(EInkDantianStage::GoldenCore),Pink=StageColor(EInkDantianStage::NascentSoul);
    TestTrue(TEXT("Qi is blue"),Qi.B>Qi.R&&Qi.B>Qi.G);
    TestTrue(TEXT("Golden Core is gold"),Gold.R>Gold.G&&Gold.G>Gold.B);
    TestTrue(TEXT("Nascent Soul is cherry blossom pink"),Pink.R>Pink.B&&Pink.B>Pink.G);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkSlotBorderTest,"BoundlessImmortality.Frontend.CharacterCultivation.SlotBorderBounds",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkSlotBorderTest::RunTest(const FString&)
{
    using namespace InkCharacterCultivation;
    for(const FVector2D Size:{FVector2D(100,104),FVector2D(112,104),FVector2D(50,52)})
    {
        const FVector2D Center(312,104);
        for(int32 I=0;I<100;++I)
        {
            const auto Delta=BorderPoint(Center,Size,I*.1f)-Center;
            TestTrue(TEXT("Motes never leave their own item frame"),FMath::Abs(Delta.X)<=Size.X*.5+.01&&FMath::Abs(Delta.Y)<=Size.Y*.5+.01);
            TestTrue(TEXT("Motes stay on the border instead of entering the central playfield"),FMath::Max(FMath::Abs(Delta.X)/(Size.X*.5),FMath::Abs(Delta.Y)/(Size.Y*.5))>.999);
        }
    }
    return true;
}
#endif
