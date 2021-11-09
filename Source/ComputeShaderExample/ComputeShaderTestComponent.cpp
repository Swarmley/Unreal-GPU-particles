// Fill out your copyright notice in the Description page of Project Settings.


#include "ComputeShaderTestComponent.h"
#include "ShaderCompilerCore.h"
#include "RHIStaticStates.h"
#include "DrawBoundaryComponent.h"
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

	auto box = GetOwner()->FindComponentByClass<UDrawBoundaryComponent>()->Bounds.GetBox();
	auto loc = GetOwner()->GetActorLocation();

	minBoundary = box.Min - loc;
	maxBoundary = box.Max - loc;

	FRandomStream rng;
	{
		TResourceArray<Particle> particleResourceArray_read;
		TResourceArray<Particle> particleResourceArray_write;

		TResourceArray<ParticleForce> particleForceResourceArray_read;
		TResourceArray<ParticleForce> particleForceResourceArray_write;

		TResourceArray<ParticleDensity> particleDensityResourceArray_read;
		TResourceArray<ParticleDensity> particleDensityResourceArray_write;

		//numBoids = width * height * depth;
		int width, height, depth;
		width = maxBoundary.X - minBoundary.X;
		height = maxBoundary.Y - minBoundary.Y;
		depth = maxBoundary.Z - minBoundary.Z;

		{
			Particle p;
			ParticleDensity d;
			ParticleForce f;
			p.position = FVector::ZeroVector;
			p.velocity = FVector::ZeroVector;
			particleResourceArray_read.Init(p, numBoids);
			particleResourceArray_write.Init(p, numBoids);
			particleForceResourceArray_read.Init(f, numBoids);
			particleForceResourceArray_write.Init(f, numBoids);
			particleDensityResourceArray_read.Init(d, numBoids);
			particleDensityResourceArray_write.Init(d, numBoids);
		}
		int counter = 0;
		for (int z = 0; z < depth && counter < numBoids; z++) {
			for (int y = 0; y < height && counter < numBoids; y++) {
				for (int x = 0; x < width && counter < numBoids; x++) {
					FVector position =  FVector(maxBoundary.X - 1, maxBoundary.Y - 1, maxBoundary.Z - 1) - FVector(x / 2.f, y / 2.f, z / 2.f) - FVector(rng.RandRange(0.f, 0.01f), rng.RandRange(0.f, 0.01f), rng.RandRange(0.f, 0.01f));
					int idx = z * width * height + y * width + x;
					particleResourceArray_read[idx].position = position;
					particleResourceArray_read[idx].velocity = startVelocity; 
					counter++;
				}
			}
		}
		particleResourceArray_write = particleResourceArray_read;
		//for (Particle& p : particleResourceArray_read)
		//{
		//	p.position = rng.GetUnitVector() * rng.GetFraction() * 100;
		//}
		//particleResourceArray_write = particleResourceArray_read;
	{
		FRHIResourceCreateInfo createInfo_read;
		createInfo_read.ResourceArray = &particleResourceArray_read;
		FRHIResourceCreateInfo createInfo_write;
		createInfo_write.ResourceArray = &particleResourceArray_write;
		buffers[Read].Buffer = RHICreateStructuredBuffer(sizeof(Particle), sizeof(Particle) * numBoids, BUF_UnorderedAccess | BUF_ShaderResource, createInfo_read);
		buffers[Read].BufferUAV = RHICreateUnorderedAccessView(buffers[Read].Buffer, false, false);

		buffers[Write].Buffer = RHICreateStructuredBuffer(sizeof(Particle), sizeof(Particle) * numBoids, BUF_UnorderedAccess | BUF_ShaderResource, createInfo_write);
		buffers[Write].BufferUAV = RHICreateUnorderedAccessView(buffers[Write].Buffer, false, false);
	}
	{
		FRHIResourceCreateInfo createInfo_read;
		createInfo_read.ResourceArray = &particleForceResourceArray_read;
		FRHIResourceCreateInfo createInfo_write;
		createInfo_write.ResourceArray = &particleForceResourceArray_read;
		force_buffers[Read].Buffer = RHICreateStructuredBuffer(sizeof(ParticleForce), sizeof(ParticleForce) * numBoids, BUF_UnorderedAccess | BUF_ShaderResource, createInfo_read);
		force_buffers[Read].BufferUAV = RHICreateUnorderedAccessView(force_buffers[Read].Buffer, false, false);

		force_buffers[Write].Buffer = RHICreateStructuredBuffer(sizeof(ParticleForce), sizeof(ParticleForce) * numBoids, BUF_UnorderedAccess | BUF_ShaderResource, createInfo_write);
		force_buffers[Write].BufferUAV = RHICreateUnorderedAccessView(force_buffers[Write].Buffer, false, false);
	}
	{
		FRHIResourceCreateInfo createInfo_read;
		createInfo_read.ResourceArray = &particleDensityResourceArray_read;
		FRHIResourceCreateInfo createInfo_write;
		createInfo_write.ResourceArray = &particleDensityResourceArray_read;
		density_buffers[Read].Buffer = RHICreateStructuredBuffer(sizeof(ParticleDensity), sizeof(ParticleDensity) * numBoids, BUF_UnorderedAccess | BUF_ShaderResource, createInfo_read);
		density_buffers[Read].BufferUAV = RHICreateUnorderedAccessView(density_buffers[Read].Buffer, false, false);

		density_buffers[Write].Buffer = RHICreateStructuredBuffer(sizeof(ParticleDensity), sizeof(ParticleDensity) * numBoids, BUF_UnorderedAccess | BUF_ShaderResource, createInfo_write);
		density_buffers[Write].BufferUAV = RHICreateUnorderedAccessView(density_buffers[Write].Buffer, false, false);
	}
	
	}


	if (outputParticles.Num() != numBoids)
	{
		const FVector zero(0.0f);
		Particle p;
		p.position = FVector::ZeroVector;
		p.velocity = FVector::ZeroVector;
		outputParticles.Init(p, numBoids);
		ParticleForce f;
		f.force = zero;
		outputForces.Init(f, numBoids);
		ParticleDensity d;
		d.density = 0;
		outputDensities.Init(d, numBoids);

	}
}

