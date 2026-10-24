#include "InkPermanentActivitiesPanel.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Dom/JsonObject.h"
#include "InkPresentationStyle.h"
#include "WulingFrontendData.h"

namespace
{
FString ReadText(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, const FString& Missing = TEXT("未提供"))
{
	FString Value;
	return Object && Object->TryGetStringField(Key, Value) && !Value.IsEmpty() ? Value : Missing;
}

FString ReadAuthorityBool(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, const TCHAR* Yes, const TCHAR* No)
{
	bool Value = false;
	if (!Object || !Object->TryGetBoolField(Key, Value)) return TEXT("资格待服务端确认");
	return Value ? Yes : No;
}

FString ReadDisplayField(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, const TCHAR* Label)
{
	const TSharedPtr<FJsonValue>* Value = Object ? Object->Values.Find(Key) : nullptr;
    if (!Value || !Value->IsValid()) return FString(Label) + TEXT("：待同步");
	if ((*Value)->Type == EJson::String)
	{
		const FString Text = (*Value)->AsString();
        return FString(Label) + TEXT("：") + (Text.IsEmpty() ? TEXT("待同步") : Text);
	}
	if ((*Value)->Type == EJson::Number) return FString::Printf(TEXT("%s：%.0f"), Label, (*Value)->AsNumber());
	if ((*Value)->Type == EJson::Boolean) return FString(Label) + TEXT("：") + ((*Value)->AsBool() ? TEXT("是") : TEXT("否"));
    return FString(Label) + TEXT("：待同步");
}

TArray<TSharedPtr<FJsonObject>> FindPermanentActivities(const TSharedPtr<FJsonObject>& Activities, bool& bHasClassification)
{
	TArray<TSharedPtr<FJsonObject>> Result;
	bHasClassification = false;
	const TArray<TSharedPtr<FJsonValue>>* Categories = nullptr;
	if (Activities && Activities->TryGetArrayField(TEXT("categories"), Categories))
	{
		bHasClassification = true;
		for (const TSharedPtr<FJsonValue>& Value : *Categories)
		{
			const TSharedPtr<FJsonObject> Category = Value.IsValid() ? Value->AsObject() : nullptr;
			const FString Id = ReadText(Category, TEXT("id"), ReadText(Category, TEXT("type"), ReadText(Category, TEXT("key"), FString())));
			if (!Id.Equals(TEXT("permanent"), ESearchCase::IgnoreCase) && !Id.Equals(TEXT("常驻"), ESearchCase::CaseSensitive)) continue;
			const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
			if (Category && (Category->TryGetArrayField(TEXT("items"), Rows) || Category->TryGetArrayField(TEXT("activities"), Rows)))
				for (const TSharedPtr<FJsonValue>& RowValue : *Rows) if (RowValue.IsValid() && RowValue->AsObject()) Result.Add(RowValue->AsObject());
			return Result;
		}
	}

	// Current authority versions expose templates rather than categorized rows.
	// Classify only an explicit permanent cycle; never infer it from event/limited.
	const TArray<TSharedPtr<FJsonValue>>* Templates = nullptr;
	if (Activities && Activities->TryGetArrayField(TEXT("templates"), Templates))
	{
		bHasClassification = true;
		for (const TSharedPtr<FJsonValue>& RowValue : *Templates)
		{
			const TSharedPtr<FJsonObject> Row = RowValue.IsValid() ? RowValue->AsObject() : nullptr;
			if (ReadText(Row, TEXT("cycle"), FString()).Equals(TEXT("permanent"), ESearchCase::IgnoreCase)) Result.Add(Row);
		}
	}
	return Result;
}
}

