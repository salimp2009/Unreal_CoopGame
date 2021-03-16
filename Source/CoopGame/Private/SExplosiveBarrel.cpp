// Fill out your copyright notice in the Description page of Project Settings.


#include "SExplosiveBarrel.h"
#include "Components/SHealthComponent.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "Particles/ParticleSystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ASExplosiveBarrel::ASExplosiveBarrel()
{
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetSimulatePhysics(true);
	// when another barrel explodes nearby; it will effect other barrels!!
	MeshComp->SetCollisionObjectType(ECC_PhysicsBody); 
	RootComponent = MeshComp;
	
	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));
	HealthComp->OnHealthChanged.AddDynamic(this, &ASExplosiveBarrel::OnHealthChanged);

	RadialForceComp = CreateDefaultSubobject<URadialForceComponent>(TEXT("RadialForceComp"));
	RadialForceComp->SetupAttachment(MeshComp);
	RadialForceComp->Radius = 250.0f;
	RadialForceComp->bImpulseVelChange = true;
	RadialForceComp->bAutoActivate = false;  // to avoid ticking; will fire a when FireImpulse() is called
	RadialForceComp->bIgnoreOwningActor = true;

	ExplosionImpulse = 400.0f;
	SetReplicates(true);
	SetReplicateMovement(true);

}

void ASExplosiveBarrel::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASExplosiveBarrel, bExploded);
}

void ASExplosiveBarrel::OnHealthChanged(USHealthComponent* OwningHealthComp, float Health, float HealthDelta, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	
	if (bExploded)
	{
		return;
	}
	
	if (Health <= 0)
	{
		bExploded = true;
		OnRep_bExploded();

		// Push the mesh upwards
		FVector BoostIntensity = FVector::UpVector * ExplosionImpulse;
		MeshComp->AddImpulse(BoostIntensity, NAME_None, true);

		RadialForceComp->FireImpulse();

		//Radial Damage
		TArray<AActor*> IgnoredActors;
		IgnoredActors.Add(this);
		UGameplayStatics::ApplyRadialDamage(GetWorld(), 100.0f, GetActorLocation(), 500.0f, RadialDamageType, IgnoredActors, this, GetInstigatorController(), true, ECC_Visibility);

	}
}

void ASExplosiveBarrel::OnRep_bExploded()
{
	// Play particles FX
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionFX, GetActorLocation());
	MeshComp->SetMaterial(0, ExplodedMaterial);
}
