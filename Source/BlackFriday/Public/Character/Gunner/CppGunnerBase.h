// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/Common/BFCharacterBase.h"
#include "CppGunnerBase.generated.h"

//class USpringArmComponent;
//class UCameraComponent;
//class UInputMappingContext;
//class UInputAction;
//struct FInputActionValue;

/**
 * 
 */
UCLASS()
class BLACKFRIDAY_API ACppGunnerBase : public ABFCharacterBase
{
	GENERATED_BODY()

public:
	/**
	 * 현재 손 위치에서 가장 가까운 원 위의 시작점으로 이동하기 위한 오프셋 계산
	 * @param CurrentHandLocation 현재 손의 월드 위치
	 * @param TorsoLocation 몸통(중심)의 월드 위치
	 * @param CircleRadius 원의 반지름 (기본값 50)
	 * @param Tolerance 이미 원 위에 있다고 판단하는 허용 오차 (기본값 1.0)
	 * @return 시작점으로 이동하기 위한 오프셋 벡터
	 */
	UFUNCTION(BlueprintCallable, Category = "Hand Offset")
	static FVector CalculateStartPositionOffset(
		const FVector& CurrentHandLocation,
		const FVector& TorsoLocation,
		const FVector& PlayerForward,
		const FVector& PlayerRight,  // 추가!
		float CircleRadius = 50.0f,
		float Tolerance = 1.0f
	);
	
	/**
	 * 마우스 좌우 입력에 따라 원 궤도를 따라 이동하기 위한 오프셋 계산
	 * @param CurrentHandLocation 현재 손의 월드 위치
	 * @param TorsoLocation 몸통(중심)의 월드 위치
	 * @param PlayerForward 플레이어의 전방 벡터 (정규화된 벡터)
	 * @param PlayerRight 플레이어의 우측 벡터 (정규화된 벡터) - 추가
	 * @param MouseDeltaX 마우스 좌우 델타 입력값
	 * @param CircleRadius 원의 반지름
	 * @param Sensitivity 회전 민감도
	 * @param DeltaTime 프레임 델타 타임
	 * @return 월드 공간에서의 오프셋 벡터
	 */
	UFUNCTION(BlueprintCallable, Category = "Hand Offset")
	static FVector CalculateCircularMovementOffset(
		const FVector& CurrentHandLocation,
		const FVector& TorsoLocation,
		const FVector& PlayerForward,
		const FVector& PlayerRight,  // 추가!
		float MouseDeltaX,
		float CircleRadius = 50.0f,
		float Sensitivity = 1.0f,
		float DeltaTime = 0.016f
	);



//private:
//	/**
//	 * 각도 제한 적용 (전방 180도 + 후방 90도)
//	 * @param Angle 현재 각도 (라디안)
//	 * @param ForwardAngle 전방 각도 기준 (라디안)
//	 * @return 제한된 각도 (라디안)
//	 */
//	static float ClampAngleToValidRange(float Angle, float ForwardAngle);


	///** Camera boom positioning the camera behind the character */
	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	//USpringArmComponent* CameraBoom;

	///** Follow camera */
	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	//UCameraComponent* FollowCamera;

	///** MappingContext */
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	//UInputMappingContext* DefaultMappingContext;

	///** Jump Input Action */
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	//UInputAction* JumpAction;

	///** Move Input Action */
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	//UInputAction* MoveAction;

	///** Look Input Action */
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	//UInputAction* LookAction;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand", meta = (AllowPrivateAccess = "true"))
	//bool IsHandMoving;


public:
	ACppGunnerBase();
//
//protected:
//
//	/** Called for movement input */
//	void Move(const FInputActionValue& Value);
//
//	/** Called for looking input */
//	void Look(const FInputActionValue& Value);
//
//
//protected:
//	// APawn interface
//	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
//
//	// To add mapping context
//	virtual void BeginPlay();
//
//public:
//	/** Returns CameraBoom subobject **/
//	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
//	/** Returns FollowCamera subobject **/
//	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
