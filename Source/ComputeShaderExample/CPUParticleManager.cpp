// Fill out your copyright notice in the Description page of Project Settings.


#include "CPUParticleManager.h"
#include "DrawBoundaryComponent.h"
// Sets default values
ACPUParticleManager::ACPUParticleManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ACPUParticleManager::BeginPlay()
{
	Super::BeginPlay();
	initIsmc();
	auto box = FindComponentByClass<UDrawBoundaryComponent>()->Bounds.GetBox();
	auto loc = GetActorLocation();

	minBoundary = box.Min - loc;
	maxBoundary = box.Max - loc;


	FVector resolution = (maxBoundary - minBoundary) / (effective_radious);
	grid_dimensions = FIntVector(ceil(resolution.X), ceil(resolution.Y), ceil(resolution.Z));
	grid_size = grid_dimensions.X * grid_dimensions.Y * grid_dimensions.Z;
	FRandomStream rng;
	{
		Particle p;
		ParticleDensity d;
		ParticleForce f;
		p.position = FVector::ZeroVector;
		p.velocity = FVector::ZeroVector;
		d.density = 0;
		f.force = FVector::ZeroVector;
		particles.Init(p, numBoids);
		forces.Init(f, numBoids);
		densities.Init(d, numBoids);
		
		GridTracker.Init(0, grid_size);
		GridCells.Init(0, grid_size * maxParticlesPerCell);
	}
	int counter = 0;
	for (int z = 0; z < grid_dimensions.Z && counter < numBoids; z++) {
		for (int y = 0; y < grid_dimensions.Y && counter < numBoids; y++) {
			for (int x = 0; x < grid_dimensions.X && counter < numBoids; x++) {
				FVector position = FVector(maxBoundary.X - 1, maxBoundary.Y - 1, maxBoundary.Z - 1) - FVector(x / 2.f, y / 2.f, z / 2.f) - FVector(rng.RandRange(effective_radious, effective_radious + 0.1), rng.RandRange(effective_radious, effective_radious + 0.1), rng.RandRange(effective_radious, effective_radious + 0.1));
				int idx = z * grid_dimensions.X * grid_dimensions.Y + y * grid_dimensions.X + x;
				particles[idx].position = position;
				particles[idx].velocity = startVelocity;
				counter++;
			}
		}
	}




}

// Called every frame
void ACPUParticleManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	poly6Kernel = 315.f / (65.f * PI * pow(effective_radious, 9.f));
	spikyKernel = -45.f / (PI * pow(effective_radious, 6.f));
	lapKernel = 45.f / (PI * pow(effective_radious, 6.f));
	gravityForce = gravity * 2000.f;
	recomputeGrid();
	computeDensity();
	computeForces();
	integrate();
	draw();
}

float ACPUParticleManager::ComputePressure(float density)
{
	return pressure_coef * (density - rest_density);
}

FVector ACPUParticleManager::ComputeLapVelocity(float dist, FVector velocity, FVector i_velocity, float i_density)
{
	FVector vdiff = i_velocity - velocity;
	float mass_ratio = mass / mass;
	float diff = effective_radious - dist;
	float lap = lapKernel * diff;
	return mass_ratio * viscosity * (1 / i_density) * lap * vdiff;
}

float ACPUParticleManager::ComputeGradPressure(float dist, float pressure, float density, float i_pressure, float i_density)
{
	float i_density2 = i_density * i_density;
	float density2 = density * density;
	float avgPressure = 0.5 * (i_pressure / i_density + pressure / density);
	float mass_ratio = mass / mass;
	float diff = effective_radious - dist;
	float spiky = spikyKernel * diff * diff;
	return mass_ratio * avgPressure * spiky;
}

float ACPUParticleManager::ComputeDensity(float dist_sq, float er_sq)
{
	float x = er_sq - dist_sq;
	return mass * poly6Kernel * x * x * x;
}

void ACPUParticleManager::recomputeGrid()
{
	memset(GridTracker.GetData(),0, GridTracker.Num());

	for (int i = 0;  i < numBoids; i++) {
		Particle p = particles[i];
		const int cellId = GetCellId(GetGridCellCoord(p.position, minBoundary, effective_radious), grid_dimensions);
		const int index = GridTracker[cellId]++;
		if (index < maxParticlesPerCell) {
			GridCells[cellId * maxParticlesPerCell + index] = i;
		}
	}
}

