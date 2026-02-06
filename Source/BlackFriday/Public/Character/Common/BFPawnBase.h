// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "BFPawnBase.generated.h"

class UCapsuleComponent;

UCLASS()
class BLACKFRIDAY_API ABFPawnBase : public APawn
{
	GENERATED_BODY()

public:
	ABFPawnBase();
	USkeletalMeshComponent* GetMesh() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="BF|Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USkeletalMeshComponent> Mesh;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="BF|Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCapsuleComponent> CapsuleComp;
};
