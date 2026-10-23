#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InkExtensionWidget.generated.h"
class UInkFrontendWidget;
class UVerticalBox;
class UTextBlock;
class UInkFrontendActionButton;
class UImage;

/** Browsable development surfaces. Preview actions never grant items or spend currency. */
UCLASS(Blueprintable)
class BOUNDLESSIMMORTALITY_API UInkExtensionWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    virtual void Configure(UInkFrontendWidget* InFrontend,FName InPage);
    virtual void HandleAction(FName Action);
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI") FName PageId;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI") FString ActiveTab;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI") FString SelectedEntry;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI") FString PreviewFeedback;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI") int32 CalendarOffset=0;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI") int32 PrizePage=0;
protected:
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& KeyEvent)override;
private:
    UPROPERTY(Transient) TObjectPtr<UInkFrontendWidget> Frontend;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> Content;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Feedback;
    FString SelectedStyle=TEXT("普通");
    FString SelectedAvatarFrame=TEXT("素墨头像框");
    bool bSkipAnimation=false;
    void RebuildSurface();
    void RenderContent();
    UTextBlock* Text(UVerticalBox* Box,const FString& Value,int32 Size=22);
    UInkFrontendActionButton* Button(UVerticalBox* Box,const FString& Label,const FString& Action,bool bSelected=false,bool bCompact=false);
    UVerticalBox* Card(UVerticalBox* Box,const FString& Title,const FString& Caption=FString(),int32 SceneTile=-1,FName Name=NAME_None);
    UImage* Scene(int32 Tile);
    void Section(UVerticalBox* Box,const FString& Title);
    void Entry(UVerticalBox* Box,const FString& Title,const FString& Caption,const FString& Action,int32 Tile);
    void Identity(UVerticalBox* Box);
    void Tabs(UVerticalBox* Box,const TArray<FString>& Names);
    void Choices(UVerticalBox* Box,const TArray<FString>& Names,const FString& Prefix);
    void Progress(UVerticalBox* Box,const FString& Label,float Fraction);
    void Slots(UVerticalBox* Box,const FString& Label,int32 Count);
    void Detail(UVerticalBox* Box);
};
