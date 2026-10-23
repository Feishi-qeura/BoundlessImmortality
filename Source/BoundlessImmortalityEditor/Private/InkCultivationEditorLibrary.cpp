#include "InkCultivationEditorLibrary.h"
#include "Modules/ModuleManager.h"
#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraScriptSource.h"
#include "NiagaraSystemFactoryNew.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraParameterMapHistory.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraEditorUtilities.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/Package.h"
#include "Misc/PackageName.h"
#include "Misc/OutputDeviceNull.h"
#include "NiagaraNodeAssignment.h"
#include "NiagaraNodeCustomHlsl.h"
#include "NiagaraNodeOutput.h"
#include "EdGraphSchema_Niagara.h"
#include "UObject/UnrealType.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/LevelScriptActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "StaticMeshComponentLODInfo.h"
#include "NiagaraComponent.h"
#include "NiagaraSystemInstanceController.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraEmitterInstance.h"
#include "NiagaraDataSetAccessor.h"
#include "NiagaraGpuComputeDispatchInterface.h"
#include "NiagaraWorldManager.h"
#include "Niagara/Internal/NiagaraSystemEmitterState.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "NiagaraMeshRendererProperties.h"
#include "Engine/StaticMesh.h"
#include "BlueprintEditor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Framework/Docking/TabManager.h"
#include "Editor.h"
#include "LevelEditorViewport.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, BoundlessImmortalityEditor)

namespace
{
// Niagara's engine helper for rebuilding emitter nodes is not exported in 5.8.
// Create the same native graph node through its reflected properties instead.
bool EnsureEmitterInvocationNodes(UNiagaraSystem& System)
{
    auto* Source = Cast<UNiagaraScriptSource>(System.GetSystemSpawnScript()->GetLatestSource());
    if (!Source || !Source->NodeGraph) return false;
    UNiagaraGraph* Graph = Source->NodeGraph;
    UClass* NodeClass = FindObject<UClass>(nullptr, TEXT("/Script/NiagaraEditor.NiagaraNodeEmitter"));
    if (!NodeClass) return false;
    int32 Existing = 0;
    for (UEdGraphNode* Node : Graph->Nodes) Existing += Node->IsA(NodeClass) ? 1 : 0;
    if (Existing == System.GetNumEmitters()*2) return true;
    // This authoring helper supports newly generated systems without invocation nodes.
    if (Existing != 0) return false;
    auto* OwnerProperty = FindFProperty<FObjectPropertyBase>(NodeClass, TEXT("OwnerSystem"));
    auto* IdProperty = FindFProperty<FStructProperty>(NodeClass, TEXT("EmitterHandleId"));
    auto* UsageProperty = FindFProperty<FEnumProperty>(NodeClass, TEXT("ScriptType"));
    if (!OwnerProperty || !IdProperty || !UsageProperty) return false;
    Graph->Modify();
    for (int32 Stage=0; Stage<2; ++Stage)
    {
        auto* Output = Graph->FindEquivalentOutputNode(Stage==0 ? ENiagaraScriptUsage::SystemSpawnScript : ENiagaraScriptUsage::SystemUpdateScript);
        if (!Output) return false;
        UEdGraphPin* Destination = Output->GetInputPin(0);
        if (!Destination || Destination->LinkedTo.Num()!=1) return false;
        for (const auto& Handle : System.GetEmitterHandles())
        {
            UEdGraphNode* Node = NewObject<UEdGraphNode>(Graph, NodeClass, NAME_None, RF_Transactional);
            OwnerProperty->SetObjectPropertyValue_InContainer(Node, &System);
            const FGuid Id = Handle.GetId();
            IdProperty->CopyCompleteValue(IdProperty->ContainerPtrToValuePtr<void>(Node), &Id);
            UsageProperty->GetUnderlyingProperty()->SetIntPropertyValue(UsageProperty->ContainerPtrToValuePtr<void>(Node), int64(Stage==0 ? ENiagaraScriptUsage::EmitterSpawnScript : ENiagaraScriptUsage::EmitterUpdateScript));
            Graph->AddNode(Node, true, false);
            Node->CreateNewGuid();
            Node->AllocateDefaultPins();
            UEdGraphPin* Input=nullptr;
            UEdGraphPin* OutputPin=nullptr;
            for (UEdGraphPin* Pin : Node->Pins)
            {
                if (Pin->Direction==EGPD_Input && !Input) Input=Pin;
                if (Pin->Direction==EGPD_Output && !OutputPin) OutputPin=Pin;
            }
            if (!Input || !OutputPin) return false;
            UEdGraphPin* Previous=Destination->LinkedTo[0];
            Destination->BreakAllPinLinks();
            Previous->MakeLinkTo(Input);
            OutputPin->MakeLinkTo(Destination);
        }
    }
    Graph->NotifyGraphChanged();
    return true;
}
}

void UInkCultivationEditorLibrary::SetLevelPreviewRealtime(bool Enabled)
{
    if(GEditor)for(auto* Viewport:GEditor->GetLevelViewportClients())if(Viewport)Viewport->SetRealtime(Enabled);
}

AActor* UInkCultivationEditorLibrary::GetBlueprintPreviewActor(UBlueprint* Blueprint)
{
    if(!GEditor||!Blueprint)return nullptr;
    auto* Subsystem=GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
    auto* Instance=Subsystem->FindEditorForAsset(Blueprint,false);
    if(!Instance){Subsystem->OpenEditorForAsset(Blueprint);return nullptr;}
    if(Instance->GetEditorName()!=TEXT("BlueprintEditor"))return nullptr;
    auto* Editor=static_cast<FBlueprintEditor*>(Instance);
    Editor->GetTabManager()->TryInvokeTab(FName(TEXT("SCSViewport")));
    if(auto* Preview=Editor->GetPreviewActor())
        for(auto* Viewport:GEditor->GetAllViewportClients())if(Viewport&&Viewport->GetWorld()==Preview->GetWorld())Viewport->SetRealtime(true);
    return Editor->GetPreviewActor();
}

bool UInkCultivationEditorLibrary::UpgradeQiSystem(UNiagaraSystem* System, UMaterialInterface* LightningMaterial)
{
    if(!System||System->GetNumEmitters()!=7||!LightningMaterial)return false;
    for(int32 I=0;I<7;++I)
    {
        auto Instance=System->GetEmitterHandles()[I].GetInstance();auto* Data=Instance.GetEmitterData();
        auto* Source=Cast<UNiagaraScriptSource>(Data->UpdateScriptProps.Script->GetLatestSource());UNiagaraGraph* Graph=Source->NodeGraph;
        // RGB.r 仅用作材质随机种子；不改变 alpha 生命周期，也不影响实时数量。
        for(UEdGraphNode* N:Graph->Nodes)if(auto* E=Cast<UNiagaraNodeCustomHlsl>(N))
        {
            if(E->Signature.Outputs.Num()==1 && E->Signature.Outputs[0].GetType()==FNiagaraTypeDefinition::GetColorDef())
                FindFProperty<FStrProperty>(E->GetClass(),TEXT("CustomHlsl"))->SetPropertyValue_InContainer(E,TEXT("float4(frac(float(Map.Particles.UniqueID)*0.618034),1.0,1.0,saturate(Map.Particles.NormalizedAge*6.0)*saturate((1.0-Map.Particles.NormalizedAge)*8.0))"));
        }
        if(I==6)
        {
            bool HasMesh=false;
            auto Renderers=Data->GetRenderers();
            for(auto* R:Renderers)
            {
                if(auto* M=Cast<UNiagaraMeshRendererProperties>(R)){HasMesh=true;M->OverrideMaterials[0].ExplicitMat=LightningMaterial;}
                else Instance.Emitter->RemoveRenderer(R,Instance.Version);
            }
            if(!HasMesh)
            {
                auto* Renderer=NewObject<UNiagaraMeshRendererProperties>(Instance.Emitter);
                FNiagaraMeshRendererMeshProperties Mesh;Mesh.Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere"));
                Renderer->Meshes={Mesh};Renderer->bOverrideMaterials=true;
                FNiagaraMeshMaterialOverride Override;Override.ExplicitMat=LightningMaterial;Renderer->OverrideMaterials={Override};
                Instance.Emitter->AddRenderer(Renderer,Instance.Version);
                auto* Output=Graph->FindEquivalentOutputNode(ENiagaraScriptUsage::ParticleUpdateScript);
                const auto Type=FNiagaraTypeDefinition::GetVec3Def();const auto* Schema=GetDefault<UEdGraphSchema_Niagara>();
                FNiagaraVariable V(Type,TEXT("Particles.Scale"));V.AllocateData();FString Default;Schema->TryGetPinDefaultValueFromNiagaraVariable(V,Default);
                auto* A=FNiagaraStackGraphUtilities::AddParameterModuleToStack({V},*Output,INDEX_NONE,{Default});
                auto Alias=FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(FNiagaraParameterHandle(TEXT("Module.Particles.Scale")),A);
                auto& Pin=FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(*A,Alias,Type,FGuid(),FGuid());
                UEdGraphPin* Map=nullptr;
                for(auto* P:Pin.GetOwningNode()->Pins)if(P->Direction==EGPD_Input&&Schema->PinToTypeDefinition(P).IsSameBaseDefinition(FNiagaraTypeDefinition::GetParameterMapDef()))Map=P;
                if(!Map||Map->LinkedTo.IsEmpty())return false;
                FGraphNodeCreator<UNiagaraNodeCustomHlsl> Creator(*Graph);auto* E=Creator.CreateNode();E->ScriptUsage=ENiagaraScriptUsage::DynamicInput;
                E->Signature.Inputs={FNiagaraVariable(FNiagaraTypeDefinition::GetParameterMapDef(),TEXT("Map"))};E->Signature.Outputs={FNiagaraVariable(Type,TEXT("CustomHLSLOutput"))};
                FindFProperty<FStrProperty>(E->GetClass(),TEXT("CustomHlsl"))->SetPropertyValue_InContainer(E,TEXT("float3(1.0,1.0,1.0)*clamp(Map.Transient.QiSize,8.0,120.0)*0.006*saturate((1.0-Map.Particles.NormalizedAge)*6.0)"));
                Creator.Finalize();E->FindPin(TEXT("Map"),EGPD_Input)->MakeLinkTo(Map->LinkedTo[0]);E->FindPin(TEXT("CustomHLSLOutput"),EGPD_Output)->MakeLinkTo(&Pin);
            }
        }
        Graph->NotifyGraphChanged();
    }
    System->MarkPackageDirty();return CompileSystem(System);
}

