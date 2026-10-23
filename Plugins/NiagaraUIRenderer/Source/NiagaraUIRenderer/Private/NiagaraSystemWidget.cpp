// Copyright 2024 - Michal Smoleň

#include "NiagaraSystemWidget.h"

#include "NiagaraSystem.h"
#include "SNiagaraUISystemWidget.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraUIActor.h"
#include "NiagaraUIComponent.h"
#include "Engine/World.h"

UNiagaraSystemWidget::UNiagaraSystemWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bIsVolatile = true;
}

TSharedRef<SWidget> UNiagaraSystemWidget::RebuildWidget()
{
	NiagaraSlateWidget =
		SNew(SNiagaraUISystemWidget)
		.DesiredSize(DesiredWidgetSize)
		.ColorAndOpacity(ColorAndOpacity);

	InitializeNiagaraUI();

	return NiagaraSlateWidget.ToSharedRef();
}

void UNiagaraSystemWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (!NiagaraSlateWidget.IsValid())
		return;

	if (!IsValid(NiagaraActor) || NiagaraActor->IsActorBeingDestroyed() || !GetNiagaraComponent())
		InitializeNiagaraUI();
	
	NiagaraSlateWidget->SetDesiredSize(DesiredWidgetSize);
	NiagaraSlateWidget->SetColorAndOpacity(ColorAndOpacity);
	NiagaraSlateWidget->SetNiagaraWidgetProperties(FNiagaraWidgetProperties(&MaterialRemapList, AutoActivate, ShowDebugSystemInWorld, PassDynamicParametersFromRibbon, FakeDepthScale, FakeDepthScaleDistance));
}

void UNiagaraSystemWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	// Invalidate access before destroying the actor or releasing child Slate resources.
	TWeakObjectPtr<ANiagaraUIActor> ReleasedActor = NiagaraActor;
	NiagaraActor = nullptr;
	NiagaraComponent = nullptr;
	NiagaraSlateWidget.Reset();
	Super::ReleaseSlateResources(bReleaseChildren);
	if (auto* Actor = ReleasedActor.Get(); Actor && !Actor->IsActorBeingDestroyed())
		Actor->Destroy();
}

#if WITH_EDITOR
const FText UNiagaraSystemWidget::GetPaletteCategory()
{
	return NSLOCTEXT("NiagaraUIRenderer", "Palette Category", "Niagara");
}

void UNiagaraSystemWidget::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (PropertyChangedEvent.MemberProperty)
	{
		const FName PropertyName = PropertyChangedEvent.MemberProperty->GetFName();
		if (PropertyName == GET_MEMBER_NAME_CHECKED(UNiagaraSystemWidget, NiagaraSystemReference)
			|| PropertyName == GET_MEMBER_NAME_CHECKED(UNiagaraSystemWidget, AutoActivate)
			|| RestartSimulationOnPropertyChange
				&& (PropertyName == GET_MEMBER_NAME_CHECKED(UNiagaraSystemWidget, MaterialRemapList)
				||  PropertyName == GET_MEMBER_NAME_CHECKED(UNiagaraSystemWidget, FakeDepthScale)
				||  PropertyName == GET_MEMBER_NAME_CHECKED(UNiagaraSystemWidget, FakeDepthScaleDistance)
				||  PropertyName == GET_MEMBER_NAME_CHECKED(UNiagaraSystemWidget, PassDynamicParametersFromRibbon)
				||  PropertyName == GET_MEMBER_NAME_CHECKED(UNiagaraSystemWidget, DesiredWidgetSize)
				||  PropertyName == GET_MEMBER_NAME_CHECKED(UNiagaraSystemWidget, ColorAndOpacity))
			)
		{
			InitializeNiagaraUI();
		}
	}
}
#endif

void UNiagaraSystemWidget::InitializeNiagaraUI()
{
	if (!NiagaraSlateWidget.IsValid())
		return;
	if (UWorld* World = GetWorld())
	{
		if(World->bIsTearingDown)
			return;
		
		if (!World->PersistentLevel)
			return;

			
		if (!IsValid(NiagaraActor) || NiagaraActor->IsActorBeingDestroyed())
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnParams.ObjectFlags |= RF_Transient;
			
			NiagaraActor = World->SpawnActor<ANiagaraUIActor>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		}
		if (!IsValid(NiagaraActor))
			return;

		NiagaraComponent = NiagaraActor->SpawnNewNiagaraUIComponent(NiagaraSystemReference, AutoActivate, ShowDebugSystemInWorld, TickWhenPaused);

		NiagaraSlateWidget->SetNiagaraComponentReference(NiagaraComponent);
	}
}

void UNiagaraSystemWidget::ActivateSystem(bool Reset)
{
	if (auto* Component = GetNiagaraComponent())
		Component->RequestActivateSystem(Reset);
}

void UNiagaraSystemWidget::DeactivateSystem()
{
	if (auto* Component = GetNiagaraComponent())
		Component->RequestDeactivateSystem();
}

UNiagaraUIComponent* UNiagaraSystemWidget::GetNiagaraComponent()
{
	auto* Component = NiagaraComponent.Get();
	return IsValid(Component) && !Component->IsUnreachable() && !Component->IsBeingDestroyed()
		&& !Component->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed) ? Component : nullptr;
}

void UNiagaraSystemWidget::UpdateNiagaraSystemReference(UNiagaraSystem* NewNiagaraSystem)
{
	NiagaraSystemReference = NewNiagaraSystem;

	if (auto* Component = GetNiagaraComponent())
	{
		Component->SetAsset(NewNiagaraSystem);
		Component->ResetSystem();
	}
}

void UNiagaraSystemWidget::UpdateTickWhenPaused(bool NewTickWhenPaused)
{
	TickWhenPaused = NewTickWhenPaused;

	if (auto* Component = GetNiagaraComponent())
	{
		Component->SetTickableWhenPaused(NewTickWhenPaused);
		Component->SetForceSolo(NewTickWhenPaused);
		Component->ResetSystem();
	}
}

void UNiagaraSystemWidget::SetDesiredWidgetSize(FVector2D NewDesiredSize)
{
	DesiredWidgetSize = NewDesiredSize;

	if (NiagaraSlateWidget.IsValid())
		NiagaraSlateWidget->SetDesiredSize(DesiredWidgetSize);
}

const FVector2D& UNiagaraSystemWidget::GetDesiredWidgetSize() const
{
	return DesiredWidgetSize;
}

void UNiagaraSystemWidget::SetRemapMaterial(UMaterialInterface* OriginalMaterial, UMaterialInterface* RemapMaterial)
{
	if (OriginalMaterial && RemapMaterial)
		MaterialRemapList.Emplace(OriginalMaterial, RemapMaterial);
}

UMaterialInterface* UNiagaraSystemWidget::GetRemapMaterial(UMaterialInterface* OriginalMaterial)
{
	if (TObjectPtr<UMaterialInterface>* FoundMaterial = MaterialRemapList.Find(OriginalMaterial))
		return *FoundMaterial;

	return nullptr;
}

void UNiagaraSystemWidget::SetColorAndOpacity(FLinearColor InColorAndOpacity)
{
	ColorAndOpacity = InColorAndOpacity;
	
	if (NiagaraSlateWidget.IsValid())
		NiagaraSlateWidget->SetColorAndOpacity(ColorAndOpacity);
}

const FLinearColor& UNiagaraSystemWidget::GetColorAndOpacity() const
{
	return ColorAndOpacity;
}
