#include "Frontend/UI/InkUiAssetPaths.h"
#include "InkFeaturePanel.h"
#include "InkFrontendWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInterface.h"

void UInkDrawPanel::BuildPanelContent()
{
    if(!FrontendOwner)return;
    auto& Wheel=FrontendOwner->DrawPreviewWheel;DemoResults=FrontendOwner->DrawPreviewResults;
    const bool Wide=!bPortraitLayout&&BodyWidth>=800;
    const float D=Wide?FMath::Min(480.f,BodyWidth*.48f):FMath::Min(520.f,BodyWidth-16),X=Wide?0:(BodyWidth-D)*.5f;
    auto* Disc=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("DrawWheelDisc"));
    auto* S=BodyCanvas->AddChildToCanvas(Disc);S->SetPosition({X,18});S->SetSize({D,D});
    Disc->SetVisibility(ESlateVisibility::HitTestInvisible);Disc->SetRenderTransformPivot(FVector2D(.5));Disc->SetRenderTransformAngle(Wheel.Angle);
    auto* Art=WidgetTree->ConstructWidget<UImage>();FSlateBrush Brush;Brush.DrawAs=ESlateBrushDrawType::Image;
    Brush.SetResourceObject(InkUiAssets::Load<UMaterialInterface>(nullptr,TEXT("/Game/UI/Ink/Materials/Shared/M_InkWheel.M_InkWheel")));
    Art->SetBrush(Brush);auto* A=Disc->AddChildToCanvas(Art);A->SetPosition({0,0});A->SetSize({D,D});
    const float IconSize=D*.13f;
    for(int32 I=0;I<8;++I)
    {
        const float Angle=FMath::DegreesToRadians((I+.5f)*45.f);
        AtlasIcon(Disc,NAME_None,8+I%4,D*.5f+FMath::Sin(Angle)*D*.355f-IconSize*.5f,D*.5f-FMath::Cos(Angle)*D*.355f-IconSize*.5f,IconSize);
    }
    Label(BodyCanvas,TEXT("DrawWheelPointer"),TEXT("▼"),X+D*.5f-24,0,48,56,36,true)->SetColorAndOpacity(InkPresentationStyle::Gold());
    auto* Seal=Surface(BodyCanvas,TEXT("DrawWheelSeal"),InkPresentationStyle::ESurface::BlueButton,X+D*.375f,18+D*.375f,D*.25f,D*.25f);
    auto SealBrush=Seal->GetBrush();const float SealRadius=D*.125f;
    SealBrush.OutlineSettings.CornerRadii=FVector4(SealRadius,SealRadius,SealRadius,SealRadius);Seal->SetBrush(SealBrush);
    Label(BodyCanvas,TEXT("DrawWheelSealText"),TEXT("仙缘\n演示"),X+D*.375f,18+D*.40f,D*.25f,D*.2f,28,true)->SetColorAndOpacity(InkPresentationStyle::Paper());
    const float RX=Wide?D+24:0,RY=Wide?12:D+36,RW=Wide?BodyWidth-RX:BodyWidth;
    Label(BodyCanvas,TEXT("DrawHeading"),Wheel.bAnimating?TEXT("仙缘流转中"):TEXT("仙缘奖池 · 演示"),RX,RY,RW,56,36);
    Label(BodyCanvas,TEXT("DrawHint"),TEXT("开发演示，不消耗货币，不发放物品。"),RX,RY+64,RW,64,24);
    auto* Once=Action(BodyCanvas,TEXT("Demo_DrawOnce"),TEXT("演示单抽"),RX,RY+140,RW*.48f,88,true);
    auto* Ten=Action(BodyCanvas,TEXT("Demo_DrawTen"),TEXT("演示十连"),RX+RW*.52f,RY+140,RW*.48f,88);
    Once->SetIsEnabled(!Wheel.bAnimating);Ten->SetIsEnabled(!Wheel.bAnimating);
    Action(BodyCanvas,TEXT("Demo_DrawRules"),TEXT("奖池说明"),RX,RY+240,RW*.48f,88);
    Action(BodyCanvas,TEXT("Demo_DrawSkip"),TEXT("跳过动效"),RX+RW*.52f,RY+240,RW*.48f,88)->SetIsEnabled(Wheel.bAnimating);
    if(bShowRules)Label(BodyCanvas,TEXT("DrawRules"),TEXT("灵晶、玉简、灵丹、铜钱四类演示物品各25%。\n不设保底；结果在转动前确定。关闭、跳过或调整窗口均保留本次结果。"),RX,RY+344,RW,180,24);
    else if(!Wheel.bAnimating&&!DemoResults.IsEmpty())
    {
        Label(BodyCanvas,TEXT("DrawResultsHeading"),TEXT("本次结果 · 仅供演示"),RX,RY+344,RW,42,28);
        const int32 Columns=FMath::Clamp(FMath::FloorToInt(RW/96),1,5);
        const float Step=RW/Columns,Size=FMath::Min(92.f,Step-4);
        for(int32 I=0;I<DemoResults.Num();++I)Reward(BodyCanvas,NAME_None,DemoResults[I],TEXT("演示"),RX+(I%Columns)*Step,RY+396+(I/Columns)*(Size+12),Size);
    }
}
void UInkDrawPanel::HandlePanelAction(const FString& Id)
{
    if(!FrontendOwner)return;
    auto& Wheel=FrontendOwner->DrawPreviewWheel;
    if(Id==TEXT("Demo_DrawRules")){bShowRules=!bShowRules;RebuildPanel();return;}
    if(Id==TEXT("Demo_DrawSkip")){Wheel.Skip();RebuildPanel();return;}
    if((Id==TEXT("Demo_DrawOnce")||Id==TEXT("Demo_DrawTen"))&&!Wheel.bAnimating)
    {
        auto& Results=FrontendOwner->DrawPreviewResults;auto& Random=FrontendOwner->DrawPreviewRandom;Results.Reset();
        for(int32 I=0;I<(Id==TEXT("Demo_DrawTen")?10:1);++I)Results.Add(Random.RandRange(0,3));
        Wheel.Begin(FGuid::NewGuid().ToString(),Results[0]+4*Random.RandRange(0,1),8);
        if(FrontendOwner->bReducedMotion)Wheel.Skip();bShowRules=false;RebuildPanel();return;
    }
    Super::HandlePanelAction(Id);
}
void UInkDrawPanel::NativeTick(const FGeometry& G,float Delta)
{
    Super::NativeTick(G,Delta);if(!FrontendOwner||FrontendOwner->CurrentPage!=PageId)return;
    auto& Wheel=FrontendOwner->DrawPreviewWheel;if(!Wheel.bAnimating)return;
    if(FrontendOwner->bReducedMotion)Wheel.Skip();else Wheel.Advance(Delta);
    if(auto* Disc=WidgetTree->FindWidget(TEXT("DrawWheelDisc")))Disc->SetRenderTransformAngle(Wheel.Angle);
    if(!Wheel.bAnimating)RebuildPanel();
}
