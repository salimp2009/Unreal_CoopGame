// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/STrackerBot.h"
#include "Components/StaticMeshComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "DrawDebugHelpers.h"
#include "Components/SHealthComponent.h"


ASTrackerBot::ASTrackerBot()
{
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetCanEverAffectNavigation(false);
	MeshComp->SetSimulatePhysics(true);
	RootComponent = MeshComp;

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));
	HealthComp->OnHealthChanged.AddDynamic(this, &ASTrackerBot::HandleTakeDamage);

	MovementForce = 1000.0f;
	bUseVelocityChange = true;
	RequiredDistanceToTarget = 100.0f;
}

void ASTrackerBot::BeginPlay()
{
	Super::BeginPlay();
	//initial MoveTo location
	NextPathPoint = GetNextPathPoint();
}

void ASTrackerBot::HandleTakeDamage(USHealthComponent* OwningHealthComp, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	// Explode on hitpoints == 0
	// TODO: Pulse/change the material on hit
}

FVector ASTrackerBot::GetNextPathPoint()
{
	//TODO; this is a hack to get the player location from Zero index player; need to get the closest one instead
	ACharacter* PlayerPawn = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (PlayerPawn)
	{
		// chases the target actor until it reaches even if the target moves until target stops && AI reaches target
		UNavigationPath* NavPath=UNavigationSystemV1::FindPathToActorSynchronously(this, GetActorLocation(), PlayerPawn);
		if (NavPath->PathPoints.Num()>1)
		{
			// next point in the path(first one is the AIs location)
			return NavPath->PathPoints[1];
		}
	}

	// failed to find a target!!!
	return GetActorLocation();
}


// Called every frame
void ASTrackerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float DistanceToTarget =(GetActorLocation() - NextPathPoint).Size(); // (NextPathPoint - GetActorLocation()).Distance();

	if (DistanceToTarget<=RequiredDistanceToTarget)
	{
		NextPathPoint = GetNextPathPoint();
		DrawDebugString(GetWorld(), GetActorLocation(), "Target Reached !!!");
	}
	else
	{
		//Keep Moving Towards Target
		FVector ForceDirection = (NextPathPoint - GetActorLocation());
		ForceDirection.Normalize();
		ForceDirection *= MovementForce;   // Add Force Impulse

		MeshComp->AddForce(ForceDirection, NAME_None, bUseVelocityChange);

		DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation()+ForceDirection, 32.0f, FColor::Green, false, 0.0f, 0, 2.0f);
	}

	DrawDebugSphere(GetWorld(),	NextPathPoint, 20.0f, 12, FColor::Blue, false, 0.0f, 0, 2.0f);
	
}

