#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "LiveWeapons/LiveWeaponRoster.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveRosterTest,"BoundlessImmortality.LiveWeapons.Roster",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveRosterTest::RunTest(const FString&)
{
    FLiveWeaponRoster Roster;
    TestFalse(TEXT("empty identity rejected"),Roster.Join({}).IsValid());
    const auto First=Roster.Join({TEXT("a"),TEXT("同名"),TEXT("")});
    TestTrue(TEXT("first join valid"),First.IsValid());
    TestTrue(TEXT("repeat keeps handle"),Roster.Join({TEXT("a"),TEXT("新名字"),TEXT("")})==First);
    TestEqual(TEXT("duplicate does not add weapon"),Roster.Num(),1);
    const auto Second=Roster.Join({TEXT("b"),TEXT("同名"),TEXT("")});
    TestTrue(TEXT("same display name different identity"),Second!=First);
    TestFalse(TEXT("unknown switch ignored"),Roster.SwitchWeapon(TEXT("missing"),ELiveWeaponType::Bow));
    TestTrue(TEXT("switch joined viewer"),Roster.SwitchWeapon(TEXT("a"),ELiveWeaponType::Bow));
    Roster.Join({TEXT("a"),TEXT("third"),TEXT("")});
    if(const auto* State=Roster.Find(TEXT("a"))) TestEqual(TEXT("repeat join preserves weapon"),State->WeaponType,ELiveWeaponType::Bow);
    else AddError(TEXT("joined viewer missing"));
    for(int32 Index=2;Index<1000;++Index) Roster.Join({FString::FromInt(Index),TEXT("viewer"),TEXT("")});
    TestEqual(TEXT("1000 unique viewers"),Roster.Num(),1000);
    Roster.Reset();
    const auto New=Roster.Join({TEXT("new"),TEXT("new"),TEXT("")});
    TestTrue(TEXT("reset invalidates old handles"),New!=First);
    return true;
}
#endif
