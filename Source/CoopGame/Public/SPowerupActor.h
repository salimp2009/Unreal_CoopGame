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
	virtual void BeginPlay() override;

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

public:	

	void ActivatePowerup();

	/*Only implemented in BP; NO implementation in C++*/
	UFUNCTION(BlueprintImplementableEvent, Category="Powerups")
	void OnActivated();

	UFUNCTION(BlueprintImplementableEvent, Category = "Powerups")
	void PowerupTicked();

	UFUNCTION(BlueprintImplementableEvent, Category = "Powerups")
	void OnExpired();
};
