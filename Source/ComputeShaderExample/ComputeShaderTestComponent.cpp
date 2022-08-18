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
		numBoids = FMath::Max(0, FMath::Min(numBoids, maxBoids));

		FVector resolution = (maxBoundary - minBoundary) / (effective_radious);
		grid_dimensions = FIntVector(ceil(resolution.X), ceil(resolution.Y), ceil(resolution.Z));
		grid_size = grid_dimensions.X * grid_dimensions.Y * grid_dimensions.Z;
	}
	FRandomStream rng;
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
		InitParticles.Init(p, maxBoids);
		grid.Init(0, grid_size);
		grid_cells.Init(0, grid_size * maxParticlesPerCell);
	}
	for (int i = 0; i < maxBoids; i++) {
		int counter = i;
		int z = (counter / (gen_dimensions.Y * gen_dimensions.X)) % gen_dimensions.Z ;
		int y = (counter / gen_dimensions.X) % gen_dimensions.Y;
		int x = counter % gen_dimensions.X;
		FVector position = FVector(gen_max.X - 1, gen_max.Y - 1, gen_max.Z - 1) - FVector(x, y, z);// - FVector(rng.RandRange(effective_radious, effective_radious + 0.1), rng.RandRange(effective_radious, effective_radious + 0.1), rng.RandRange(effective_radious, effective_radious + 0.1));
		InitParticles[counter].position = position;
		InitParticles[counter].velocity = startVelocity +  FVector(rng.RandRange(-1,1), rng.RandRange(-1, 1), rng.RandRange(-1, 1));
	}
	if (outputParticles.Num() != maxBoids)
	{
		outputParticles = InitParticles;
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
	if (addParticles) numBoids += throughput;
	numBoids = FMath::Max(0, FMath::Min(numBoids, maxBoids));

	if (IsDirty)
		RecreateFrames();

	ENQUEUE_RENDER_COMMAND(FComputeShaderRunner)(
	[&](FRHICommandListImmediate& RHICommands)
	{
		Frame current = frames[current_frame];
		Frame last = frames[(current_frame + 1) % 2];

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
		FRDGBuilder GraphBuilder(RHICommands);
		FRDGBuffer* ParticleBufferInput = GraphBuilder.RegisterExternalBuffer(last.particle.Buffer);
		
		FRDGBufferUAV* ParticleBufferInputUAV = GraphBuilder.CreateUAV(ParticleBufferInput, EPixelFormat::PF_R32_UINT);




		FComputeClearGridDeclaration::FParameters* params_clear = GraphBuilder.AllocParameters<FComputeClearGridDeclaration::FParameters>();
		FRDGBufferDesc bufferDesc = FRDGBufferDesc::CreateBufferDesc(sizeof(int), grid_size);
		FRDGBuffer* particleGridBuffer = GraphBuilder.CreateBuffer(bufferDesc, TEXT("UComputeShaderTestComponent::GridBuffer"), ERDGBufferFlags::None);
		FRDGBufferUAVDesc particleGridUAVDesc(particleGridBuffer, EPixelFormat::PF_R32_SINT);
		FRDGBufferUAV* particleGridUAV = GraphBuilder.CreateUAV(particleGridUAVDesc, ERDGUnorderedAccessViewFlags::None);
		//FRDGBufferSRVDesc particleGridSRVDesc(,);
		params_clear->grid = particleGridUAV;
		params_clear->grid_size = grid_size;
		TShaderMapRef<FComputeClearGridDeclaration> cs(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
		FRHIComputeShader* rhiComputeShader = cs.GetComputeShader();
		FIntVector groupCount = groupSize(grid_size);
		GraphBuilder.AddPass(RDG_EVENT_NAME("ClearGridShader"), params_clear, ERDGPassFlags::Compute, [params_clear, cs, groupCount](FRHIComputeCommandList& RHICmdList) {
			FComputeShaderUtils::Dispatch(RHICmdList, cs, *params_clear, groupCount);
		});
		





		
		SCOPED_GPU_STAT(RHICommands, Stat_GPU_UComputeShaderTestComponent_GridCompute)

		FRDGBufferDesc gridCellBufferDesc = FRDGBufferDesc::CreateBufferDesc(sizeof(int), grid_size * maxParticlesPerCell);
		FRDGBuffer* gridCellBuffer = GraphBuilder.CreateBuffer(gridCellBufferDesc, TEXT("UComputeShaderTestComponent::gridCellBuffer"), ERDGBufferFlags::None);
		FRDGBufferUAV* gridCellBufferUAV = GraphBuilder.CreateUAV(gridCellBuffer, EPixelFormat::PF_R32_SINT);
		FComputeCreateGridDeclaration::FParameters*  paramsCreateGrid = GraphBuilder.AllocParameters<FComputeCreateGridDeclaration::FParameters>();
		paramsCreateGrid->numParticles = numBoids;
		paramsCreateGrid->radious = effective_radious;
		paramsCreateGrid->maxParticlesPerCell = maxParticlesPerCell;
		paramsCreateGrid->minBoundary = minBoundary;
		paramsCreateGrid->gridDimensions = grid_dimensions;
		
		paramsCreateGrid->particles_read = ParticleBufferInputUAV;
		paramsCreateGrid->grid = particleGridUAV;
		paramsCreateGrid->grid_cells = gridCellBufferUAV;
		
		TShaderMapRef<FComputeCreateGridDeclaration> createGridComputeShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
		FIntVector createGridGroupCount = groupSize(numBoids);
		GraphBuilder.AddPass(RDG_EVENT_NAME("CreateGridShader"), paramsCreateGrid, ERDGPassFlags::Compute, [paramsCreateGrid, createGridComputeShader, createGridGroupCount](FRHIComputeCommandList& RHICmdList) {
			FComputeShaderUtils::Dispatch(RHICmdList, createGridComputeShader, *paramsCreateGrid, createGridGroupCount);
		});



		
		
		//SCOPED_GPU_STAT(RHICommands, Stat_GPU_UComputeShaderTestComponent_Density)
		//DENSITY
		FRDGBufferDesc densityBufferDesc = FRDGBufferDesc::CreateStructuredDesc(sizeof(ParticleDensity), numBoids);
		FRDGBuffer* densityBuffer =  GraphBuilder.CreateBuffer(densityBufferDesc,TEXT("UComputeShaderTestComponent::DensityBuffer"));
		FRDGBufferUAV* densityBufferUAV = GraphBuilder.CreateUAV(densityBuffer, EPixelFormat::PF_R32_UINT);

		FComputeDensityDeclaration::FParameters* paramsComputeDensity = GraphBuilder.AllocParameters<FComputeDensityDeclaration::FParameters>();
		paramsComputeDensity->particles_read = ParticleBufferInputUAV;
		paramsComputeDensity->particlesDensity_write = densityBufferUAV;
		
		paramsComputeDensity->grid = particleGridUAV;
		paramsComputeDensity->grid_cells = gridCellBufferUAV;

		paramsComputeDensity->mass = mass;
		paramsComputeDensity->numParticles = numBoids;
		paramsComputeDensity->radious = effective_radious;
		paramsComputeDensity->poly6Kernel = poly6Kernel;
		paramsComputeDensity->maxParticlesPerCell = maxParticlesPerCell;
		paramsComputeDensity->gridDimensions = grid_dimensions;
		paramsComputeDensity->minBoundary = minBoundary;

		TShaderMapRef<FComputeDensityDeclaration> computeDensityComputeShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
		FIntVector computeDensityGroupCount = groupSize(numBoids);
		GraphBuilder.AddPass(RDG_EVENT_NAME("ComputeDensityShader"), paramsComputeDensity, ERDGPassFlags::Compute, [paramsComputeDensity, computeDensityComputeShader, computeDensityGroupCount](FRHIComputeCommandList& RHICmdList) {
			FComputeShaderUtils::Dispatch(RHICmdList, computeDensityComputeShader, *paramsComputeDensity, computeDensityGroupCount);
		});


		//FORCE
		FRDGBufferDesc forceBufferDesc = FRDGBufferDesc::CreateStructuredDesc(sizeof(ParticleForce), numBoids);
		FRDGBuffer* forceBuffer = GraphBuilder.CreateBuffer(forceBufferDesc, TEXT("UComputeShaderTestComponent::ForceBuffer"));
		FRDGBufferUAV* forceBufferUAV = GraphBuilder.CreateUAV(forceBuffer, EPixelFormat::PF_R32_UINT);



		//SCOPED_GPU_STAT(RHICommands, Stat_GPU_UComputeShaderTestComponent_Force)
		FComputeForceDeclaration::FParameters* paramsComputeForce = GraphBuilder.AllocParameters<FComputeForceDeclaration::FParameters>();
		paramsComputeForce->particles_read = ParticleBufferInputUAV;
		paramsComputeForce->particlesForce_write = forceBufferUAV;
		paramsComputeForce->particlesDensity_read = densityBufferUAV;
		paramsComputeForce->grid = particleGridUAV;
		paramsComputeForce->grid_cells = gridCellBufferUAV;


		paramsComputeForce->mass = mass;
		paramsComputeForce->gravity = gravity * 2000.f;
		paramsComputeForce->numParticles = numBoids;
		paramsComputeForce->epsilon = FLT_EPSILON;
		paramsComputeForce->radious = effective_radious;
		paramsComputeForce->spikyKernel = spikyKernel;
		paramsComputeForce->lapKernel = lapKernel;
		paramsComputeForce->pressureCoef = pressure_coef;
		paramsComputeForce->restDensity = rest_density;
		paramsComputeForce->viscosity = viscosity;
		paramsComputeForce->maxParticlesPerCell = maxParticlesPerCell;
		paramsComputeForce->gridDimensions = grid_dimensions;
		paramsComputeForce->minBoundary = minBoundary;
		TShaderMapRef<FComputeForceDeclaration> computeForceComputeShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
		FIntVector computeForceGroupCount = groupSize(numBoids);
		GraphBuilder.AddPass(RDG_EVENT_NAME("ComputeForceShader"), paramsComputeForce, ERDGPassFlags::Compute, [paramsComputeForce, computeForceComputeShader, computeForceGroupCount](FRHIComputeCommandList& RHICmdList) {
			FComputeShaderUtils::Dispatch(RHICmdList, computeForceComputeShader, *paramsComputeForce, computeForceGroupCount);
		});

		



		//SCOPED_GPU_STAT(RHICommands, Stat_GPU_UComputeShaderTestComponent_Integrate)
		FComputeShaderDeclaration::FParameters* paramsIntegrate = GraphBuilder.AllocParameters<FComputeShaderDeclaration::FParameters>();
		FRDGBuffer* ParticleBufferOutput = GraphBuilder.RegisterExternalBuffer(current.particle.Buffer);

		//FRDGBufferDesc particleBufferDesc = FRDGBufferDesc::CreateStructuredDesc(sizeof(Particle), numBoids);
		//FRDGBuffer* particleBuffer = GraphBuilder.CreateBuffer(particleBufferDesc, TEXT("UComputeShaderTestComponent::ForceBuffer"), ERDGBufferFlags::MultiFrame);
		FRDGBufferUAV* particleBufferUAV = GraphBuilder.CreateUAV(ParticleBufferOutput, EPixelFormat::PF_R32_UINT);

		paramsIntegrate->delta_time = step;
		paramsIntegrate->particles_read = ParticleBufferInputUAV;

		paramsIntegrate->particles_write = particleBufferUAV;
		paramsIntegrate->particlesForce_read = forceBufferUAV;
		paramsIntegrate->particlesDensity_read = densityBufferUAV;
		paramsIntegrate->mass = mass;
		paramsIntegrate->numParticles = numBoids;
		paramsIntegrate->minBoundary = minBoundary;
		paramsIntegrate->maxBoundary = maxBoundary;
		paramsIntegrate->damping = damping;
		paramsIntegrate->epsilon = 0.01;
		TShaderMapRef<FComputeShaderDeclaration> updateParticlesComputeShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
		FIntVector updateParticlesGroupCount = groupSize(numBoids);
		GraphBuilder.AddPass(RDG_EVENT_NAME("IntegrateParticlesShader"), paramsIntegrate, ERDGPassFlags::Compute, [paramsIntegrate, updateParticlesComputeShader, updateParticlesGroupCount](FRHIComputeCommandList& RHICmdList) {
			FComputeShaderUtils::Dispatch(RHICmdList, updateParticlesComputeShader, *paramsIntegrate, updateParticlesGroupCount);
		});
		

		GraphBuilder.QueueBufferExtraction(ParticleBufferOutput, &current.particle.Buffer, ERHIAccess::UAVCompute);

		GraphBuilder.Execute();

		uint8* particledata = (uint8*)RHICommands.LockStructuredBuffer(current.particle.Buffer->GetStructuredBufferRHI(), 0, maxBoids * sizeof(Particle), RLM_ReadOnly);
		{
			FMemory::Memcpy(outputParticles.GetData(), particledata, numBoids * sizeof(Particle));
		}
		RHICommands.UnlockStructuredBuffer(current.particle.Buffer->GetStructuredBufferRHI());
		current_frame = (current_frame + 1) % 2;
	});


}
void UComputeShaderTestComponent::RecreateFrames()
{
	ENQUEUE_RENDER_COMMAND(FComputeShaderRunner)(
		[&](FRHICommandListImmediate& RHICommands)
		{
			FRDGBuilder GraphBuilder(RHICommands);
			for (int i = 0; i < 2; i++) {

				FRDGBufferDesc particleBufferDesc = FRDGBufferDesc::CreateStructuredDesc(sizeof(Particle), maxBoids);
				FString name = FString::Printf(TEXT("PartilceBuffer%d"), i);
				FRDGBuffer* particleBuffer = GraphBuilder.CreateBuffer(particleBufferDesc, name.GetCharArray().GetData());
				FRDGBufferUAV* particleBufferUAV = GraphBuilder.CreateUAV(particleBuffer, EPixelFormat::PF_R32_UINT);
				FComputeClearParticleBufferDeclaration::FParameters* clearParamaters = GraphBuilder.AllocParameters<FComputeClearParticleBufferDeclaration::FParameters>();
				clearParamaters->numParticles = maxBoids;
				clearParamaters->particles_write = particleBufferUAV;
				TShaderMapRef<FComputeClearParticleBufferDeclaration> clearParticlesComputeShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
				FIntVector updateParticlesGroupCount = groupSize(maxBoids);
				GraphBuilder.AddPass(RDG_EVENT_NAME("ClearParticleBufferShader"), clearParamaters, ERDGPassFlags::Compute, [clearParamaters, clearParticlesComputeShader, updateParticlesGroupCount](FRHIComputeCommandList& RHICmdList) {
					FComputeShaderUtils::Dispatch(RHICmdList, clearParticlesComputeShader, *clearParamaters, updateParticlesGroupCount);
				});
				ConvertToExternalBuffer(GraphBuilder, particleBuffer, frames[i].particle.Buffer);
			}
			GraphBuilder.Execute();
			for (int i = 0; i < 2; i++) {
				uint8* particledata = (uint8*)RHICommands.LockStructuredBuffer(frames[i].particle.Buffer->GetStructuredBufferRHI(), 0, maxBoids * sizeof(Particle), RLM_WriteOnly);
				{
					FMemory::Memcpy(particledata, InitParticles.GetData(), InitParticles.Num() * sizeof(Particle));
				}
				RHICommands.UnlockStructuredBuffer(frames[i].particle.Buffer->GetStructuredBufferRHI());
			}
		});
		IsDirty = false;
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
void FComputeClearParticleBufferDeclaration::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
	OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), NUM_THREADS_PER_GROUP_DIMENSION);
	OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), 1);
	OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), 1);
}
IMPLEMENT_SHADER_TYPE(, FComputeClearParticleBufferDeclaration, TEXT("/ComputeShaderPlugin/ClearParticleBuffer.usf"), TEXT("ClearParticleBuffer"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FComputeClearGridDeclaration, TEXT("/ComputeShaderPlugin/ClearGrid.usf"), TEXT("ClearGrid"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FComputeCreateGridDeclaration, TEXT("/ComputeShaderPlugin/ComputeGrid.usf"), TEXT("CreateGrid"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FComputeForceDeclaration, TEXT("/ComputeShaderPlugin/ComputeForce.usf"), TEXT("Force"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FComputeDensityDeclaration, TEXT("/ComputeShaderPlugin/ComputeDensity.usf"), TEXT("Density"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FComputeDtDeclaration, TEXT("/ComputeShaderPlugin/ComputeDt.usf"), TEXT("DeltaTimeReduction"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FComputeShaderDeclaration, TEXT("/ComputeShaderPlugin/Boid.usf"), TEXT("MainComputeShader"), SF_Compute);


