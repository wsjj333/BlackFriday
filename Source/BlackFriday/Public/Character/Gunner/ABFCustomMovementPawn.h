// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "ABFCustomMovementPawn.generated.h"

UCLASS(BlueprintType)
class BLACKFRIDAY_API AABFCustomMovementPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AABFCustomMovementPawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


    // Enhanced Input - Input Mapping Context
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enhanced Input")
    class UInputMappingContext* InputMappingContext;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enhanced Input")
    int32 InputMappingPriority = 0;

    // Enhanced Input - Input Actions
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enhanced Input")
    class UInputAction* MoveAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enhanced Input")
    class UInputAction* LookAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enhanced Input")
    class UInputAction* JumpAction;

    // 컴포넌트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UCapsuleComponent* CapsuleComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* MeshComponent;

    // 이동 파라미터
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MoveSpeed = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MaxAcceleration = 2048.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float GroundFriction = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float AirFriction = 0.1f;

    // 중력 파라미터
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
    float GravityScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
    float MaxFallSpeed = 1200.0f;

    // 점프 파라미터
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jump")
    float JumpVelocity = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jump")
    float AirControl = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jump")
    int32 MaxJumpCount = 1;

    // 지면 감지 파라미터
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Detection")
    float GroundTraceDistance = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Detection")
    float MaxWalkableSlope = 45.0f;

private:
    // Enhanced Input 콜백 함수
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void Jump(const FInputActionValue& Value);
    void StopJumping(const FInputActionValue& Value);

    // 이동 로직
    void UpdateMovement(float DeltaTime);
    void ApplyGravity(float DeltaTime);
    void ProcessMovementInput(float DeltaTime);
    bool PerformGroundTrace(FHitResult& OutHit);
    void ApplyFriction(float DeltaTime);
    void MoveAndSlide(float DeltaTime);

    // 상태 변수
    FVector Velocity;
    FVector InputVector;
    bool bIsGrounded;
    bool bWantsToJump;
    int32 JumpCount;
    FVector LastGroundNormal;

    // 최적화를 위한 캐시
    float GravityZ;
    static constexpr float MIN_TICK_TIME = 0.0001f;

};
