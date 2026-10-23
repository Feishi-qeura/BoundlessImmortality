#include "Frontend/UI/InkUiAssetPaths.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Frontend/InkNiagaraTitleWidget.h"
#include "Misc/AutomationTest.h"
#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraSystemWidget.h"
#include "NiagaraUIComponent.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkNiagaraTitleQualityTest,"BoundlessImmortality.Frontend.NiagaraTitles.Quality",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkNiagaraTitleQualityTest::RunTest(const FString&)
{
    using namespace InkNiagaraTitlePresentation;
    float Previous=0;
    for(int32 Tier=0;Tier<=6;++Tier)
    {
        const float Rate=ParticleRateForQuality(Tier);
        TestTrue(TEXT("Every quality tier emits strictly more moving particles"),Rate>Previous);Previous=Rate;
    }
    TestEqual(TEXT("Missing title uses base tier"),QualityForTitleId(NAME_None),0);
    TestTrue(TEXT("Divine blade is stronger than novice"),QualityForTitleId(TEXT("DivineBlade"))>QualityForTitleId(TEXT("Novice")));
    TestTrue(TEXT("Highest realm is stronger than first realm"),QualityForRealmIndex(33)>QualityForRealmIndex(0));
    TestEqual(TEXT("Malformed negative realm clamps"),QualityForRealmIndex(-500),QualityForRealmIndex(0));
    TestEqual(TEXT("Malformed high realm clamps"),QualityForRealmIndex(500),QualityForRealmIndex(33));
    TestEqual(TEXT("Malformed quality clamps"),ParticleRateForQuality(-5),ParticleRateForQuality(0));
    TestEqual(TEXT("Maximum quality caps emission"),ParticleRateForQuality(500),ParticleRateForQuality(6));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkNiagaraTitleLifecycleTest,"BoundlessImmortality.Frontend.NiagaraTitles.VisibilityPolicy",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkNiagaraTitleLifecycleTest::RunTest(const FString&)
{
    using namespace InkNiagaraTitlePresentation;
    TestTrue(TEXT("Visible painted presentation with a player animates"),ShouldAnimate(true,false,true,true));
    TestFalse(TEXT("Hidden ancestor stops particles"),ShouldAnimate(false,false,true,true));
    TestFalse(TEXT("Reduced motion stops particles"),ShouldAnimate(true,true,true,true));
    TestFalse(TEXT("Player loss stops particles"),ShouldAnimate(true,false,false,true));
    TestFalse(TEXT("Unpainted or offscreen widget stops particles"),ShouldAnimate(true,false,true,false));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkNiagaraDestroyedComponentTest,"BoundlessImmortality.Frontend.NiagaraTitles.DestroyedComponentLifecycle",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkNiagaraDestroyedComponentTest::RunTest(const FString&)
{
    // Exercise the real UObject destruction path without a renderer, asset load,
    // world, or global GC. Strong references retain storage until assertions end;
    // they do not make a destroyed component valid for further particle work.
    TStrongObjectPtr<UNiagaraSystemWidget> Widget(NewObject<UNiagaraSystemWidget>(GetTransientPackage()));
    TStrongObjectPtr<UNiagaraUIComponent> Component(NewObject<UNiagaraUIComponent>(GetTransientPackage()));
    TStrongObjectPtr<UNiagaraSystem> ReplacementSystem(NewObject<UNiagaraSystem>(GetTransientPackage()));
    auto* ComponentProperty=FindFProperty<FObjectPropertyBase>(UNiagaraSystemWidget::StaticClass(),TEXT("NiagaraComponent"));
    if(!TestNotNull(TEXT("Production widget retains a reflected component reference"),ComponentProperty))return false;

    // Reflection supplies the otherwise private field, without a test-only hook
    // in production or a mock of the component's destruction/validity behavior.
    ComponentProperty->SetObjectPropertyValue_InContainer(Widget.Get(),Component.Get());
    TestTrue(TEXT("The injected live component is initially valid"),IsValid(Component.Get()));
    TestEqual(TEXT("Getter exposes the live component before destruction"),Widget->GetNiagaraComponent(),Component.Get());
    TestNull(TEXT("The unregistered component starts without a simulation asset"),Component->GetAsset());
    Component->PrimaryComponentTick.bTickEvenWhenPaused=false;

    Component->DestroyComponent();
    TestTrue(TEXT("DestroyComponent enters the real component destruction lifecycle"),Component->IsBeingDestroyed());
    // This assertion fails against the former raw-pointer getter. Stop on that
    // failure so a regression reports clearly rather than deliberately crashing.
    if(!TestNull(TEXT("A destroyed component must never escape through the getter"),Widget->GetNiagaraComponent()))
    {
        ComponentProperty->SetObjectPropertyValue_InContainer(Widget.Get(),nullptr);
        return false;
    }

    Widget->ActivateSystem(true);
    Widget->DeactivateSystem();
    Widget->UpdateNiagaraSystemReference(ReplacementSystem.Get());
    Widget->UpdateTickWhenPaused(true);
    TestNull(TEXT("Lifecycle API calls do not resurrect the destroyed component"),Widget->GetNiagaraComponent());
    TestNull(TEXT("Updating the widget must not assign an asset on the destroyed component"),Component->GetAsset());
    TestFalse(TEXT("Updating the widget must not mutate destroyed component tick settings"),Component->PrimaryComponentTick.bTickEvenWhenPaused);
    TestEqual(TEXT("Widget keeps the requested system for a future reconstruction"),Widget->NiagaraSystemReference.Get(),ReplacementSystem.Get());
    TestTrue(TEXT("Widget keeps the requested paused-tick setting"),Widget->TickWhenPaused);

    Widget->ReleaseSlateResources(true);
    Widget->ReleaseSlateResources(true);
    TestNull(TEXT("Repeated Slate release clears the component reference"),ComponentProperty->GetObjectPropertyValue_InContainer(Widget.Get()));
    Widget->ActivateSystem(false);
    Widget->DeactivateSystem();
    Widget->UpdateNiagaraSystemReference(nullptr);
    Widget->UpdateTickWhenPaused(false);
    TestNull(TEXT("Calls after release keep the component unavailable"),Widget->GetNiagaraComponent());
    TestNull(TEXT("The system reference can still be cleared after release"),Widget->NiagaraSystemReference.Get());
    TestFalse(TEXT("Paused-tick configuration can still be cleared after release"),Widget->TickWhenPaused);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkNiagaraTitleAssetsTest,"BoundlessImmortality.Frontend.NiagaraTitles.Assets",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkNiagaraTitleAssetsTest::RunTest(const FString&)
{
    for(const TCHAR* Kind:{TEXT("Honor"),TEXT("Realm"),TEXT("RealmIcon"),TEXT("AvatarFrame")})
    {
        const FString Name=FString(TEXT("NS_"))+Kind+TEXT("Particles");
        const FString Path=TEXT("/Game/UI/Ink/NiagaraRevision20261022/Flowing20261022/")+Name+TEXT(".")+Name;
        auto* System=InkUiAssets::Load<UNiagaraSystem>(nullptr,*Path);
        if(!TestNotNull(*FString::Printf(TEXT("%s real Niagara asset is installed"),Kind),System))continue;
        TestTrue(TEXT("All four systems contain an emitter"),System->GetNumEmitters()>0);
        const auto& Parameters=System->GetExposedParameters();
        TestTrue(TEXT("Quality controls real spawn rate"),Parameters.IndexOf(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(),TEXT("User.Rate")))!=INDEX_NONE);
        TestTrue(TEXT("Motion speed is tunable on the component"),Parameters.IndexOf(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(),TEXT("User.Speed")))!=INDEX_NONE);
        TestTrue(TEXT("Widget geometry drives particle extent"),Parameters.IndexOf(FNiagaraVariable(FNiagaraTypeDefinition::GetVec2Def(),TEXT("User.Extent")))!=INDEX_NONE);
        for(const auto& Handle:System->GetEmitterHandles())
        {
            const auto* Data=Handle.GetInstance().GetEmitterData();
            if(!TestNotNull(TEXT("Emitter data retained"),Data))continue;
            TestTrue(TEXT("UI renderer supports these CPU particles"),Data->SimTarget==ENiagaraSimTarget::CPUSim);
            bool Sprite=false;for(auto* Renderer:Data->GetRenderers())if(auto* S=Cast<UNiagaraSpriteRendererProperties>(Renderer)){Sprite=true;TestNotNull(TEXT("Sprite has UI material"),S->Material.Get());}
            TestTrue(TEXT("System emits actual sprites rather than metadata"),Sprite);
        }
    }
    TestFalse(TEXT("Mislettered Cyberspace atlas never overrides exact runtime text"),InkNiagaraTitlePresentation::HonorBrush(TEXT("Cyberspace")).GetResourceObject()!=nullptr);
    TSet<FString> Regions;
    for(int32 Realm=0;Realm<34;++Realm)
    {
        const auto B=InkNiagaraTitlePresentation::RealmBrush(Realm);TestNotNull(TEXT("Shared realm atlas exists"),B.GetResourceObject());
        const FBox2f UV=B.GetUVRegion();if(UV.bIsValid)Regions.Add(UV.ToString());
    }
    TestEqual(TEXT("All 34 authoritative realm stage crops stay distinct"),Regions.Num(),34);
    return true;
}
#endif
