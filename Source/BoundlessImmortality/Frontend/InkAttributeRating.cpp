#include "InkAttributeRating.h"

namespace
{
// realm_targets.csv; all minor stages in a group share these endpoints.
constexpr int32 Starts[]={0,9,13,16,22,23,24,26,31,32,33};
constexpr int32 Ends[]={8,12,15,21,22,23,25,30,31,32,33};
// Highest craftable tier at major 0/3/6/9; equipment_items.csv.
constexpr double HealthAdds[]={.03,.05,.08,.12},AttackAdds[]={.02,.03,.05,.08};
constexpr double DefenseAdds[]={.03,.04,.06,.09},MovementAdds[]={.02,.03,.04,.05};

TArray<double> Anchored(double Base,double Upper)
{
    if(!FMath::IsFinite(Base)||!FMath::IsFinite(Upper)||Base<=0)return {};
    Upper=FMath::Max(Upper,Base*1.01);
    const double Span=Upper-Base;
    return {0,.8*Base,Base,Base+.25*Span,Base+.50*Span,Base+.75*Span,Base+.95*Span};
}
}

int32 UInkAttributeRatingLibrary::MajorIndex(int32 Realm)
{Realm=FMath::Clamp(Realm,0,33);for(int32 I=10;I>=0;--I)if(Realm>=Starts[I])return I;return 0;}
FString UInkAttributeRatingLibrary::MajorName(int32 Realm)
{const TCHAR* Names[]={TEXT("练气"),TEXT("筑基"),TEXT("结晶"),TEXT("金丹"),TEXT("元婴"),TEXT("化神"),TEXT("元神"),TEXT("炼虚"),TEXT("大乘"),TEXT("飞升"),TEXT("混元无极")};return Names[MajorIndex(Realm)];}
TArray<double> UInkAttributeRatingLibrary::Thresholds(FName Attribute,const FInkAttributeRatingContext& C)
{
    const int32 Major=MajorIndex(C.RealmIndex),First=Starts[Major],Last=Ends[Major],Tier=FMath::Min(3,Major/3);
    if(Attribute==TEXT("Critical")||Attribute==TEXT("Evasion"))return {0,.01,.03,.06,.10,.15,.20};
    const double Growth=FMath::IsFinite(C.LegacyRealmGrowth)?FMath::Max(0.,C.LegacyRealmGrowth):0;
    const auto LegacyBase=[&](double Base,int32 R){return Base*(1+Growth*R);};
    const double AttackBase=C.bV14Rules?10*(1+.12*First):LegacyBase(C.BaseAttributes.Attack,First);
    if(Attribute==TEXT("SpiritDamage"))
    {if(!FMath::IsFinite(AttackBase)||AttackBase<=0)return {};return {0,.05*AttackBase,.10*AttackBase,.20*AttackBase,.35*AttackBase,.50*AttackBase,.75*AttackBase};}
    if(Attribute==TEXT("MoveSpeed"))return Anchored(C.BaseMoveSpeed,C.BaseMoveSpeed*(1+MovementAdds[Tier]));
    if(Attribute==TEXT("CultivationSpeed"))
    {
        const double Base=C.bV14Rules?1:C.LegacyMeditationPerSecond*LegacyBase(C.BaseAttributes.CultivationSpeed,First);
        const double Top=C.bV14Rules?1:C.LegacyMeditationPerSecond*LegacyBase(C.BaseAttributes.CultivationSpeed,Last);
        return Anchored(Base,Top*1.20); // online spring +20%; reserved method/event budgets are not invented.
    }
    const double Quality=C.bV14Rules?1+Major*.01:1; // ten additive snapshots, not repeated multipliers.
    double Base=0,Upper=0;
    if(Attribute==TEXT("Health"))
    {Base=C.bV14Rules?1000*(1+.15*First):LegacyBase(C.BaseAttributes.MaxHealth,First);Upper=(C.bV14Rules?1000*(1+.15*Last):LegacyBase(C.BaseAttributes.MaxHealth,Last))*Quality*(1+HealthAdds[Tier]);}
    else if(Attribute==TEXT("Attack"))
    {Base=AttackBase;Upper=(C.bV14Rules?10*(1+.12*Last):LegacyBase(C.BaseAttributes.Attack,Last))*Quality*(1+AttackAdds[Tier]);}
    else if(Attribute==TEXT("Defense"))
    {
        // Legacy zero-defense profiles use the documented defensive reference, so zero stays D.
        const bool Reference=C.bV14Rules||C.BaseAttributes.Defense<=0;
        Base=Reference?20*(1+.10*First):LegacyBase(C.BaseAttributes.Defense,First);
        Upper=(Reference?20*(1+.10*Last):LegacyBase(C.BaseAttributes.Defense,Last))*Quality*(1+DefenseAdds[Tier]);
    }
    else return {};
    return Anchored(Base,Upper);
}
EInkAttributeRating UInkAttributeRatingLibrary::Evaluate(FName Attribute,double Value,const FInkAttributeRatingContext& C)
{
    if(!FMath::IsFinite(Value)||Value<0)return EInkAttributeRating::Unrated;
    const auto Bounds=Thresholds(Attribute,C);if(Bounds.Num()!=7)return EInkAttributeRating::Unrated;
    for(int32 I=6;I>=0;--I)if(Value>=Bounds[I])return EInkAttributeRating(I);
    return EInkAttributeRating::D;
}
FString UInkAttributeRatingLibrary::GradeName(EInkAttributeRating Grade)
{const TCHAR* Names[]={TEXT("D"),TEXT("C"),TEXT("B"),TEXT("A"),TEXT("S"),TEXT("SS"),TEXT("SSS")};return int32(Grade)<7?Names[int32(Grade)]:TEXT("—");}
