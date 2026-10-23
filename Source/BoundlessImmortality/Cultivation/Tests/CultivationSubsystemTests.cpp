#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Cultivation/CultivationSubsystem.h"
#include "Cultivation/CultivationHealthComponent.h"
#include "Engine/GameInstance.h"

namespace
{
UCultivationSubsystem* Ledger()
{
    auto* C=NewObject<UCultivationSubsystem>(NewObject<UGameInstance>());C->bPersistLocalRecords=false;C->SetClockForTests([](){return 1000.0;});C->RegisterParticipant(TEXT("anchor"),TEXT("主播"),TEXT(""),true);C->RegisterParticipant(TEXT("a"),TEXT("A"),TEXT(""));C->RegisterParticipant(TEXT("b"),TEXT("B"),TEXT(""));return C;
}
FCultivationGiftEvent Gift(const FString& Id,ECultivationGift Type,const FString& Message,int32 Count=1)
{
    FCultivationGiftEvent E;E.ParticipantId=Id;E.GiftId=Type;E.MessageId=Message;E.Count=Count;return E;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCultivationDurableGiftTest,"BoundlessImmortality.Cultivation.ReceiptImportReplay",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCultivationDurableGiftTest::RunTest(const FString&)
{
    auto* C=Ledger();auto E=Gift(TEXT("a"),ECultivationGift::LoveExplosion,TEXT("receipt"),2);
    TestTrue(TEXT("gift accepted"),C->ProcessGift(E));const auto Before=C->GetParticipantSnapshot(TEXT("a"));
    TestTrue(TEXT("duplicate is safely acknowledged"),C->ProcessGift(E));TestEqual(TEXT("duplicate no XP"),C->GetParticipantSnapshot(TEXT("a")).Cultivation,Before.Cultivation);
    auto* Copy=Ledger();TestTrue(TEXT("records imported"),Copy->ImportRecords(C->ExportRecords()));TestTrue(TEXT("import keeps receipt"),Copy->ProcessGift(E));TestEqual(TEXT("replay no extra realm"),Copy->GetParticipantSnapshot(TEXT("a")).RealmIndex,Before.RealmIndex);
    E.MessageId=TEXT("wrong-environment");E.bDevelopment=false;TestFalse(TEXT("platform and test data isolated"),C->ProcessGift(E));
    auto* Expired=Ledger();Expired->SetClockForTests([](){return 22000.0;});auto OldPotion=Gift(TEXT("a"),ECultivationGift::LifePotion,TEXT("old-potion"));OldPotion.EventUtcSeconds=100;
    TestTrue(TEXT("old outbox event restored"),Expired->ProcessGift(OldPotion));TestEqual(TEXT("outbox retains original six hour expiry"),Expired->GetParticipantSnapshot(TEXT("a")).PotionExpiresUtc,21700.0);TestEqual(TEXT("expired replay cannot renew personal buff"),Expired->GetParticipantSnapshot(TEXT("a")).PotionCount,int64(0));
    auto* Remote=Ledger();TestTrue(TEXT("authority overwrite"),Copy->ImportAuthoritativeRecords(Remote->ExportRecords()));E=Gift(TEXT("a"),ECultivationGift::LoveExplosion,TEXT("receipt"),2);TestTrue(TEXT("pending replay applies against remote"),Copy->ProcessGift(E));TestEqual(TEXT("authoritative recovery exactly once"),Copy->GetParticipantSnapshot(TEXT("a")).RealmIndex,Before.RealmIndex);
    TestFalse(TEXT("malformed import is atomic"),C->ImportRecords(TEXT("{\"records\":[{}]}")));TestEqual(TEXT("invalid import preserves ledger"),C->GetParticipantSnapshot(TEXT("a")).RealmIndex,Before.RealmIndex);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCultivationAuraTest,"BoundlessImmortality.Cultivation.AuraNoRecursion",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCultivationAuraTest::RunTest(const FString&)
{
    auto* C=Ledger();C->ProcessGift(Gift(TEXT("a"),ECultivationGift::RareChest,TEXT("soul-a")));C->ProcessGift(Gift(TEXT("b"),ECultivationGift::RareChest,TEXT("soul-b")));
    const auto A=C->GetParticipantSnapshot(TEXT("a")),B=C->GetParticipantSnapshot(TEXT("b"));
    TestTrue(TEXT("a receives b independent base only"),FMath::IsNearlyEqual(A.Attributes.Attack,A.PersonalAttributes.Attack+B.PersonalAttributes.Attack*.2));
    TestTrue(TEXT("anchor receives holder attributes"),FMath::IsNearlyEqual(C->GetParticipantSnapshot(TEXT("anchor")).Attributes.Attack,10+(A.PersonalAttributes.Attack+B.PersonalAttributes.Attack)*.5));
    C->ProcessGift(Gift(TEXT("a"),ECultivationGift::SuperJet,TEXT("jet-a")));C->ProcessGift(Gift(TEXT("b"),ECultivationGift::SuperJet,TEXT("jet-b")));
    TestTrue(TEXT("global flat only one copy"),FMath::IsNearlyEqual(C->GetParticipantSnapshot(TEXT("a")).Attributes.Attack,A.Attributes.Attack+200000));
    TestEqual(TEXT("flat never grants audience health"),C->GetParticipantSnapshot(TEXT("a")).Attributes.MaxHealth,0.0);
    C->ProcessGift(Gift(TEXT("a"),ECultivationGift::MysteryAirdrop,TEXT("curse-a")));C->ProcessGift(Gift(TEXT("b"),ECultivationGift::MysteryAirdrop,TEXT("curse-b")));TestEqual(TEXT("enemy debuff one copy"),C->GetEnemyAttributeMultiplier(),.8);
    C->ProcessGift(Gift(TEXT("a"),ECultivationGift::SuperAirdrop,TEXT("immune-a")));C->ProcessGift(Gift(TEXT("b"),ECultivationGift::SuperAirdrop,TEXT("immune-b")));TestEqual(TEXT("immunity not compounded"),C->GetAnchorImmunityChance(),.2);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCultivationPotionLeaseTest,"BoundlessImmortality.Cultivation.PotionDepartureMigration",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCultivationPotionLeaseTest::RunTest(const FString&)
{
    auto* C=Ledger();C->ProcessGift(Gift(TEXT("a"),ECultivationGift::LifePotion,TEXT("potion"),2));
    TestTrue(TEXT("personal confirmed stacks"),FMath::IsNearlyEqual(C->GetParticipantSnapshot(TEXT("a")).EffectiveCultivationPerSecond,5.3625));
    TestTrue(TEXT("audience room multiplier"),FMath::IsNearlyEqual(C->GetParticipantSnapshot(TEXT("b")).EffectiveCultivationPerSecond,1.28));
    TestEqual(TEXT("anchor room multiplier"),C->GetParticipantSnapshot(TEXT("anchor")).EffectiveCultivationPerSecond,2.0);
    C->LeaveParticipant(TEXT("a"));TestTrue(TEXT("departure retains room potion"),FMath::IsNearlyEqual(C->GetParticipantSnapshot(TEXT("b")).EffectiveCultivationPerSecond,1.28));
    C->SetRoomPotionSources({});TestEqual(TEXT("authority revokes transferred source"),C->GetParticipantSnapshot(TEXT("b")).EffectiveCultivationPerSecond,1.0);
    C->RegisterParticipant(TEXT("a"),TEXT("A"),TEXT(""));TestTrue(TEXT("owner resumes retained personal buff"),C->GetParticipantSnapshot(TEXT("a")).EffectiveCultivationPerSecond>5.3);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCultivationDamageStarTest,"BoundlessImmortality.Cultivation.ActualDamageAndStarCharges",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCultivationDamageStarTest::RunTest(const FString&)
{
    auto* C=Ledger();C->RecordActualDamage(TEXT("a"),60,true);auto A=C->GetParticipantSnapshot(TEXT("a"));TestTrue(TEXT("rolling actual DPS fixed sixty denominator"),FMath::IsNearlyEqual(A.ActualDPS,1.0));
    TestEqual(TEXT("hit plus kill reward"),A.Cultivation,11.0);TestEqual(TEXT("kill drops breakthrough material to anchor"),C->GetParticipantSnapshot(TEXT("anchor")).BreakthroughMaterialCount,1);
    C->ProcessGift(Gift(TEXT("a"),ECultivationGift::Strawberry,TEXT("stars")));TestEqual(TEXT("threshold strict above half"),C->ConsumeStarHealing(.5001),0.0);
    for(int32 I=0;I<7;++I)TestEqual(TEXT("one heal per charge"),C->ConsumeStarHealing(.5),.2);TestEqual(TEXT("all seven cooldowns unavailable"),C->ConsumeStarHealing(.5),0.0);TestEqual(TEXT("star cooldown state visible"),C->GetParticipantSnapshot(TEXT("a")).ReadyStarCharges,0);
    TestEqual(TEXT("no resurrection"),C->ConsumeStarHealing(0),0.0);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCultivationExpiryTest,"BoundlessImmortality.Cultivation.ExpiryAndCooldownBoundaries",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCultivationExpiryTest::RunTest(const FString&)
{
    auto* C=Ledger();double Time=1000;C->SetClockForTests([&Time](){return Time;});
    C->ProcessGift(Gift(TEXT("a"),ECultivationGift::FairyWand,TEXT("wand"),2));C->ProcessGift(Gift(TEXT("a"),ECultivationGift::RareChest,TEXT("soul")));C->ProcessGift(Gift(TEXT("a"),ECultivationGift::SuperJet,TEXT("jet")));C->RecordActualDamage(TEXT("a"),120,false);
    TestTrue(TEXT("effects start immediately"),C->GetParticipantSnapshot(TEXT("b")).Attributes.Attack>200000);
    Time=1060;TestEqual(TEXT("wand expires at exact boundary"),C->GetParticipantSnapshot(TEXT("a")).WandCount,int64(0));TestEqual(TEXT("damage expires at exact sixty seconds"),C->GetParticipantSnapshot(TEXT("a")).ActualDPS,0.0);
    Time=1090;TestTrue(TEXT("soul expires at ninety while jet persists"),FMath::IsNearlyEqual(C->GetParticipantSnapshot(TEXT("b")).Attributes.Attack,200010.0));
    Time=1180;C->Tick(.1f);TestTrue(TEXT("soul retriggers at cooldown"),C->GetParticipantSnapshot(TEXT("b")).Attributes.Attack>200010);
    Time=1301;TestTrue(TEXT("jet refresh does not expire at old boundary"),C->GetParticipantSnapshot(TEXT("b")).Attributes.Attack>=200010);
    Time=1480;TestTrue(TEXT("same source never adds jet twice"),C->GetParticipantSnapshot(TEXT("b")).Attributes.Attack<400000);
    auto* Health=NewObject<UCultivationHealthComponent>();Health->Health=30;TestEqual(TEXT("overkill actual health loss capped"),Health->ApplyIncomingDamage(10000),30.0);TestTrue(TEXT("lethal damage sets dead"),Health->bDead);TestEqual(TEXT("dead cannot heal"),Health->Heal(100),0.0);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCultivationRealmFeedbackTest,"BoundlessImmortality.Cultivation.RealmFeedbackOnce",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCultivationRealmFeedbackTest::RunTest(const FString&)
{
    auto* C=Ledger();C->Rules.DefaultThreshold=100;C->Rules.ThresholdGrowth=1;double Time=1000;C->SetClockForTests([&Time](){return Time;});
    int32 Events=0,Previous=-1,Current=-1;C->OnRealmAdvanced.AddLambda([&](const FString& Id,int32 Old,int32 New){if(Id==TEXT("a")){++Events;Previous=Old;Current=New;}});
    C->AddCultivation(TEXT("a"),350);TestEqual(TEXT("one summarized event for multi-realm gain"),Events,1);TestEqual(TEXT("previous tier"),Previous,0);TestEqual(TEXT("final tier"),Current,3);
    TestTrue(TEXT("headplate pulse begins"),C->GetParticipantSnapshot(TEXT("a")).RealmAdvancePulse>.99);
    C->AddCultivation(TEXT("a"),.1);TestEqual(TEXT("ordinary tick does not announce"),Events,1);
    auto E=Gift(TEXT("a"),ECultivationGift::LoveExplosion,TEXT("level-feedback"));C->ProcessGift(E);TestEqual(TEXT("gift emits one transition event"),Events,2);C->ProcessGift(E);TestEqual(TEXT("gift replay does not announce"),Events,2);
    Time=1002;TestEqual(TEXT("pulse expires without persisted state"),C->GetParticipantSnapshot(TEXT("a")).RealmAdvancePulse,0.0);return true;
}
#endif