// Called every frame
void UComputeShaderTestComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	auto box = GetOwner()->FindComponentByClass<UDrawBoundaryComponent>()->Bounds.GetBox();
	auto loc = GetOwner()->GetActorLocation();

	minBoundary = box.Min - loc;
	maxBoundary = box.Max - loc;

	ENQUEUE_RENDER_COMMAND(FComputeShaderRunner)(
	[&](FRHICommandList& RHICommands)
	{
		const float poly6Kernel = 315.f / (65.f * PI * pow(effective_radious, 9.f));
		const float spikeyKernel = -45.f / (PI * pow(effective_radious, 6.f));
		const float lapKernel = 45.f / (PI * pow(effective_radious, 6.f));

		{

			TShaderMapRef<FComputeDensityDeclaration> cs(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
			FRHIComputeShader* rhiComputeShader = cs.GetComputeShader();
			FComputeDensityDeclaration::FParameters params;
			params.delta_time = timeStep;
			params.particles_read = buffers[Read].BufferUAV;
			params.particles_write = buffers[Write].BufferUAV;
			params.particlesForce_read = force_buffers[Read].BufferUAV;
			params.particlesForce_write = force_buffers[Write].BufferUAV;
			params.particlesDensity_read = density_buffers[Read].BufferUAV;
			params.particlesDensity_write = density_buffers[Write].BufferUAV;
			params.mass = mass;
			params.gravity = gravity * 2000.f;
			params.eps = eps;
			params.sig = sig;
			params.numParticles = numBoids;
			params.minBoundary = minBoundary;
			params.maxBoundary = maxBoundary;
			params.damping = damping;
			params.epsilon = FLT_EPSILON;
			params.radious = effective_radious;
			params.poly6Kernel = poly6Kernel;
			params.spikyKernel = spikeyKernel;
			params.lapKernel = lapKernel;
			params.pressureCoef = pressure_coef;
			params.restDensity = rest_density;
			params.viscosity = viscosity;
			RHICommands.SetComputeShader(rhiComputeShader);
			{
				FRHITransitionInfo info(force_buffers[Write].BufferUAV, ERHIAccess::EReadable, ERHIAccess::EWritable, EResourceTransitionFlags::None);
				RHICommands.Transition(info);
			}
			FComputeShaderUtils::Dispatch(RHICommands, cs, params, FIntVector(FMath::DivideAndRoundUp(numBoids, NUM_THREADS_PER_GROUP_DIMENSION), 1, 1));
			{
				FRHITransitionInfo info(force_buffers[Write].BufferUAV, ERHIAccess::EWritable, ERHIAccess::EReadable, EResourceTransitionFlags::None);
				RHICommands.Transition(info);
			}

			uint8* particledata = (uint8*)RHILockStructuredBuffer(density_buffers[Write].Buffer, 0, numBoids * sizeof(ParticleDensity), RLM_ReadOnly);
			FMemory::Memcpy(outputDensities.GetData(), particledata, numBoids * sizeof(ParticleDensity));
			RHIUnlockStructuredBuffer(density_buffers[Write].Buffer);
			std::swap(density_buffers[Read], density_buffers[Write]);
		}


		{
			{
				uint8* particledata = (uint8*)RHILockStructuredBuffer(density_buffers[Read].Buffer, 0, numBoids * sizeof(ParticleDensity), RLM_ReadOnly);
				FMemory::Memcpy(outputDensities.GetData(), particledata, numBoids * sizeof(ParticleDensity));
				RHIUnlockStructuredBuffer(density_buffers[Read].Buffer);
			}
			TShaderMapRef<FComputeForceDeclaration> cs(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
			FRHIComputeShader* rhiComputeShader = cs.GetComputeShader();
			FComputeForceDeclaration::FParameters params;
			params.delta_time = timeStep;
			params.particles_read = buffers[Read].BufferUAV;
			params.particles_write = buffers[Write].BufferUAV;
			params.particlesForce_read = force_buffers[Read].BufferUAV;
			params.particlesForce_write = force_buffers[Write].BufferUAV;
			params.particlesDensity_read = density_buffers[Read].BufferUAV;
			params.particlesDensity_write = density_buffers[Write].BufferUAV;
			params.mass = mass;
			params.gravity = gravity * 2000.f;
			params.eps = eps;
			params.sig = sig;
			params.numParticles = numBoids;
			params.minBoundary = minBoundary;
			params.maxBoundary = maxBoundary;
			params.damping = damping;
			params.epsilon = FLT_EPSILON;
			params.radious = effective_radious;
			params.poly6Kernel = poly6Kernel;
			params.spikyKernel = spikeyKernel;
			params.lapKernel = lapKernel;
			params.pressureCoef = pressure_coef;
			params.restDensity = rest_density;
			params.viscosity = viscosity;
			RHICommands.SetComputeShader(rhiComputeShader);
			{
				FRHITransitionInfo info(force_buffers[Write].BufferUAV, ERHIAccess::EReadable, ERHIAccess::EWritable, EResourceTransitionFlags::None);
				RHICommands.Transition(info);
			}
			//RHICommands.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EComputeToCompute, force_buffers[Write].BufferUAV);
			FComputeShaderUtils::Dispatch(RHICommands, cs, params, FIntVector(FMath::DivideAndRoundUp(numBoids, NUM_THREADS_PER_GROUP_DIMENSION), 1, 1));
			{
				FRHITransitionInfo info(force_buffers[Write].BufferUAV, ERHIAccess::EWritable, ERHIAccess::EReadable, EResourceTransitionFlags::None);
				RHICommands.Transition(info);
			}
			uint8* particledata = (uint8*)RHILockStructuredBuffer(force_buffers[Write].Buffer, 0, numBoids * sizeof(ParticleForce), RLM_ReadOnly);
			FMemory::Memcpy(outputForces.GetData(), particledata, numBoids * sizeof(ParticleForce));
			RHIUnlockStructuredBuffer(force_buffers[Write].Buffer);
			std::swap(force_buffers[Read], force_buffers[Write]);
		}
		
		{
			RHICommands.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EComputeToCompute, buffers[Write].BufferUAV);
			TShaderMapRef<FComputeShaderDeclaration> cs(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
			FRHIComputeShader* rhiComputeShader = cs.GetComputeShader();
			FComputeShaderDeclaration::FParameters params;
			params.delta_time = timeStep;
			params.particles_read = buffers[Read].BufferUAV;
			params.particles_write = buffers[Write].BufferUAV;
			params.particlesForce_read = force_buffers[Read].BufferUAV;
			params.particlesForce_write = force_buffers[Write].BufferUAV;
			params.particlesDensity_read = density_buffers[Read].BufferUAV;
			params.particlesDensity_write = density_buffers[Write].BufferUAV;
			params.mass = mass;
			params.gravity = gravity * 2000.f;
			params.eps = eps;
			params.sig = sig;
			params.numParticles = numBoids;
			params.minBoundary = minBoundary;
			params.maxBoundary = maxBoundary;
			params.damping = damping;
			params.epsilon = FLT_EPSILON;
			params.radious = effective_radious;
			params.poly6Kernel = poly6Kernel;
			params.spikyKernel = spikeyKernel;
			params.lapKernel = lapKernel;
			params.pressureCoef = pressure_coef;
			params.restDensity = rest_density;
			params.viscosity = viscosity;
			{
				FRHITransitionInfo info(buffers[Write].BufferUAV, ERHIAccess::EReadable, ERHIAccess::EWritable, EResourceTransitionFlags::None);
				RHICommands.Transition(info);
			}
			RHICommands.SetComputeShader(rhiComputeShader);
			//RHICommands.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EComputeToCompute, buffers[Write].BufferUAV);
			FComputeShaderUtils::Dispatch(RHICommands, cs, params, FIntVector(FMath::DivideAndRoundUp(numBoids, NUM_THREADS_PER_GROUP_DIMENSION), 1, 1));
			{
				FRHITransitionInfo info(buffers[Write].BufferUAV, ERHIAccess::EWritable, ERHIAccess::EReadable, EResourceTransitionFlags::None);
				RHICommands.Transition(info);
			}
			// read back the data
			uint8* particledata = (uint8*)RHILockStructuredBuffer(buffers[Write].Buffer, 0, numBoids * sizeof(Particle), RLM_ReadOnly);
			FMemory::Memcpy(outputParticles.GetData(), particledata, numBoids * sizeof(Particle));
			RHIUnlockStructuredBuffer(buffers[Write].Buffer);
			std::swap(buffers[Read], buffers[Write]);
		}
		//DispatchComputeShader(RHICommands, cs, FMath::DivideAndRoundUp(numBoids, NUM_THREADS_PER_GROUP_DIMENSION), 1, 1);


		
	}); 
	
	
}
void FComputeShaderDeclaration::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
	OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), NUM_THREADS_PER_GROUP_DIMENSION);
	OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), 1);
	OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), 1);


}


void FComputeForceDeclaration::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
	OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), NUM_THREADS_PER_GROUP_DIMENSION);
	OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), 1);
	OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), 1);

}

void FComputeDensityDeclaration::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
	OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), NUM_THREADS_PER_GROUP_DIMENSION);
	OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), 1);
	OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), 1);

}
IMPLEMENT_SHADER_TYPE(, FComputeForceDeclaration, TEXT("/ComputeShaderPlugin/Boid.usf"), TEXT("Force"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FComputeDensityDeclaration, TEXT("/ComputeShaderPlugin/Boid.usf"), TEXT("Density"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FComputeShaderDeclaration, TEXT("/ComputeShaderPlugin/Boid.usf"), TEXT("MainComputeShader"), SF_Compute);
