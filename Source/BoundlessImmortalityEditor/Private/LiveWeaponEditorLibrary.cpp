#include "LiveWeaponEditorLibrary.h"
#include "Engine/StaticMesh.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Materials/Material.h"
#include "Misc/PackageName.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "UnrealClient.h"
#include "LiveWeapons/LiveGMOverlay.h"
#include "Framework/Application/SlateApplication.h"
#include "AssetCompilingManager.h"
#include "Layout/WidgetPath.h"
#include "LiveInteractionSubsystem.h"
#include "Cultivation/CultivationSubsystem.h"
#include "Cultivation/CultivationPrototypeClient.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "LiveWeapons/LiveMonster.h"
#include "Wuling/WulingPrototypeSubsystem.h"
UWulingPrototypeSubsystem* ULiveWeaponEditorLibrary::GetWulingState(UObject* Context)
{
    UWorld* World=Context?Context->GetWorld():nullptr;
    return World&&World->GetGameInstance()?World->GetGameInstance()->GetSubsystem<UWulingPrototypeSubsystem>():nullptr;
}
ALiveMonster* ULiveWeaponEditorLibrary::SpawnTestMonster(UObject* Context,FVector Location,float MaxHealth,float AttackDamage,float MoveSpeed,APawn* FollowTarget)
{
    UWorld* World=Context?Context->GetWorld():nullptr;
    if(!World||World->WorldType!=EWorldType::PIE||!FMath::IsFinite(MaxHealth)||MaxHealth<=0)return nullptr;
    const FTransform Transform(FRotator::ZeroRotator,Location);
    auto* Monster=World->SpawnActorDeferred<ALiveMonster>(ALiveMonster::StaticClass(),Transform,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if(!Monster)return nullptr;
    Monster->MaxHealth=MaxHealth;Monster->AttackDamage=FMath::Max(0.f,AttackDamage);Monster->MoveSpeed=FMath::Max(0.f,MoveSpeed);Monster->FollowTarget=FollowTarget;
    Monster->FinishSpawning(Transform);return Monster;
}
UCultivationSubsystem* ULiveWeaponEditorLibrary::GetCultivationState(UObject* Context)
{
    UWorld* World=Context?Context->GetWorld():nullptr;
    return World&&World->GetGameInstance()?World->GetGameInstance()->GetSubsystem<UCultivationSubsystem>():nullptr;
}
UCultivationPrototypeClient* ULiveWeaponEditorLibrary::GetPrototypeState(UObject* Context)
{
    UWorld* World=Context?Context->GetWorld():nullptr;
    return World&&World->GetGameInstance()?World->GetGameInstance()->GetSubsystem<UCultivationPrototypeClient>():nullptr;
}
ULiveInteractionSubsystem* ULiveWeaponEditorLibrary::GetLiveInteractionState(UObject* Context)
{
    UWorld* World=Context?Context->GetWorld():nullptr;
    return World&&World->GetGameInstance()?World->GetGameInstance()->GetSubsystem<ULiveInteractionSubsystem>():nullptr;
}
bool ULiveWeaponEditorLibrary::SimulatePresence(ULiveInteractionSubsystem* Host,const FString& UserId,const FString& Nickname,int32 EnterType)
{
    if(!Host||!Host->IsDevelopmentSimulation()||UserId.IsEmpty()||(EnterType!=1&&EnterType!=2))return false;
    FLivePresence Event;Event.Session=Host->GetCurrentSession();Event.UserId=UserId;Event.Nickname=Nickname;
    Event.MessageId=FGuid::NewGuid().ToString(EGuidFormats::Digits);Event.EnterType=EnterType;
    Event.TimestampMs=FDateTime::UtcNow().ToUnixTimestamp()*1000;
    return Host->DeliverPresence(Event);
}
void ULiveWeaponEditorLibrary::FinishAssetCompilation(){FAssetCompilingManager::Get().FinishAllCompilation();}
bool ULiveWeaponEditorLibrary::ClickGMButton(ALiveGMOverlay* Overlay,FName Action)
{
    if(!Overlay||!Overlay->Panel||!FSlateApplication::IsInitialized())return false;
    for(auto Button:Overlay->Panel->ActionButtons)if(Button->Action==Action&&Button->GetIsEnabled())
    {
        const auto& Geometry=Button->GetCachedGeometry();if(Geometry.GetLocalSize().IsNearlyZero())return false;
        const FVector2D Position=Geometry.LocalToAbsolute(Geometry.GetLocalSize()*.5f);
        auto& Slate=FSlateApplication::Get();Slate.SetCursorPos(Position);
        FPointerEvent Move(FSlateApplication::CursorPointerIndex,Position,Position,TSet<FKey>{},EKeys::Invalid,0,FModifierKeysState());
        Slate.ProcessMouseMoveEvent(Move);
        const FWidgetPath Hit=Slate.LocateWindowUnderMouse(Position,Slate.GetInteractiveTopLevelWindows());
        if(!Hit.ContainsWidget(Button->GetCachedWidget().Get()))return false;
        FPointerEvent Down(FSlateApplication::CursorPointerIndex,Position,Position,TSet<FKey>{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0,FModifierKeysState());
        const bool Handled=Slate.ProcessMouseButtonDownEvent(nullptr,Down);
        FPointerEvent Up(FSlateApplication::CursorPointerIndex,Position,Position,TSet<FKey>{},EKeys::LeftMouseButton,0,FModifierKeysState());
        Slate.ProcessMouseButtonUpEvent(Up);return Handled;
    }
    return false;
}
void ULiveWeaponEditorLibrary::CaptureGameFrame(const FString& Filename,bool bShowUI){FScreenshotRequest::RequestScreenshot(Filename,bShowUI,false);}
bool ULiveWeaponEditorLibrary::ClickHUDAt(APlayerController* Controller,FVector2D Position)
{
    AHUD* HUD=Controller?Controller->GetHUD():nullptr;
    if(!HUD||!HUD->GetHitBoxAtCoordinates(Position,true))return false;
    const bool Clicked=HUD->UpdateAndDispatchHitBoxClickEvents(Position,IE_Pressed);
    HUD->UpdateAndDispatchHitBoxClickEvents(Position,IE_Released);
    return Clicked;
}
UStaticMesh* ULiveWeaponEditorLibrary::CreatePlaceholderMesh(const FString& PackagePath,bool bTriangle)
{
    if(!PackagePath.StartsWith(TEXT("/Game/LiveWeapons/")))return nullptr;
    if(auto* Existing=LoadObject<UStaticMesh>(nullptr,*(PackagePath+TEXT(".")+FPackageName::GetLongPackageAssetName(PackagePath))))return Existing;
    UPackage* Package=CreatePackage(*PackagePath);
    auto* Mesh=NewObject<UStaticMesh>(Package,*FPackageName::GetLongPackageAssetName(PackagePath),RF_Public|RF_Standalone);
    FMeshDescription Description;FStaticMeshAttributes Attributes(Description);Attributes.Register();
    auto Positions=Attributes.GetVertexPositions();auto Normals=Attributes.GetVertexInstanceNormals();auto UVs=Attributes.GetVertexInstanceUVs();UVs.SetNumChannels(1);
    const auto Group=Description.CreatePolygonGroup();
    Attributes.GetPolygonGroupMaterialSlotNames()[Group]=TEXT("Default");
    const auto Triangle=[&](FVector3f A,FVector3f B,FVector3f C)
    {
        const FVector3f Normal=FVector3f::CrossProduct(B-A,C-A).GetSafeNormal();TArray<FVertexInstanceID> Corners;
        for(auto Point:{A,B,C}){const auto V=Description.CreateVertex();Positions[V]=Point;const auto VI=Description.CreateVertexInstance(V);Normals[VI]=Normal;UVs.Set(VI,0,FVector2f((Point.X+50)/100,(Point.Z+50)/100));Corners.Add(VI);}
        Description.CreatePolygon(Group,Corners);
    };
    const int32 Sides=bTriangle?3:32;
    for(int32 I=0;I<Sides;++I)
    {
        const float A=PI*.5f+I*2*PI/Sides,B=PI*.5f+(I+1)*2*PI/Sides;
        const FVector3f P(FMath::Cos(A)*50,-6,FMath::Sin(A)*50),Q(FMath::Cos(B)*50,-6,FMath::Sin(B)*50);
        const FVector3f R(P.X,6,P.Z),S(Q.X,6,Q.Z);
        Triangle(FVector3f(0,-6,0),Q,P);Triangle(FVector3f(0,6,0),R,S);Triangle(P,Q,S);Triangle(P,S,R);
    }
    Mesh->GetStaticMaterials().Add(FStaticMaterial(UMaterial::GetDefaultMaterial(MD_Surface),TEXT("Default")));
    UStaticMesh::FBuildMeshDescriptionsParams Params;Params.bBuildSimpleCollision=false;Params.bFastBuild=false;
    if(!Mesh->BuildFromMeshDescriptions({&Description},Params))return nullptr;
    FAssetRegistryModule::AssetCreated(Mesh);Mesh->MarkPackageDirty();return Mesh;
}
UFont* ULiveWeaponEditorLibrary::CreateNameFont()
{
    const FString FontPath=TEXT("/Game/LiveWeapons/Fonts/F_LiveNames"),FacePath=TEXT("/Game/LiveWeapons/Fonts/FF_LiveNames");
    if(auto* Existing=LoadObject<UFont>(nullptr,*(FontPath+TEXT(".F_LiveNames"))))return Existing;
    TArray<uint8> Data;const FString Source=FPaths::EngineContentDir()/TEXT("Slate/Fonts/DroidSansFallback.ttf");if(!FFileHelper::LoadFileToArray(Data,*Source))return nullptr;
    auto* Face=NewObject<UFontFace>(CreatePackage(*FacePath),TEXT("FF_LiveNames"),RF_Public|RF_Standalone);
    Face->SourceFilename=Source;Face->LoadingPolicy=EFontLoadingPolicy::Inline;Face->FontFaceData=FFontFaceData::MakeFontFaceData(MoveTemp(Data));Face->CacheSubFaces();
    FAssetRegistryModule::AssetCreated(Face);Face->MarkPackageDirty();
    auto* Font=NewObject<UFont>(CreatePackage(*FontPath),TEXT("F_LiveNames"),RF_Public|RF_Standalone);Font->FontCacheType=EFontCacheType::Runtime;Font->LegacyFontSize=14;
    FTypefaceEntry Entry;Entry.Name=TEXT("Regular");Entry.Font=FFontData(Face);Font->GetMutableInternalCompositeFont().DefaultTypeface.Fonts.Add(Entry);
    FAssetRegistryModule::AssetCreated(Font);Font->MarkPackageDirty();return Font;
}