bool UInkCultivationEditorLibrary::UpgradeQiAbsorption(UNiagaraSystem* System)
{
    if(!System||System->GetNumEmitters()!=7)return false;
    const auto Position=FNiagaraTypeDefinition::GetPositionDef();
    const auto* Schema=GetDefault<UEdGraphSchema_Niagara>();
    const FString Age=TEXT("Map.Particles.NormalizedAge");
    const auto Ease=[&](float Start,float End)
    {
        const FString T=FString::Printf(TEXT("saturate((%s-%.4f)/%.4f)"),*Age,Start,End-Start);
        return FString::Printf(TEXT("(%s*%s*(3.0-2.0*%s))"),*T,*T,*T);
    };
    const FString Rise=Ease(0,.38f),Absorb=Ease(.38f,1),FadeIn=Ease(0,.28f),FadeOut=Ease(.94f,1);
    for(const auto& Handle:System->GetEmitterHandles())
    {
        auto Instance=Handle.GetInstance();auto* Data=Instance.GetEmitterData();
        Data->bLocalSpace=false;
        // Custom-HLSL reads through a transient map are invisible to the attribute trimmer.
        // Keep the spawn origin across update ticks and in cooked VM data.
        Data->AttributesToPreserve.AddUnique(TEXT("Particles.QiOrigin"));
        auto* Source=Cast<UNiagaraScriptSource>(Data->UpdateScriptProps.Script->GetLatestSource());
        UNiagaraGraph* Graph=Source->NodeGraph;
        bool HasOrigin=false,HasRead=false;
        for(UEdGraphNode* Node:Graph->Nodes)if(auto* Assignment=Cast<UNiagaraNodeAssignment>(Node))
            for(const auto& Target:Assignment->GetAssignmentTargets())
            {
                if(Target.GetName()==TEXT("Particles.QiOrigin"))HasOrigin=true;
                if(Target.GetName()==TEXT("Transient.QiOrigin"))HasRead=true;
            }
        if(!HasOrigin)
        {
            auto* Spawn=Graph->FindEquivalentOutputNode(ENiagaraScriptUsage::ParticleSpawnScript);
            if(!Spawn)return false;
            FNiagaraVariable Origin(Position,TEXT("Particles.QiOrigin"));Origin.AllocateData();FString Default;
            Schema->TryGetPinDefaultValueFromNiagaraVariable(Origin,Default);
            auto* Assignment=FNiagaraStackGraphUtilities::AddParameterModuleToStack({Origin},*Spawn,INDEX_NONE,{Default});
            const auto Alias=FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(FNiagaraParameterHandle(TEXT("Module.Particles.QiOrigin")),Assignment);
            auto& Pin=FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(*Assignment,Alias,Position,FGuid(),FGuid());
            TSet<FNiagaraVariableBase> Known;Known.Add(FNiagaraVariableBase(Position,TEXT("Engine.Owner.Position")));
            FNiagaraStackGraphUtilities::SetLinkedParameterValueForFunctionInput(Pin,FNiagaraVariableBase(Position,TEXT("Engine.Owner.Position")),Known);
        }
        if(!HasRead)
        {
            auto* Update=Graph->FindEquivalentOutputNode(ENiagaraScriptUsage::ParticleUpdateScript);
            FNiagaraVariable Origin(Position,TEXT("Transient.QiOrigin"));Origin.AllocateData();FString Default;
            Schema->TryGetPinDefaultValueFromNiagaraVariable(Origin,Default);
            auto* Assignment=FNiagaraStackGraphUtilities::AddParameterModuleToStack({Origin},*Update,0,{Default});
            const auto Alias=FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(FNiagaraParameterHandle(TEXT("Module.Transient.QiOrigin")),Assignment);
            auto& Pin=FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(*Assignment,Alias,Position,FGuid(),FGuid());
            TSet<FNiagaraVariableBase> Known;Known.Add(FNiagaraVariableBase(Position,TEXT("Particles.QiOrigin")));
            FNiagaraStackGraphUtilities::SetLinkedParameterValueForFunctionInput(Pin,FNiagaraVariableBase(Position,TEXT("Particles.QiOrigin")),Known);
        }
        for(UEdGraphNode* Node:Graph->Nodes)if(auto* Hlsl=Cast<UNiagaraNodeCustomHlsl>(Node);Hlsl&&Hlsl->Signature.Outputs.Num()==1)
        {
            const auto Type=Hlsl->Signature.Outputs[0].GetType();FString Code;
            if(Type==Position)
                Code=FString::Printf(TEXT("lerp(Map.Transient.QiOrigin + float3(cos(float(Map.Particles.UniqueID)*2.399963)*clamp(Map.Transient.QiRange,20.0,2000.0)*(0.65+0.35*frac(float(Map.Particles.UniqueID)*0.618034)),sin(float(Map.Particles.UniqueID)*2.399963)*clamp(Map.Transient.QiRange,20.0,2000.0)*(0.65+0.35*frac(float(Map.Particles.UniqueID)*0.618034)),-Map.Transient.QiHeight*0.65+Map.Transient.QiHeight*%s),Map.Engine.Owner.Position,%s)"),*Rise,*Absorb);
            else if(Type==FNiagaraTypeDefinition::GetColorDef())
                Code=FString::Printf(TEXT("float4(frac(float(Map.Particles.UniqueID)*0.618034),1.0,1.0,%s*(1.0-%s))"),*FadeIn,*FadeOut);
            if(!Code.IsEmpty())FindFProperty<FStrProperty>(Hlsl->GetClass(),TEXT("CustomHlsl"))->SetPropertyValue_InContainer(Hlsl,Code);
        }
        Graph->NotifyGraphChanged();
    }
    System->MarkPackageDirty();return CompileSystem(System);
}

void UInkCultivationEditorLibrary::InjectQiToggleKey(APlayerController* Controller, bool Pressed)
{
    if(Controller && Controller->GetWorld()->WorldType==EWorldType::PIE)
        Controller->InputKey(FInputKeyEventArgs(nullptr,FInputDeviceId::CreateFromInternalId(0),EKeys::C,Pressed?IE_Pressed:IE_Released,FPlatformTime::Cycles64()));
}

void UInkCultivationEditorLibrary::InjectGameplayKey(APlayerController* Controller, FKey Key, bool Pressed)
{
    if(Controller && Controller->GetWorld() && Controller->GetWorld()->WorldType==EWorldType::PIE && Key.IsValid())
        Controller->InputKey(FInputKeyEventArgs(nullptr,FInputDeviceId::CreateFromInternalId(0),Key,Pressed?IE_Pressed:IE_Released,FPlatformTime::Cycles64()));
}

