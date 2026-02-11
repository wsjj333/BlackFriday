// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "COmponent/BFPhysicsNetTypes.h" // FBFRepState
#include "BFRepStateApplier.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UBFRepStateApplier : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class BLACKFRIDAY_API IBFRepStateApplier
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	// 서버/네트워크에서 받은 상태를 실제 물리에 적용
	// virtual void ApplyRepState(const FBFRepState& State);
};
