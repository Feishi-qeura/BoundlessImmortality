#include "InkUiViewModels.h"

bool FInkUiRequestContext::Begin(const FInkUiSnapshotContext& S)
{
    // This presentation request contract covers the isolated prototype capabilities only.
    if(IsPending()||!S.bIsAuthoritative||S.Profile!=TEXT("prototype_full")||S.App.IsEmpty()||S.Room.IsEmpty()||S.RuleVersion.IsEmpty()||!FMath::IsFinite(S.ServerUtc)||S.ServerUtc<=0)return false;
    RequestId=FGuid::NewGuid();SessionGeneration=S.SessionGeneration;ExpectedVersion=S.SnapshotVersion;
    ReceiptId.Reset();Status=EInkUiRequestStatus::Submitting;return true;
}
void FInkUiRequestContext::MarkTimeout(){if(Status==EInkUiRequestStatus::Submitting)Status=EInkUiRequestStatus::PendingReconcile;}
bool FInkUiRequestContext::ApplyReply(FGuid Id,int64 Generation,int64 Version,EInkUiRequestStatus Result,const FString& Receipt)
{
    if(!IsPending()||Id!=RequestId||Generation!=SessionGeneration||Version<ExpectedVersion)return false;
    if(Result!=EInkUiRequestStatus::Confirmed&&Result!=EInkUiRequestStatus::Rejected)return false;
    if(Result==EInkUiRequestStatus::Confirmed&&Receipt.IsEmpty())return false;
    ExpectedVersion=Version;Status=Result;ReceiptId=Receipt;return true;
}
