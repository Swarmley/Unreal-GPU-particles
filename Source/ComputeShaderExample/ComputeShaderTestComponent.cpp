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
	
		TResourceArray<Particle> particleResourceArray_read;
		TResourceArray<Particle> particleResourceArray_write;

		TResourceArray<ParticleForce> particleForceResourceArray_read;
		TResourceArray<ParticleForce> particleForceResourceArray_write;

		TResourceArray<ParticleDensity> particleDensityResourceArray_read;
		TResourceArray<ParticleDensity> particleDensityResourceArray_write;

		TResourceArray<float> dt_read;
		TResourceArray<float> dt_write;

		TResourceArray<FVector> cvf_max;
		TResourceArray<uint32> mutex;

		TResourceArray<int> grid_read;
		TResourceArray<int> grid_write;

		TResourceArray<int> grid_cells_read;
		TResourceArray<int> grid_cells_write;


		int width, height, depth;
		width = maxBoundary.X - minBoundary.X;
		height = maxBoundary.Y - minBoundary.Y;
		depth = maxBoundary.Z - minBoundary.Z;

		FVector resolution = (maxBoundary - minBoundary) / (effective_radious);
		grid_dimensions = FIntVector(ceil(resolution.X), ceil(resolution.Y), ceil(resolution.Z));
		grid_size = grid_dimensions.X * grid_dimensions.Y * grid_dimensions.Z;


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

			grid_read.Init(0, grid_size);
			grid_write.Init(0, grid_size);

			grid_cells_read.Init(0, grid_size * maxParticlesPerCell);
			grid_cells_write.Init(0, grid_size * maxParticlesPerCell);


			dt_read.Init(0, 1);
			dt_write.Init(0, 1);
			FVector cfv(0);
			cvf_max.Init(cfv, 1);
			mutex.Init(0, 1);

		}
		int counter = 0;
		for (int z = 0; z < grid_dimensions.Z && counter < numBoids; z++) {
			for (int y = 0; y < grid_dimensions.Y && counter < numBoids; y++) {
				for (int x = 0; x < grid_dimensions.X && counter < numBoids; x++) {
					FVector position =  FVector(maxBoundary.X - 1, maxBoundary.Y - 1, maxBoundary.Z - 1) - FVector(x / 2.f, y / 2.f, z / 2.f) - FVector(rng.RandRange(effective_radious / 2, effective_radious), rng.RandRange(effective_radious / 2, effective_radious), rng.RandRange(effective_radious / 2, effective_radious));
					int idx = z * grid_dimensions.X * grid_dimensions.Y + y * grid_dimensions.X + x;
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
		createInfo_write.ResourceArray = &particleForceResourceArray_write;
		force_buffers[Read].Buffer = RHICreateStructuredBuffer(sizeof(ParticleForce), sizeof(ParticleForce) * numBoids, BUF_UnorderedAccess | BUF_ShaderResource, createInfo_read);
		force_buffers[Read].BufferUAV = RHICreateUnorderedAccessView(force_buffers[Read].Buffer, false, false);

		force_buffers[Write].Buffer = RHICreateStructuredBuffer(sizeof(ParticleForce), sizeof(ParticleForce) * numBoids, BUF_UnorderedAccess | BUF_ShaderResource, createInfo_write);
		force_buffers[Write].BufferUAV = RHICreateUnorderedAccessView(force_buffers[Write].Buffer, false, false);
	}
	{
		FRHIResourceCreateInfo createInfo_read;
		createInfo_read.ResourceArray = &particleDensityResourceArray_read;
		FRHIResourceCreateInfo createInfo_write;
		createInfo_write.ResourceArray = &particleDensityResourceArray_write;
		density_buffers[Read].Buffer = RHICreateStructuredBuffer(sizeof(ParticleDensity), sizeof(ParticleDensity) * numBoids, BUF_UnorderedAccess | BUF_ShaderResource, createInfo_read);
		density_buffers[Read].BufferUAV = RHICreateUnorderedAccessView(density_buffers[Read].Buffer, false, false);

		density_buffers[Write].Buffer = RHICreateStructuredBuffer(sizeof(ParticleDensity), sizeof(ParticleDensity) * numBoids, BUF_UnorderedAccess | BUF_ShaderResource, createInfo_write);
		density_buffers[Write].BufferUAV = RHICreateUnorderedAccessView(density_buffers[Write].Buffer, false, false);
	}
	{
		FRHIResourceCreateInfo createInfo_read;
		createInfo_read.ResourceArray = &dt_read;
		FRHIResourceCreateInfo createInfo_write;
		createInfo_write.ResourceArray = &dt_write;
		dt_buffers[Read].Buffer = RHICreateStructuredBuffer(sizeof(float), sizeof(float), BUF_UnorderedAccess | BUF_ShaderResource, createInfo_read);
		dt_buffers[Read].BufferUAV = RHICreateUnorderedAccessView(dt_buffers[Read].Buffer, false, false);

		dt_buffers[Write].Buffer = RHICreateStructuredBuffer(sizeof(float), sizeof(float) , BUF_UnorderedAccess | BUF_ShaderResource, createInfo_write);
		dt_buffers[Write].BufferUAV = RHICreateUnorderedAccessView(dt_buffers[Write].Buffer, false, false);

		FRHIResourceCreateInfo createInfo_cvf;
		createInfo_cvf.ResourceArray = &cvf_max;
		cvf_buffer.Buffer = RHICreateStructuredBuffer(sizeof(FVector), sizeof(FVector), BUF_UnorderedAccess | BUF_ShaderResource, createInfo_cvf);
		cvf_buffer.BufferUAV = RHICreateUnorderedAccessView(cvf_buffer.Buffer, false, false);

		FRHIResourceCreateInfo createInfo_mutex;
		createInfo_cvf.ResourceArray = &mutex;
		mutex_buffer.Buffer = RHICreateStructuredBuffer(sizeof(uint32), sizeof(uint32), BUF_UnorderedAccess | BUF_ShaderResource, createInfo_mutex);
		mutex_buffer.BufferUAV = RHICreateUnorderedAccessView(mutex_buffer.Buffer, false, false);

	}
	{
		FRHIResourceCreateInfo createInfo_read;
		createInfo_read.ResourceArray = &grid_read;
		FRHIResourceCreateInfo createInfo_write;
		createInfo_write.ResourceArray = &grid_write;
		grid_buffers[Read].Buffer = RHICreateStructuredBuffer(sizeof(int), sizeof(int) * grid_size, BUF_UnorderedAccess | BUF_ShaderResource, createInfo_read);
		grid_buffers[Read].BufferUAV = RHICreateUnorderedAccessView(grid_buffers[Read].Buffer, false, false);

		grid_buffers[Write].Buffer = RHICreateStructuredBuffer(sizeof(int), sizeof(int) * grid_size, BUF_UnorderedAccess | BUF_ShaderResource, createInfo_write);
		grid_buffers[Write].BufferUAV = RHICreateUnorderedAccessView(grid_buffers[Write].Buffer, false, false);
	}
	{
		FRHIResourceCreateInfo createInfo_read;
		createInfo_read.ResourceArray = &grid_cells_read;
		FRHIResourceCreateInfo createInfo_write;
		createInfo_write.ResourceArray = &grid_cells_write;
		grid_cells_buffers[Read].Buffer = RHICreateStructuredBuffer(sizeof(int), sizeof(int) * grid_size * maxParticlesPerCell, BUF_UnorderedAccess | BUF_ShaderResource, createInfo_read);
		grid_cells_buffers[Read].BufferUAV = RHICreateUnorderedAccessView(grid_cells_buffers[Read].Buffer, false, false);

		grid_cells_buffers[Write].Buffer = RHICreateStructuredBuffer(sizeof(int), sizeof(int) * grid_size* maxParticlesPerCell, BUF_UnorderedAccess | BUF_ShaderResource, createInfo_write);
		grid_cells_buffers[Write].BufferUAV = RHICreateUnorderedAccessView(grid_cells_buffers[Write].Buffer, false, false);
	}
	
	debugGrid.Init(0, grid_size);
	debugGridCells.Init(0, grid_size* maxParticlesPerCell);
	if (outputParticles.Num() != numBoids)
	{
		const FVector zero(0.0f);
		Particle p;
		p.position = FVector::ZeroVector;
		p.velocity = FVector::ZeroVector;
		outputParticles.Init(p ,numBoids);
		memcpy(outputParticles.GetData(), particleResourceArray_read.GetData(), numBoids * sizeof(Particle));

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
	


	ENQUEUE_RENDER_COMMAND(FComputeShaderRunner)(
	[&](FRHICommandListImmediate& RHICommands)
	{
		const float poly6Kernel = 315.f / (65.f * PI * pow(effective_radious, 9.f));
		const float spikeyKernel = -45.f / (PI * pow(effective_radious, 6.f));
		const float lapKernel = 45.f / (PI * pow(effective_radious, 6.f));
		auto box = GetOwner()->FindComponentByClass<UDrawBoundaryComponent>()->Bounds.GetBox();
		auto loc = GetOwner()->GetActorLocation();
		

		minBoundary = box.Min - loc;
		maxBoundary = box.Max - loc;
		int gridIterations;
		
		{
		
			TShaderMapRef<FComputeClearGridDeclaration> cs(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
			FRHIComputeShader* rhiComputeShader = cs.GetComputeShader();
			FComputeClearGridDeclaration::FParameters params;
			params.grid = grid_buffers[Write].BufferUAV;
			params.grid_size = grid_size;
			params.gridDimensions = grid_dimensions;
			RHICommands.SetComputeShader(rhiComputeShader);
			{
				FRHITransitionInfo info(grid_buffers[Write].BufferUAV, ERHIAccess::EReadable, ERHIAccess::ERWBarrier, EResourceTransitionFlags::None);
				RHICommands.Transition(info);
			}
			FComputeShaderUtils::Dispatch(RHICommands, cs, params, FIntVector(FMath::DivideAndRoundUp(grid_size, NUM_THREADS_PER_GROUP_DIMENSION), 1, 1));
			{
				//TArray<uint32> debugGrid;
				uint8* particledata = (uint8*)RHILockStructuredBuffer(grid_buffers[Write].Buffer, 0, grid_size * sizeof(int), RLM_ReadOnly);
				FMemory::Memcpy(debugGrid.GetData(), particledata, debugGrid.Num() * sizeof(int));
				RHIUnlockStructuredBuffer(grid_buffers[Write].Buffer);
			}
			//{
			//	uint8* particledata = (uint8*)RHILockStructuredBuffer(grid_cells_buffers[Write].Buffer, 0, debugGridCells.Num() * sizeof(int), RLM_WriteOnly);
			//	FMemory::Memzero(particledata, debugGridCells.Num() * sizeof(int));
			//	RHIUnlockStructuredBuffer(grid_cells_buffers[Write].Buffer);
			//}
		}
		{

			TShaderMapRef<FComputeCreateGridDeclaration> cs(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
			FRHIComputeShader* rhiComputeShader = cs.GetComputeShader();
			FComputeCreateGridDeclaration::FParameters params;
			params.particles_read = buffers[Read].BufferUAV;
			params.grid = grid_buffers[Write].BufferUAV;
			params.grid_cells = grid_cells_buffers[Write].BufferUAV;
			params.numParticles = numBoids;
			params.radious = effective_radious;
			params.maxParticlesPerCell = maxParticlesPerCell;
			params.minBoundary = minBoundary;
			params.maxBoundary = maxBoundary;
			params.grid_size = grid_size;
			params.gridDimensions = grid_dimensions;

			RHICommands.SetComputeShader(rhiComputeShader);
			//{
			//	FRHITransitionInfo info(grid_cells_buffers[Write].BufferUAV, ERHIAccess::ERWBarrier, ERHIAccess::ERWBarrier, EResourceTransitionFlags::None);
			//	RHICommands.Transition(info);
			//}
			FComputeShaderUtils::Dispatch(RHICommands, cs, params, FIntVector(FMath::DivideAndRoundUp(numBoids, NUM_THREADS_PER_GROUP_DIMENSION), 1, 1));
			//{
			//	FRHITransitionInfo info(grid_cells_buffers[Write].BufferUAV, ERHIAccess::ERWBarrier, ERHIAccess::EReadable, EResourceTransitionFlags::None);
			//	RHICommands.Transition(info);
			//}

			//{
			//	uint8* particledata = (uint8*)RHILockStructuredBuffer(grid_buffers[Write].Buffer, 0, debugGrid.Num() * sizeof(int), RLM_ReadOnly);
			//	FMemory::Memcpy(debugGrid.GetData(), particledata, debugGrid.Num() * sizeof(int));
			//	RHIUnlockStructuredBuffer(grid_buffers[Write].Buffer);
			//	int sum = 0;
			//	for (auto i : debugGrid) {
			//		sum += i;
			//	}
			//	int l = 0;;
			//}
			//{
			//	uint8* particledata = (uint8*)RHILockStructuredBuffer(grid_cells_buffers[Write].Buffer, 0, debugGridCells.Num() * sizeof(int), RLM_ReadOnly);
			//	FMemory::Memcpy(debugGridCells.GetData(), particledata, debugGridCells.Num() * sizeof(int));
			//	RHIUnlockStructuredBuffer(grid_cells_buffers[Write].Buffer);
			//	int sum = 0;
			//	for (auto i : debugGridCells) {
			//		sum= std::max(sum, i);
			//	}
			//	int l = 0;;
			//}
			//{
			//	uint8* particledata = (uint8*)RHILockStructuredBuffer(grid_buffers[Write].Buffer, 0, debugGrid.Num() * sizeof(int), RLM_ReadOnly);
			//	FMemory::Memcpy(debugGrid.GetData(), particledata, debugGrid.Num() * sizeof(int));
			//	RHIUnlockStructuredBuffer(grid_buffers[Write].Buffer);
			//}
			std::swap(grid_buffers[Read], grid_buffers[Write]);
			std::swap(grid_cells_buffers[Read], grid_cells_buffers[Write]);
		}

		{
		
			TShaderMapRef<FComputeDensityDeclaration> cs(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
			FRHIComputeShader* rhiComputeShader = cs.GetComputeShader();
			FComputeDensityDeclaration::FParameters params;
			params.particles_read = buffers[Read].BufferUAV;
			params.particlesDensity_write = density_buffers[Write].BufferUAV;
			params.mass = mass;
			params.numParticles = numBoids;
			params.radious = effective_radious;
			params.poly6Kernel = poly6Kernel;
			params.grid = grid_buffers[Read].BufferUAV;
			params.grid_cells = grid_cells_buffers[Read].BufferUAV;
			params.grid_size = grid_size;
			params.maxParticlesPerCell = maxParticlesPerCell;
			params.gridDimensions = grid_dimensions;
			RHICommands.SetComputeShader(rhiComputeShader);
			//{
			//	FRHITransitionInfo info(force_buffers[Write].BufferUAV, ERHIAccess::EReadable, ERHIAccess::EWritable, EResourceTransitionFlags::None);
			//	RHICommands.Transition(info);
			//}
			FComputeShaderUtils::Dispatch(RHICommands, cs, params, FIntVector(FMath::DivideAndRoundUp(numBoids, NUM_THREADS_PER_GROUP_DIMENSION), 1, 1));
			//RHICommands.WaitForDispatch();
			//{
			//	FRHITransitionInfo info(force_buffers[Write].BufferUAV, ERHIAccess::EWritable, ERHIAccess::EReadable, EResourceTransitionFlags::None);
			//	RHICommands.Transition(info);
			//}
		
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
			params.particles_read = buffers[Read].BufferUAV;
			params.particlesForce_write = force_buffers[Write].BufferUAV;
			params.particlesDensity_read = density_buffers[Read].BufferUAV;
			params.grid = grid_buffers[Read].BufferUAV;
			params.grid_cells = grid_cells_buffers[Read].BufferUAV;
			params.mass = mass;
			params.gravity = gravity * 2000.f;
			params.numParticles = numBoids;
			params.epsilon = FLT_EPSILON;
			params.radious = effective_radious;
			params.spikyKernel = spikeyKernel;
			params.lapKernel = lapKernel;
			params.pressureCoef = pressure_coef;
			params.restDensity = rest_density;
			params.viscosity = viscosity;
			params.grid_size = grid_size;
			params.maxParticlesPerCell = maxParticlesPerCell;
			params.gridDimensions = grid_dimensions;
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
			//uint8* particledata = (uint8*)RHILockStructuredBuffer(force_buffers[Write].Buffer, 0, numBoids * sizeof(ParticleForce), RLM_ReadOnly);
			//FMemory::Memcpy(outputForces.GetData(), particledata, numBoids * sizeof(ParticleForce));
			//RHIUnlockStructuredBuffer(force_buffers[Write].Buffer);
			std::swap(force_buffers[Read], force_buffers[Write]);
		}
		
		//double dt;
		////dt
		//{
		//
		//	{
		//		FVector cvf_init = { 0,0,0 };
		//		uint8* particledata = (uint8*)RHILockStructuredBuffer(cvf_buffer.Buffer, 0, sizeof(FVector), RLM_WriteOnly);
		//		FMemory::Memcpy(particledata, &cvf_init, sizeof(FVector));
		//		RHIUnlockStructuredBuffer(cvf_buffer.Buffer);
		//	}
		//	TShaderMapRef<FComputeDtDeclaration> cs(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
		//	FRHIComputeShader* rhiComputeShader = cs.GetComputeShader();
		//	FComputeDtDeclaration::FParameters params;
		//	params.delta_time = timeStep;
		//	params.particles_read = buffers[Read].BufferUAV;
		//	params.particlesForce_read = force_buffers[Read].BufferUAV;
		//	params.particlesDensity_read = density_buffers[Read].BufferUAV;
		//	params.cvf_max = cvf_buffer.BufferUAV;
		//	params.dt_write = dt_buffers[Write].BufferUAV;
		//	params.numParticles = numBoids;
		//	params.mutex = mutex_buffer.BufferUAV;
		//	params.pressureCoef = pressure_coef;
		//	params.restDensity = rest_density;
		//	params.mass = mass;
		//	{
		//		FRHITransitionInfo info(cvf_buffer.BufferUAV, ERHIAccess::ERWBarrier, ERHIAccess::ERWBarrier, EResourceTransitionFlags::None);
		//		RHICommands.Transition(info);
		//	}
		//	{
		//		FRHITransitionInfo info(dt_buffers[Write].BufferUAV, ERHIAccess::EReadable, ERHIAccess::EWritable, EResourceTransitionFlags::None);
		//		RHICommands.Transition(info);
		//	}
		//
		//	RHICommands.SetComputeShader(rhiComputeShader);
		//	//RHICommands.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EComputeToCompute, buffers[Write].BufferUAV);
		//	FComputeShaderUtils::Dispatch(RHICommands, cs, params, FIntVector(FMath::DivideAndRoundUp(numBoids, NUM_THREADS_PER_GROUP_DIMENSION), 1, 1));
		//	{
		//		FRHITransitionInfo info(dt_buffers[Write].BufferUAV, ERHIAccess::EWritable, ERHIAccess::EReadable, EResourceTransitionFlags::None);
		//		RHICommands.Transition(info);
		//	}
		//	FVector cvf_value = { 0,0,0 };
		//	{
		//		uint8* particledata = (uint8*)RHILockStructuredBuffer(cvf_buffer.Buffer, 0, sizeof(FVector), RLM_ReadOnly);
		//		FMemory::Memcpy(&cvf_value, particledata, sizeof(FVector));
		//		RHIUnlockStructuredBuffer(cvf_buffer.Buffer);
		//	}
		//	double c_max = sqrt(cvf_value.X + FLT_EPSILON);
		//	double v_max = sqrt(cvf_value.Y);
		//	double f_max = sqrt(cvf_value.Z);
		//
		//	dt = std::max(std::max(1.0 * effective_radious / std::max(1.0,v_max), sqrt(effective_radious / f_max)), (1.0 * effective_radious) / c_max) * 0.01;
		//	// read back the data
		//	//uint8* particledata = (uint8*)RHILockStructuredBuffer(dt_buffers[Write].Buffer, 0, sizeof(float), RLM_ReadOnly);
		//	//
		//	int k = 0;
		//	//FMemory::Memcpy(&dt, particledata, sizeof(float));
		//	//RHIUnlockStructuredBuffer(dt_buffers[Write].Buffer);
		//	//std::swap(dt_buffers[Read], dt_buffers[Write]);
		//}
		
		
		{
			TShaderMapRef<FComputeShaderDeclaration> cs(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
			FRHIComputeShader* rhiComputeShader = cs.GetComputeShader();
			FComputeShaderDeclaration::FParameters params;
			params.delta_time = timeStep;//std::min(dt,(double)timeStep);
			params.particles_read = buffers[Read].BufferUAV;
			params.particles_write = buffers[Write].BufferUAV;
			params.particlesForce_read = force_buffers[Read].BufferUAV;
			params.particlesDensity_read = density_buffers[Read].BufferUAV;
			params.mass = mass;
			params.numParticles = numBoids;
			params.minBoundary = minBoundary;
			params.maxBoundary = maxBoundary;
			params.damping = damping;
			params.epsilon = 0.01;
			
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
IMPLEMENT_SHADER_TYPE(, FComputeDtDeclaration, TEXT("/ComputeShaderPlugin/Boid.usf"), TEXT("DeltaTimeReduction"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FComputeShaderDeclaration, TEXT("/ComputeShaderPlugin/Boid.usf"), TEXT("MainComputeShader"), SF_Compute);