void UInkPermanentActivitiesPanel::BuildPanelContent()
{
	const auto* Service = WulingFrontendData::Service(this);
	const TSharedPtr<FJsonObject> Activities = WulingFrontendData::Activity(this);
	bool bHasClassification = false;
	const TArray<TSharedPtr<FJsonObject>> Rows = FindPermanentActivities(Activities, bHasClassification);
	const float Width = BodyWidth;
	auto ShowState = [&](const FString& Message)
	{
		Surface(BodyCanvas, TEXT("PermanentActivitiesStatePaper"), InkPresentationStyle::ESurface::FeatureCard, 0, 0, Width, 120);
		Surface(BodyCanvas, TEXT("PermanentActivitiesStateHeading"), InkPresentationStyle::ESurface::FeatureHeading, 8, 8, Width - 16, 56);
		Label(BodyCanvas, TEXT("PermanentActivitiesStatus"), Message, 20, 20, Width - 40, 88, 24);
	};

	if (!Service || !Service->IsEnabled() || !Service->IsConnected())
	{
		ShowState(TEXT("活动服务未连接，常驻活动状态暂不可用。"));
		return;
	}
	if (Service->IsBusy())
	{
		ShowState(TEXT("正在读取常驻活动状态…"));
		return;
	}
	if (!Activities.IsValid())
	{
		ShowState(Service->GetStatus().IsEmpty() ? TEXT("活动状态读取失败，请稍后重试。") : Service->GetStatus());
		return;
	}
	if (!bHasClassification)
	{
		ShowState(TEXT("服务端尚未提供活动分类。"));
		return;
	}
	if (Rows.IsEmpty())
	{
		ShowState(TEXT("当前活动配置没有常驻周期条目。"));
		return;
	}

	constexpr float CardHeight = 430.f;
	for (int32 Index = 0; Index < Rows.Num(); ++Index)
	{
		const TSharedPtr<FJsonObject> Row = Rows[Index];
		if (!Row.IsValid()) continue;
		const float Y = Index * (CardHeight + 16.f);
		Surface(BodyCanvas, NAME_None, InkPresentationStyle::ESurface::FeatureCard, 0, Y, Width, CardHeight);
		Surface(BodyCanvas, NAME_None, InkPresentationStyle::ESurface::FeatureHeading, 12, Y + 10, Width - 24, 56);
		Label(BodyCanvas, FName(*FString::Printf(TEXT("PermanentActivityTitle_%d"), Index)), ReadText(Row, TEXT("title"), ReadText(Row, TEXT("name"), TEXT("活动名称待同步"))), 24, Y + 16, Width - 48, 44, 30);
		bool bEnabled = false;
		const FString OpenState = Row->TryGetBoolField(TEXT("enabled"), bEnabled) ? (bEnabled ? TEXT("配置状态：已启用") : TEXT("配置状态：已关闭")) : TEXT("配置状态：待服务端确认");
		Label(BodyCanvas, FName(*FString::Printf(TEXT("PermanentActivityState_%d"), Index)), OpenState, 20, Y + 74, Width - 40, 36, 22);
		Label(BodyCanvas, FName(*FString::Printf(TEXT("PermanentActivityParticipation_%d"), Index)), ReadAuthorityBool(Row, TEXT("participation_eligible"), TEXT("可参与"), TEXT("暂不可参与")), 20, Y + 112, Width - 40, 36, 22);
		Label(BodyCanvas, FName(*FString::Printf(TEXT("PermanentActivityClaim_%d"), Index)), ReadAuthorityBool(Row, TEXT("claim_eligible"), TEXT("可领取"), TEXT("暂不可领取")), 20, Y + 150, Width - 40, 36, 22);
		Label(BodyCanvas, FName(*FString::Printf(TEXT("PermanentActivityObjective_%d"), Index)), ReadDisplayField(Row, TEXT("objective"), TEXT("目标")), 20, Y + 188, Width - 40, 44, 21);
		Label(BodyCanvas, FName(*FString::Printf(TEXT("PermanentActivityProgress_%d"), Index)), ReadDisplayField(Row, TEXT("progress"), TEXT("当前进度")) + TEXT(" / ") + ReadDisplayField(Row, TEXT("target"), TEXT("目标进度")), 20, Y + 232, Width - 40, 44, 21);
		Label(BodyCanvas, FName(*FString::Printf(TEXT("PermanentActivityReward_%d"), Index)), ReadDisplayField(Row, TEXT("reward_summary"), TEXT("奖励摘要")), 20, Y + 276, Width - 40, 44, 21);
		Label(BodyCanvas, FName(*FString::Printf(TEXT("PermanentActivitySource_%d"), Index)), ReadDisplayField(Row, TEXT("source"), TEXT("来源")), 20, Y + 320, Width - 40, 44, 21);
		const FString EndAt = ReadText(Row, TEXT("end_at"), FString());
		const FString Deadline = EndAt.IsEmpty() ? TEXT("结束时间待同步") : TEXT("结束时间：") + EndAt;
		Label(BodyCanvas, FName(*FString::Printf(TEXT("PermanentActivityDeadline_%d"), Index)), Deadline, 20, Y + 368, Width - 40, 36, 20);
	}
}
