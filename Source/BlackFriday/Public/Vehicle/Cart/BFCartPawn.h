#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "BFCartPawn.generated.h"

class USphereComponent;
class UInputAction;
class UInputMappingContext;
class UBoxComponent;
class UStaticMeshComponent;
class USceneComponent;

UCLASS()
class BLACKFRIDAY_API ABFCartPawn : public APawn
{
	GENERATED_BODY()

public:
	ABFCartPawn();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	// ----- Getter/Setter -----
	USceneComponent* GetPusherStandAnkerComponent() const;
	
	UFUNCTION(BlueprintCallable)
	FTransform GetHandleLTransform() const;
	
	UFUNCTION(BlueprintCallable)
	FTransform GetHandleRTransform() const;

	UFUNCTION(BlueprintCallable)
	float GetAcceleration() const;
	
	void SetAccelerationInput(const FInputActionValue& Value);
	void OnAccelerationEnded(const FInputActionValue& Value);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// ----- Physics / Movement -----
	void SuspensionCast(USceneComponent* WheelComp) const;
	bool IsOnGround() const;

	// 입력 처리(로컬)
	void SteerCart(const FInputActionValue& Value);
	void OnSteeringEnded(const FInputActionValue& Value);
	void OnMouseLook(const FInputActionValue& Value);
	
	// 카메라(로컬)
	void HardClampControlRotation();

	// 서버에서만 호출되는 물리 적용 루틴
	void ServerSimTick(float DeltaSeconds);
	void CalculateAcceleration(float DeltaSeconds);
	void AccelerateCart() const;

	// Cosmetic (클라에서 복제값 기반으로만)
	void RotateMeshes(float DeltaSeconds);

	// ----- RPCs -----
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetAccelerationAxis(float Axis);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetSteeringAxis(float Axis);

	// ----- Replicated State -----
	// “플레이어가 누르고 있는” 입력축 (서버 권한)
	UPROPERTY(Replicated)
	float Rep_AccelAxis = 0.0f;

	UPROPERTY(Replicated)
	float Rep_SteerAxis = 0.0f;

	// 서버가 만든 “물리/애니메이션용 스무딩 결과”
	UPROPERTY(Replicated)
	float Rep_AccelerationInput = 0.0f;

	UPROPERTY(Replicated)
	float Rep_Acceleration = 0.0f;

	UPROPERTY(Replicated)
	float Rep_DriftSteer = 0.0f;

	UPROPERTY(Replicated)
	FRotator Rep_DriftRotation = FRotator::ZeroRotator;

	// ----- Tunables -----
	float SpeedModifier = 1.0f;

	UPROPERTY(EditAnywhere, Category="BF|Movement")
	float DownForce = -4900000.0f;

	UPROPERTY(EditDefaultsOnly, Category="BF|Movement")
	double SteeringTorque = 90000000.0f;

	UPROPERTY(EditAnywhere, Category="BF|Movement")
	float SteeringMultiplier = 2.0f;

	UPROPERTY(EditAnywhere, Category="BF|Cart")
	float SuspensionForceMultiplier = 10000000.0f;

	UPROPERTY(EditAnywhere, Category="BF|Cart")
	float WheelRadius = 18.0f;

	UPROPERTY(EditAnywhere, Category="BF|Cart")
	FVector GroundTraceEnd = FVector(0.0f, 0.0f, 150.0f);

	UPROPERTY(EditDefaultsOnly, Category="BF|Movement")
	float MaxAcceleration = 15000.0f;

	UPROPERTY(EditAnywhere, Category="BF|Movement")
	float CartSpeed = 10000.0f;
	
	// 클라이언트(AnimInstance/코스메틱)에서 사용할 가속도 캐시
	UPROPERTY(BlueprintReadOnly, Category="Cart|Anim", Transient)
	float Acceleration = 0.0f;

	// ----- Input -----
	UPROPERTY(EditDefaultsOnly, Category = "BF|Input")
	TObjectPtr<UInputMappingContext> CartMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "BF|Input")
	TObjectPtr<UInputAction> AccelerationAction;

	UPROPERTY(EditDefaultsOnly, Category = "BF|Input")
	TObjectPtr<UInputAction> SteeringAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "BF|Input")
	TObjectPtr<UInputAction> LookAction;

	// ----- Components -----
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UBoxComponent> Root;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> CartBody;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> CartHandle;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USceneComponent> Pivot;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USceneComponent> WheelFRComp;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USceneComponent> WheelFLComp;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USceneComponent> WheelBRComp;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USceneComponent> WheelBLComp;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> WheelFRMesh;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> WheelFLMesh;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> WheelBRMesh;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> WheelBLMesh;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> CasterForkFRMesh;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> CasterForkFLMesh;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> CasterForkBRMesh;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> CasterForkBLMesh;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<USceneComponent> PusherStandAnker;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<USceneComponent> HandleL;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<USceneComponent> HandleR;
};
