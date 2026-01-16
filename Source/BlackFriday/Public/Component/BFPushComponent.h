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
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BF|Physics")
	float PushStrength;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BF|Physics")
	float PushRange;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BF|Physics")
	float PushInterval;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BF|Physics")
	bool bFlattenZ;

private:
	float LastPushTime;

	float ServerLastPushTime;
	
	UPROPERTY()
	AActor* CurrentIgnoredActor;
	
	UFUNCTION(Server, Reliable)
	void Server_ApplyPush(UPrimitiveComponent* HitComp, FVector PushForce, FVector Location);
};
