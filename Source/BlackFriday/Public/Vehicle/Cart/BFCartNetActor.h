// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BFCartNetActor.generated.h"

UCLASS()
class BLACKFRIDAY_API ABFCartNetActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ABFCartNetActor();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BF|Component")
	UStaticMeshComponent* CartMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BF|Network", meta = (ClampMin = "0.1", ClampMax = "100.0"))
	float InterpSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BF|Network")
	float TeleportThreshold;

protected:

	UPROPERTY(ReplicatedUsing=OnRep_ServerTransform)
	FTransform ServerTransform;

	FTransform TargetTransform;

	UFUNCTION()
	void OnRep_ServerTransform();
};
