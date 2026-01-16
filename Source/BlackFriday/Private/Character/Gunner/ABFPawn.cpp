// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Gunner/ABFPawn.h"

// Sets default values
AABFPawn::AABFPawn()
{
	PrimaryActorTick.bCanEverTick = true;
    //제로벡터로 초기화
	PendingInputVector = FVector::ZeroVector;
	CurrentVelocity = FVector::ZeroVector;
}

// Called when the game starts or when spawned
void AABFPawn::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AABFPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 1. 이번 프레임에 모인 입력을 가져옴
    FVector Input = Internal_ConsumeInputVector();
    Input = Input.GetClampedToSize(0.f, 1.f); // 대각선 속도 보정

    // 2. 속도(Velocity) 계산 (가속 및 마찰력 적용)
    if (!Input.IsNearlyZero()) //제로벡터가 아니라면(입력이 있다면)
    {
        // 입력이 있으면 그 방향으로 속도 증가
        CurrentVelocity = Input * MoveSpeed;
    }
    else
    {
        // 입력이 없으면 마찰력에 의해 서서히 감속
        CurrentVelocity = FMath::VInterpTo(CurrentVelocity, FVector::ZeroVector, DeltaTime, Friction);
    }

    // 3. 실제 이동 시도
    FVector DeltaLocation = CurrentVelocity * DeltaTime;

    if (!DeltaLocation.IsNearlyZero())
    {
        FHitResult Hit;
        // bSweep = true: 이동 경로에 충돌체가 있는지 확인하며 움직임
        AddActorWorldOffset(DeltaLocation, true, &Hit);

        // 4. 충돌 시 처리 (벽 타고 미끄러지기)
        if (Hit.IsValidBlockingHit())
        {
            // 부딪힌 면의 법선(Normal)을 기준으로 남은 이동 거리를 투영시킴
            FVector SlideDirection = FVector::VectorPlaneProject(DeltaLocation, Hit.Normal);
            AddActorWorldOffset(SlideDirection, true);

            // 벽에 부딪혔으니 해당 방향 속도는 죽임
            CurrentVelocity = FVector::VectorPlaneProject(CurrentVelocity, Hit.Normal);
        }
    }

}

// Called to bind functionality to input
void AABFPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AABFPawn::AddMovementInput(FVector WorldDirection, float ScaleValue, bool bForce)
{
		// 방향 * 크기를 해서 PendingInputVector에 계속 더해서 축적한다
		PendingInputVector += WorldDirection * ScaleValue;
}

FVector AABFPawn::Internal_ConsumeInputVector()
{
    //축적된 입력을 내보내는 함수
	FVector ReturnVector = PendingInputVector;
	PendingInputVector = FVector::ZeroVector; //내보냈으면 초기화
	return ReturnVector;
}

