#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Frontend/UI/InkWheelPresentation.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkWheelResultTest,"BoundlessImmortality.Frontend.Visual.WheelResultStable",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkWheelResultTest::RunTest(const FString&)
{
    for(int32 Sector=0;Sector<8;++Sector)for(double Start:{0.,359.,720.})
    {
        FInkWheelPresentation Wheel;Wheel.Angle=Start;
        TestTrue(TEXT("Accept fixed result"),Wheel.Begin(TEXT("fixed-receipt"),Sector,8));
        Wheel.Advance(.2);const double Angle=Wheel.Angle;
        TestFalse(TEXT("Pending duplicate cannot restart"),Wheel.Begin(TEXT("another-receipt"),(Sector+1)%8,8));
        TestTrue(TEXT("Duplicate did not move angle"),Wheel.Angle==Angle);
        Wheel.Skip();TestEqual(TEXT("Pointer geometry agrees with fixed result"),Wheel.PointedSector(),Sector);
        TestFalse(TEXT("Already consumed receipt does not replay"),Wheel.Begin(TEXT("fixed-receipt"),Sector,8));
    }
    FInkWheelPresentation LongFrame;LongFrame.Begin(TEXT("long-frame"),7,8);LongFrame.Advance(30);
    TestFalse(TEXT("Long frame finishes"),LongFrame.bAnimating);TestEqual(TEXT("Long frame retains result"),LongFrame.PointedSector(),7);
    return true;
}
#endif