UNiagaraSystem* UInkCultivationEditorLibrary::CreateQiSystem(const FString& PackagePath, const TArray<UMaterialInterface*>& Materials)
{
    if(Materials.Num()!=7 || !PackagePath.StartsWith(TEXT("/Game/1/QiCultivation/"))) return nullptr;
    for(auto* M:Materials) if(!M) return nullptr;
    UPackage* Package=CreatePackage(*PackagePath);
    auto* System=NewObject<UNiagaraSystem>(Package,*FPackageName::GetLongPackageAssetName(PackagePath),RF_Public|RF_Standalone|RF_Transactional);
    UNiagaraSystemFactoryNew::InitializeSystem(System,true);
    auto* Template=LoadObject<UNiagaraEmitter>(nullptr,TEXT("/Niagara/DefaultAssets/Templates/Emitters/Fountain.Fountain"));
    if(!Template) return nullptr;
    const TCHAR* Names[]={TEXT("White"),TEXT("Fire"),TEXT("Wind"),TEXT("Earth"),TEXT("Water"),TEXT("Ice"),TEXT("Lightning")};
    const auto Float=FNiagaraTypeDefinition::GetFloatDef();
    const auto* Schema=GetDefault<UEdGraphSchema_Niagara>();
    auto Expose=[&](FString Name,float Value){System->GetExposedParameters().SetParameterValue(Value,FNiagaraVariable(Float,*Name),true);};
    Expose(TEXT("User.Duration"),5); Expose(TEXT("User.SpawnHeight"),100); Expose(TEXT("User.OrbSize"),44);
    for(int32 I=0;I<7;++I)
    {
        FString Prefix=FString(TEXT("User."))+Names[I];
        Expose(Prefix+TEXT("Count"),8); Expose(Prefix+TEXT("Range"),240);
        auto Handle=System->AddEmitterHandle(*Template,Names[I],Template->GetExposedVersion().VersionGuid);
        auto Instance=Handle.GetInstance(); auto* Data=Instance.GetEmitterData();
        Data->bLocalSpace=true; Data->SimTarget=ENiagaraSimTarget::CPUSim;
        for(auto* Renderer:Data->GetRenderers()) if(auto* Sprite=Cast<UNiagaraSpriteRendererProperties>(Renderer)) Sprite->Material=Materials[I];
        auto* Source=Cast<UNiagaraScriptSource>(Data->UpdateScriptProps.Script->GetLatestSource());
        UNiagaraGraph* Graph=Source->NodeGraph;
        for(UEdGraphNode* Node:Graph->Nodes) if(auto* Call=Cast<UNiagaraNodeFunctionCall>(Node))
        {
            FString N=Call->GetFunctionName();
            if(N==TEXT("SolveForcesAndVelocity")||N==TEXT("GravityForce")||N==TEXT("Drag")||N==TEXT("ScaleColor")||N==TEXT("AddVelocity")||N==TEXT("ShapeLocation")) FNiagaraStackGraphUtilities::SetModuleIsEnabled(*Call,false);
        }
        auto Expression=[&](UEdGraphPin& Pin,const FNiagaraTypeDefinition& Type,const FString& Code)
        {
            UEdGraphPin* Map=nullptr;
            for(auto* P:Pin.GetOwningNode()->Pins) if(P->Direction==EGPD_Input&&Schema->PinToTypeDefinition(P).IsSameBaseDefinition(FNiagaraTypeDefinition::GetParameterMapDef())) Map=P;
            if(!Map||Map->LinkedTo.IsEmpty()) return false;
            FGraphNodeCreator<UNiagaraNodeCustomHlsl> Creator(*Graph);
            auto* E=Creator.CreateNode(); E->ScriptUsage=ENiagaraScriptUsage::DynamicInput;
            E->Signature.Inputs={FNiagaraVariable(FNiagaraTypeDefinition::GetParameterMapDef(),TEXT("Map"))};
            E->Signature.Outputs={FNiagaraVariable(Type,TEXT("CustomHLSLOutput"))};
            FindFProperty<FStrProperty>(E->GetClass(),TEXT("CustomHlsl"))->SetPropertyValue_InContainer(E,Code);
            Creator.Finalize(); Pin.BreakAllPinLinks();
            E->FindPin(TEXT("Map"),EGPD_Input)->MakeLinkTo(Map->LinkedTo[0]);
            E->FindPin(TEXT("CustomHLSLOutput"),EGPD_Output)->MakeLinkTo(&Pin); return true;
        };
        auto Read=[&](UNiagaraNodeOutput* Output,FString Local,FString User)
        {
            FNiagaraVariable V(Float,*Local);V.AllocateData(); FString Default;Schema->TryGetPinDefaultValueFromNiagaraVariable(V,Default);
            auto* A=FNiagaraStackGraphUtilities::AddParameterModuleToStack({V},*Output,0,{Default});
            auto Alias=FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(FNiagaraParameterHandle(*(TEXT("Module.")+Local)),A);
            auto& P=FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(*A,Alias,Float,FGuid(),FGuid());
            TSet<FNiagaraVariableBase> Known; Known.Add(FNiagaraVariableBase(Float,*User));
            FNiagaraStackGraphUtilities::SetLinkedParameterValueForFunctionInput(P,FNiagaraVariableBase(Float,*User),Known);
        };
        auto* EU=Graph->FindEquivalentOutputNode(ENiagaraScriptUsage::EmitterUpdateScript);
        Read(EU,TEXT("Transient.QiCount"),Prefix+TEXT("Count")); Read(EU,TEXT("Transient.QiDuration"),TEXT("User.Duration"));
        auto Nodes=Graph->Nodes;
        for(UEdGraphNode* Node:Nodes) if(auto* Call=Cast<UNiagaraNodeFunctionCall>(Node))
        {
            FString N=Call->GetFunctionName();
            if(N==TEXT("SpawnRate")||N==TEXT("InitializeParticle"))
            {
                const FString Input=N==TEXT("SpawnRate")?TEXT("Module.SpawnRate"):TEXT("Module.Lifetime");
                auto Alias=FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(FNiagaraParameterHandle(*Input),Call);
                auto& P=FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(*Call,Alias,Float,FGuid(),FGuid());
                if(N==TEXT("SpawnRate")) {if(!Expression(P,Float,TEXT("clamp(Map.Transient.QiCount,0.0,64.0)/clamp(Map.Transient.QiDuration,1.0,20.0)"))) return nullptr;}
                else { P.BreakAllPinLinks(); TSet<FNiagaraVariableBase> Known;Known.Add(FNiagaraVariableBase(Float,TEXT("User.Duration"))); FNiagaraStackGraphUtilities::SetLinkedParameterValueForFunctionInput(P,FNiagaraVariableBase(Float,TEXT("User.Duration")),Known); }
            }
        }
        SetEmitterInput(System,I,TEXT("InitializeParticle"),TEXT("Lifetime Mode"),TEXT("NewEnumerator0"),true);
        auto* PU=Graph->FindEquivalentOutputNode(ENiagaraScriptUsage::ParticleUpdateScript);
        Read(PU,TEXT("Transient.QiRange"),Prefix+TEXT("Range"));Read(PU,TEXT("Transient.QiHeight"),TEXT("User.SpawnHeight"));Read(PU,TEXT("Transient.QiSize"),TEXT("User.OrbSize"));
        auto Write=[&](FNiagaraTypeDefinition Type,FString Target,FString Code)
        {
            FNiagaraVariable V(Type,*Target);V.AllocateData();FString Default;Schema->TryGetPinDefaultValueFromNiagaraVariable(V,Default);
            auto* A=FNiagaraStackGraphUtilities::AddParameterModuleToStack({V},*PU,INDEX_NONE,{Default});
            auto Alias=FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(FNiagaraParameterHandle(*(TEXT("Module.")+Target)),A);
            auto& P=FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(*A,Alias,Type,FGuid(),FGuid());
            return Expression(P,Type,Code);
        };
        const FString Age=TEXT("Map.Particles.NormalizedAge");
        const FString Theta=FString::Printf(TEXT("(float(Map.Particles.UniqueID)*2.399963+%.4f+Map.Particles.NormalizedAge*2.8)"),I*.8976);
        const FString R=TEXT("(clamp(Map.Transient.QiRange,20.0,2000.0)*(0.65+0.35*frac(float(Map.Particles.UniqueID)*0.618034))*(1.0-Map.Particles.NormalizedAge*Map.Particles.NormalizedAge))");
        FString Pos=FString::Printf(TEXT("float3(cos(%s)*%s,sin(%s)*%s,(sin(float(Map.Particles.UniqueID)*1.73+%.4f)*Map.Transient.QiHeight)*(1.0-Map.Particles.NormalizedAge))"),*Theta,*R,*Theta,*R,I*.7);
        if(!Write(FNiagaraTypeDefinition::GetPositionDef(),TEXT("Particles.Position"),Pos)||
           !Write(FNiagaraTypeDefinition::GetVec2Def(),TEXT("Particles.SpriteSize"),TEXT("float2(1.0,1.0)*clamp(Map.Transient.QiSize,8.0,120.0)*(0.75+0.25*sin(float(Map.Particles.UniqueID)))*saturate((1.0-Map.Particles.NormalizedAge)*6.0)"))||
           !Write(FNiagaraTypeDefinition::GetColorDef(),TEXT("Particles.Color"),TEXT("float4(1.0,1.0,1.0,saturate(Map.Particles.NormalizedAge*6.0)*saturate((1.0-Map.Particles.NormalizedAge)*8.0))"))||
           !Write(Float,TEXT("Particles.SpriteRotation"),TEXT("0.0"))) return nullptr;
        Graph->NotifyGraphChanged();
    }
    if(!EnsureEmitterInvocationNodes(*System)) return nullptr;
    System->SetWarmupTime(0);FAssetRegistryModule::AssetCreated(System);System->MarkPackageDirty();return System;
}

UNiagaraSystem* UInkCultivationEditorLibrary::CreateTitleSystem(const FString& PackagePath,UMaterialInterface* Material)
{
    if(!Material||!(PackagePath.StartsWith(TEXT("/Game/UI/Ink/Cohesion20261021/"))||PackagePath.StartsWith(TEXT("/Game/UI/Ink/SignIn20261022/"))))return nullptr;
    auto* Template=LoadObject<UNiagaraEmitter>(nullptr,TEXT("/Niagara/DefaultAssets/Templates/Emitters/SingleLoopingParticle.SingleLoopingParticle"));if(!Template)return nullptr;
    auto* Package=CreatePackage(*PackagePath);auto* System=NewObject<UNiagaraSystem>(Package,*FPackageName::GetLongPackageAssetName(PackagePath),RF_Public|RF_Standalone|RF_Transactional);
    UNiagaraSystemFactoryNew::InitializeSystem(System,true);
    const auto Handle=System->AddEmitterHandle(*Template,TEXT("TitleAura"),Template->GetExposedVersion().VersionGuid);
    auto* Data=Handle.GetInstance().GetEmitterData();Data->bLocalSpace=true;Data->SimTarget=ENiagaraSimTarget::CPUSim;
    for(auto* R:Data->GetRenderers())if(auto* S=Cast<UNiagaraSpriteRendererProperties>(R)){S->Material=Material;S->FacingMode=ENiagaraSpriteFacingMode::FaceCameraPlane;}
    const auto Vec2=FNiagaraTypeDefinition::GetVec2Def(),Color=FNiagaraTypeDefinition::GetColorDef();
    System->GetExposedParameters().SetParameterValue(FVector2f(148,34),FNiagaraVariable(Vec2,TEXT("User.TitleSize")),true);
    System->GetExposedParameters().SetParameterValue(FLinearColor(1,.67f,.18f,.65f),FNiagaraVariable(Color,TEXT("User.TitleColor")),true);
    auto* Source=Cast<UNiagaraScriptSource>(Data->UpdateScriptProps.Script->GetLatestSource());UNiagaraGraph* Graph=Source->NodeGraph;
    auto* Output=Graph->FindEquivalentOutputNode(ENiagaraScriptUsage::ParticleUpdateScript);const auto* Schema=GetDefault<UEdGraphSchema_Niagara>();
    auto Link=[&](FNiagaraTypeDefinition Type,const TCHAR* Target,const TCHAR* User)
    {
        FNiagaraVariable V(Type,Target);V.AllocateData();FString Default;Schema->TryGetPinDefaultValueFromNiagaraVariable(V,Default);
        auto* Assign=FNiagaraStackGraphUtilities::AddParameterModuleToStack({V},*Output,INDEX_NONE,{Default});
        const auto Alias=FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(FNiagaraParameterHandle(*(FString(TEXT("Module."))+Target)),Assign);
        auto& Pin=FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(*Assign,Alias,Type,FGuid(),FGuid());TSet<FNiagaraVariableBase> Known;Known.Add(FNiagaraVariableBase(Type,User));FNiagaraStackGraphUtilities::SetLinkedParameterValueForFunctionInput(Pin,FNiagaraVariableBase(Type,User),Known);
    };
    Link(Vec2,TEXT("Particles.SpriteSize"),TEXT("User.TitleSize"));Link(Color,TEXT("Particles.Color"),TEXT("User.TitleColor"));
    Graph->NotifyGraphChanged();if(!EnsureEmitterInvocationNodes(*System))return nullptr;
    System->SetWarmupTime(0);FAssetRegistryModule::AssetCreated(System);System->MarkPackageDirty();return System;
}

