// Fill out your copyright notice in the Description page of Project Settings.


#include "ComputeShaderTestComponent.h"


#include "ShaderCompilerCore.h"

#include "RHIStaticStates.h"


#define NUM_THREADS_PER_GROUP_DIMENSION 256

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
	{
		TResourceArray<Particle> particleResourceArray_read;
		TResourceArray<Particle> particleResourceArray_write;


		{
			Particle p;
			p.position = FVector::ZeroVector;
			p.time = 0.0f;
			p.fce = FVector::ZeroVector;
			particleResourceArray_read.Init(p, numBoids);
			particleResourceArray_write.Init(p, numBoids);
		}

		for (Particle& p : particleResourceArray_read)
		{
			p.position = rng.GetUnitVector() * rng.GetFraction() * spawnRadius;
			p.time = rng.GetFraction();
		}
		particleResourceArray_write = particleResourceArray_read;

		FRHIResourceCreateInfo createInfo_read;
		createInfo_read.ResourceArray = &particleResourceArray_read;
		FRHIResourceCreateInfo createInfo_write;
		createInfo_write.ResourceArray = &particleResourceArray_write;



		buffers[Read].Buffer = RHICreateStructuredBuffer(sizeof(Particle), sizeof(Particle) * numBoids, BUF_UnorderedAccess | BUF_ShaderResource, createInfo_read);
		buffers[Read].BufferUAV = RHICreateUnorderedAccessView(buffers[Read].Buffer, false, false);

		buffers[Write].Buffer = RHICreateStructuredBuffer(sizeof(Particle), sizeof(Particle) * numBoids, BUF_UnorderedAccess | BUF_ShaderResource, createInfo_write);
		buffers[Write].BufferUAV = RHICreateUnorderedAccessView(buffers[Write].Buffer, false, false);

		//_particleBuffer = RHICreateStructuredBuffer(sizeof(Particle), sizeof(Particle) * numBoids, BUF_UnorderedAccess | BUF_ShaderResource, createInfo);
		//_particleBufferUAV = RHICreateUnorderedAccessView(_particleBuffer, false, false);
	}

	if (outputParticles.Num() != numBoids)
	{
		const FVector zero(0.0f);
		Particle p;
		p.position = FVector::ZeroVector;
		p.time = 0.0f;
		p.fce = FVector::ZeroVector;
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
		
		uint8* particledata = (uint8*)RHILockStructuredBuffer(buffers[Read].Buffer, 0, numBoids * sizeof(Particle), RLM_ReadOnly);
		FMemory::Memcpy(outputParticles.GetData(), particledata, numBoids * sizeof(Particle));
		RHIUnlockStructuredBuffer(buffers[Read].Buffer);
		
		RHICommands.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EGfxToCompute, buffers[Write].BufferUAV);
		TShaderMapRef<FComputeShaderDeclaration> cs(GetGlobalShaderMap(ERHIFeatureLevel::SM5));

		FRHIComputeShader* rhiComputeShader = cs.GetComputeShader();
		
		int i = cs->global.GetBaseIndex();

		RHICommands.SetUAVParameter(rhiComputeShader, cs->particles_write.GetBaseIndex(), buffers[Write].BufferUAV);
		RHICommands.SetUAVParameter(rhiComputeShader, cs->particles_read.GetBaseIndex(), buffers[Read].BufferUAV);
		RHICommands.SetComputeShader(rhiComputeShader);
		
		DispatchComputeShader(RHICommands, cs, FMath::DivideAndRoundUp(numBoids, NUM_THREADS_PER_GROUP_DIMENSION), 1, 1);
		std::swap(buffers[Read], buffers[Write]);
		// read back the data

	}); 
	
	
}

FComputeShaderDeclaration::FComputeShaderDeclaration(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer)
{
	particles_read.Bind(Initializer.ParameterMap, TEXT("particles_read"));
	particles_write.Bind(Initializer.ParameterMap, TEXT("particles_write"));
	global.Bind(Initializer.ParameterMap, TEXT("Global"));


}

void FComputeShaderDeclaration::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
	OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), NUM_THREADS_PER_GROUP_DIMENSION);
	OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), 1);
	OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), 1);


}

IMPLEMENT_SHADER_TYPE(, FComputeShaderDeclaration, TEXT("/ComputeShaderPlugin/Boid.usf"), TEXT("MainComputeShader"), SF_Compute);
