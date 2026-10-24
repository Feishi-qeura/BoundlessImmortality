#pragma once
#include <cmath>

// Engine-independent decision boundary shared by the real adapter and host tests.
namespace InkCorePolicy
{
enum class EState { Unavailable, Offline, Loading, Empty, Data, Error };
enum class EInventoryAction { Use, Sell, Decompose, Equip, Unequip };
struct FTransport { bool Available=false, Enabled=false, Connected=false, Busy=false, Active=false; };
struct FQuoteFacts
{
    bool Complete=false, ServiceReady=false, SelectionMatches=false, SlotsMatch=false;
    bool CoreSlotsValid=false, KindMatches=false, HasSelection=false, AttemptActive=false;
};
struct FInventoryFacts
{
    bool Exists=false, Equipped=false, Usable=false, Sellable=false, Decomposable=false, Equippable=false;
    int Owned=0, Requested=0;
};
inline bool CanDispatch(const FTransport& T)
{ return T.Available&&T.Enabled&&T.Connected&&!T.Busy&&T.Active; }
inline bool ValidCount(double Count, int Maximum=100)
{ return std::isfinite(Count)&&Count>=1&&Count<=Maximum&&std::floor(Count)==Count; }
inline EState State(const FTransport& T, bool HasPayload, bool HasData, bool Malformed=false)
{
    if(!T.Available||!T.Enabled)return EState::Unavailable;
    if(!T.Connected)return EState::Offline;
    if(Malformed)return EState::Error;
    if(!HasPayload)return EState::Loading;
    return HasData?EState::Data:EState::Empty;
}
inline bool CanSubmitQuote(const FQuoteFacts& Q)
{ return Q.Complete&&Q.ServiceReady&&Q.SelectionMatches&&Q.SlotsMatch&&Q.CoreSlotsValid&&Q.KindMatches&&Q.HasSelection&&!Q.AttemptActive; }
inline bool CanCancel(bool Prepare, bool HasId, bool HasNow, double Now, bool HasStart, double Start)
{ return Prepare&&HasId&&HasNow&&HasStart&&std::isfinite(Now)&&std::isfinite(Start)&&Now<Start; }
inline bool InventoryAllowed(const FInventoryFacts& I, EInventoryAction Action)
{
    if(!I.Exists||!ValidCount(I.Requested,I.Owned))return false;
    switch(Action)
    {
    case EInventoryAction::Use:return !I.Equipped&&I.Usable;
    case EInventoryAction::Sell:return !I.Equipped&&I.Sellable;
    case EInventoryAction::Decompose:return !I.Equipped&&I.Decomposable;
    case EInventoryAction::Equip:return !I.Equipped&&I.Equippable;
    case EInventoryAction::Unequip:return I.Equipped;
    }
    return false;
}
}
