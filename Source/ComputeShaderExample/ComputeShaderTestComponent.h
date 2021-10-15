// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "GlobalShader.h"
#include "UniformBuffer.h"
#include "RHICommandList.h"

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
	FVector fce;
	float	time;

};




class FComputeShaderDeclaration : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FComputeShaderDeclaration, Global);


	FComputeShaderDeclaration() {}

	explicit FComputeShaderDeclaration(const ShaderMetaType::CompiledShaderInitializerType& Initializer);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return GetMaxSupportedFeatureLevel(Parameters.Platform) >= ERHIFeatureLevel::SM5;
	};

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

public:
	LAYOUT_FIELD(FShaderResourceParameter, particles_read);
	LAYOUT_FIELD(FShaderResourceParameter, particles_write);
	LAYOUT_FIELD(FShaderUniformBufferParameter, global);
	
	
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

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;



public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int numBoids = 1000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float spawnRadius = 600.0f;

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
		
		FComputeFenceRHIRef Fence;
		
	};
	GPUBuffer buffers[2];

	//FStructuredBufferRHIRef _particleBuffer;
	//FUnorderedAccessViewRHIRef _particleBufferUAV;
};
