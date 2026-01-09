// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BFItemActor.generated.h"

UCLASS()
class BLACKFRIDAY_API ABFItemActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ABFItemActor();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BF|Component")
	UStaticMeshComponent* ItemMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BF|Physics")
	float MaxLinearVelocity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BF|Physics")
	float MaxAngularVelocity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BF|Physics")
	float LinearDamping;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BF|Physics")
	float AngularDamping;

	UFUNCTION(BlueprintCallable, Category = "BF|Interaction")
	void PickUp(AActor* Parent, FName Socketname);

	UFUNCTION(BlueprintCallable, Category = "BF|Interaction")
	void Throw(FVector ThrowVelocity, AActor* Thrower);

protected:
	FTimerHandle CollisionResetTimerHandle;

	void RestoreCollision(AActor* Thrower);
};
