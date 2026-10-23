#pragma once
#include "CultivationTypes.h"

/** Pure rules shared by gameplay, UI, deterministic tests and imported service records. */
namespace CultivationRules
{
    BOUNDLESSIMMORTALITY_API double StackedMultiplier(double First,double Coefficient,int64 Count);
    BOUNDLESSIMMORTALITY_API double Threshold(const FCultivationRuleConfig& Rules,int32 Realm);
    BOUNDLESSIMMORTALITY_API int32 MaterialCost(const FCultivationRuleConfig& Rules,int32 Realm);
    BOUNDLESSIMMORTALITY_API bool AddExperience(FCultivationParticipantRecord& Record,double Amount,const FCultivationRuleConfig& Rules);
    BOUNDLESSIMMORTALITY_API bool Breakthrough(FCultivationParticipantRecord& Record,const FCultivationRuleConfig& Rules);
    BOUNDLESSIMMORTALITY_API FName GiftTitle(ECultivationGift Gift);
    BOUNDLESSIMMORTALITY_API FText TitleName(FName Title);
    BOUNDLESSIMMORTALITY_API bool ApplyGift(FCultivationParticipantRecord& Record,const FCultivationGiftEvent& Event,double Now,const FCultivationRuleConfig& Rules);
    BOUNDLESSIMMORTALITY_API FCultivationAttributes PersonalAttributes(const FCultivationParticipantRecord& Record,const FCultivationRuleConfig& Rules,int32 AnchorRealm);
    BOUNDLESSIMMORTALITY_API void AddScaled(FCultivationAttributes& Target,const FCultivationAttributes& Source,double Scale);
    BOUNDLESSIMMORTALITY_API void Scale(FCultivationAttributes& Target,double Multiplier);
    BOUNDLESSIMMORTALITY_API void AddFlat(FCultivationAttributes& Target,double Amount);
    BOUNDLESSIMMORTALITY_API bool Has(const FCultivationParticipantRecord& Record,ECultivationGift Gift);
}
