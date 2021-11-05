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
	float	time;
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
		SHADER_PARAMETER(float, delta_time)
		SHADER_PARAMETER(float, mass)
		SHADER_PARAMETER(float, gravity)
		SHADER_PARAMETER(float, eps)
		SHADER_PARAMETER(float, sig)
		SHADER_PARAMETER(int, numParticles)
	END_SHADER_PARAMETER_STRUCT()


	//DECLARE_SHADER_TYPE(FComputeShaderDeclaration, Global);


	//FComputeShaderDeclaration() {}

	//explicit FComputeShaderDeclaration(const ShaderMetaType::CompiledShaderInitializerType& Initializer);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return GetMaxSupportedFeatureLevel(Parameters.Platform) >= ERHIFeatureLevel::SM5;
	};

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

public:
	//LAYOUT_FIELD(FShaderResourceParameter, particles_read);
	//LAYOUT_FIELD(FShaderResourceParameter, particles_write);
	//LAYOUT_FIELD(FShaderUniformBufferParameter, global);
	
	
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

	int numBoids = 1000;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;



public:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int width = 32;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int height = 32;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int depth = 32;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float step = 100;


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float spawnRadius = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float mass = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float eps = 17.7f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float sig = 41.f;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float gravity = 6.67e-11;

	//UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<Particle> outputParticles;
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

	//FStructuredBufferRHIRef _particleBuffer;
	//FUnorderedAccessViewRHIRef _particleBufferUAV;
};
