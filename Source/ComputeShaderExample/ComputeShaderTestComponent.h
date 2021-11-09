// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "GlobalShader.h"
#include "UniformBuffer.h"
#include "RHICommandList.h"
#include "ShaderParameterStruct.h"
#include "RenderGraphUtils.h"

#include <atomic>

#include "ComputeShaderTestComponent.generated.h"


//BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FBoidPosition, )
//SHADER_PARAMETER(FVector, position)
//END_GLOBAL_SHADER_PARAMETER_STRUCT()
//
//BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FBoidVelocity, )
//SHADER_PARAMETER(FVector, velocity)
//END_GLOBAL_SHADER_PARAMETER_STRUCT()

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

class FComputeShaderDeclaration : public FGlobalShader
{
public:
	//Declare this class as a global shader
	DECLARE_GLOBAL_SHADER(FComputeShaderDeclaration);
	//Tells the engine that this shader uses a structure for its parameters
	SHADER_USE_PARAMETER_STRUCT(FComputeShaderDeclaration, FGlobalShader);
	/// <summary>
	/// DECLARATION OF THE PARAMETER STRUCTURE
	/// The parameters must match the parameters in the HLSL code
	/// For each parameter, provide the C++ type, and the name (Same name used in HLSL code)
	/// </summary>
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_UAV(RWTexture2D<Particle>, particles_read)
		SHADER_PARAMETER_UAV(RWTexture2D<Particle>, particles_write)

		SHADER_PARAMETER_UAV(RWTexture2D<ParticleForce>, particlesForce_read)
		SHADER_PARAMETER_UAV(RWTexture2D<ParticleForce>, particlesForce_write)

		SHADER_PARAMETER_UAV(RWTexture2D<ParticleDensity>, particlesDensity_read)
		SHADER_PARAMETER_UAV(RWTexture2D<ParticleDensity>, particlesDensity_write)

		SHADER_PARAMETER(float, delta_time)
		SHADER_PARAMETER(float, mass)
		SHADER_PARAMETER(float, gravity)
		SHADER_PARAMETER(float, eps)
		SHADER_PARAMETER(float, sig)
		SHADER_PARAMETER(int, numParticles)
		SHADER_PARAMETER(FVector, minBoundary)
		SHADER_PARAMETER(FVector, maxBoundary)
		SHADER_PARAMETER(float, damping)
		SHADER_PARAMETER(float, epsilon)
		SHADER_PARAMETER(float, radious)
		SHADER_PARAMETER(float, poly6Kernel)
		SHADER_PARAMETER(float, spikyKernel)
		SHADER_PARAMETER(float, lapKernel)
		SHADER_PARAMETER(float, pressureCoef)
		SHADER_PARAMETER(float, restDensity)
		SHADER_PARAMETER(float, viscosity)

	END_SHADER_PARAMETER_STRUCT()
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return GetMaxSupportedFeatureLevel(Parameters.Platform) >= ERHIFeatureLevel::SM5;
	};

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

public:
};
class FComputeDensityDeclaration : public FGlobalShader
{
public:
	//Declare this class as a global shader
	DECLARE_GLOBAL_SHADER(FComputeDensityDeclaration);
	//Tells the engine that this shader uses a structure for its parameters
	SHADER_USE_PARAMETER_STRUCT(FComputeDensityDeclaration, FGlobalShader);
	/// <summary>
	/// DECLARATION OF THE PARAMETER STRUCTURE
	/// The parameters must match the parameters in the HLSL code
	/// For each parameter, provide the C++ type, and the name (Same name used in HLSL code)
	/// </summary>
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_UAV(RWTexture2D<Particle>, particles_read)
		SHADER_PARAMETER_UAV(RWTexture2D<Particle>, particles_write)

		SHADER_PARAMETER_UAV(RWTexture2D<ParticleForce>, particlesForce_read)
		SHADER_PARAMETER_UAV(RWTexture2D<ParticleForce>, particlesForce_write)

		SHADER_PARAMETER_UAV(RWTexture2D<ParticleDensity>, particlesDensity_read)
		SHADER_PARAMETER_UAV(RWTexture2D<ParticleDensity>, particlesDensity_write)

