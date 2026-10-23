#include "InkLeaderboardAvatarRequest.h"
#include "InkFrontendWidget.h"
#include "LiveInteractionSubsystem.h"
#include "Engine/GameInstance.h"

void UInkLeaderboardAvatarRequest::Start(UInkFrontendWidget* InOwner,const FString& InKey,const FString& Url)
{
    Owner=InOwner;Key=InKey;StartedAt=FPlatformTime::Seconds();
    Host=InOwner&&InOwner->GetGameInstance()?InOwner->GetGameInstance()->GetSubsystem<ULiveInteractionSubsystem>():nullptr;
    if(auto* Source=Host.Get()){Source->OnAvatarReady.AddDynamic(this,&UInkLeaderboardAvatarRequest::Succeeded);Source->RequestAvatar(Key,Url);}
}
void UInkLeaderboardAvatarRequest::Succeeded(const FString& UserId,UTexture2D* Texture){if(UserId!=Key)return;Cancel();if(auto* Widget=Owner.Get())Widget->CompleteLeaderboardAvatar(Key,Texture);}
void UInkLeaderboardAvatarRequest::Cancel(){if(auto* Source=Host.Get())Source->OnAvatarReady.RemoveDynamic(this,&UInkLeaderboardAvatarRequest::Succeeded);Host.Reset();}
void UInkLeaderboardAvatarRequest::BeginDestroy(){Cancel();Super::BeginDestroy();}
