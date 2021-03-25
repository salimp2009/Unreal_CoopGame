// by Salim Pamukcu, 2021


#include "SPowerupActor.h"


ASPowerupActor::ASPowerupActor()
{

	PowerupInterval = 0.0f;
	TotalNrOfTicks = 0;
	TicksProcessed = 0;
}


void ASPowerupActor::BeginPlay()
{
	Super::BeginPlay();
	
}

void ASPowerupActor::OnTickPowerup()
{
	++TicksProcessed;

	OnPowerupTicked();

	if (TicksProcessed >=TotalNrOfTicks)
	{
		OnExpired();
		GetWorldTimerManager().ClearTimer(TimerHandle_PowerupTick);
	}

}

void ASPowerupActor::ActivatePowerup()
{
	if (PowerupInterval > 0)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_PowerupTick, this, &ASPowerupActor::OnTickPowerup, PowerupInterval, true, 0.0f);
	}
	else
	{
		OnTickPowerup();
	}

}

