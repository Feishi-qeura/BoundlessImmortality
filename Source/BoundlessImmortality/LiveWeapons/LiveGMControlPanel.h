#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "LiveGMControlPanel.generated.h"
class ALiveGMOverlay;
class ULiveGMControlPanel;
class UCanvasPanel;
class UTextBlock;
class UInkBufferedProgressWidget;
class UEditableTextBox;

UCLASS() class BOUNDLESSIMMORTALITY_API ULiveGMControlButton : public UButton
{
    GENERATED_BODY()
public:
    UPROPERTY(Transient) FName Action;
    void Bind(ULiveGMControlPanel* Owner,FName Id);
private:
    TWeakObjectPtr<ULiveGMControlPanel> Panel;
    UFUNCTION() void Apply();
};

UCLASS() class BOUNDLESSIMMORTALITY_API ULiveGMControlPanel : public UUserWidget
{
    GENERATED_BODY()
public:
    void Configure(ALiveGMOverlay* Owner);
    UFUNCTION(BlueprintCallable,Category="GM") void ExecuteControl(FName Action);
    UPROPERTY(Transient,BlueprintReadOnly,Category="GM") FName SelectedTab=TEXT("Session");
    UPROPERTY(Transient,BlueprintReadOnly,Category="GM") TArray<TObjectPtr<ULiveGMControlButton>> Buttons;
protected:
    virtual void NativeTick(const FGeometry& Geometry,float Delta)override;
private:
    TWeakObjectPtr<ALiveGMOverlay> Host;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Canvas;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Status;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ResultText;
    UPROPERTY(Transient) TObjectPtr<UEditableTextBox> ViewerId;
    UPROPERTY(Transient) TObjectPtr<UEditableTextBox> ViewerName;
    UPROPERTY(Transient) TObjectPtr<UEditableTextBox> AvatarUrl;
    UPROPERTY(Transient) TObjectPtr<UEditableTextBox> Comment;
    UPROPERTY(Transient) TObjectPtr<UInkBufferedProgressWidget> ShieldSample;
    UPROPERTY(Transient) TObjectPtr<UInkBufferedProgressWidget> CultivationSample;
    float RefreshTime=0;
    void Rebuild();
    UTextBlock* Text(const FString& Label,float X,float Y,float W,float H,int32 Size=20);
    void Button(FName Id,const FString& Label,float X,float Y,float W=244,float H=48,const TCHAR* IconPath=nullptr,const FString& Description=FString());
    void RefreshGiftDescriptions();
    UEditableTextBox* Input(const FString& Value,float X,float Y,float W);
};