UNiagaraSystem* UInkCultivationEditorLibrary::CreateTitleParticleSystem(const FString& PackagePath,UMaterialInterface* Material,int32 Kind)
{
    const bool Character=Kind==4||Kind==5;
    if(!Material||Kind<0||Kind>5||!PackagePath.StartsWith(Character?TEXT("/Game/UI/Ink/CharacterProfile20261022/"):TEXT("/Game/UI/Ink/NiagaraRevision20261022/")))return nullptr;
    auto* Template=LoadObject<UNiagaraEmitter>(nullptr,TEXT("/Niagara/DefaultAssets/Templates/Emitters/Fountain.Fountain"));if(!Template)return nullptr;
    auto* Package=CreatePackage(*PackagePath);auto* System=NewObject<UNiagaraSystem>(Package,*FPackageName::GetLongPackageAssetName(PackagePath),RF_Public|RF_Standalone|RF_Transactional);
    UNiagaraSystemFactoryNew::InitializeSystem(System,true);
    const auto Handle=System->AddEmitterHandle(*Template,TEXT("MovingTitleMotes"),Template->GetExposedVersion().VersionGuid);
    auto* Data=Handle.GetInstance().GetEmitterData();Data->bLocalSpace=true;Data->SimTarget=ENiagaraSimTarget::CPUSim;
    for(auto* R:Data->GetRenderers())if(auto* S=Cast<UNiagaraSpriteRendererProperties>(R))
    {
        S->Material=Material;S->FacingMode=ENiagaraSpriteFacingMode::FaceCameraPlane;
        if(!Character)S->Alignment=ENiagaraSpriteAlignment::Unaligned;
    }
    const auto Float=FNiagaraTypeDefinition::GetFloatDef(),Vec2=FNiagaraTypeDefinition::GetVec2Def(),Color=FNiagaraTypeDefinition::GetColorDef();
    auto Expose=[&](const TCHAR* Name,float Value){System->GetExposedParameters().SetParameterValue(Value,FNiagaraVariable(Float,Name),true);};
    Expose(TEXT("User.Rate"),17);Expose(TEXT("User.Speed"),.85f);Expose(TEXT("User.ParticleSize"),3.2f);
    if(!Character)Expose(TEXT("User.Quality"),2);
    System->GetExposedParameters().SetParameterValue(FVector2f(65,15),FNiagaraVariable(Vec2,TEXT("User.Extent")),true);
    System->GetExposedParameters().SetParameterValue(FLinearColor(1,.76f,.25f,1),FNiagaraVariable(Color,TEXT("User.Color")),true);
    if(Character)
    {
        Expose(TEXT("User.Radius"),18);Expose(TEXT("User.Halo"),0);
        System->GetExposedParameters().SetParameterValue(FVector2f(0),FNiagaraVariable(Vec2,TEXT("User.Source")),true);
        System->GetExposedParameters().SetParameterValue(FVector2f(0),FNiagaraVariable(Vec2,TEXT("User.Target")),true);
    }
    auto* Source=Cast<UNiagaraScriptSource>(Data->UpdateScriptProps.Script->GetLatestSource());UNiagaraGraph* Graph=Source->NodeGraph;const auto* Schema=GetDefault<UEdGraphSchema_Niagara>();
    const auto Nodes=Graph->Nodes;
    for(UEdGraphNode* N:Nodes)if(auto* Call=Cast<UNiagaraNodeFunctionCall>(N))
    {
        const FString Name=Call->GetFunctionName();
        if(Name==TEXT("SolveForcesAndVelocity")||Name==TEXT("GravityForce")||Name==TEXT("Drag")||Name==TEXT("ScaleColor")||Name==TEXT("AddVelocity")||Name==TEXT("ShapeLocation"))FNiagaraStackGraphUtilities::SetModuleIsEnabled(*Call,false);
        if(Name==TEXT("SpawnRate"))
        {
            const auto Alias=FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(FNiagaraParameterHandle(TEXT("Module.SpawnRate")),Call);
            auto& Pin=FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(*Call,Alias,Float,FGuid(),FGuid());
            TSet<FNiagaraVariableBase> Known;Known.Add(FNiagaraVariableBase(Float,TEXT("User.Rate")));
            FNiagaraStackGraphUtilities::SetLinkedParameterValueForFunctionInput(Pin,FNiagaraVariableBase(Float,TEXT("User.Rate")),Known);
        }
    }
    SetEmitterInput(System,0,TEXT("InitializeParticle"),TEXT("Lifetime Mode"),TEXT("NewEnumerator0"),true);
    SetEmitterInput(System,0,TEXT("InitializeParticle"),TEXT("Lifetime"),TEXT("2.4"));
    auto* PU=Graph->FindEquivalentOutputNode(ENiagaraScriptUsage::ParticleUpdateScript);if(!PU)return nullptr;
    auto Assignment=[&](const FNiagaraTypeDefinition& Type,const TCHAR* Target,int32 Index)
    {
        FNiagaraVariable V(Type,Target);V.AllocateData();FString Default;Schema->TryGetPinDefaultValueFromNiagaraVariable(V,Default);
        auto* A=FNiagaraStackGraphUtilities::AddParameterModuleToStack({V},*PU,Index,{Default});
        const auto Alias=FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(FNiagaraParameterHandle(*(FString(TEXT("Module."))+Target)),A);
        return &FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(*A,Alias,Type,FGuid(),FGuid());
    };
    auto Read=[&](const FNiagaraTypeDefinition& Type,const TCHAR* Local,const TCHAR* User)
    {
        auto* Pin=Assignment(Type,Local,0);TSet<FNiagaraVariableBase> Known;Known.Add(FNiagaraVariableBase(Type,User));
        FNiagaraStackGraphUtilities::SetLinkedParameterValueForFunctionInput(*Pin,FNiagaraVariableBase(Type,User),Known);
    };
    Read(Vec2,TEXT("Transient.TitleExtent"),TEXT("User.Extent"));Read(Float,TEXT("Transient.TitleSpeed"),TEXT("User.Speed"));
    Read(Float,TEXT("Transient.TitleSize"),TEXT("User.ParticleSize"));Read(Color,TEXT("Transient.TitleColor"),TEXT("User.Color"));
    Read(Float,TEXT("Transient.TitleSystemAge"),TEXT("System.Age"));
    if(!Character)Read(Float,TEXT("Transient.TitleQuality"),TEXT("User.Quality"));
    if(Character)
    {
        Read(Vec2,TEXT("Transient.CharacterSource"),TEXT("User.Source"));Read(Vec2,TEXT("Transient.CharacterTarget"),TEXT("User.Target"));
        Read(Float,TEXT("Transient.CharacterRadius"),TEXT("User.Radius"));Read(Float,TEXT("Transient.CharacterHalo"),TEXT("User.Halo"));
    }
    auto Write=[&](const FNiagaraTypeDefinition& Type,const TCHAR* Target,const FString& Code)
    {
        auto* Pin=Assignment(Type,Target,INDEX_NONE);UEdGraphPin* Map=nullptr;
        for(auto* P:Pin->GetOwningNode()->Pins)if(P->Direction==EGPD_Input&&Schema->PinToTypeDefinition(P).IsSameBaseDefinition(FNiagaraTypeDefinition::GetParameterMapDef()))Map=P;
        if(!Map||Map->LinkedTo.IsEmpty())return false;
        FGraphNodeCreator<UNiagaraNodeCustomHlsl> Creator(*Graph);auto* E=Creator.CreateNode();E->ScriptUsage=ENiagaraScriptUsage::DynamicInput;
        E->Signature.Inputs={FNiagaraVariable(FNiagaraTypeDefinition::GetParameterMapDef(),TEXT("Map"))};E->Signature.Outputs={FNiagaraVariable(Type,TEXT("CustomHLSLOutput"))};
        FindFProperty<FStrProperty>(E->GetClass(),TEXT("CustomHlsl"))->SetPropertyValue_InContainer(E,Code);Creator.Finalize();
        E->FindPin(TEXT("Map"),EGPD_Input)->MakeLinkTo(Map->LinkedTo[0]);E->FindPin(TEXT("CustomHLSLOutput"),EGPD_Output)->MakeLinkTo(Pin);return true;
    };
    // Each particle has a stable phase and a moving position. Material has no Time node:
    // particle motion, birth/death, rotation, size, and color come from the Niagara VM.
    const FString Theta=TEXT("(float(Map.Particles.UniqueID)*2.399963+Map.Transient.TitleSystemAge*Map.Transient.TitleSpeed+Map.Particles.NormalizedAge*.45)");
    FString Position;
    if(Kind==4)
    {
        Position=FString::Printf(TEXT("float3(Map.Transient.CharacterSource.x+clamp(cos(%s)*1.414214,-1.0,1.0)*Map.Transient.TitleExtent.x,0.0,Map.Transient.CharacterSource.y+clamp(sin(%s)*1.414214,-1.0,1.0)*Map.Transient.TitleExtent.y)"),*Theta,*Theta);
    }
    else if(Kind==5)
    {
        const FString R=TEXT("(Map.Transient.CharacterRadius*((frac(float(Map.Particles.UniqueID)/3.0) < .01 && Map.Transient.CharacterHalo > 0.5) ? 1.65 : (0.28+0.62*frac(float(Map.Particles.UniqueID)*0.618034))))");
        Position=FString::Printf(TEXT("float3(Map.Transient.CharacterTarget.x+cos(%s)*%s,0.0,Map.Transient.CharacterTarget.y+sin(%s)*%s)"),*Theta,*R,*Theta,*R);
    }
    if(Character)
    {
        if(!Write(FNiagaraTypeDefinition::GetPositionDef(),TEXT("Particles.Position"),Position)||
           !Write(Vec2,TEXT("Particles.SpriteSize"),TEXT("float2(1.8,1.0)*Map.Transient.TitleSize*(.8+.2*sin(float(Map.Particles.UniqueID)))*saturate(Map.Particles.NormalizedAge*8.0)*saturate((1.0-Map.Particles.NormalizedAge)*7.0)"))||
           !Write(Color,TEXT("Particles.Color"),TEXT("float4(Map.Transient.TitleColor.rgb,Map.Transient.TitleColor.a*saturate(Map.Particles.NormalizedAge*8.0)*saturate((1.0-Map.Particles.NormalizedAge)*7.0))"))||
           !Write(Float,TEXT("Particles.SpriteRotation"),FString::Printf(TEXT("%s*57.29578"),*Theta)))return nullptr;
    }
    else
    {
        // Segmented silk: every live CPU sprite is a long feathered brush stroke,
        // placed and rotated on the tangent of a moving, open contour stream.
        // Quality introduces additional parallel strands rather than an orbit.
        if(!Write(Float,TEXT("Transient.TitleLayers"),TEXT("(1.0+floor(clamp(Map.Transient.TitleQuality,0.0,6.0)/2.0))"))||
           !Write(Float,TEXT("Transient.TitleLane"),TEXT("fmod(float(Map.Particles.UniqueID),Map.Transient.TitleLayers)"))||
           !Write(Float,TEXT("Transient.TitleSide"),TEXT("(fmod(floor(float(Map.Particles.UniqueID)/Map.Transient.TitleLayers),2.0)*2.0-1.0)"))||
           !Write(Float,TEXT("Transient.TitleU"),TEXT("frac(Map.Transient.TitleSystemAge*clamp(Map.Transient.TitleSpeed,0.05,3.0)*.19+float(Map.Particles.UniqueID)*.61803399)"))||
           !Write(Float,TEXT("Transient.TitleFade"),TEXT("saturate(Map.Particles.NormalizedAge*7.0)*saturate((1.0-Map.Particles.NormalizedAge)*5.0)*saturate(Map.Transient.TitleU*6.0)*saturate((1.0-Map.Transient.TitleU)*6.0)")))return nullptr;
        FString TrailPosition,TrailRotation;
        if(Kind<2)
        {
            // Open horizontal streams stay on the painted plaque shoulders;
            // their middle remains clear for title lettering.
            TrailPosition=TEXT("float3((Map.Transient.TitleU*2.0-1.0)*Map.Transient.TitleExtent.x*.84,0.0,Map.Transient.TitleSide*Map.Transient.TitleExtent.y*(.49+Map.Transient.TitleLane*.055)+sin(Map.Transient.TitleU*6.283185+Map.Transient.TitleSystemAge*.6+Map.Transient.TitleLane*.85)*Map.Transient.TitleExtent.y*.075)");
            TrailRotation=TEXT("-atan2(cos(Map.Transient.TitleU*6.283185+Map.Transient.TitleSystemAge*.6+Map.Transient.TitleLane*.85)*Map.Transient.TitleExtent.y*.471239,max(1.0,Map.Transient.TitleExtent.x*1.68))*57.29578");
        }
        else
        {
            // Four separate cubic corner wisps, with open gaps at the middle of
            // each edge. A square icon/frame never becomes a circular halo.
            if(!Write(Float,TEXT("Transient.TitleCorner"),TEXT("fmod(floor(float(Map.Particles.UniqueID)/Map.Transient.TitleLayers),4.0)"))||
               !Write(Float,TEXT("Transient.TitleCornerX"),TEXT("(fmod(Map.Transient.TitleCorner,2.0)*2.0-1.0)"))||
               !Write(Float,TEXT("Transient.TitleCornerZ"),TEXT("(floor(Map.Transient.TitleCorner/2.0)*2.0-1.0)"))||
               !Write(Float,TEXT("Transient.TitleCurveX"),TEXT("(.13*pow(1.0-Map.Transient.TitleU,3.0)+2.61*pow(1.0-Map.Transient.TitleU,2.0)*Map.Transient.TitleU+2.49*(1.0-Map.Transient.TitleU)*Map.Transient.TitleU*Map.Transient.TitleU+.78*pow(Map.Transient.TitleU,3.0))"))||
               !Write(Float,TEXT("Transient.TitleCurveZ"),TEXT("(.77*pow(1.0-Map.Transient.TitleU,3.0)+2.40*pow(1.0-Map.Transient.TitleU,2.0)*Map.Transient.TitleU+1.95*(1.0-Map.Transient.TitleU)*Map.Transient.TitleU*Map.Transient.TitleU+.10*pow(Map.Transient.TitleU,3.0))")))return nullptr;
            TrailPosition=TEXT("float3(Map.Transient.TitleCornerX*Map.Transient.TitleExtent.x*(Map.Transient.TitleCurveX-Map.Transient.TitleLane*.035),0.0,Map.Transient.TitleCornerZ*Map.Transient.TitleExtent.y*(Map.Transient.TitleCurveZ-Map.Transient.TitleLane*.035))");
            TrailRotation=TEXT("-atan2(Map.Transient.TitleCornerZ*Map.Transient.TitleExtent.y*(.09*pow(1.0-Map.Transient.TitleU,2.0)-.90*(1.0-Map.Transient.TitleU)*Map.Transient.TitleU-1.65*Map.Transient.TitleU*Map.Transient.TitleU),Map.Transient.TitleCornerX*Map.Transient.TitleExtent.x*(2.22*pow(1.0-Map.Transient.TitleU,2.0)-.24*(1.0-Map.Transient.TitleU)*Map.Transient.TitleU-.15*Map.Transient.TitleU*Map.Transient.TitleU))*57.29578");
        }
        if(!Write(FNiagaraTypeDefinition::GetPositionDef(),TEXT("Particles.Position"),TrailPosition)||
           !Write(Vec2,TEXT("Particles.SpriteSize"),TEXT("float2(min(Map.Transient.TitleSize*(8.0+Map.Transient.TitleLane*.9),max(7.0,Map.Transient.TitleExtent.x*.52)),clamp(Map.Transient.TitleSize*.32,.65,1.7))*(.72+.28*saturate(Map.Transient.TitleFade*2.0))"))||
           !Write(Color,TEXT("Particles.Color"),TEXT("float4(Map.Transient.TitleColor.rgb*(.58+.42*frac(float(Map.Particles.UniqueID)*.381966)),min(Map.Transient.TitleColor.a,.78)*Map.Transient.TitleFade*(.60-Map.Transient.TitleLane*.055))"))||
           !Write(Float,TEXT("Particles.SpriteRotation"),TrailRotation))return nullptr;
    }
    Graph->NotifyGraphChanged();if(!EnsureEmitterInvocationNodes(*System))return nullptr;
    System->SetWarmupTime(0);FAssetRegistryModule::AssetCreated(System);System->MarkPackageDirty();return CompileSystem(System)?System:nullptr;
}

