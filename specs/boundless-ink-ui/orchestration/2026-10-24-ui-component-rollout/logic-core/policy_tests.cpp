#include "../../../../../Source/BoundlessImmortality/Frontend/UiRollout20261024/LogicCore/InkCorePolicy.h"
#include <initializer_list>
#include <cstdio>
#include <limits>

int main()
{
    using namespace InkCorePolicy;
    int Checks=0, Failed=0;
    auto Check=[&](bool Value,const char* Name){++Checks;if(!Value){++Failed;std::printf("FAIL: %s\n",Name);}};
    FTransport T{true,true,true,false,true};
    Check(CanDispatch(T),"connected active service accepts action");
    T.Busy=true;Check(!CanDispatch(T),"repeat click while pending blocked");T.Busy=false;
    T.Connected=false;Check(!CanDispatch(T),"offline mutation blocked");
    Check(State(T,true,true)==EState::Offline,"offline stale payload cannot become data");T.Connected=true;
    T.Active=false;Check(!CanDispatch(T),"hidden route action blocked");T.Active=true;
    T.Enabled=false;Check(!CanDispatch(T),"disabled service action blocked");T.Enabled=true;
    Check(State(T,false,false)==EState::Loading,"no response is loading, not empty");
    Check(State(T,true,false)==EState::Empty,"confirmed empty response is empty");
    Check(State(T,true,true)==EState::Data,"confirmed nonempty response is data");
    Check(State(T,true,false,true)==EState::Error,"malformed response is error");
    T.Busy=true;Check(State(T,true,true)==EState::Data,"pending request preserves confirmed data");T.Busy=false;
    Check(ValidCount(1)&&ValidCount(100),"integer boundary counts allowed");
    for(double V:{0.,-1.,1.5,101.,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()})
        Check(!ValidCount(V),"invalid quantity rejected");
    FQuoteFacts Q{true,true,true,true,true,true,true,false};
    Check(CanSubmitQuote(Q),"fully attested matching quote accepted");
    auto Bad=Q;Bad.SelectionMatches=false;Check(!CanSubmitQuote(Bad),"stale selection rejected");
    Bad=Q;Bad.SlotsMatch=false;Check(!CanSubmitQuote(Bad),"mismatched slot sum rejected");
    Bad=Q;Bad.KindMatches=false;Check(!CanSubmitQuote(Bad),"rework quote cannot prepare normal breakthrough");
    Bad=Q;Bad.ServiceReady=false;Check(!CanSubmitQuote(Bad),"service refusal respected");
    Bad=Q;Bad.CoreSlotsValid=false;Check(!CanSubmitQuote(Bad),"invalid core slots rejected");
    Bad=Q;Bad.Complete=false;Check(!CanSubmitQuote(Bad),"missing authority data blocks submit");
    Bad=Q;Bad.HasSelection=false;Check(!CanSubmitQuote(Bad),"no material selection blocks submit");
    Bad=Q;Bad.AttemptActive=true;Check(!CanSubmitQuote(Bad),"active attempt blocks second preparation");
    Check(CanCancel(true,true,true,100,true,101),"cancel before server trial time");
    Check(!CanCancel(true,true,true,101,true,101),"cancel exactly at trial time denied");
    Check(!CanCancel(true,false,true,100,true,101),"cancel without attempt id denied");
    Check(!CanCancel(true,true,false,0,true,101),"missing server time denied");
    Check(!CanCancel(false,true,true,100,true,101),"cancel outside Prepare denied");
    Check(!CanCancel(true,true,true,std::numeric_limits<double>::quiet_NaN(),true,101),"nonfinite time denied");
    FInventoryFacts I{true,false,true,true,true,true,3,2};
    Check(InventoryAllowed(I,EInventoryAction::Use),"use actual owned quantity");
    I.Requested=4;Check(!InventoryAllowed(I,EInventoryAction::Use),"overdraw blocked rather than clamped");I.Requested=2;
    I.Equipped=true;Check(!InventoryAllowed(I,EInventoryAction::Sell),"equipped item protected");
    Check(InventoryAllowed(I,EInventoryAction::Unequip),"equipped item can unequip");
    I.Equipped=false;Check(!InventoryAllowed(I,EInventoryAction::Unequip),"non equipped cannot unequip");
    I.Usable=false;Check(!InventoryAllowed(I,EInventoryAction::Use),"reserved or forbidden use blocked");
    I.Decomposable=false;Check(!InventoryAllowed(I,EInventoryAction::Decompose),"unsupported decomposition blocked");
    I.Exists=false;Check(!InventoryAllowed(I,EInventoryAction::Equip),"removed stable identity rejected");
    std::printf("%d checks, %d failed\n",Checks,Failed);return Failed?1:0;
}
