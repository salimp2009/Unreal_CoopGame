// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SPowerupActor.generated.h"

UCLASS()
class COOPGAME_API ASPowerupActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ASPowerupActor();

protected:
	
	/* Time between powerup applies/ticks */
	UPROPERTY(EditDefaultsOnly, Category="Powerups")
	float PowerupInterval;
	
	/* Total time the effect is applied*/
	UPROPERTY(EditDefaultsOnly, Category = "Powerups")
	int32 TotalNrOfTicks;

	FTimerHandle TimerHandle_PowerupTick;

	UFUNCTION()
	void OnTickPowerup();

	int32 TicksProcessed;

	UPROPERTY(ReplicatedUsing=OnRep_PowerupActive)
	bool bIsPowerupActive;

	UFUNCTION()
	void OnRep_PowerupActive();

	UFUNCTION(BlueprintImplementableEvent, Category = "Powerups")
	void OnPowerupStateChanged(bool bNewIsActive);

public:	

	void ActivatePowerup(AActor* ActivateFor);

	/*Only implemented in BP; NO implementation in C++*/
	UFUNCTION(BlueprintImplementableEvent, Category="Powerups")
	void OnActivated(AActor* ActivateFor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Powerups")
	void OnPowerupTicked();

	UFUNCTION(BlueprintImplementableEvent, Category = "Powerups")
	void OnExpired();

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

};
