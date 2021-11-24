// Fill out your copyright notice in the Description page of Project Settings.


#include "DrawPositionsComponent.h"

#include "ComputeShaderTestComponent.h"
#include "DrawBoundaryComponent.h"
// Sets default values for this component's properties
UDrawPositionsComponent::UDrawPositionsComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	// ...
}


// Called when the game starts
void UDrawPositionsComponent::BeginPlay()
{
	Super::BeginPlay();
	_initISMC();
}


// Called every frame
void UDrawPositionsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	_updateInstanceTransforms();
}

void UDrawPositionsComponent::_initISMC()
{
	UInstancedStaticMeshComponent * ismc = GetOwner()->FindComponentByClass<UInstancedStaticMeshComponent>();

	if (!ismc) return;

	ismc->SetSimulatePhysics(false);

	ismc->SetMobility(EComponentMobility::Movable);
	ismc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ismc->SetCanEverAffectNavigation(false);
	ismc->SetCollisionProfileName(TEXT("NoCollision"));
}

void UDrawPositionsComponent::_updateInstanceTransforms()
{
	UInstancedStaticMeshComponent * ismc = GetOwner()->FindComponentByClass<UInstancedStaticMeshComponent>();

	if (!ismc) return;

	UComputeShaderTestComponent * boidsComponent = GetOwner()->FindComponentByClass<UComputeShaderTestComponent>();

	if (!boidsComponent) return;

	TArray<Particle>& particles = boidsComponent->outputParticles;
	
	// resize up/down the ismc
	int toAdd = FMath::Max(0, boidsComponent->numBoids - ismc->GetInstanceCount());
	int toRemove = FMath::Max(0, ismc->GetInstanceCount() - boidsComponent->numBoids);

	for (int i = 0; i < toAdd; ++i)
		ismc->AddInstance(FTransform::Identity);
	for (int i = 0; i < toRemove; ++i)
		ismc->RemoveInstance(ismc->GetInstanceCount() - 1);

	// update the transforms
	_instanceTransforms.SetNum(boidsComponent->numBoids);

	for (int i = 0; i < boidsComponent->numBoids; ++i)
	{
		FTransform& transform = _instanceTransforms[i];
		transform.SetTranslation(particles[i].position);
		transform.SetScale3D(FVector(renderScale));
		transform.SetRotation(FQuat::Identity);
	}
	
	ismc->BatchUpdateInstancesTransforms(0, _instanceTransforms, false, true, true);
}

