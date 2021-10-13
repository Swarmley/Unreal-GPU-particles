// Fill out your copyright notice in the Description page of Project Settings.


#include "ComputeShaderTestComponent.h"

#if ENGINE_MINOR_VERSION < 26

#include "ShaderParameterUtils.h"

#else

#include "ShaderCompilerCore.h"

#endif

#include "RHIStaticStates.h"

// Some useful links
// -----------------
// [Enqueue render commands using lambdas](https://github.com/EpicGames/UnrealEngine/commit/41f6b93892dcf626a5acc155f7d71c756a5624b0)
//



// Sets default values for this component's properties
UComputeShaderTestComponent::UComputeShaderTestComponent() 
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UComputeShaderTestComponent::BeginPlay()
{
	Super::BeginPlay();

    FRHICommandListImmediate& RHICommands = GRHICommandList.GetImmediateCommandList();

	FRandomStream rng;
	int k = 0;
	int rows = 256;
	int cols = 256;
	numBoids = cols * rows;

	{
		TResourceArray<Particle> particleResourceArray;
		{
			Particle p;
			p.position = FVector::ZeroVector;
			p.time = 0.0f;
			particleResourceArray.Init(p, numBoids);
		}

		//float time = rng.GetFraction();
		//for (int32 i = 0; i < rows; i++) {
		//	float time = rng.GetFraction();
		//	
		//	for (int32 j = 0; j < cols; j++) {
		//		time += 0.16f;
		//		particleResourceArray[k].position = FVector(i*10, j*10, 0);
		//		particleResourceArray[k].time = time;
		//		k++;
		//	}
		//}

		for (Particle& p : particleResourceArray)
		{
			p.position = rng.GetUnitVector() * rng.GetFraction() * spawnRadius;
			p.time = rng.GetFraction();
		}

		FRHIResourceCreateInfo createInfo;
		createInfo.ResourceArray = &particleResourceArray;

		_particleBuffer = RHICreateStructuredBuffer(sizeof(Particle), sizeof(Particle) * numBoids, BUF_UnorderedAccess | BUF_ShaderResource, createInfo);
		_particleBufferUAV = RHICreateUnorderedAccessView(_particleBuffer, false, false);
	}

	if (outputParticles.Num() != numBoids)
	{
		const FVector zero(0.0f);
		Particle p;
		p.position = FVector::ZeroVector;
		p.time = 0.0f;
		outputParticles.Init(p, numBoids);
	}
}

// Called every frame
void UComputeShaderTestComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ENQUEUE_RENDER_COMMAND(FComputeShaderRunner)(
	[&](FRHICommandListImmediate& RHICommands)
	{
		TShaderMapRef<FComputeShaderDeclaration> cs(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
		FRHIComputeShader* rhiComputeShader = cs.GetComputeShader();

		RHICommands.SetUAVParameter(rhiComputeShader, cs->particles.GetBaseIndex(), _particleBufferUAV);

		RHICommands.SetComputeShader(rhiComputeShader);
		DispatchComputeShader(RHICommands, cs, 256, 1, 1);

		// read back the data
		uint8 * particledata = (uint8*)RHILockStructuredBuffer(_particleBuffer, 0, numBoids * sizeof(Particle), RLM_ReadOnly);	
		FMemory::Memcpy(outputParticles.GetData(), particledata, numBoids * sizeof(Particle));

		RHIUnlockStructuredBuffer(_particleBuffer);
	});
}

FComputeShaderDeclaration::FComputeShaderDeclaration(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer)
{
	particles.Bind(Initializer.ParameterMap, TEXT("particles"));
}

void FComputeShaderDeclaration::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
}

IMPLEMENT_SHADER_TYPE(, FComputeShaderDeclaration, TEXT("/ComputeShaderPlugin/Boid.usf"), TEXT("MainComputeShader"), SF_Compute);
