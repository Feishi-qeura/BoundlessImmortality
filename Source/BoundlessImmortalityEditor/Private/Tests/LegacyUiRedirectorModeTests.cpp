#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "../LegacyUiRedirectorFixupCommandlet.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLegacyUiRedirectorModeTest,
    "BoundlessImmortality.Editor.LegacyUi.RedirectorMode",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLegacyUiRedirectorModeTest::RunTest(const FString&)
{
    bool Preflight=false;
    TestTrue(TEXT("Direct read-only mode accepted"),ULegacyUiRedirectorFixupCommandlet::ParseMode(TEXT("-PreflightOnly"),Preflight));
    TestTrue(TEXT("Read-only remains read-only"),Preflight);
    TestTrue(TEXT("UE commandlet selector and headless flags accepted"),ULegacyUiRedirectorFixupCommandlet::ParseMode(TEXT("-run=LegacyUiRedirectorFixup -PreflightOnly -unattended -NoLiveCoding -nullrhi -nop4"),Preflight));
    TestTrue(TEXT("Engine flags cannot enable deletion"),Preflight);
    TestTrue(TEXT("Explicit exact-six confirmation accepted"),ULegacyUiRedirectorFixupCommandlet::ParseMode(TEXT("-ConfirmExactLegacyUiRedirectorFixup"),Preflight));
    TestFalse(TEXT("Confirmation is not read-only"),Preflight);
    for(const TCHAR* Invalid:{TEXT(""),TEXT("-unattended"),TEXT("-PreflightOnly -ConfirmExactLegacyUiRedirectorFixup"),TEXT("-PreflightOnly -PreflightOnly"),TEXT("-PreflightOnly -Everything"),TEXT("-run=OtherCommandlet -PreflightOnly")})
        TestFalse(FString::Printf(TEXT("Reject %s"),Invalid),ULegacyUiRedirectorFixupCommandlet::ParseMode(Invalid,Preflight));
    return true;
}
#endif
