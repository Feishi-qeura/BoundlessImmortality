#include "LiveWeaponSkill.h"
#include "LiveMonster.h"
bool ULiveWeaponSkill::CanExecute_Implementation(const FLiveSkillContext& Context)const{return IsValid(Context.Target)&&Context.Target->IsAlive();}
bool ULiveWeaponSkill::Execute_Implementation(const FLiveSkillContext& Context){return false;}