void ACPUParticleManager::computeForces()
{
	for (int index = 0; index < numBoids; index++) {
		Particle p1 = particles[index];
		FVector position = particles[index].position;
		FVector velocity = particles[index].velocity;
		float density = densities[index].density;
		float pressure = ComputePressure(density);
		const float er_sq = effective_radious * effective_radious;
		FVector force = FVector(0, 0, 0);
		FIntVector gridCoord = GetGridCellCoord(p1.position,minBoundary, effective_radious);

		for (int i = -1; i < 2; i++) {
			for (int j = -1; j < 2; j++) {
				for (int k = -1; k < 2; k++) {
					FIntVector cell = gridCoord + FIntVector(i, j, k);
					if (!isInGrid(cell, grid_dimensions))
						continue;
					int cellId = GetCellId(cell, grid_dimensions);
					const int size = std::min(GridTracker[cellId], maxParticlesPerCell);
					for (int l = 0; l < size; l++) {
						int idx = GridCells[cellId * maxParticlesPerCell + l];
						if (idx == index)
							continue;
						FVector res = particles[idx].position - p1.position;
						float dist_sq = FVector::DotProduct(res, res);
						if (dist_sq < er_sq && dist_sq > 0) {
							FVector i_velocity = particles[idx].velocity;
							float i_density = densities[idx].density;
							float i_pressure = ComputePressure(i_density);
							float dist = sqrt(dist_sq);
							force -= (res / dist) * ComputeGradPressure(dist, pressure, density, i_pressure, i_density);
							force += ComputeLapVelocity(dist, velocity, i_velocity, i_density);
						}
					}
				}
			}
		}
		force += gravityForce;
		forces[index].force = force;
	}
}

void ACPUParticleManager::computeDensity()
{
	for (int index = 0; index < numBoids; index++) {
		Particle p1 = particles[index];
		const float er_sq = effective_radious * effective_radious;
		float density = 0;
		FVector position = p1.position;

		FIntVector gridCoord = GetGridCellCoord(position, minBoundary, effective_radious);
		for (int i = -1; i < 2; i++) {
			for (int j = -1; j < 2; j++) {
				for (int k = -1; k < 2; k++) {
					FIntVector cell = gridCoord + FIntVector(i, j, k);
					if (!isInGrid(cell, grid_dimensions))
						continue;
					int cellId = GetCellId(cell, grid_dimensions);
					const int size = std::min(GridTracker[cellId], maxParticlesPerCell);
					for (int l = 0; l < size; l++) {
						int idx = GridCells[cellId * maxParticlesPerCell + l];
						FVector other = particles[idx].position;
						FVector res = other - position;
						float dist_sq = FVector::DotProduct(res, res);
						if (dist_sq <= er_sq) {
							density += ComputeDensity(dist_sq, er_sq);
						}
					}
				}
			}
		}
		densities[index].density = density;

	}



}

void ACPUParticleManager::integrate()
{
	for (int index = 0; index < numBoids; index++) {
		Particle& p = particles[index];
		FVector position = p.position;
		FVector velocity = p.velocity;
		float density = densities[index].density;

		FVector force = forces[index].force;

		FVector acceleration = FVector(0, 0, 0);
		acceleration += force / mass;

		velocity += acceleration * timeStep;
		position += velocity * timeStep;


		//enforce bounds
		if (position.X - epsilon < minBoundary.X) {
			velocity.X *= damping;
			position.X = minBoundary.X + epsilon;
		}
		else if (position.X + epsilon + 1 > maxBoundary.X) {
			velocity.X *= damping;
			position.X = maxBoundary.X - epsilon - 1;
		}


		if (position.Y - epsilon < minBoundary.Y) {
			velocity.Y *= damping;
			position.Y = minBoundary.Y + epsilon;
		}
		else if (position.Y + epsilon + 1 > maxBoundary.Y) {
			velocity.Y *= damping;
			position.Y = maxBoundary.Y - epsilon - 1;
		}
		if (position.Z - epsilon < minBoundary.Z) {
			velocity.Z *= damping;
			position.Z = minBoundary.Z + epsilon;
		}
		else if (position.Z + epsilon + 1 > maxBoundary.Z) {
			velocity.Z *= damping;
			position.Z = maxBoundary.Z - epsilon - 1;
		}


		p.velocity = velocity;
		p.position = position;

	}
}

void ACPUParticleManager::draw()
{
	UInstancedStaticMeshComponent* ismc = FindComponentByClass<UInstancedStaticMeshComponent>();

	if (!ismc) return;



	// resize up/down the ismc
	int toAdd = FMath::Max(0, particles.Num() - ismc->GetInstanceCount());
	int toRemove = FMath::Max(0, ismc->GetInstanceCount() - particles.Num());

	for (int i = 0; i < toAdd; ++i)
		ismc->AddInstance(FTransform::Identity);
	for (int i = 0; i < toRemove; ++i)
		ismc->RemoveInstance(ismc->GetInstanceCount() - 1);

	// update the transforms
	_instanceTransforms.SetNum(particles.Num());

	for (int i = 0; i < particles.Num(); ++i)
	{
		FTransform& transform = _instanceTransforms[i];
		transform.SetTranslation(particles[i].position);
		transform.SetScale3D(FVector(renderScale));
		transform.SetRotation(FQuat::Identity);
	}
	ismc->BatchUpdateInstancesTransforms(0, _instanceTransforms, false, true, true);
}

void ACPUParticleManager::initIsmc()
{
	UInstancedStaticMeshComponent* ismc = FindComponentByClass<UInstancedStaticMeshComponent>();
	if (!ismc) return;
	ismc->SetSimulatePhysics(false);
	ismc->SetMobility(EComponentMobility::Movable);
	ismc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ismc->SetCanEverAffectNavigation(false);
	ismc->SetCollisionProfileName(TEXT("NoCollision"));
}

