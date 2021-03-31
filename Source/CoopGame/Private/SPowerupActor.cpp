// by Salim Pamukcu, 2021


#include "SPowerupActor.h"
#include "Net/UnrealNetwork.h"


ASPowerupActor::ASPowerupActor()
{

	PowerupInterval = 0.0f;
	TotalNrOfTicks = 0;
	TicksProcessed = 0;
	bIsPowerupActive = false;

	SetReplicates(true);

}


void ASPowerupActor::OnTickPowerup()
{
	++TicksProcessed;

	OnPowerupTicked();

	if (TicksProcessed >=TotalNrOfTicks)
	{
		OnExpired();

		bIsPowerupActive = false;
		OnRep_PowerupActive();

		GetWorldTimerManager().ClearTimer(TimerHandle_PowerupTick);
	}

}


void ASPowerupActor::OnRep_PowerupActive()
{	
		/* To Notify clients PowerupState; the effects implemented in BP*/
		OnPowerupStateChanged(bIsPowerupActive);
}

void ASPowerupActor::ActivatePowerup(AActor* ActivateFor)
{
	/* this is called by Server only ; in the PickupActor class!!!*/
	OnActivated(ActivateFor);
	
	bIsPowerupActive = true;
	OnRep_PowerupActive();

	if (PowerupInterval > 0)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_PowerupTick, this, &ASPowerupActor::OnTickPowerup, PowerupInterval, true);
	}
	else
	{
		OnTickPowerup();
	}

}

void ASPowerupActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASPowerupActor, bIsPowerupActive);
}

