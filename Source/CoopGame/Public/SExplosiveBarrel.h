// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SExplosiveBarrel.generated.h"

UCLASS()
class COOPGAME_API ASExplosiveBarrel : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASExplosiveBarrel();

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


protected:

	UPROPERTY(VisibleAnywhere, Category = "Components")
	class UStaticMeshComponent* MeshComp;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	class USHealthComponent* HealthComp;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	class URadialForceComponent* RadialForceComp;

	UFUNCTION()
	void OnHealthChanged(USHealthComponent* OwningHealthComp, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	UPROPERTY(ReplicatedUsing = OnRep_bExploded)
	bool bExploded = false;

	UFUNCTION()
	void OnRep_bExploded();

	UPROPERTY(EditDefaultsOnly, Category = "FX")
	float ExplosionImpulse;

	/* will play when Health is zero*/
	UPROPERTY(EditDefaultsOnly, Category="FX")
	class UParticleSystem* ExplosionFX;

	/* Material to replace the original material when Health iz zero*/
	UPROPERTY(EditDefaultsOnly, Category = "FX")
	class UMaterialInterface* ExplodedMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "RadialDamage")
	TSubclassOf<class UDamageType> RadialDamageType;
};