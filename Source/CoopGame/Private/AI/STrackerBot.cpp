// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/STrackerBot.h"
#include "Components/StaticMeshComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

// Sets default values
ASTrackerBot::ASTrackerBot()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetCanEverAffectNavigation(false);
	RootComponent = MeshComp;

}

void ASTrackerBot::BeginPlay()
{
	Super::BeginPlay();
	
}

FVector ASTrackerBot::GetNextPathPoint()
{
	//TODO; hack to get the player location; need to get the closest one instead
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

}