UNiagaraSystem* UInkCultivationEditorLibrary::CreateSignInChestSystem(const FString& PackagePath,UMaterialInterface* Material)
{
    if(!PackagePath.StartsWith(TEXT("/Game/UI/Ink/SignIn20261022/")))return nullptr;
    auto* System=CreateTitleSystem(PackagePath,Material);if(!System)return nullptr;
    System->GetExposedParameters().SetParameterValue(FVector2f(96),FNiagaraVariable(FNiagaraTypeDefinition::GetVec2Def(),TEXT("User.TitleSize")),true);
    System->GetExposedParameters().SetParameterValue(FLinearColor(1,.72f,.25f,1),FNiagaraVariable(FNiagaraTypeDefinition::GetColorDef(),TEXT("User.TitleColor")),true);
    return CompileSystem(System)?System:nullptr;
}

UNiagaraSystem* UInkCultivationEditorLibrary::CreateInkSystem(const FString& PackagePath, UMaterialInterface* Ring, UMaterialInterface* Mist, UMaterialInterface* Flecks)
{
    if (!Ring || !Mist || !Flecks || !PackagePath.StartsWith(TEXT("/Game/1/"))) return nullptr;
    UPackage* Package = CreatePackage(*PackagePath);
    UNiagaraSystem* System = NewObject<UNiagaraSystem>(Package, *FPackageName::GetLongPackageAssetName(PackagePath), RF_Public | RF_Standalone | RF_Transactional);
    UNiagaraSystemFactoryNew::InitializeSystem(System, true);
    const TCHAR* Templates[] = {TEXT("/Niagara/DefaultAssets/Templates/Emitters/SingleLoopingParticle.SingleLoopingParticle"), TEXT("/Niagara/DefaultAssets/Templates/Emitters/Fountain.Fountain"), TEXT("/Niagara/DefaultAssets/Templates/Emitters/Fountain.Fountain")};
    const FName Names[] = {TEXT("InkRing"), TEXT("InkMist"), TEXT("InkFlecks")};
    UMaterialInterface* Materials[] = {Ring, Mist, Flecks};
    for (int32 Index = 0; Index < 3; ++Index)
    {
        UNiagaraEmitter* Template = LoadObject<UNiagaraEmitter>(nullptr, Templates[Index]);
        if (!Template) return nullptr;
        const FNiagaraEmitterHandle Handle = System->AddEmitterHandle(*Template, Names[Index], Template->GetExposedVersion().VersionGuid);
        FVersionedNiagaraEmitterData* Data = Handle.GetInstance().GetEmitterData();
        if (!Data) return nullptr;
        Data->bLocalSpace = true;
        Data->SimTarget = ENiagaraSimTarget::CPUSim;
        for (UNiagaraRendererProperties* Renderer : Data->GetRenderers())
        {
            if (UNiagaraSpriteRendererProperties* Sprite = Cast<UNiagaraSpriteRendererProperties>(Renderer))
            {
                Sprite->Material = Materials[Index];
                if (Index == 0) Sprite->FacingMode = ENiagaraSpriteFacingMode::CustomFacingVector;
            }
        }
        Handle.GetInstance().Emitter->MarkPackageDirty();
    }
    if (!EnsureEmitterInvocationNodes(*System)) return nullptr;
    System->SetWarmupTime(2.0f);
    System->SetWarmupTickDelta(1.0f/30.0f);
    FAssetRegistryModule::AssetCreated(System);
    System->MarkPackageDirty();
    return System;
}

FString UInkCultivationEditorLibrary::InspectSystem(UNiagaraSystem* System)
{
    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> Emitters;
    if (!System) return TEXT("{}");
    Root->SetBoolField(TEXT("valid"), System->IsValid());
    Root->SetBoolField(TEXT("ready_to_run"), System->IsReadyToRun());
    const auto& State = System->GetSystemStateData();
    Root->SetNumberField(TEXT("loop_behavior"), int32(State.LoopBehavior));
    Root->SetBoolField(TEXT("ignore_system_state"), State.bIgnoreSystemState);
    Root->SetBoolField(TEXT("run_update_script"), State.bRunUpdateScript);
    TArray<TSharedPtr<FJsonValue>> SystemModules;
    if (auto* Source = Cast<UNiagaraScriptSource>(System->GetSystemUpdateScript()->GetLatestSource()))
    {
        for (UEdGraphNode* Node : Source->NodeGraph->Nodes)
        {
            auto* Call = Cast<UNiagaraNodeFunctionCall>(Node);
            if (!Call) continue;
            TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
            Row->SetStringField(TEXT("name"), Call->GetFunctionName());
            TArray<TSharedPtr<FJsonValue>> Pins;
            for (UEdGraphPin* Pin : Call->Pins)
            {
                if (Pin->Direction != EGPD_Input) continue;
                TSharedRef<FJsonObject> PinRow = MakeShared<FJsonObject>();
                PinRow->SetStringField(TEXT("name"), Pin->PinName.ToString());
                PinRow->SetStringField(TEXT("value"), Pin->DefaultValue);
                if (auto* Enum = Cast<UEnum>(Pin->PinType.PinSubCategoryObject.Get()))
                {
                    TArray<TSharedPtr<FJsonValue>> Values;
                    for (int32 Index=0; Index<Enum->NumEnums(); ++Index)
                        Values.Add(MakeShared<FJsonValueString>(Enum->GetNameStringByIndex(Index)+TEXT("=")+Enum->GetDisplayNameTextByIndex(Index).ToString()));
                    PinRow->SetArrayField(TEXT("enum_values"), Values);
                }
                Pins.Add(MakeShared<FJsonValueObject>(PinRow));
            }
            Row->SetArrayField(TEXT("pins"), Pins);
            SystemModules.Add(MakeShared<FJsonValueObject>(Row));
        }
    }
    Root->SetArrayField(TEXT("system_modules"), SystemModules);
    for (const FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
    {
        const FVersionedNiagaraEmitter Instance = Handle.GetInstance();
        FVersionedNiagaraEmitterData* Data = Instance.GetEmitterData();
        TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("name"), Handle.GetName().ToString());
        Row->SetBoolField(TEXT("enabled"), Handle.GetIsEnabled());
        Row->SetBoolField(TEXT("local_space"), Data->bLocalSpace);
        TArray<TSharedPtr<FJsonValue>> Modules;
        auto InspectScript = [&](UNiagaraScript* Script)
        {
            UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(Script->GetLatestSource());
            if (!Source || !Source->NodeGraph) return;
            for (UEdGraphNode* Node : Source->NodeGraph->Nodes)
            {
                auto* Call = Cast<UNiagaraNodeFunctionCall>(Node);
                if (!Call || !Call->FunctionScript) continue;
                TSharedRef<FJsonObject> Module = MakeShared<FJsonObject>();
                Module->SetStringField(TEXT("name"), Call->GetFunctionName());
                Module->SetStringField(TEXT("asset"), Call->FunctionScript->GetPathName());
                TArray<FNiagaraVariable> Inputs;
                FCompileConstantResolver Resolver(Instance, Script->GetUsage());
                FNiagaraStackGraphUtilities::GetStackFunctionInputs(*Call, Inputs, Resolver, FNiagaraStackGraphUtilities::ENiagaraGetStackFunctionInputPinsOptions::AllInputs);
                TArray<TSharedPtr<FJsonValue>> InputRows;
                for (const auto& Input : Inputs)
                {
                    TSharedRef<FJsonObject> InputRow = MakeShared<FJsonObject>();
                    InputRow->SetStringField(TEXT("name"), Input.GetName().ToString());
                    InputRow->SetStringField(TEXT("type"), Input.GetType().GetName());
                    InputRows.Add(MakeShared<FJsonValueObject>(InputRow));
                }
                Module->SetArrayField(TEXT("inputs"), InputRows);
                TArray<TSharedPtr<FJsonValue>> Pins;
                for (UEdGraphPin* Pin : Call->Pins)
                {
                    if (Pin->Direction != EGPD_Input) continue;
                    TSharedRef<FJsonObject> PinRow = MakeShared<FJsonObject>();
                    PinRow->SetStringField(TEXT("name"), Pin->PinName.ToString());
                    PinRow->SetStringField(TEXT("value"), Pin->DefaultValue);
                    if (auto* Enum = Cast<UEnum>(Pin->PinType.PinSubCategoryObject.Get()))
                    {
                        TArray<TSharedPtr<FJsonValue>> Values;
                        for (int32 EnumIndex=0; EnumIndex<Enum->NumEnums(); ++EnumIndex)
                            Values.Add(MakeShared<FJsonValueString>(Enum->GetNameStringByIndex(EnumIndex)+TEXT("=")+Enum->GetDisplayNameTextByIndex(EnumIndex).ToString()));
                        PinRow->SetArrayField(TEXT("enum_values"), Values);
                    }
                    Pins.Add(MakeShared<FJsonValueObject>(PinRow));
                }
                Module->SetArrayField(TEXT("pins"), Pins);
                Modules.Add(MakeShared<FJsonValueObject>(Module));
            }
        };
        InspectScript(Data->SpawnScriptProps.Script);
        Row->SetArrayField(TEXT("modules"), Modules);
        Emitters.Add(MakeShared<FJsonValueObject>(Row));
    }
    Root->SetArrayField(TEXT("emitters"), Emitters);
    FString Json;
    auto Writer = TJsonWriterFactory<>::Create(&Json);
    FJsonSerializer::Serialize(Root, Writer);
    return Json;
}

