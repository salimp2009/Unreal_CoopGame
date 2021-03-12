// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SPlayerInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class USPlayerInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class COOPGAME_API ISPlayerInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(Blueprintcallable, BlueprintNativeEvent, Category="PlayerInterface")
	bool HasDied() ;
};
