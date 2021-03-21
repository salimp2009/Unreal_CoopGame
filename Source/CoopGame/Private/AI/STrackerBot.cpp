// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/STrackerBot.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "DrawDebugHelpers.h"
#include "CoopGame/Public/Components/SHealthComponent.h"
#include "CoopGame/Public/SPlayerInterface.h"
#include "Sound/SoundCue.h"



ASTrackerBot::ASTrackerBot()
{
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetCanEverAffectNavigation(false);
	MeshComp->SetSimulatePhysics(true);
	RootComponent = MeshComp;

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));
	/* OnHealthChanged does not work if it is in constructor therefore moved to BeginPlay() and it works*/
	//HealthComp->OnHealthChanged.AddDynamic(this, &ASTrackerBot::HandleTakeDamage);

	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->SetSphereRadius(200.0f);
	SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SphereComp->SetupAttachment(RootComponent);
	
	MovementForce = 1000.0f;
	bUseVelocityChange = true;
	RequiredDistanceToTarget = 100.0f;

	ExplosionRadius = 200.0f;
	ExplosionDamage = 40.0f;
	SelfDamageInterval = 0.25f;
}


void ASTrackerBot::BeginPlay()
{
	Super::BeginPlay();
	HealthComp->OnHealthChanged.AddDynamic(this, &ASTrackerBot::HandleTakeDamage);

	if (HasAuthority())
	{
		//initial MoveTo location
		NextPathPoint = GetNextPathPoint();
	}

}

void ASTrackerBot::HandleTakeDamage(USHealthComponent* OwningHealthComp, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	if (MatInst==nullptr)
	{
		MatInst = MeshComp->CreateDynamicMaterialInstance(0, MeshComp->GetMaterial(0));
	}

	if (MatInst)
	{
		MatInst->SetScalarParameterValue("LastTimeDamageTaken", GetWorld()->TimeSeconds);
	}
	
	UE_LOG(LogTemp, Log, TEXT("Health %s of %s"), *FString::SanitizeFloat(Health), *GetName());

	// Explode on hitpoints == 0
	if (Health <= 0.0f)
	{
		SelfDestruct();
	}
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


void ASTrackerBot::SelfDestruct()
{
	if (bExploded)
	{
		return;
	}

	bExploded = true;

	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());
	
	// Apply Damage
	TArray<AActor*> IgnoredActors;
	IgnoredActors.Add(this);
	UGameplayStatics::ApplyRadialDamage(this, ExplosionDamage, GetActorLocation(), ExplosionRadius, nullptr, IgnoredActors, this, GetInstigatorController(), true);

	DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRadius, 12, FColor::Red, false, 2.0f, 0, 1.0f);
	UGameplayStatics::PlaySoundAtLocation(this, ExplodeSound, GetActorLocation());

	Destroy();
}

// Called every frame
void ASTrackerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (HasAuthority())
	{
		float DistanceToTarget = (GetActorLocation() - NextPathPoint).Size();

		if (DistanceToTarget <= RequiredDistanceToTarget)
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

			DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + ForceDirection, 32.0f, FColor::Green, false, 0.0f, 0, 2.0f);
		}

		DrawDebugSphere(GetWorld(), NextPathPoint, 20.0f, 12, FColor::Blue, false, 0.0f, 0, 2.0f);

	}
}


void ASTrackerBot::NotifyActorBeginOverlap(AActor* OtherActor)
{

	if (!bStartedSelfDestruction)
	{
		/** Alternative is to use ActorTag;
				e.g; set the Player Character added an element to Tags.Add(FName("Player")) in the constructor
				then just check; if(OtherActor->ActorHasTag("Player")); NO casting needed
			*/
		ISPlayerInterface* PlayerInterface = Cast<ISPlayerInterface>(OtherActor);

		if (PlayerInterface)
		{
			// Start self damage sequence every SelfDamageInterval without delay
			GetWorldTimerManager().SetTimer(TimerHandle_SelfDamage, this, &ASTrackerBot::DamageSelf, SelfDamageInterval, true, 0.0f);

			bStartedSelfDestruction = true;

			/*No need to check for nullptr for Sound; it is included in the function but might be better to avoid one less function*/
			UGameplayStatics::SpawnSoundAttached(SelfDestructSound, RootComponent);
		}
	}

}

void ASTrackerBot::DamageSelf()
{
	UGameplayStatics::ApplyDamage(this, 20.0f, GetInstigatorController(), this, nullptr);
}

