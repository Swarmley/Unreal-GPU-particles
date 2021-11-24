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
		SHADER_PARAMETER_UAV(RWStructuredBuffer<Particle>, particles_read)
		SHADER_PARAMETER_UAV(RWStructuredBuffer<Particle>, particles_write)

		SHADER_PARAMETER_UAV(RWStructuredBuffer<ParticleForce>, particlesForce_read)
		SHADER_PARAMETER_UAV(RWStructuredBuffer<ParticleDensity>, particlesDensity_read)

		SHADER_PARAMETER(float, mass)
		SHADER_PARAMETER(float, delta_time)
		SHADER_PARAMETER(int, numParticles)
		SHADER_PARAMETER(FVector, minBoundary)
		SHADER_PARAMETER(FVector, maxBoundary)
		SHADER_PARAMETER(float, damping)
		SHADER_PARAMETER(float, epsilon)
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
		SHADER_PARAMETER_UAV(RWStructuredBuffer<Particle>, particles_read)
		SHADER_PARAMETER_UAV(RWStructuredBuffer<ParticleDensity>, particlesDensity_write)

		SHADER_PARAMETER_UAV(RWStructuredBuffer<int>, grid)
		SHADER_PARAMETER_UAV(RWStructuredBuffer<int>, grid_cells)


		SHADER_PARAMETER(float, mass)
		SHADER_PARAMETER(int, numParticles)
		SHADER_PARAMETER(float, epsilon)
		SHADER_PARAMETER(float, radious)
		SHADER_PARAMETER(float, poly6Kernel)
		SHADER_PARAMETER(uint32, maxParticlesPerCell)
		SHADER_PARAMETER(FIntVector, gridDimensions)
		SHADER_PARAMETER(FVector, minBoundary)

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
		SHADER_PARAMETER_UAV(RWStructuredBuffer<Particle>, particles_read)
		SHADER_PARAMETER_UAV(RWStructuredBuffer<ParticleForce>, particlesForce_write)
		SHADER_PARAMETER_UAV(RWStructuredBuffer<ParticleDensity>, particlesDensity_read)


		SHADER_PARAMETER_UAV(RWStructuredBuffer<int>, grid)
		SHADER_PARAMETER_UAV(RWStructuredBuffer<int>, grid_cells)

		SHADER_PARAMETER(float, mass)
		SHADER_PARAMETER(FVector, gravity)
		SHADER_PARAMETER(int, numParticles)
		SHADER_PARAMETER(float, epsilon)
		SHADER_PARAMETER(float, radious)
		SHADER_PARAMETER(float, spikyKernel)
		SHADER_PARAMETER(float, lapKernel)
		SHADER_PARAMETER(float, pressureCoef)
		SHADER_PARAMETER(float, restDensity)
		SHADER_PARAMETER(float, viscosity)
		SHADER_PARAMETER(uint32, maxParticlesPerCell)
		SHADER_PARAMETER(FIntVector, gridDimensions)
		SHADER_PARAMETER(FVector, minBoundary)
		END_SHADER_PARAMETER_STRUCT()



		static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return GetMaxSupportedFeatureLevel(Parameters.Platform) >= ERHIFeatureLevel::SM5;
	};

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

public:
};
class FComputeDtDeclaration : public FGlobalShader
{
public:
	//Declare this class as a global shader
	DECLARE_GLOBAL_SHADER(FComputeDtDeclaration);
	//Tells the engine that this shader uses a structure for its parameters
	SHADER_USE_PARAMETER_STRUCT(FComputeDtDeclaration, FGlobalShader);
	/// <summary>
	/// DECLARATION OF THE PARAMETER STRUCTURE
	/// The parameters must match the parameters in the HLSL code
	/// For each parameter, provide the C++ type, and the name (Same name used in HLSL code)
	/// </summary>
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_UAV(RWStructuredBuffer<Particle>, particles_read)
		SHADER_PARAMETER_UAV(RWStructuredBuffer<ParticleForce>, particlesForce_read)
		SHADER_PARAMETER_UAV(RWStructuredBuffer<ParticleDensity>, particlesDensity_read)
		SHADER_PARAMETER_UAV(RWStructuredBuffer<FVector>, cvf_max)
		SHADER_PARAMETER_UAV(RWStructuredBuffer<uint32>, mutex)

		SHADER_PARAMETER(int, numParticles)
		SHADER_PARAMETER(float, pressureCoef)
		SHADER_PARAMETER(float, restDensity)

		END_SHADER_PARAMETER_STRUCT()
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return GetMaxSupportedFeatureLevel(Parameters.Platform) >= ERHIFeatureLevel::SM5;
	};

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

