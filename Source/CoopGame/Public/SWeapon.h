// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SWeapon.generated.h"


/** Contains SingleLineTrace info to be sent from server to other play FX */
USTRUCT()
struct FHitScanTrace  
{
	GENERATED_BODY()

public:
	/** zero decimal precision to be used sending less data to network; For efficiency */
	UPROPERTY()
	FVector_NetQuantize TraceFrom;

	UPROPERTY()
	FVector_NetQuantize TraceTo;
};


UCLASS()
class COOPGAME_API ASWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASWeapon();

	/** Returns the properties used for network replication, this needs to be overridden by all actor classes with native replicated properties */
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	class USkeletalMeshComponent* MeshComp;

	void PlayFireEffects(const FVector& TracerEndPoint) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	TSubclassOf<class UDamageType> DamageType;

	/** Shown in Editor but variable name specified in C++ constructor; it can be EditDefaultsOnly !!!
		FName is different than String (used for msg to display); FName is used for variable names
		also FTEXT used for menu texts; can be localized using international aplhabet
	*/
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName MuzzleSocketName;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName TracerTargetName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	class UParticleSystem* MuzzleEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	class UParticleSystem* DefaultImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	class UParticleSystem* FleshImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	class UParticleSystem* TracerEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<class UCameraShake> FireCamShake;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float BaseDamage;

	virtual void Fire();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFire();

	FTimerHandle TimerHandle_TimeBetweenShots;

	float LastFiredTime=0.0f;

	/* RPM; Bullets per minutes fired by weapon*/
	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (ClampMin = 60.0f))
	float RateofFire;

	// Derived from RateofFire
	float TimeBetweenShots=0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	bool bAutomaticWeapon;

	UPROPERTY(ReplicatedUsing=OnRep_HitScanTrace)
	FHitScanTrace HitScanTrace;

	UFUNCTION()
	void OnRep_HitScanTrace();
public:
	void StartFire();
	void StopFire();
	
};