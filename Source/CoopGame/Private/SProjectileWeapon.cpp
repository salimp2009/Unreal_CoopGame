// Fill out your copyright notice in the Description page of Project Settings.


#include "SProjectileWeapon.h"



void ASProjectileWeapon::Fire()
{

	if (!HasAuthority())
	{
		ServerFire();
	}
	AActor* MyOwner = GetOwner();
	if (MyOwner && ProjectileClass)
	{
		FVector EyeLocation;
		FRotator EyeRotation;
		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
		//FRotator MuzzleRotation = MeshComp->GetSocketRotation(MuzzleSocketName); // TOBE USED when AimOfset for Aiming is set
		
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		//SpawnParams.Owner = MyOwner;
		//SpawnParams.Instigator = MyOwner->GetInstigator();

		GetWorld()->SpawnActor<AActor>(ProjectileClass, MuzzleLocation, EyeRotation, SpawnParams);


	}

	
}