public:
};
class FComputeCreateGridDeclaration : public FGlobalShader
{
public:
	//Declare this class as a global shader
	DECLARE_GLOBAL_SHADER(FComputeCreateGridDeclaration);
	//Tells the engine that this shader uses a structure for its parameters
	SHADER_USE_PARAMETER_STRUCT(FComputeCreateGridDeclaration, FGlobalShader);
	/// <summary>
	/// DECLARATION OF THE PARAMETER STRUCTURE
	/// The parameters must match the parameters in the HLSL code
	/// For each parameter, provide the C++ type, and the name (Same name used in HLSL code)
	/// </summary>
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_UAV(RWStructuredBuffer<Particle>, particles_read)
		SHADER_PARAMETER_UAV(RWStructuredBuffer<Particle>, particles_write)

		SHADER_PARAMETER_UAV(RWStructuredBuffer<int>, grid)
		SHADER_PARAMETER_UAV(RWStructuredBuffer<int>, grid_cells)




		SHADER_PARAMETER(float, radious)
		SHADER_PARAMETER(int, numParticles)
		SHADER_PARAMETER(FVector, minBoundary)
		SHADER_PARAMETER(FVector, maxBoundary)
		SHADER_PARAMETER(uint32, maxParticlesPerCell)
		SHADER_PARAMETER(uint32, grid_size)
		SHADER_PARAMETER(FIntVector, gridDimensions)
		END_SHADER_PARAMETER_STRUCT()

		static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return GetMaxSupportedFeatureLevel(Parameters.Platform) >= ERHIFeatureLevel::SM5;
	};

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

public:
};
class FComputeClearGridDeclaration : public FGlobalShader
{
public:
	//Declare this class as a global shader
	DECLARE_GLOBAL_SHADER(FComputeClearGridDeclaration);
	//Tells the engine that this shader uses a structure for its parameters
	SHADER_USE_PARAMETER_STRUCT(FComputeClearGridDeclaration, FGlobalShader);
	/// <summary>
	/// DECLARATION OF THE PARAMETER STRUCTURE
	/// The parameters must match the parameters in the HLSL code
	/// For each parameter, provide the C++ type, and the name (Same name used in HLSL code)
	/// </summary>
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_UAV(RWStructuredBuffer<Particle>, particles_read)
		SHADER_PARAMETER_UAV(RWStructuredBuffer<Particle>, particles_write)

		SHADER_PARAMETER_UAV(RWStructuredBuffer<int>, grid)
		SHADER_PARAMETER_UAV(RWStructuredBuffer<int>, grid_cells)



		SHADER_PARAMETER(float, radious)
		SHADER_PARAMETER(int, numParticles)
		SHADER_PARAMETER(FVector, minBoundary)
		SHADER_PARAMETER(FVector, maxBoundary)
		SHADER_PARAMETER(uint32, maxParticlesPerCell)
		SHADER_PARAMETER(uint32, grid_size)
		SHADER_PARAMETER(FIntVector, gridDimensions)

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
		int maxBoids = 1000;


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float timeStep = 0.0013f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float mass = 0.0002;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector gravity = { 0.f, 0.f, -9.81f };
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		 int	maxParticlesPerCell = 500;

	//UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<Particle> outputParticles;
	TArray<ParticleForce> outputForces;
	TArray<ParticleDensity> outputDensities;
	int grid_size;
	FIntVector grid_dimensions;
	TArray<int> debugGrid;
	TArray<int> debugGridCells;

protected:

	enum {
		Read = 0,
		Write = 1
	};
	struct GPUBuffer {
		FStructuredBufferRHIRef Buffer;
		FUnorderedAccessViewRHIRef BufferUAV;
	};
	//GPUBuffer buffers[2];
	//GPUBuffer force_buffers[2];
	//GPUBuffer density_buffers[2];
	//GPUBuffer dt_buffers[2];
	//GPUBuffer grid_buffers[2];
	//GPUBuffer grid_cells_buffers[2];

	struct Frame {
		GPUBuffer paricle_buffers;
		GPUBuffer force_buffers;
		GPUBuffer density_buffers;
		GPUBuffer dt_buffers;
		GPUBuffer grid_buffers;
		GPUBuffer grid_cells_buffers;
	};
	Frame frames[2];
	int current_frame = 0;

	GPUBuffer cvf_buffer;
	GPUBuffer mutex_buffer;


	//FStructuredBufferRHIRef _particleBuffer;
	//FUnorderedAccessViewRHIRef _particleBufferUAV;
};