bool UInkCultivationEditorLibrary::SetEmitterInput(UNiagaraSystem* System, int32 EmitterIndex, const FString& ModuleName, const FString& InputName, const FString& Value, bool bStaticSwitch)
{
    if (!System || (EmitterIndex != -1 && !System->GetEmitterHandles().IsValidIndex(EmitterIndex))) return false;
    const auto Instance = EmitterIndex == -1 ? FVersionedNiagaraEmitter() : System->GetEmitterHandles()[EmitterIndex].GetInstance();
    UNiagaraScript* Script = EmitterIndex == -1 ? System->GetSystemUpdateScript() : Instance.GetEmitterData()->SpawnScriptProps.Script.Get();
    auto* Source = Cast<UNiagaraScriptSource>(Script->GetLatestSource());
    if (!Source || !Source->NodeGraph) return false;
    for (UEdGraphNode* Node : Source->NodeGraph->Nodes)
    {
        auto* Call = Cast<UNiagaraNodeFunctionCall>(Node);
        if (!Call || Call->GetFunctionName() != ModuleName) continue;
        if (bStaticSwitch)
        {
            UEdGraphPin* Pin = Call->FindPin(*InputName, EGPD_Input);
            if (!Pin) return false;
            Call->Modify();
            Pin->DefaultValue = Value;
            Source->NodeGraph->NotifyGraphChanged();
            System->MarkPackageDirty();
            return true;
        }
        FCompileConstantResolver Resolver = EmitterIndex == -1 ? FCompileConstantResolver(System, Script->GetUsage()) : FCompileConstantResolver(Instance, Script->GetUsage());
        TArray<FNiagaraVariable> InputVariables;
        FNiagaraStackGraphUtilities::GetStackFunctionInputs(*Call, InputVariables, Resolver, FNiagaraStackGraphUtilities::ENiagaraGetStackFunctionInputPinsOptions::ModuleInputsOnly);
        const FNiagaraVariable* MatchingInput = InputVariables.FindByPredicate([&](const FNiagaraVariable& V)
        { return V.GetName().ToString() == InputName || V.GetName().ToString() == TEXT("Module.") + InputName; });
        if (!MatchingInput) return false;
        const auto Type = MatchingInput->GetType();
        FNiagaraVariable Variable(Type, *InputName);
        Variable.AllocateData();
        if (Type.GetScriptStruct())
        {
            FOutputDeviceNull Errors;
            if (!Type.GetScriptStruct()->ImportText(*Value, Variable.GetData(), nullptr, PPF_None, &Errors, TEXT("InkInput"))) return false;
        }
        else return false;
        FString PinDefault;
        const auto* Schema = GetDefault<UEdGraphSchema_Niagara>();
        if (!Schema->TryGetPinDefaultValueFromNiagaraVariable(Variable, PinDefault)) return false;
        const auto Alias = FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(FNiagaraParameterHandle(MatchingInput->GetName()), Call);
        auto& Pin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(*Call, Alias, Type, FGuid(), FGuid());
        // A copied template can contain a curve/dynamic input. Replace that input connection
        // with the explicit literal instead of trying to edit a linked rapid-iteration value.
        Pin.BreakAllPinLinks();
        Pin.DefaultValue = PinDefault;
        Source->NodeGraph->NotifyGraphChanged();
        System->MarkPackageDirty();
        return true;
    }
    return false;
}

bool UInkCultivationEditorLibrary::CompileSystem(UNiagaraSystem* System)
{
    if (!System) return false;
    if (!EnsureEmitterInvocationNodes(*System)) return false;
    System->RequestCompile(true);
    System->WaitForCompilationComplete(true, false);
    return System->IsValid();
}