		SHADER_PARAMETER(float, delta_time)
		SHADER_PARAMETER(float, mass)
		SHADER_PARAMETER(float, gravity)
		SHADER_PARAMETER(float, eps)
		SHADER_PARAMETER(float, sig)
		SHADER_PARAMETER(int, numParticles)
		SHADER_PARAMETER(FVector, minBoundary)
		SHADER_PARAMETER(FVector, maxBoundary)
		SHADER_PARAMETER(float, damping)
		SHADER_PARAMETER(float, epsilon)
		SHADER_PARAMETER(float, radious)
		SHADER_PARAMETER(float, poly6Kernel)
		SHADER_PARAMETER(float, spikyKernel)
		SHADER_PARAMETER(float, lapKernel)
		SHADER_PARAMETER(float, pressureCoef)
		SHADER_PARAMETER(float, restDensity)
		SHADER_PARAMETER(float, viscosity)

		END_SHADER_PARAMETER_STRUCT()
		static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return GetMaxSupportedFeatureLevel(Parameters.Platform) >= ERHIFeatureLevel::SM5;
	};

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

public:
};
class FComputeForceDeclaration : public FGlobalShader
{
public:
	//Declare this class as a global shader
	DECLARE_GLOBAL_SHADER(FComputeForceDeclaration);
	//Tells the engine that this shader uses a structure for its parameters
	SHADER_USE_PARAMETER_STRUCT(FComputeForceDeclaration, FGlobalShader);
	/// <summary>
	/// DECLARATION OF THE PARAMETER STRUCTURE
	/// The parameters must match the parameters in the HLSL code
	/// For each parameter, provide the C++ type, and the name (Same name used in HLSL code)
	/// </summary>
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_UAV(RWTexture2D<Particle>, particles_read)
		SHADER_PARAMETER_UAV(RWTexture2D<Particle>, particles_write)

		SHADER_PARAMETER_UAV(RWTexture2D<ParticleForce>, particlesForce_read)
		SHADER_PARAMETER_UAV(RWTexture2D<ParticleForce>, particlesForce_write)

		SHADER_PARAMETER_UAV(RWTexture2D<ParticleDensity>, particlesDensity_read)
		SHADER_PARAMETER_UAV(RWTexture2D<ParticleDensity>, particlesDensity_write)

		SHADER_PARAMETER(float, delta_time)
		SHADER_PARAMETER(float, mass)
		SHADER_PARAMETER(float, gravity)
		SHADER_PARAMETER(float, eps)
		SHADER_PARAMETER(float, sig)
		SHADER_PARAMETER(int, numParticles)
		SHADER_PARAMETER(FVector, minBoundary)
		SHADER_PARAMETER(FVector, maxBoundary)
		SHADER_PARAMETER(float, damping)
		SHADER_PARAMETER(float, epsilon)
		SHADER_PARAMETER(float, radious)
		SHADER_PARAMETER(float, poly6Kernel)
		SHADER_PARAMETER(float, spikyKernel)
		SHADER_PARAMETER(float, lapKernel)
		SHADER_PARAMETER(float, pressureCoef)
		SHADER_PARAMETER(float, restDensity)
		SHADER_PARAMETER(float, viscosity)
		END_SHADER_PARAMETER_STRUCT()
		static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return GetMaxSupportedFeatureLevel(Parameters.Platform) >= ERHIFeatureLevel::SM5;
	};

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

public:
};





UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class COMPUTESHADEREXAMPLE_API UComputeShaderTestComponent : public UActorComponent
{
	GENERATED_BODY()


public:
	// Sets default values for this component's properties
	UComputeShaderTestComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	//int numBoids = 1000;
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boundary")
		FVector minBoundary = FVector::ZeroVector;
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boundary")
		FVector maxBoundary = { 10,10,10 };

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;



public:
	
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="placement")
	//	int width = 32;
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "placement")
	//	int height = 32;
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "placement")
	//	int depth = 32;
	//
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int numBoids = 1000;
	

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float timeStep = 0.0013f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float mass = 0.0002;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float eps = 17.7f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float sig = 41.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float gravity = -9.81f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float damping = -0.37f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float effective_radious = 90.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float viscosity = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float pressure_coef = 200.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float rest_density = 1000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector	 startVelocity = {0,0,0};

	//UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<Particle> outputParticles;
	TArray<ParticleForce> outputForces;
	TArray<ParticleDensity> outputDensities;

protected:

	enum {
		Read = 0,
		Write = 1
	};
	struct GPUBuffer {
		FStructuredBufferRHIRef Buffer;
		FUnorderedAccessViewRHIRef BufferUAV;
	};
	GPUBuffer buffers[2];
	GPUBuffer force_buffers[2];
	GPUBuffer density_buffers[2];

	//FStructuredBufferRHIRef _particleBuffer;
	//FUnorderedAccessViewRHIRef _particleBufferUAV;
};
