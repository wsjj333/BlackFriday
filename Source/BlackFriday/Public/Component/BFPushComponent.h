// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BFPushComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLACKFRIDAY_API UBFPushComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UBFPushComponent();

protected:
	virtual void BeginPlay() override;

public:	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BF|Physics")
	float PushStrength;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BF|Physics")
	float MaxForceLimit;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BF|Physics")
	bool bFlattenZ;

private:
	UFUNCTION()
	void OnOwnerHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit);	
};
