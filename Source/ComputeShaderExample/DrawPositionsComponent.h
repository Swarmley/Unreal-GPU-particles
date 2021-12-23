// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DrawPositionsComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class COMPUTESHADEREXAMPLE_API UDrawPositionsComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UDrawPositionsComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	/// @brief Called every frame
	/// @param DeltaTime delta time between frames
	/// @param TickType Type of tick we wish to perform on a level
	/// @param ThisTickFunction  a funcion we wish to perform in a tick
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float renderScale = 1.0f;

protected:
	/// @brief initiates UInstancedStaticMeshComponent
	void _initISMC();
	/// @brief copies particle positions into UInstancedStaticMeshComponent
	void _updateInstanceTransforms();

protected:
	UMaterial* Mat;
	UMaterialInstanceDynamic* Material;
	TArray<FTransform> _instanceTransforms;
};
