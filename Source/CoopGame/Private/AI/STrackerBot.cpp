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
#include "Net/UnrealNetwork.h"



ASTrackerBot::ASTrackerBot()
{
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetCanEverAffectNavigation(false);
	MeshComp->SetCollisionObjectType(ECC_WorldDynamic);
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
	PowerLevel = 0;

	Tags.Add(FName("TrackerBot"));

	/*Not neccessary Pawn replicates by default but might still be better to use if defaults changes in the future*/
	//SetReplicates(true);
}


void ASTrackerBot::BeginPlay()
{
	Super::BeginPlay();
	/* AddDynamic did not work in constructor moved here!!!*/
	HealthComp->OnHealthChanged.AddDynamic(this, &ASTrackerBot::HandleTakeDamage);

	if (HasAuthority())
	{
		//initial MoveTo location
		NextPathPoint = GetNextPathPoint();

		FTimerHandle TimerHandle_CheckPowerLevel;
		GetWorldTimerManager().SetTimer(TimerHandle_CheckPowerLevel,this, &ASTrackerBot::OnCheckNearbyBots, 1.0f, true);
	}

}

void ASTrackerBot::HandleTakeDamage(USHealthComponent* OwningHealthComp, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	if (MatInst==nullptr)
	{
		MatInst = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));
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

	UGameplayStatics::PlaySoundAtLocation(this, ExplodeSound, GetActorLocation());

	MeshComp->SetVisibility(false, true);
	MeshComp->SetSimulatePhysics(false);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	if (HasAuthority())
	{
		// Apply Damage
		TArray<AActor*> IgnoredActors;
		IgnoredActors.Add(this);

		float ActualDamage = ExplosionDamage + (ExplosionDamage * PowerLevel);

		UGameplayStatics::ApplyRadialDamage(this, ActualDamage, GetActorLocation(), ExplosionRadius, nullptr, IgnoredActors, this, GetInstigatorController(), true, ECC_Pawn);

		DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRadius, 12, FColor::Red, false, 2.0f, 0, 1.0f);

		/* Actor will be destroyed after 2 secs on the server to give time client to see the explosion effect*/
		SetLifeSpan(2.0f);
	}
	
}

// Called every frame
void ASTrackerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (HasAuthority() && !bExploded)
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
	Super::NotifyActorBeginOverlap(OtherActor);

	if (!bStartedSelfDestruction && !bExploded)
	{
		/** Alternative is to use ActorTag;
				e.g; set the Player Character added an element to Tags.Add(FName("Player")) in the constructor
				then just check; if(OtherActor->ActorHasTag("Player")); NO casting needed
			*/
		ISPlayerInterface* PlayerInterface = Cast<ISPlayerInterface>(OtherActor);

		if (PlayerInterface)
		{
			if (HasAuthority())
			{
				// Start self damage sequence every SelfDamageInterval without delay
				GetWorldTimerManager().SetTimer(TimerHandle_SelfDamage, this, &ASTrackerBot::DamageSelf, SelfDamageInterval, true, 0.0f);
			}

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


void ASTrackerBot::OnCheckNearbyBots()
{
	//Distance to check for other bots; TODO; check if we can use static so we specify only once!!!
	static constexpr float Radius = 600.0f;

	//Temp CollisionSphere; added static to initialize only once
	static FCollisionShape CollShape;
	CollShape.SetSphere(Radius);

	// Filter out only certain objects to apply collision;
	FCollisionObjectQueryParams QuerryParams;
	/*Tracker bot mesh is set as Physics_Body*/
	QuerryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	QuerryParams.AddObjectTypesToQuery(ECC_Pawn);
	

	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, QuerryParams, CollShape);

	DrawDebugSphere(GetWorld(), GetActorLocation(), Radius, 12, FColor::White, false, 1.0f);

	/* Check if we overlap anythng; if not we dont have to anythng*/
	if (Overlaps.Num() < 1)
	{
		return;
	}

	int32 NumofBots = 0;
		for (FOverlapResult Result : Overlaps)
		{
			/* Alternative the if my original does not work*/
			//ASTrackerBot* Bot = Cast<ASTrackerBot>(Result.GetActor());
			AActor* Bot = Result.GetActor();
			// check if this works !!!! ; try to use Actor which is a weak_ptr; might need to use Get() to access referred ptr!!!
			if(Bot && Bot->ActorHasTag("TrackerBot") && Bot != this)
			{
				NumofBots++;
			}
		}
	
	/* Needs to be initialized only once*/
	static constexpr int32 MaxPowerLevel = 4;

	PowerLevel = FMath::Clamp(NumofBots, 0, MaxPowerLevel);
	if (MatInst == nullptr)
	{
		MatInst = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));
	}

	if (MatInst)
	{
		float Alpha = PowerLevel / (float)MaxPowerLevel;
		AlphaToClients = Alpha;
		MatInst->SetScalarParameterValue("PowerLevelAlpha", Alpha);

		OnRep_PowerLevel();
	}

	DrawDebugString(GetWorld(), FVector(0.0f, 0.0f, 0.0f), FString::FromInt(PowerLevel), this, FColor::Red, 1.0f, true);
}

void ASTrackerBot::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASTrackerBot, PowerLevel);
	DOREPLIFETIME(ASTrackerBot, AlphaToClients);
}

void ASTrackerBot::OnRep_PowerLevel()
{
	/* Create a copy of MatInst on clients so they see the effects as well*/
	MatInst = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));
	
	if (MatInst) MatInst->SetScalarParameterValue("PowerLevelAlpha", AlphaToClients);
}
