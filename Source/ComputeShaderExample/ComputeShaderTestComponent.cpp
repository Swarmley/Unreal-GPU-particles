// Fill out your copyright notice in the Description page of Project Settings.


#include "ComputeShaderTestComponent.h"
#include "ShaderCompilerCore.h"
#include "RHIStaticStates.h"
#include "DrawBoundaryComponent.h"
#include "ParticleGeneratorBoundsComponent.h"
#define NUM_THREADS_PER_GROUP_DIMENSION 32
DECLARE_STATS_GROUP(TEXT("LODZERO_Game"), STATGROUP_LODZERO, STATCAT_Advanced);
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
	auto loc = GetOwner()->GetActorLocation();
	{
		auto box = GetOwner()->FindComponentByClass<UDrawBoundaryComponent>()->Bounds.GetBox();


		minBoundary = box.Min - loc;
		maxBoundary = box.Max - loc;
		numBoids = std::max(0, std::min(numBoids, maxBoids));

		FVector resolution = (maxBoundary - minBoundary) / (effective_radious);
		grid_dimensions = FIntVector(ceil(resolution.X), ceil(resolution.Y), ceil(resolution.Z));
		grid_size = grid_dimensions.X * grid_dimensions.Y * grid_dimensions.Z;
	}
		FRandomStream rng;
		TResourceArray<Particle> particleResourceArray;
		TResourceArray<ParticleForce> particleForceResourceArray;
		TResourceArray<ParticleDensity> particleDensityResourceArray;
		TResourceArray<int> grid;
		TResourceArray<int> grid_cells;
		FIntVector gen_dimensions = grid_dimensions;
		FVector gen_max = maxBoundary;
		UParticleGeneratorBoundsComponent* gen = GetOwner()->FindComponentByClass<UParticleGeneratorBoundsComponent>();
		if (gen) {
			auto box = gen->Bounds.GetBox();
			gen_max = box.Max - loc;
			gen_dimensions = FIntVector(ceil(box.Max.X - box.Min.X), ceil(box.Max.Y - box.Min.Y), ceil(box.Max.Z - box.Min.Z));
		}
		{
			Particle p;
			ParticleDensity d;
			ParticleForce f;
			p.position = FVector::ZeroVector;
			p.velocity = FVector::ZeroVector;
			particleResourceArray.Init(p, maxBoids);
			particleForceResourceArray.Init(f, maxBoids);
			particleDensityResourceArray.Init(d, maxBoids);
			grid.Init(0, grid_size);
			grid_cells.Init(0, grid_size * maxParticlesPerCell);
		}
		for (int i = 0; i < maxBoids; i++) {
			int counter = i;
			int z = (counter / (gen_dimensions.Y * gen_dimensions.X)) % gen_dimensions.Z ;
			int y = (counter / gen_dimensions.X) % gen_dimensions.Y;
			int x = counter % gen_dimensions.X;
			FVector position = FVector(gen_max.X - 1, gen_max.Y - 1, gen_max.Z - 1) - FVector(x, y, z);// - FVector(rng.RandRange(effective_radious, effective_radious + 0.1), rng.RandRange(effective_radious, effective_radious + 0.1), rng.RandRange(effective_radious, effective_radious + 0.1));
			particleResourceArray[counter].position = position;
			particleResourceArray[counter].velocity = startVelocity +  FVector(rng.RandRange(-1,1), rng.RandRange(-1, 1), rng.RandRange(-1, 1));
		}
	{
		TResourceArray<Particle> particleResourceArray_write = particleResourceArray;
		FRHIResourceCreateInfo createInfo_read;
		createInfo_read.ResourceArray = &particleResourceArray;
		FRHIResourceCreateInfo createInfo_write;
		createInfo_write.ResourceArray = &particleResourceArray_write;
		frames[Read].paricle.Buffer = RHICreateStructuredBuffer(sizeof(Particle), sizeof(Particle) * maxBoids, BUF_UnorderedAccess | BUF_ShaderResource, createInfo_read);
		frames[Read].paricle.BufferUAV = RHICreateUnorderedAccessView(frames[Read].paricle.Buffer, false, false);

		frames[Write].paricle.Buffer = RHICreateStructuredBuffer(sizeof(Particle), sizeof(Particle) * maxBoids, BUF_UnorderedAccess | BUF_ShaderResource, createInfo_write);
		frames[Write].paricle.BufferUAV = RHICreateUnorderedAccessView(frames[Write].paricle.Buffer, false, false);
	}
	{
		TResourceArray<ParticleForce> particleForceResourceArray_write = particleForceResourceArray;
		FRHIResourceCreateInfo createInfo_read;
		createInfo_read.ResourceArray = &particleForceResourceArray;
		FRHIResourceCreateInfo createInfo_write;
		createInfo_write.ResourceArray = &particleForceResourceArray_write;
		frames[Read].force.Buffer = RHICreateStructuredBuffer(sizeof(ParticleForce), sizeof(ParticleForce) * maxBoids, BUF_UnorderedAccess | BUF_ShaderResource, createInfo_read);
		frames[Read].force.BufferUAV = RHICreateUnorderedAccessView(frames[Read].force.Buffer, false, false);

		frames[Write].force.Buffer = RHICreateStructuredBuffer(sizeof(ParticleForce), sizeof(ParticleForce) * maxBoids, BUF_UnorderedAccess | BUF_ShaderResource, createInfo_write);
		frames[Write].force.BufferUAV = RHICreateUnorderedAccessView(frames[Write].force.Buffer, false, false);
	}
	{
		TResourceArray<ParticleDensity> particleDensityResourceArray_write = particleDensityResourceArray;
		FRHIResourceCreateInfo createInfo_read;
		createInfo_read.ResourceArray = &particleDensityResourceArray;
		FRHIResourceCreateInfo createInfo_write;
		createInfo_write.ResourceArray = &particleDensityResourceArray_write;
		frames[Read].density.Buffer = RHICreateStructuredBuffer(sizeof(ParticleDensity), sizeof(ParticleDensity) * maxBoids, BUF_UnorderedAccess | BUF_ShaderResource, createInfo_read);
		frames[Read].density.BufferUAV = RHICreateUnorderedAccessView(frames[Read].density.Buffer, false, false);
		frames[Write].density.Buffer = RHICreateStructuredBuffer(sizeof(ParticleDensity), sizeof(ParticleDensity) * maxBoids, BUF_UnorderedAccess | BUF_ShaderResource, createInfo_write);
		frames[Write].density.BufferUAV = RHICreateUnorderedAccessView(frames[Write].density.Buffer, false, false);
	}
	{
		TResourceArray<int> grid_write = grid;
		FRHIResourceCreateInfo createInfo_read;
		createInfo_read.ResourceArray = &grid;
		FRHIResourceCreateInfo createInfo_write;
		createInfo_write.ResourceArray = &grid_write;
		frames[Read].grid_tracker.Buffer = RHICreateStructuredBuffer(sizeof(int), sizeof(int) * grid_size, BUF_UnorderedAccess | BUF_ShaderResource, createInfo_read);
		frames[Read].grid_tracker.BufferUAV = RHICreateUnorderedAccessView(frames[Read].grid_tracker.Buffer, false, false);
		frames[Write].grid_tracker.Buffer = RHICreateStructuredBuffer(sizeof(int), sizeof(int) * grid_size, BUF_UnorderedAccess | BUF_ShaderResource, createInfo_write);
		frames[Write].grid_tracker.BufferUAV = RHICreateUnorderedAccessView(frames[Write].grid_tracker.Buffer, false, false);
	}
	{
		TResourceArray<int> grid_cells_write = grid_cells;
		FRHIResourceCreateInfo createInfo_read;
		createInfo_read.ResourceArray = &grid_cells;
		FRHIResourceCreateInfo createInfo_write;
		createInfo_write.ResourceArray = &grid_cells_write;
		frames[Read].grid_cells.Buffer = RHICreateStructuredBuffer(sizeof(int), sizeof(int) * grid_size * maxParticlesPerCell, BUF_UnorderedAccess | BUF_ShaderResource, createInfo_read);
		frames[Read].grid_cells.BufferUAV = RHICreateUnorderedAccessView(frames[Read].grid_cells.Buffer, false, false);
		frames[Write].grid_cells.Buffer = RHICreateStructuredBuffer(sizeof(int), sizeof(int) * grid_size * maxParticlesPerCell, BUF_UnorderedAccess | BUF_ShaderResource, createInfo_write);
		frames[Write].grid_cells.BufferUAV = RHICreateUnorderedAccessView(frames[Write].grid_cells.Buffer, false, false);
	}
	if (outputParticles.Num() != maxBoids)
	{
		const FVector zero(0.0f);
		Particle p;
		p.position = FVector::ZeroVector;
		p.velocity = FVector::ZeroVector;
		outputParticles.Init(p , maxBoids);
		//memcpy(outputParticles.GetData(), particleResourceArray.GetData(), maxBoids * sizeof(Particle));
	}
}
static FIntVector groupSize(int numElements)
{
	const int threadCount = NUM_THREADS_PER_GROUP_DIMENSION;
	int count = ((numElements - 1) / threadCount) + 1;
	return FIntVector(count, 1, 1);
}
DECLARE_GPU_STAT_NAMED(Stat_GPU_UComputeShaderTestComponent_Tick, TEXT("UComputeShaderTestComponent::Tick"));
DECLARE_GPU_STAT_NAMED(Stat_GPU_UComputeShaderTestComponent_ClearGrid, TEXT("UComputeShaderTestComponent::ClearGrid"));
DECLARE_GPU_STAT_NAMED(Stat_GPU_UComputeShaderTestComponent_GridCompute, TEXT("UComputeShaderTestComponent::GridCompute"));
DECLARE_GPU_STAT_NAMED(Stat_GPU_UComputeShaderTestComponent_Density, TEXT("UComputeShaderTestComponent::Density"));
DECLARE_GPU_STAT_NAMED(Stat_GPU_UComputeShaderTestComponent_Force, TEXT("UComputeShaderTestComponent::Force"));
DECLARE_GPU_STAT_NAMED(Stat_GPU_UComputeShaderTestComponent_Integrate, TEXT("UComputeShaderTestComponent::Integrate"));
// Called every frame
DECLARE_CYCLE_STAT(TEXT("GPU_Tick"), STAT_GPUParticleManger_Tick, STATGROUP_LODZERO);
void UComputeShaderTestComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if(addParticles)numBoids += throughput;
	numBoids = std::max(0, std::min(numBoids, maxBoids));


	ENQUEUE_RENDER_COMMAND(FComputeShaderRunner)(
	[&](FRHICommandListImmediate& RHICommands)
	{
		
		Frame last = frames[(current_frame + 1) % 2];
		Frame current = frames[current_frame];

		float step = timeStep;
		const float poly6Kernel = 315.f / (65.f * PI * pow(effective_radious, 9.f));
		const float spikyKernel = -45.f / (PI * pow(effective_radious, 6.f));
		const float lapKernel = 45.f / (PI * pow(effective_radious, 6.f));
		auto box = GetOwner()->FindComponentByClass<UDrawBoundaryComponent>()->Bounds.GetBox();
		auto loc = GetOwner()->GetActorLocation();
		minBoundary = box.Min - loc;
		maxBoundary = box.Max - loc;
		SCOPED_GPU_STAT(RHICommands, Stat_GPU_UComputeShaderTestComponent_Tick)
		SCOPED_DRAW_EVENT(RHICommands, Stat_GPU_UComputeShaderTestComponent_Tick)
		SCOPE_CYCLE_COUNTER(STAT_GPUParticleManger_Tick);

		{
			//SCOPED_GPU_STAT(RHICommands, Stat_GPU_UComputeShaderTestComponent_ClearGrid)
			FComputeClearGridDeclaration::FParameters params_clear;
			params_clear.grid = current.grid_tracker.BufferUAV;
			params_clear.grid_size = grid_size;
			params_clear.gridDimensions = grid_dimensions;
			TShaderMapRef<FComputeClearGridDeclaration> cs(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
			FRHIComputeShader* rhiComputeShader = cs.GetComputeShader();
			RHICommands.SetComputeShader(rhiComputeShader);
			FComputeShaderUtils::Dispatch(RHICommands,
					cs,
					params_clear,
					groupSize(grid_size));
			RHICommands.TransitionResource(
				EResourceTransitionAccess::ERWBarrier,
				EResourceTransitionPipeline::EGfxToCompute,
				current.grid_tracker.BufferUAV
			);
		}
		{
			//SCOPED_GPU_STAT(RHICommands, Stat_GPU_UComputeShaderTestComponent_GridCompute)
			FComputeCreateGridDeclaration::FParameters params_grid;
			params_grid.particles_read = last.paricle.BufferUAV;
			params_grid.grid = current.grid_tracker.BufferUAV;
			params_grid.grid_cells = current.grid_cells.BufferUAV;
			params_grid.numParticles = numBoids;
			params_grid.radious = effective_radious;
			params_grid.maxParticlesPerCell = maxParticlesPerCell;
			params_grid.minBoundary = minBoundary;
			params_grid.maxBoundary = maxBoundary;
			params_grid.grid_size = grid_size;
			params_grid.gridDimensions = grid_dimensions;
			TShaderMapRef<FComputeCreateGridDeclaration> cs(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
			FRHIComputeShader* rhiComputeShader = cs.GetComputeShader();
			RHICommands.SetComputeShader(rhiComputeShader);
			FComputeShaderUtils::Dispatch(RHICommands,
				cs,
				params_grid,
				groupSize(numBoids));
			RHICommands.TransitionResource(
				EResourceTransitionAccess::ERWBarrier,
				EResourceTransitionPipeline::EGfxToCompute,
				current.grid_cells.BufferUAV
			);
		}

		{
			//SCOPED_GPU_STAT(RHICommands, Stat_GPU_UComputeShaderTestComponent_Density)
			FComputeDensityDeclaration::FParameters params_density;
			params_density.particles_read = last.paricle.BufferUAV;
			params_density.particlesDensity_write = current.density.BufferUAV;
			params_density.mass = mass;
			params_density.numParticles = numBoids;
			params_density.radious = effective_radious;
			params_density.poly6Kernel = poly6Kernel;
			params_density.grid = current.grid_tracker.BufferUAV;
			params_density.grid_cells = current.grid_cells.BufferUAV;
			params_density.maxParticlesPerCell = maxParticlesPerCell;
			params_density.gridDimensions = grid_dimensions;
			params_density.minBoundary = minBoundary;
			TShaderMapRef<FComputeDensityDeclaration> cs(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
			FRHIComputeShader* rhiComputeShader = cs.GetComputeShader();

			RHICommands.SetComputeShader(rhiComputeShader);
			FComputeShaderUtils::Dispatch(RHICommands,
				cs,
				params_density,
				groupSize(numBoids));
			RHICommands.TransitionResource(
				EResourceTransitionAccess::ERWBarrier,
				EResourceTransitionPipeline::EGfxToCompute,
				current.density.BufferUAV
			);
		}

		{
			//SCOPED_GPU_STAT(RHICommands, Stat_GPU_UComputeShaderTestComponent_Force)
			FComputeForceDeclaration::FParameters params_force;
			params_force.particles_read = last.paricle.BufferUAV;
			params_force.particlesForce_write = current.force.BufferUAV;
			params_force.particlesDensity_read = current.density.BufferUAV;
			params_force.grid = current.grid_tracker.BufferUAV;
			params_force.grid_cells = current.grid_cells.BufferUAV;
			params_force.mass = mass;
			params_force.gravity = gravity * 2000.f;
			params_force.numParticles = numBoids;
			params_force.epsilon = FLT_EPSILON;
			params_force.radious = effective_radious;
			params_force.spikyKernel = spikyKernel;
			params_force.lapKernel = lapKernel;
			params_force.pressureCoef = pressure_coef;
			params_force.restDensity = rest_density;
			params_force.viscosity = viscosity;
			params_force.maxParticlesPerCell = maxParticlesPerCell;
			params_force.gridDimensions = grid_dimensions;
			params_force.minBoundary = minBoundary;
			TShaderMapRef<FComputeForceDeclaration> cs(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
			FRHIComputeShader* rhiComputeShader = cs.GetComputeShader();

			RHICommands.SetComputeShader(rhiComputeShader);
			FComputeShaderUtils::Dispatch(RHICommands,
				cs,
				params_force,
				groupSize(numBoids));
			RHICommands.TransitionResource(
				EResourceTransitionAccess::ERWBarrier,
				EResourceTransitionPipeline::EGfxToCompute,
				current.force.BufferUAV
			);
		}

		{
			//SCOPED_GPU_STAT(RHICommands, Stat_GPU_UComputeShaderTestComponent_Integrate)
			FComputeShaderDeclaration::FParameters params_integrate;
			params_integrate.delta_time = step;
			params_integrate.particles_read = last.paricle.BufferUAV;
			params_integrate.particles_write = current.paricle.BufferUAV;
			params_integrate.particlesForce_read = current.force.BufferUAV;
			params_integrate.particlesDensity_read = current.density.BufferUAV;
			params_integrate.mass = mass;
			params_integrate.numParticles = numBoids;
			params_integrate.minBoundary = minBoundary;
			params_integrate.maxBoundary = maxBoundary;
			params_integrate.damping = damping;
			params_integrate.epsilon = 0.01;
			TShaderMapRef<FComputeShaderDeclaration> cs(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
			FRHIComputeShader* rhiComputeShader = cs.GetComputeShader();

			RHICommands.SetComputeShader(rhiComputeShader);
			FComputeShaderUtils::Dispatch(RHICommands,
				cs,
				params_integrate,
				groupSize(numBoids));
			RHICommands.TransitionResource(
				EResourceTransitionAccess::ERWBarrier,
				EResourceTransitionPipeline::EGfxToCompute,
				current.paricle.BufferUAV
			);
		}

		uint8* particledata = (uint8*)RHICommands.LockStructuredBuffer(current.paricle.Buffer, 0, maxBoids * sizeof(Particle), RLM_ReadOnly);
		{
			FMemory::Memcpy(outputParticles.GetData(), particledata, numBoids * sizeof(Particle));
		}
		RHICommands.UnlockStructuredBuffer(current.paricle.Buffer);
		current_frame = (current_frame + 1) % 2;
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
void FComputeDtDeclaration::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
	OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), NUM_THREADS_PER_GROUP_DIMENSION);
	OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), 1);
	OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), 1);
}
void FComputeCreateGridDeclaration::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
	OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), NUM_THREADS_PER_GROUP_DIMENSION);
	OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), 1);
	OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), 1);
}
void FComputeClearGridDeclaration::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
	OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), NUM_THREADS_PER_GROUP_DIMENSION);
	OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), 1);
	OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), 1);
}
IMPLEMENT_SHADER_TYPE(, FComputeClearGridDeclaration, TEXT("/ComputeShaderPlugin/ComputeGrid.usf"), TEXT("ClearGrid"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FComputeCreateGridDeclaration, TEXT("/ComputeShaderPlugin/ComputeGrid.usf"), TEXT("CreateGrid"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FComputeForceDeclaration, TEXT("/ComputeShaderPlugin/ComputeForce.usf"), TEXT("Force"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FComputeDensityDeclaration, TEXT("/ComputeShaderPlugin/ComputeDensity.usf"), TEXT("Density"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FComputeDtDeclaration, TEXT("/ComputeShaderPlugin/ComputeDt.usf"), TEXT("DeltaTimeReduction"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FComputeShaderDeclaration, TEXT("/ComputeShaderPlugin/Boid.usf"), TEXT("MainComputeShader"), SF_Compute);


