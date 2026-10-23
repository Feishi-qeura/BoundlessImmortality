#include "CultivationRules.h"

FCultivationRuleConfig::FCultivationRuleConfig()
{
    const TCHAR* Names[]={TEXT("练气一阶"),TEXT("练气二阶"),TEXT("练气三阶"),TEXT("练气四阶"),TEXT("练气五阶"),TEXT("练气六阶"),TEXT("练气七阶"),TEXT("练气八阶"),TEXT("练气九阶"),TEXT("筑基初期"),TEXT("筑基中期"),TEXT("筑基后期"),TEXT("筑基大圆满"),TEXT("结晶一阶"),TEXT("结晶二阶"),TEXT("结晶三阶"),TEXT("金丹一阶"),TEXT("金丹二阶"),TEXT("金丹三阶"),TEXT("金丹四阶"),TEXT("金丹五阶"),TEXT("金丹六阶"),TEXT("元婴"),TEXT("化神"),TEXT("元神一阶"),TEXT("元神二阶"),TEXT("炼虚一阶"),TEXT("炼虚二阶"),TEXT("炼虚三阶"),TEXT("炼虚四阶"),TEXT("炼虚五阶"),TEXT("大乘"),TEXT("飞升"),TEXT("混元无极")};
    for(const TCHAR* Name:Names)RealmNames.Add(FText::FromString(Name));
}
namespace CultivationRules
{
double StackedMultiplier(double First,double Coefficient,int64 Count)
{
    if(Count<=0)return 1;
    if(!FMath::IsFinite(First)||!FMath::IsFinite(Coefficient)||First<0||Coefficient<0)return 1;
    return FMath::Clamp(First+double(Count-1)*First*Coefficient,0.0,1.e15);
}
double Threshold(const FCultivationRuleConfig& C,int32 Realm)
{
    if(Realm>=C.RealmNames.Num()-1)return 0;
    const double Value=C.ExperienceThresholds.IsValidIndex(Realm)?C.ExperienceThresholds[Realm]:C.DefaultThreshold*FMath::Pow(FMath::Max(1.0,C.ThresholdGrowth),FMath::Max(0,Realm));
    return FMath::IsFinite(Value)?FMath::Max(1.0,Value):1.e30;
}
int32 MaterialCost(const FCultivationRuleConfig& C,int32 Realm)
{
    return C.BreakthroughMaterialCosts.IsValidIndex(Realm)?FMath::Max(0,C.BreakthroughMaterialCosts[Realm]):1;
}
bool AddExperience(FCultivationParticipantRecord& R,double Amount,const FCultivationRuleConfig& C)
{
    if(!FMath::IsFinite(Amount)||Amount<=0)return false;
    R.Cultivation=FMath::Min(1.e30,R.Cultivation+Amount);
    if(!R.bIsAnchor)while(R.RealmIndex<C.RealmNames.Num()-1)
    {
        const double Required=Threshold(C,R.RealmIndex);if(R.Cultivation<Required)break;
        R.Cultivation-=Required;++R.RealmIndex;
    }
    ++R.Version;return true;
}
bool Breakthrough(FCultivationParticipantRecord& R,const FCultivationRuleConfig& C)
{
    if(!R.bIsAnchor||R.RealmIndex>=C.RealmNames.Num()-1||R.Cultivation<Threshold(C,R.RealmIndex))return false;
    const int32 Cost=MaterialCost(C,R.RealmIndex);if(R.Materials.FindRef(C.BreakthroughMaterialId)<Cost)return false;
    R.Materials.FindOrAdd(C.BreakthroughMaterialId)-=Cost;R.Cultivation-=Threshold(C,R.RealmIndex);++R.RealmIndex;++R.Version;return true;
}
FName GiftTitle(ECultivationGift G)
{
    switch(G)
    {
    case ECultivationGift::RareChest:return TEXT("SoulEater");
    case ECultivationGift::SuperJet:return TEXT("Cyberspace");
    case ECultivationGift::MysteryAirdrop:return TEXT("DivineBlade");
    case ECultivationGift::Strawberry:return TEXT("SevenStar");
    case ECultivationGift::SuperAirdrop:return TEXT("Godslayer");
    default:return NAME_None;
    }
}
FText TitleName(FName T)
{
    if(T==TEXT("SoulEater"))return FText::FromString(TEXT("噬魂剑"));
    if(T==TEXT("Cyberspace"))return FText::FromString(TEXT("无机态那"));
    if(T==TEXT("DivineBlade"))return FText::FromString(TEXT("驱煞神刃"));
    if(T==TEXT("SevenStar"))return FText::FromString(TEXT("七星光芒剑"));
    if(T==TEXT("Godslayer"))return FText::FromString(TEXT("弑神剑"));
    return FText::GetEmpty();
}
bool Has(const FCultivationParticipantRecord& R,ECultivationGift G){return R.UnlockedGifts.Contains(G);}
bool ApplyGift(FCultivationParticipantRecord& R,const FCultivationGiftEvent& E,double Now,const FCultivationRuleConfig& C)
{
    if(E.Count<=0||!FMath::IsFinite(Now)||Now<0||uint8(E.GiftId)>uint8(ECultivationGift::LifePotion))return false;
    const auto Nonnegative=[](double V){return FMath::IsFinite(V)&&V>=0;};
    double NewExpiry=0;
    if(E.GiftId==ECultivationGift::FairyWand||E.GiftId==ECultivationGift::LifePotion)
    {
        const bool Wand=E.GiftId==ECultivationGift::FairyWand;const double Seconds=Wand?C.WandSeconds:C.PotionSeconds;
        if(!Nonnegative(Seconds)||!Nonnegative(Wand?C.WandBase:C.PotionBase)||!Nonnegative(Wand?C.WandStackCoefficient:C.PotionStackCoefficient))return false;
        NewExpiry=FMath::Max(Now,Wand?R.WandExpiresUtc:R.PotionExpiresUtc)+Seconds*E.Count;
        if(!FMath::IsFinite(NewExpiry))return false;
    }
    if(E.GiftId==ECultivationGift::LoveExplosion&&(!Nonnegative(C.LoveExperience)||!FMath::IsFinite(C.LoveExperience*E.Count)))return false;
    if(E.GiftId==ECultivationGift::RareChest&&(!Nonnegative(C.SoulDuration)||!Nonnegative(C.SoulCooldown)||!Nonnegative(C.SoulPersonalMultiplier)||!FMath::IsFinite(Now+C.SoulDuration)||!FMath::IsFinite(Now+C.SoulCooldown)))return false;
    if(E.GiftId==ECultivationGift::SuperJet&&(!Nonnegative(C.JetDuration)||!Nonnegative(C.JetCooldown)||!Nonnegative(C.JetFlatBonus)||!FMath::IsFinite(Now+C.JetDuration)||!FMath::IsFinite(Now+C.JetCooldown)))return false;
    if(E.GiftId==ECultivationGift::Strawberry&&(!Nonnegative(C.StarCooldown)||!Nonnegative(C.StarHealFraction)||C.StarHealFraction>1||!FMath::IsFinite(Now+C.StarCooldown)))return false;
    const bool New=!Has(R,E.GiftId);R.UnlockedGifts.AddUnique(E.GiftId);
    const FName Title=GiftTitle(E.GiftId);if(!Title.IsNone())R.TitleId=Title;
    switch(E.GiftId)
    {
    case ECultivationGift::FairyWand:
        if(R.WandExpiresUtc<=Now)R.WandCount=0;
        R.WandCount=FMath::Min(int64(9007199254740991),FMath::Clamp(R.WandCount,int64(0),int64(9007199254740991))+E.Count);R.WandExpiresUtc=NewExpiry;break;
    case ECultivationGift::LoveExplosion:AddExperience(R,C.LoveExperience*E.Count,C);break;
    case ECultivationGift::LifePotion:
        if(R.PotionExpiresUtc<=Now)R.PotionCount=0;
        R.PotionCount=FMath::Min(int64(9007199254740991),FMath::Clamp(R.PotionCount,int64(0),int64(9007199254740991))+E.Count);R.PotionExpiresUtc=NewExpiry;break;
    case ECultivationGift::RareChest:if(New){R.SoulActiveUntil=Now+C.SoulDuration;R.SoulReadyAt=Now+FMath::Max(.01,C.SoulCooldown);}break;
    case ECultivationGift::SuperJet:if(New){R.JetActiveUntil=Now+C.JetDuration;R.JetReadyAt=Now+FMath::Max(.01,C.JetCooldown);}break;
    case ECultivationGift::Strawberry:if(New)R.StarReadyAt.Init(0,FMath::Clamp(C.StarCharges,1,64));break;
    default:break;
    }
    ++R.Version;return true;
}
void Scale(FCultivationAttributes& A,double M)
{
    A.MaxHealth*=M;A.HealthRegeneration*=M;A.Attack*=M;A.Defense*=M;A.SpiritDamage*=M;A.CultivationSpeed*=M;
    A.CriticalChance*=M;A.EvasionChance*=M;A.Luck*=M;A.MoveSpeed*=M;A.Swordsmanship*=M;
}
void AddScaled(FCultivationAttributes& A,const FCultivationAttributes& B,double M)
{
    A.MaxHealth+=B.MaxHealth*M;A.HealthRegeneration+=B.HealthRegeneration*M;A.Attack+=B.Attack*M;A.Defense+=B.Defense*M;A.SpiritDamage+=B.SpiritDamage*M;A.CultivationSpeed+=B.CultivationSpeed*M;
    A.CriticalChance+=B.CriticalChance*M;A.EvasionChance+=B.EvasionChance*M;A.Luck+=B.Luck*M;A.MoveSpeed+=B.MoveSpeed*M;A.Swordsmanship+=B.Swordsmanship*M;
}
void AddFlat(FCultivationAttributes& A,double Amount){A.MaxHealth+=Amount;A.Attack+=Amount;A.Defense+=Amount;A.SpiritDamage+=Amount;}
FCultivationAttributes PersonalAttributes(const FCultivationParticipantRecord& R,const FCultivationRuleConfig& C,int32 AnchorRealm)
{
    FCultivationAttributes A=C.BaseAttributes;
    double M=FMath::Max(0.0,1+R.RealmIndex*C.AttributeGrowthPerRealm);
    if(!R.bIsAnchor)M*=FMath::Max(0.0,1+AnchorRealm*C.AnchorBonusPerRealm);
    if(Has(R,ECultivationGift::RareChest))M*=C.SoulPersonalMultiplier;
    Scale(A,M);
    if(!R.bIsAnchor){A.MaxHealth=0;A.HealthRegeneration=0;A.Defense=0;}
    return A;
}
}
