// Developed by Salim Pamukcu 2021 following Tom Looman's class


#include "SWeapon.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles\ParticleSystem.h"
#include "Components\SkeletalMeshComponent.h"
#include "Particles\ParticleSystemComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "CoopGame/CoopGame.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"


/** Console Command to activate/deactivate Debug Line for Line Trace*/
static int32 DebugWeaponDrawing = 0;
FAutoConsoleVariableRef CVARDebugWeaponDrawing(
	TEXT("COOP.DebugWeapons"), 
	DebugWeaponDrawing, 
	TEXT("Draw Debug Lines for Weapons"), 
	ECVF_Cheat);

/**TODO Feature List to Add;
	- Add Ammo; (Must Have!!!)
	- Sound Effect when Fired and when Hit (Must Have!!!)
	- Add Destructible object or explode (optional)
	- Show Ammo in UMG and name of Rifle and add it to Players UMG!! (Must Have!!!)
*/

// Sets default values
ASWeapon::ASWeapon()
{
	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	MuzzleSocketName = "MuzzleSocket";
	TracerTargetName = "Target";

	BaseDamage = 20.0f;

	RateofFire = 600.0f;

	bAutomaticWeapon = true;

	SetReplicates(true);

	/* Default value is 100.0f in AActor InitializeDefaults()*/
	/* Changed to be able to automatic weapon update when hitting the same location otherwise clients dont see the updates*/
	NetUpdateFrequency = 66.0f;
	/* Default value is 2.0f in AActor InitializeDefaults()*/
	MinNetUpdateFrequency = 33.0f;

}

void ASWeapon::BeginPlay()
{
	Super::BeginPlay();

	/** Bullets per second (e.g: 10 bullets per sec) 
		Refactored because if RateofFire =0.0 then will give an infite or NAN and no bullets
		Can be deleted the RateofFire is clamped to 60 in the header so it cant be set lower than 650;
	*/
	
	if (bAutomaticWeapon)
	{
		if (RateofFire > 0.0f)
		{
			TimeBetweenShots = 60.0f / RateofFire;
		}
		else
		{
			TimeBetweenShots = 1.0f;
		}
	}
}


void ASWeapon::Fire()
{
	//Trace the world from PawnEye's location to Cross Hair Location
	
	if (!HasAuthority())
	{
		ServerFire();
	}

	AActor* MyOwner = GetOwner();
	if (MyOwner)
	{
		FVector EyeLocation;
		FRotator EyeRotation;
		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);
	
		FVector ShotDirection = EyeRotation.Vector();

		FVector TraceEnd = EyeLocation + (ShotDirection * 10000.0f);
		
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(MyOwner);
		QueryParams.AddIgnoredActor(this);
		QueryParams.bTraceComplex = true;    // to have exact location of hit; goes thru all Triangle faces; more expensive
		QueryParams.bReturnPhysicalMaterial = true;

		// Particle "Target" parameter
		FVector TracerEndPoint=TraceEnd;

		EPhysicalSurface SurfaceType = SurfaceType_Default;
		
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, COLLISION_WEAPON, QueryParams)) // COLLISION_WEAPON is macro defined CoopGame.h
		{
			//Blocking Hit true; Process Damage
			AActor* HitActor = Hit.GetActor();

			SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());

			float ActualDamage = BaseDamage;

			if (SurfaceType == SURFACE_FLESHVULNERABLE)
			{
				ActualDamage *= 4.0f;
			}

			UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, ShotDirection, Hit, MyOwner->GetInstigatorController(),this, DamageType);
			
			PlayImpactEffects(SurfaceType, Hit.ImpactPoint);
			
			TracerEndPoint = Hit.ImpactPoint;
		}

		/** Debug Lines will be drawn if activated via Console Command 
			"COOP.DebugWeaponsand" and set value to other than 0 
			e.g; COOP.DebugWeaponsand 1
			*/
		if (DebugWeaponDrawing>0)
		{
			DrawDebugLine(GetWorld(), EyeLocation, TraceEnd, FColor::White, false, 1.0f, 0, 1.0f);
		}

		PlayFireEffects(TracerEndPoint);

		if (HasAuthority())
		{
			HitScanTrace.TraceTo = TracerEndPoint;
			HitScanTrace.SurfaceType = SurfaceType;
			// Seed is incremented every update to prevent server not updating (UE4bug) when location does not change!!!
			// TODO ; check if +1 might not work if the player does not move long duration then add %2 (%3might be expensive)!!!
			HitScanTrace.Seed+=1;
		}

		if(bAutomaticWeapon) LastFiredTime = GetWorld()->TimeSeconds;
	}

}

// UE specific C++ implementation of ServerFire
void ASWeapon::ServerFire_Implementation()
{
	Fire();

}

// UE specific C++ implementation of ServerFire since Validate is used in UFUNCTION property
bool ASWeapon::ServerFire_Validate()
{
	return true;
}

void ASWeapon::OnRep_HitScanTrace()
{
	// Play FX
	PlayFireEffects(HitScanTrace.TraceTo);
	PlayImpactEffects(HitScanTrace.SurfaceType, HitScanTrace.TraceTo);
}

void ASWeapon::StartFire()
{
	if (bAutomaticWeapon)
	{
		float FirstDelay = FMath::Max(LastFiredTime + TimeBetweenShots - GetWorld()->TimeSeconds, 0.0f);
		GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &ASWeapon::Fire, TimeBetweenShots, true, FirstDelay);
	}
	else
	{
		Fire();
	}
}

void ASWeapon::StopFire()
{
	if (bAutomaticWeapon)
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);
	}
}


/** TODO; make FVector && and use std::forward<FVector>(TracerEndPoint) when calling the function; check if it is safe to steal the value */
void ASWeapon::PlayFireEffects(const FVector& TracerEndPoint) const
{
	// muzzle flash FX
	if (MuzzleEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);
	}

	if (TracerEffect)
	{
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
		UParticleSystemComponent* TracerComp = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TracerEffect, MuzzleLocation);

		if (TracerComp)
		{
			TracerComp->SetVectorParameter(TracerTargetName, TracerEndPoint);
		}

	}

	APawn* MyOwner = Cast<APawn>(GetOwner());
	if (MyOwner)
	{
		APlayerController* PC=Cast<APlayerController>(MyOwner->GetController());
		if (PC)
		{
			PC->ClientPlayCameraShake(FireCamShake);
		}
	}
	
}

void ASWeapon::PlayImpactEffects(EPhysicalSurface SurfaceType, const FVector& ImpactPoint) const
{

	UParticleSystem* SelectedEffect = nullptr;
	switch (SurfaceType)
	{
	case SURFACE_FLESHDEAFULT:				//SurfaceType1 macro define CoopGame.h
	case SURFACE_FLESHVULNERABLE:			//SurfaceType2:
		SelectedEffect = FleshImpactEffect;
		break;
	default:
		SelectedEffect = DefaultImpactEffect;
		break;
	}

	if (SelectedEffect)
	{
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
		FVector ShotDirection = ImpactPoint - MuzzleLocation;
		ShotDirection.Normalize();
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, ImpactPoint, ShotDirection.Rotation());
	}

}

void ASWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// The client who owns the weapon will be excluded from Replication; it will be sent to other clients to see each other FX
	DOREPLIFETIME_CONDITION(ASWeapon, HitScanTrace, COND_SkipOwner);
}