bool UInkCultivationEditorLibrary::AddInkMotion(UNiagaraSystem* System)
{
    if (!System || System->GetNumEmitters()!=3) return false;
    auto& Store = System->GetExposedParameters();
    Store.SetParameterValue(140.0f, FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("User.Radius")), true);
    Store.SetParameterValue(1.0f, FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("User.Intensity")), true);
    Store.SetParameterValue(FLinearColor(0.013f,0.022f,0.019f,1.0f), FNiagaraVariable(FNiagaraTypeDefinition::GetColorDef(), TEXT("User.InkColor")), true);
    const auto* Schema = GetDefault<UEdGraphSchema_Niagara>();
    for (int32 Index=0; Index<3; ++Index)
    {
        auto Instance = System->GetEmitterHandles()[Index].GetInstance();
        auto* Data = Instance.GetEmitterData();
        auto* Source = Cast<UNiagaraScriptSource>(Data->UpdateScriptProps.Script->GetLatestSource());
        UNiagaraGraph* Graph = Source->NodeGraph;
        // Remove fountain physics and its color curve; the final stack module writes analytic motion.
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (auto* Call = Cast<UNiagaraNodeFunctionCall>(Node))
            {
                const FString Name = Call->GetFunctionName();
                if (Name==TEXT("SolveForcesAndVelocity") || Name==TEXT("GravityForce") || Name==TEXT("Drag") || Name==TEXT("ScaleColor") || Name==TEXT("AddVelocity") || Name==TEXT("ShapeLocation"))
                    FNiagaraStackGraphUtilities::SetModuleIsEnabled(*Call, false);
            }
        }
        auto* Output = Graph->FindEquivalentOutputNode(ENiagaraScriptUsage::ParticleUpdateScript);
        if (!Output) return false;
        // Explicit parameter-map reads register user parameters before Custom HLSL uses them.
        // This also gives the Niagara editor visible, typed links for all exposed controls.
        const FNiagaraTypeDefinition ReadTypes[] = {FNiagaraTypeDefinition::GetFloatDef(), FNiagaraTypeDefinition::GetFloatDef(), FNiagaraTypeDefinition::GetColorDef(), FNiagaraTypeDefinition::GetFloatDef()};
        const TCHAR* ReadNames[] = {TEXT("User.Radius"), TEXT("User.Intensity"), TEXT("User.InkColor"), TEXT("System.Age")};
        const TCHAR* LocalNames[] = {TEXT("Transient.InkRadius"), TEXT("Transient.InkIntensity"), TEXT("Transient.InkColor"), TEXT("Transient.InkSystemAge")};
        TSet<FNiagaraVariableBase> KnownParameters;
        for (int32 ReadIndex=0; ReadIndex<4; ++ReadIndex) KnownParameters.Add(FNiagaraVariableBase(ReadTypes[ReadIndex], ReadNames[ReadIndex]));
        for (int32 ReadIndex=0; ReadIndex<4; ++ReadIndex)
        {
            FNiagaraVariable Target(ReadTypes[ReadIndex], LocalNames[ReadIndex]);
            Target.AllocateData();
            FString Default;
            Schema->TryGetPinDefaultValueFromNiagaraVariable(Target, Default);
            auto* ReadAssignment = FNiagaraStackGraphUtilities::AddParameterModuleToStack({Target}, *Output, 0, {Default});
            const auto Alias = FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(FNiagaraParameterHandle(*(TEXT("Module.")+FString(LocalNames[ReadIndex]))), ReadAssignment);
            auto& ReadPin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(*ReadAssignment, Alias, ReadTypes[ReadIndex], FGuid(), FGuid());
            FNiagaraStackGraphUtilities::SetLinkedParameterValueForFunctionInput(ReadPin, FNiagaraVariableBase(ReadTypes[ReadIndex], ReadNames[ReadIndex]), KnownParameters);
        }
        auto AddExpression = [&](const FNiagaraTypeDefinition& Type, const TCHAR* Target, const FString& Code)
        {
            FNiagaraVariable Variable(Type, Target);
            Variable.AllocateData();
            FString Default;
            Schema->TryGetPinDefaultValueFromNiagaraVariable(Variable, Default);
            auto* Assignment = FNiagaraStackGraphUtilities::AddParameterModuleToStack({Variable}, *Output, INDEX_NONE, {Default});
            const auto Alias = FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(FNiagaraParameterHandle(*(TEXT("Module.")+FString(Target))), Assignment);
            UEdGraphPin& Pin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(*Assignment, Alias, Type, FGuid(), FGuid());
            UEdGraphNode* OverrideNode = Pin.GetOwningNode();
            UEdGraphPin* MapPin = nullptr;
            for (auto* Candidate : OverrideNode->Pins)
                if (Candidate->Direction==EGPD_Input && Schema->PinToTypeDefinition(Candidate).IsSameBaseDefinition(FNiagaraTypeDefinition::GetParameterMapDef())) MapPin = Candidate;
            if (!MapPin || MapPin->LinkedTo.Num()==0) return false;
            FGraphNodeCreator<UNiagaraNodeCustomHlsl> Creator(*Graph);
            auto* Expression = Creator.CreateNode();
            Expression->ScriptUsage = ENiagaraScriptUsage::DynamicInput;
            Expression->Signature.Inputs = {FNiagaraVariable(FNiagaraTypeDefinition::GetParameterMapDef(), TEXT("Map"))};
            Expression->Signature.Outputs = {FNiagaraVariable(Type, TEXT("CustomHLSLOutput"))};
            auto* Hlsl = FindFProperty<FStrProperty>(Expression->GetClass(), TEXT("CustomHlsl"));
            if (!Hlsl) return false;
            FString RegisteredCode = Code.Replace(TEXT("Map.User.Radius"), TEXT("Map.Transient.InkRadius"))
                .Replace(TEXT("Map.User.Intensity"), TEXT("Map.Transient.InkIntensity"))
                .Replace(TEXT("Map.User.InkColor"), TEXT("Map.Transient.InkColor"))
                .Replace(TEXT("Map.System.Age"), TEXT("Map.Transient.InkSystemAge"));
            RegisteredCode.ReplaceInline(TEXT("CustomHLSLOutput ="), TEXT(""));
            RegisteredCode.ReplaceInline(TEXT("CustomHLSLOutput="), TEXT(""));
            RegisteredCode.RemoveFromEnd(TEXT(";"));
            Hlsl->SetPropertyValue_InContainer(Expression, RegisteredCode);
            Creator.Finalize();
            auto* ExpressionMap = Expression->FindPin(TEXT("Map"), EGPD_Input);
            auto* ExpressionOut = Expression->FindPin(TEXT("CustomHLSLOutput"), EGPD_Output);
            if (!ExpressionMap || !ExpressionOut) return false;
            ExpressionMap->MakeLinkTo(MapPin->LinkedTo[0]);
            ExpressionOut->MakeLinkTo(&Pin);
            return true;
        };
        FString Position;
        FString Size;
        FString Color;
        if (Index==0)
        {
            Position = TEXT("CustomHLSLOutput = float3(0,0,3);");
            Size = TEXT("CustomHLSLOutput = float2(Map.User.Radius*2.0,Map.User.Radius*2.0);");
            Color = TEXT("CustomHLSLOutput = float4(Map.User.InkColor.rgb, Map.User.Intensity*(0.76+0.14*sin(Map.System.Age*1.1)));");
            if (!AddExpression(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Particles.SpriteFacing"), TEXT("CustomHLSLOutput=float3(0,0,1);"))) return false;
        }
        else
        {
            const FString Theta = FString::Printf(TEXT("(float(Map.Particles.UniqueID)*2.399963+Map.Particles.Age*%s)"), Index==1 ? TEXT("0.48") : TEXT("-0.32"));
            const FString Radius = FString::Printf(TEXT("(Map.User.Radius*(%s+0.12*sin(float(Map.Particles.UniqueID)*2.399963)))"), Index==1 ? TEXT("0.44") : TEXT("0.58"));
            Position = FString::Printf(TEXT("float3(cos(%s)*%s,sin(%s)*%s,8+Map.Particles.Age*%s)"), *Theta,*Radius,*Theta,*Radius,Index==1 ? TEXT("28.0") : TEXT("38.0"));
            Size = Index==1 ? TEXT("float2(0.8,1.2)*Map.User.Radius*(0.65+0.15*sin(float(Map.Particles.UniqueID)*1.7))") : TEXT("float2(1.0,1.3)*(3.0+3.5*frac(sin(float(Map.Particles.UniqueID)*17.1)*43758.5))");
            // VectorVM supports arithmetic and saturate, but not the HLSL smoothstep intrinsic.
            Color = TEXT("float4(Map.User.InkColor.rgb,saturate(Map.Particles.NormalizedAge*6.25)*saturate((1.0-Map.Particles.NormalizedAge)*2.857143)*Map.User.Intensity)");
        }
        if (!AddExpression(FNiagaraTypeDefinition::GetPositionDef(), TEXT("Particles.Position"), Position) ||
            !AddExpression(FNiagaraTypeDefinition::GetVec2Def(), TEXT("Particles.SpriteSize"), Size) ||
            !AddExpression(FNiagaraTypeDefinition::GetColorDef(), TEXT("Particles.Color"), Color) ||
            !AddExpression(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Particles.SpriteRotation"), Index==0 ? TEXT("CustomHLSLOutput=Map.System.Age*3.0;") : TEXT("CustomHLSLOutput=float(Map.Particles.UniqueID)*137.5+Map.Particles.Age*6.0;"))) return false;
        Graph->NotifyGraphChanged();
    }
    System->MarkPackageDirty();
    return true;
}

bool UInkCultivationEditorLibrary::ConfigureInkUserControls(UNiagaraSystem* System)
{
    if (!System || System->GetNumEmitters()!=3) return false;
    auto& Store=System->GetExposedParameters();
    const TCHAR* Names[]={TEXT("User.MistIntensity"),TEXT("User.MistSpawnRate"),TEXT("User.FleckSpawnRate"),TEXT("User.TaijiRotationSpeed")};
    const float Defaults[]={1.0f,12.0f,8.0f,12.0f};
    TSet<FNiagaraVariableBase> Known;
    for (int32 Index=0;Index<4;++Index)
    {
        FNiagaraVariable Variable(FNiagaraTypeDefinition::GetFloatDef(),Names[Index]);
        Store.SetParameterValue(Defaults[Index],Variable,true);
        Known.Add(Variable);
    }
    const auto* Schema=GetDefault<UEdGraphSchema_Niagara>();
    for (int32 Index=0;Index<3;++Index)
    {
        auto Instance=System->GetEmitterHandles()[Index].GetInstance();
        auto* Data=Instance.GetEmitterData();
        auto* Source=Cast<UNiagaraScriptSource>(Data->UpdateScriptProps.Script->GetLatestSource());
        if (!Source || !Source->NodeGraph) return false;
        UNiagaraGraph* Graph=Source->NodeGraph;
        if (Index>0)
        {
            UNiagaraNodeFunctionCall* Rate=nullptr;
            for (UEdGraphNode* Node:Graph->Nodes)
                if (auto* Call=Cast<UNiagaraNodeFunctionCall>(Node); Call && Call->GetFunctionName()==TEXT("SpawnRate")) Rate=Call;
            if (!Rate) return false;
            FCompileConstantResolver Resolver(Instance,ENiagaraScriptUsage::EmitterUpdateScript);
            TArray<FNiagaraVariable> Inputs;
            FNiagaraStackGraphUtilities::GetStackFunctionInputs(*Rate,Inputs,Resolver,FNiagaraStackGraphUtilities::ENiagaraGetStackFunctionInputPinsOptions::ModuleInputsOnly);
            const auto* Input=Inputs.FindByPredicate([](const FNiagaraVariable& V){return V.GetName()==TEXT("Module.SpawnRate");});
            if (!Input) return false;
            const auto Alias=FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(FNiagaraParameterHandle(Input->GetName()),Rate);
            auto& Pin=FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(*Rate,Alias,Input->GetType(),FGuid(),FGuid());
            Pin.BreakAllPinLinks();
            FNiagaraStackGraphUtilities::SetLinkedParameterValueForFunctionInput(Pin,FNiagaraVariableBase(FNiagaraTypeDefinition::GetFloatDef(),Names[Index==1 ? 1 : 2]),Known);
        }
        if (Index<2)
        {
            const TCHAR* Local=Index==0 ? TEXT("Transient.TaijiRotationSpeed") : TEXT("Transient.MistIntensity");
            const TCHAR* User=Index==0 ? Names[3] : Names[0];
            auto* Output=Graph->FindEquivalentOutputNode(ENiagaraScriptUsage::ParticleUpdateScript);
            if (!Output) return false;
            FNiagaraVariable Variable(FNiagaraTypeDefinition::GetFloatDef(),Local);
            Variable.AllocateData();
            FString Default;
            Schema->TryGetPinDefaultValueFromNiagaraVariable(Variable,Default);
            auto* Read=FNiagaraStackGraphUtilities::AddParameterModuleToStack({Variable},*Output,0,{Default});
            const auto Alias=FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(FNiagaraParameterHandle(*(TEXT("Module.")+FString(Local))),Read);
            auto& Pin=FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(*Read,Alias,Variable.GetType(),FGuid(),FGuid());
            FNiagaraStackGraphUtilities::SetLinkedParameterValueForFunctionInput(Pin,FNiagaraVariableBase(Variable.GetType(),User),Known);
            for (UEdGraphNode* Node:Graph->Nodes)
            {
                auto* Expression=Cast<UNiagaraNodeCustomHlsl>(Node);
                if (!Expression) continue;
                auto* Property=FindFProperty<FStrProperty>(Expression->GetClass(),TEXT("CustomHlsl"));
                FString Code=Property->GetPropertyValue_InContainer(Expression);
                if (Index==0 && Code.Contains(TEXT("InkIntensity*(0.76")))
                    Code=TEXT("float4(Map.Transient.InkColor.rgb,Map.Transient.InkIntensity*(0.24+0.20*sin(Map.Transient.InkSystemAge*0.72)))");
                else if (Index==0 && Code.Contains(TEXT("InkSystemAge*3.0")))
                    Code=TEXT("Map.Transient.InkSystemAge*Map.Transient.TaijiRotationSpeed");
                else if (Index==1 && Code.Contains(TEXT("float4(Map.Transient.InkColor.rgb")))
                    Code=TEXT("float4(Map.Transient.InkColor.rgb,saturate(Map.Particles.NormalizedAge*6.25)*saturate((1.0-Map.Particles.NormalizedAge)*2.857143)*Map.Transient.InkIntensity*max(Map.Transient.MistIntensity,0.0))");
                Expression->Modify();
                Property->SetPropertyValue_InContainer(Expression,Code);
            }
        }
        Graph->NotifyGraphChanged();
    }
    System->MarkPackageDirty();
    return true;
}

AActor* UInkCultivationEditorLibrary::CreateDecorBatch(const TArray<AStaticMeshActor*>& Sources, const FString& Label, bool bDecorationOnly)
{
    if (Sources.Num()<2 || !Sources[0]) return nullptr;
    UStaticMeshComponent* Original = Sources[0]->GetStaticMeshComponent();
    UWorld* World = Sources[0]->GetWorld();
    FActorSpawnParameters Params;
    Params.ObjectFlags = RF_Transactional;
    Params.OverrideLevel = Sources[0]->GetLevel();
    AActor* Batch = World->SpawnActor<AActor>(AActor::StaticClass(), Sources[0]->GetActorLocation(), FRotator::ZeroRotator, Params);
    if (!Batch) return nullptr;
    Batch->SetActorLabel(Label);
    Batch->SetFolderPath(TEXT("OptimizedDecor"));
    Batch->Tags.Add(TEXT("CodexOptimizedDecor"));
    USceneComponent* Root = NewObject<USceneComponent>(Batch, TEXT("DecorRoot"), RF_Transactional);
    Batch->AddInstanceComponent(Root);
    Root->SetMobility(EComponentMobility::Static);
    Batch->SetRootComponent(Root);
    Root->SetWorldLocation(Sources[0]->GetActorLocation());
    Root->RegisterComponent();
    auto* Instances = NewObject<UHierarchicalInstancedStaticMeshComponent>(Batch, TEXT("DecorInstances"), RF_Transactional);
    Batch->AddInstanceComponent(Instances);
    Instances->SetMobility(EComponentMobility::Static);
    Instances->SetupAttachment(Root);
    Instances->SetStaticMesh(Original->GetStaticMesh());
    for (int32 Slot=0; Slot<Original->GetNumMaterials(); ++Slot) Instances->SetMaterial(Slot, Original->GetMaterial(Slot));
    const TCHAR* Properties[] = { TEXT("CastShadow"), TEXT("bCastDynamicShadow"), TEXT("bCastStaticShadow"), TEXT("bCastContactShadow"), TEXT("bCastFarShadow"), TEXT("bCastInsetShadow"), TEXT("bCastHiddenShadow"), TEXT("bCastShadowAsTwoSided"), TEXT("bAffectDistanceFieldLighting"), TEXT("bAffectDynamicIndirectLighting"), TEXT("bVisibleInRayTracing"), TEXT("bRenderInMainPass"), TEXT("bRenderInDepthPass"), TEXT("bReceivesDecals"), TEXT("bRenderCustomDepth"), TEXT("CustomDepthStencilValue"), TEXT("CustomDepthStencilWriteMask"), TEXT("LightingChannels"), TEXT("TranslucencySortPriority"), TEXT("bEvaluateWorldPositionOffset"), TEXT("WorldPositionOffsetDisableDistance"), TEXT("bReverseCulling"), TEXT("BoundsScale"), TEXT("bVisible"), TEXT("bHiddenInGame") };
    for (const TCHAR* Name : Properties)
        if (FProperty* P = FindFProperty<FProperty>(UStaticMeshComponent::StaticClass(), Name))
            P->CopyCompleteValue(P->ContainerPtrToValuePtr<void>(Instances), P->ContainerPtrToValuePtr<void>(Original));
    Instances->SetCollisionProfileName(Original->GetCollisionProfileName());
    Instances->SetCollisionResponseToChannels(Original->GetCollisionResponseToChannels());
    const ECollisionEnabled::Type Collision = bDecorationOnly || Original->GetCollisionEnabled()==ECollisionEnabled::NoCollision ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly;
    Instances->SetCollisionEnabled(Collision);
    Instances->SetGenerateOverlapEvents(false);
    Instances->SetCanEverAffectNavigation(!bDecorationOnly && Original->CanEverAffectNavigation());
    Instances->RegisterComponent();
    TArray<FTransform> Transforms;
    for (auto* Source : Sources)
    {
        if (!Source || Source->GetLevel()!=Batch->GetLevel() || Source->GetStaticMeshComponent()->GetStaticMesh()!=Original->GetStaticMesh())
        { Batch->Destroy(); return nullptr; }
        Transforms.Add(Source->GetStaticMeshComponent()->GetComponentTransform());
    }
    Instances->bAutoRebuildTreeOnInstanceChanges = false;
    Instances->AddInstances(Transforms, false, true, false);
    Instances->bAutoRebuildTreeOnInstanceChanges = true;
    Instances->BuildTreeIfOutdated(false, true);
    Batch->MarkPackageDirty();
    return Batch;
}

TArray<AActor*> UInkCultivationEditorLibrary::GetProtectedActors(UWorld* World)
{
    TSet<AActor*> Protected;
    if (!World) return {};
    auto Inspect = [&](UObject* Object)
    {
        TArray<UObject*> References;
        FReferenceFinder Finder(References, nullptr, false, true, false, true);
        Finder.FindReferences(Object);
        for (UObject* Ref : References)
            if (auto* Actor = Cast<AStaticMeshActor>(Ref)) Protected.Add(Actor);
    };
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (auto* MeshActor = Cast<AStaticMeshActor>(Actor))
        {
            auto* Mesh = MeshActor->GetStaticMeshComponent();
            if (Actor->Tags.Num()>0 || Actor->GetAttachParentActor() || Mesh->IsSimulatingPhysics() || Mesh->Mobility!=EComponentMobility::Static)
                Protected.Add(Actor);
            for (const FStaticMeshComponentLODInfo& LOD : Mesh->LODData)
                if (LOD.OverrideVertexColors) Protected.Add(Actor);
            continue;
        }
        Inspect(Actor);
        TArray<UActorComponent*> Components;
        Actor->GetComponents(Components);
        for (auto* Component : Components) Inspect(Component);
        if (Actor->IsA<ALevelScriptActor>())
            for (TFieldIterator<UFunction> Fn(Actor->GetClass()); Fn; ++Fn) Inspect(*Fn);
    }
    return Protected.Array();
}

FString UInkCultivationEditorLibrary::InspectComponent(UNiagaraComponent* Component)
{
    if (!Component) return TEXT("{}");
    TSharedRef<FJsonObject> Root=MakeShared<FJsonObject>();
    Root->SetBoolField(TEXT("registered"), Component->IsRegistered());
    Root->SetBoolField(TEXT("active"), Component->IsActive());
    Root->SetBoolField(TEXT("complete"), Component->IsComplete());
    Root->SetBoolField(TEXT("world_dispatch"), FNiagaraGpuComputeDispatchInterface::Get(Component->GetWorld())!=nullptr);
    Root->SetBoolField(TEXT("world_manager"), FNiagaraWorldManager::Get(Component->GetWorld())!=nullptr);
    Root->SetBoolField(TEXT("ready"), Component->GetAsset() && Component->GetAsset()->IsReadyToRun());
    auto Controller=Component->GetSystemInstanceController();
    Root->SetBoolField(TEXT("controller"), Controller.IsValid());
    if (Controller.IsValid())
    {
        Controller->WaitForConcurrentTickAndFinalize();
        Root->SetNumberField(TEXT("requested_state"), int32(Controller->GetRequestedExecutionState()));
        Root->SetNumberField(TEXT("actual_state"), int32(Controller->GetActualExecutionState()));
        if (auto* Instance=Controller->GetSystemInstance_Unsafe())
        {
            Root->SetBoolField(TEXT("simulation"), Instance->GetSystemSimulation().IsValid());
            TArray<TSharedPtr<FJsonValue>> Counts;
            for (const auto& Emitter : Instance->GetEmitters()) Counts.Add(MakeShared<FJsonValueNumber>(Emitter->GetNumParticles()));
            Root->SetArrayField(TEXT("counts"), Counts);
            // CPU particle evidence for rise/fade/absorption; bounded to 16 particles per emitter.
            TArray<TSharedPtr<FJsonValue>> Samples;
            for(const auto& Emitter:Instance->GetEmitters())
            {
                if(!Emitter->AsStateful())continue;
                const auto& Data=Emitter->GetParticleData();
                const auto Attribute=[&](const TCHAR* Name){const FName Full(*(FString(TEXT("Particles."))+Name));return Data.HasVariable(Full)?Full:FName(Name);};
                auto Positions=FNiagaraDataSetAccessor<FNiagaraPosition>(Data,Attribute(TEXT("Position"))).GetReader(Data.GetCurrentData());
                auto PreviousPositions=FNiagaraDataSetAccessor<FNiagaraPosition>(Data,Attribute(TEXT("Previous.Position"))).GetReader(Data.GetCurrentData());
                auto Ages=FNiagaraDataSetAccessor<float>(Data,Attribute(TEXT("NormalizedAge"))).GetReader(Data.GetCurrentData());
                auto Colors=FNiagaraDataSetAccessor<FLinearColor>(Data,Attribute(TEXT("Color"))).GetReader(Data.GetCurrentData());
                auto Sizes=FNiagaraDataSetAccessor<FVector2f>(Data,Attribute(TEXT("SpriteSize"))).GetReader(Data.GetCurrentData());
                auto Rotations=FNiagaraDataSetAccessor<float>(Data,Attribute(TEXT("SpriteRotation"))).GetReader(Data.GetCurrentData());
                auto Ids=FNiagaraDataSetAccessor<int32>(Data,Attribute(TEXT("UniqueID"))).GetReader(Data.GetCurrentData());
                if(Emitter->GetNumParticles()>0)
                {
                    TArray<TSharedPtr<FJsonValue>> Variables;
                    for(const auto& Variable:Data.GetVariables())Variables.Add(MakeShared<FJsonValueString>(Variable.GetName().ToString()+TEXT(":")+Variable.GetType().GetName()));
                    Root->SetArrayField(TEXT("particle_attributes"),Variables);
                    Root->SetBoolField(TEXT("position_reader"),Positions.IsValid());Root->SetBoolField(TEXT("age_reader"),Ages.IsValid());
                    Root->SetBoolField(TEXT("previous_position_reader"),PreviousPositions.IsValid());
                    Root->SetBoolField(TEXT("color_reader"),Colors.IsValid());Root->SetBoolField(TEXT("id_reader"),Ids.IsValid());
                }
                for(int32 I=0;I<FMath::Min(16,Emitter->GetNumParticles());++I)
                {
                    if(!Positions.IsValid()||!Ages.IsValid()||!Colors.IsValid()||!Ids.IsValid())continue;
                    auto Row=MakeShared<FJsonObject>();const auto P=Positions.Get(I);
                    Row->SetNumberField(TEXT("id"),Ids.Get(I));Row->SetNumberField(TEXT("age"),Ages.Get(I));Row->SetNumberField(TEXT("alpha"),Colors.Get(I).A);
                    Row->SetArrayField(TEXT("position"),{MakeShared<FJsonValueNumber>(P.X),MakeShared<FJsonValueNumber>(P.Y),MakeShared<FJsonValueNumber>(P.Z)});
                    if(Sizes.IsValid()){const auto S=Sizes.Get(I);Row->SetArrayField(TEXT("sprite_size"),{MakeShared<FJsonValueNumber>(S.X),MakeShared<FJsonValueNumber>(S.Y)});}
                    if(Rotations.IsValid())Row->SetNumberField(TEXT("sprite_rotation"),Rotations.Get(I));
                    if(PreviousPositions.IsValid())
                    {
                        const auto Previous=PreviousPositions.Get(I);
                        Row->SetArrayField(TEXT("previous_position"),{MakeShared<FJsonValueNumber>(Previous.X),MakeShared<FJsonValueNumber>(Previous.Y),MakeShared<FJsonValueNumber>(Previous.Z)});
                        Row->SetNumberField(TEXT("displacement"),FMath::Sqrt(FMath::Square(P.X-Previous.X)+FMath::Square(P.Y-Previous.Y)+FMath::Square(P.Z-Previous.Z)));
                    }
                    Samples.Add(MakeShared<FJsonValueObject>(Row));
                }
            }
            Root->SetArrayField(TEXT("particles"),Samples);
        }
    }
    FString Json;
    auto Writer=TJsonWriterFactory<>::Create(&Json);
    FJsonSerializer::Serialize(Root,Writer);
    return Json;
}
