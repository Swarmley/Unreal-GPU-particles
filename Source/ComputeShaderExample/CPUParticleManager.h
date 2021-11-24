// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPUParticleManager.generated.h"
struct Particle {
	FVector position;
	FVector velocity;
};
struct ParticleForce {
	FVector force;
};
struct ParticleDensity {
	float density;
};

UCLASS()
class COMPUTESHADEREXAMPLE_API ACPUParticleManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACPUParticleManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
private:
	inline FIntVector GetGridCellCoord(FVector coord, FVector start, float cell_size) {
		return FIntVector(FVector(floor((coord.X - start.X) / cell_size), floor((coord.Y - start.Y) / cell_size), floor((coord.Z - start.Z) / cell_size)) );
	}
	inline bool isInGrid(FIntVector coord, FIntVector resolution) {
		return coord.X >= 0 && coord.Y >= 0 && coord.Z >= 0 && coord.X < resolution.X && coord.Y < resolution.Y && coord.Z < resolution.Z;
	}
	inline int GetCellId(FIntVector coord, FIntVector resolution) {
		return coord.X + coord.Y * resolution.X + coord.Z * resolution.X * resolution.Y;
	}
	float ComputePressure(float density);
	FVector ComputeLapVelocity(float dist, FVector velocity, FVector i_velocity, float i_density);
	float ComputeGradPressure(float dist, float pressure, float density, float i_pressure, float i_density);
	float ComputeDensity(float dist_sq, float er_sq);

	void recomputeGrid();
	void computeForces();
	void computeDensity();
	void integrate();
	void draw();
	void initIsmc();

private:
	int grid_size;
	FIntVector grid_dimensions;
	FVector minBoundary = FVector::ZeroVector;
	FVector maxBoundary = { 10,10,10 };
	float  poly6Kernel;
	float  spikyKernel;
	float  lapKernel;
	FVector gravityForce;
	float epsilon = 0.0001f;
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int numBoids = 1024;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float timeStep = 0.0003f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float mass = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector gravity = { 0.f, 0.f, -9.81f };
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float damping = -0.37f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float effective_radious = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float viscosity = 250;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float pressure_coef = 2000;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float rest_density = 1000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector	 startVelocity = { 0,0,0 };
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int	maxParticlesPerCell = 500;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float renderScale = 1.0f;

public:
	TArray<FTransform> _instanceTransforms;
	TArray<Particle> particles;
	TArray<ParticleForce> forces;
	TArray<ParticleDensity> densities;
	TArray<int> GridTracker;
	TArray<int> GridCells;
};
