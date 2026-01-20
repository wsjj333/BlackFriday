// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ABFPawn.generated.h"

UCLASS(BlueprintType)
class BLACKFRIDAY_API AABFPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AABFPawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void AddMovementInput(FVector WorldDirection, float ScaleValue = 1.0f, bool bForce = false) override;

private:
	// 1. 입력 축적용 변수
	FVector PendingInputVector;

	// 2. 이동 속성
	UPROPERTY(EditAnywhere)
	float MoveSpeed = 500.f;

	UPROPERTY(EditAnywhere)
	float Friction = 10.f; // 마찰력 (자연스러운 멈춤을 위해)

	FVector CurrentVelocity; // 현재 속도

	// 축적된 입력을 소비하는 내부 함수
	FVector Internal_ConsumeInputVector();



};
