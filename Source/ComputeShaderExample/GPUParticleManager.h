// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GPUParticleManager.generated.h"

UCLASS()
class COMPUTESHADEREXAMPLE_API AGPUParticleManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGPUParticleManager();

protected:
	/// brief Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	/// @brief Called every frame
	/// @param DeltaTime delta time between frames
	virtual void Tick(float DeltaTime) override;

};
