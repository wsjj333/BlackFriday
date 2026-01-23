// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "BFCharacterAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class BLACKFRIDAY_API UBFCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FTransform HandleTargetL_CS;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FTransform HandleTargetR_CS;
};
