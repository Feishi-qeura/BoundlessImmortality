#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Cultivation/CultivationRules.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCultivationStackRulesTest,"BoundlessImmortality.Cultivation.GiftStackFormula",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCultivationStackRulesTest::RunTest(const FString&)
{
    TestEqual(TEXT("two wands"),CultivationRules::StackedMultiplier(1.15,.5,2),1.725);
    TestEqual(TEXT("two potions"),CultivationRules::StackedMultiplier(3.25,.65,2),5.3625);
    FCultivationRuleConfig C;FCultivationParticipantRecord R;R.ParticipantId=TEXT("v");FCultivationGiftEvent E;E.Count=2;
    TestTrue(TEXT("batch accepted"),CultivationRules::ApplyGift(R,E,100,C));TestEqual(TEXT("batch wand expiry"),R.WandExpiresUtc,160.0);
    E.Count=1;CultivationRules::ApplyGift(R,E,161,C);TestEqual(TEXT("expired stacks restart"),R.WandCount,int64(1));TestEqual(TEXT("new expiry"),R.WandExpiresUtc,191.0);
    E.GiftId=ECultivationGift::LifePotion;E.Count=2;CultivationRules::ApplyGift(R,E,100,C);TestEqual(TEXT("potion six hours each"),R.PotionExpiresUtc,43300.0);
    E.Count=0;TestFalse(TEXT("zero count rejected"),CultivationRules::ApplyGift(R,E,100,C));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCultivationGiftSafetyTest,"BoundlessImmortality.Cultivation.GiftQuantityAndFiniteTimers",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCultivationGiftSafetyTest::RunTest(const FString&)
{
    FCultivationRuleConfig Rules;FCultivationParticipantRecord R;FCultivationGiftEvent E;E.Count=MAX_int32;
    TestTrue(TEXT("full positive int32 platform quantity accepted"),CultivationRules::ApplyGift(R,E,100,Rules));
    TestEqual(TEXT("gift stack quantity never silently truncated"),R.WandCount,int64(MAX_int32));
    TestEqual(TEXT("duration follows every actual unit"),R.WandExpiresUtc,100.0+30.0*MAX_int32);
    const auto Before=R;Rules.WandSeconds=-1;E.Count=1;
    TestFalse(TEXT("invalid negative duration cannot consume receipt"),CultivationRules::ApplyGift(R,E,100,Rules));
    TestEqual(TEXT("rejected tuning preserves stacks"),R.WandCount,Before.WandCount);TestEqual(TEXT("rejected tuning preserves duration"),R.WandExpiresUtc,Before.WandExpiresUtc);
    Rules.WandSeconds=MAX_dbl;E.Count=2;TestFalse(TEXT("overflowing UTC cannot be committed"),CultivationRules::ApplyGift(R,E,100,Rules));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCultivationLevelRulesTest,"BoundlessImmortality.Cultivation.RealmMaterialGate",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCultivationLevelRulesTest::RunTest(const FString&)
{
    FCultivationRuleConfig C;C.DefaultThreshold=100;C.ThresholdGrowth=1;FCultivationParticipantRecord V;
    CultivationRules::AddExperience(V,350,C);TestEqual(TEXT("viewer multi-realm breakthrough"),V.RealmIndex,3);TestEqual(TEXT("remainder preserved"),V.Cultivation,50.0);
    FCultivationParticipantRecord A;A.bIsAnchor=true;CultivationRules::AddExperience(A,350,C);TestEqual(TEXT("anchor remains gated"),A.RealmIndex,0);
    TestFalse(TEXT("no material no breakthrough"),CultivationRules::Breakthrough(A,C));A.Materials.Add(C.BreakthroughMaterialId,1);
    TestTrue(TEXT("material breakthrough"),CultivationRules::Breakthrough(A,C));TestEqual(TEXT("one tier per request"),A.RealmIndex,1);TestEqual(TEXT("surplus retained"),A.Cultivation,250.0);
    TestEqual(TEXT("34 realms"),C.RealmNames.Num(),34);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCultivationPermanentRulesTest,"BoundlessImmortality.Cultivation.AllUnlockedCoexist",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCultivationPermanentRulesTest::RunTest(const FString&)
{
    FCultivationRuleConfig C;FCultivationParticipantRecord R;FCultivationGiftEvent E;E.GiftId=ECultivationGift::RareChest;
    CultivationRules::ApplyGift(R,E,100,C);const double Before=CultivationRules::PersonalAttributes(R,C,0).Attack;CultivationRules::ApplyGift(R,E,101,C);
    TestEqual(TEXT("permanent bonus is not repeated"),CultivationRules::PersonalAttributes(R,C,0).Attack,Before);
    E.GiftId=ECultivationGift::MysteryAirdrop;CultivationRules::ApplyGift(R,E,102,C);TestTrue(TEXT("old permanent remains"),CultivationRules::Has(R,ECultivationGift::RareChest));
    TestEqual(TEXT("display equips newest"),R.TitleId,FName(TEXT("DivineBlade")));const auto P=CultivationRules::PersonalAttributes(R,C,1);
    TestEqual(TEXT("audience health absent"),P.MaxHealth,0.0);TestEqual(TEXT("audience defense absent"),P.Defense,0.0);TestTrue(TEXT("host tier buffs viewer attack"),P.Attack>Before);return true;
}
#endif
