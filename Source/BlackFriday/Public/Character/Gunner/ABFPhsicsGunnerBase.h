// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "ABFPhsicsGunnerBase.generated.h"

class UInputMappingContext;
class UInputAction;
class UCapsuleComponent;
class USpringArmComponent;
class UCameraComponent;
class USkeletalMeshComponent;

UCLASS(BlueprintType)
class BLACKFRIDAY_API AABFPhsicsGunnerBase : public APawn
{
	GENERATED_BODY()

private:
	//매핑컨텍스트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	//인풋액션들
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LeftHandHold;

	//캡슐컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision", meta = (AllowPrivateAccess = "true"))
	UCapsuleComponent* CapsuleCollisionComp;

	//스켈레탈메시 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SkeletalMesh", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* SkeletalMeshComp;

	//스프링 암과 카메라
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	//폰이 공중에 떠 있나?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move", meta = (AllowPrivateAccess = "true"))
	bool Isfalling;

	//바닥을 향해 쏘는 라인트레이스 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move", meta = (AllowPrivateAccess = "true"))
	float GroundLineTraceDistance = -150;
	//바닥 라인 트레이스 디버그
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move", meta = (AllowPrivateAccess = "true"))
	bool IsActiveGroundLineTrace = true;

	//핸드의 타겟 로케이션
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HandMove", meta = (AllowPrivateAccess = "true"))
	FVector TargetLocation;

public:
	AABFPhsicsGunnerBase();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	//인풋액션 바인드용 함수
	void Move(const FInputActionValue& value);
	void Look(const FInputActionValue& value);
	void HandHoldOffset(const FInputActionValue& value);
	void HandHoldOStart(const FInputActionValue& value);
	void HandHoldOEnd(const FInputActionValue& value);

};

