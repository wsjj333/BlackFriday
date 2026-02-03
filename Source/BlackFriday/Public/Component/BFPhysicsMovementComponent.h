// 물리 기반 Pawn 이동을 담당하는 컴포넌트
// - CharacterMovementComponent를 사용하지 않고
// - 물리 시뮬레이션(Force / Impulse) 기반 이동을 구현
// - 네트워크 환경에서 입력/보간/점프 동기화를 고려한 구조

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "BFPhysicsNetTypes.h"
#include "BFPhysicsMovementComponent.generated.h"

UCLASS(ClassGroup=(BF), meta=(BlueprintSpawnableComponent))
class BLACKFRIDAY_API UBFPhysicsMovementComponent : public UPawnMovementComponent
{
	GENERATED_BODY()

public:
	UBFPhysicsMovementComponent();

	// 컴포넌트 초기화 시점
	virtual void BeginPlay() override;

	// PrePhysics Tick에서 물리 힘/감쇠/회전 처리
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

	/**
	 * 네트워크/로컬 공통 입력 세팅
	 * - NetworkPhysicsComponent 또는 로컬 입력 시스템에서 호출
	 * - 실제 힘 적용은 Tick에서 수행
	 */
	void SetCurrentInput(const FBFMoveInputNet& InInput);

	/**
	 * 입력을 즉시 물리에 반영
	 * - 주 용도: 점프
	 * - 네트워크 보정 또는 서버 즉시 반응용
	 */
	void ApplyInputImmediately(const FBFMoveInputNet& InInput);
	
	// ================= Debug =================
	UPROPERTY(EditAnywhere, Category="BF|Debug")
	bool bDebugMove = false;

	// 디버그 로그 출력 주기
	UPROPERTY(EditAnywhere, Category="BF|Debug", meta=(ClampMin="0.01"))
	float DebugInterval = 0.25f;
	
	// ================= 이동 튜닝 =================
	UPROPERTY(EditAnywhere, Category="BF|Move")
	float RotationSpeed = 15.0f;
	
	// 루트가 아닌 다른 물리 컴포넌트를 이동 대상으로 쓰고 싶을 때
	UPROPERTY(EditAnywhere, Category="BF|Move")
	TObjectPtr<UPrimitiveComponent> PhysicsPrimitiveOverride = nullptr;

	UPROPERTY(EditAnywhere, Category="BF|Move")
	float MoveForce = 500000.f;

	// 공중에서의 입력 반영 비율
	UPROPERTY(EditAnywhere, Category="BF|Move")
	float AirControl = 0.35f;

	UPROPERTY(EditAnywhere, Category="BF|Move")
	float MaxSpeed = 700.f;
	
	// 가속도 보정용 배율
	UPROPERTY(EditAnywhere, Category="BF|Move")
	float AccelMultiplier = 2.5f;

	UPROPERTY(EditAnywhere, Category="BF|Move")
	float MovingLinearDamping = 3.0f;

	UPROPERTY(EditAnywhere, Category="BF|Move")
	float BrakingLinearDamping = 20.0f;
	
	UPROPERTY(EditAnywhere, Category="BF|Move")
	float JumpImpulse = 420.f;

	// ================= 지면 판정 =================
	UPROPERTY(EditAnywhere, Category="BF|Ground")
	float GroundTraceLength = 120.f;

	UPROPERTY(EditAnywhere, Category="BF|Ground")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

	// 바닥으로 인정할 법선 Z 최소값
	UPROPERTY(EditAnywhere, Category="BF|Ground")
	float GroundedDotThreshold = 0.6f;

	// ================= 상체 물리 애니메이션 =================
	UPROPERTY(EditAnywhere, Category="BF|PhysicalAnimation")
	bool bEnableUpperBodyPhysics = false;

	UPROPERTY(EditAnywhere, Category="BF|PhysicalAnimation")
	FName UpperBodyBoneName = TEXT("spine_02");

	UPROPERTY(EditAnywhere, Category="BF|PhysicalAnimation")
	float OrientationStrength = 1000.f;

	UPROPERTY(EditAnywhere, Category="BF|PhysicalAnimation")
	float AngularVelocityStrength = 100.f;

	UPROPERTY(EditAnywhere, Category="BF|PhysicalAnimation")
	float PositionStrength = 1000.f;

	UPROPERTY(EditAnywhere, Category="BF|PhysicalAnimation")
	float VelocityStrength = 100.f;

	// ================= 상태 =================
	UPROPERTY(BlueprintReadOnly, Category="BF|Ground")
	bool bGrounded = false;

	UPROPERTY(BlueprintReadOnly, Category="BF|Ground")
	FVector GroundNormal = FVector::UpVector;

	// 애니메이션에서 사용할 최종 속도
	UFUNCTION(BlueprintCallable, Category="BF|Motion")
	FVector GetBFVelocity() const;

	// 충돌 발생 시 네트워크 업데이트 트리거
	UFUNCTION()
	void OnComponentHit(
		UPrimitiveComponent* HitComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit
	);

protected:
	// ================= 캐싱 =================
	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> Prim = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> CachedMesh = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UPhysicalAnimationComponent> PhysicalAnimationComp = nullptr;

	// 실제 이동에 사용할 물리 컴포넌트 결정
	void CachePrimitive();

	// 상체 물리 애니메이션 초기 설정
	void SetupUpperBodyPhysics();

private:
	// ================= 입력 상태 =================
	float MoveX = 0.f;
	float MoveY = 0.f;
	float InputYawDeg = 0.f;

	bool bJumpHeld = false;
	bool bPrevJumpHeld = false;

	// ================= 내부 상태 =================
	float DebugAcc = 0.f;

	// SimulatedProxy에서 애니메이션 보간용 속도
	FVector SmoothAnimVelocity = FVector::ZeroVector;

	// 점프 입력 버퍼 (미세 타이밍 보정)
	float JumpBufferTime = 0.f;

	// 점프 쿨타임 (연속 점프 방지)
	float JumpCooldownTime = 0.f;
};
