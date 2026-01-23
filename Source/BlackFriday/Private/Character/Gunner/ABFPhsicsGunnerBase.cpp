// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Gunner/ABFPhsicsGunnerBase.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

AABFPhsicsGunnerBase::AABFPhsicsGunnerBase()
{
	PrimaryActorTick.bCanEverTick = true;

	//스피어 컴포넌트 객체 생성
	CapsuleCollisionComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("SphereRoot"));
	RootComponent = CapsuleCollisionComp;
	//스프링 암과 카메라 생성
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true; //컨트롤러 기반으로 붐 회전
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FolowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	//스켈레탈 메시 생성
	SkeletalMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMeshComp->SetupAttachment(RootComponent);
}

void AABFPhsicsGunnerBase::BeginPlay()
{
	Super::BeginPlay();

	//서브시스템 셋팅
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
}

void AABFPhsicsGunnerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//시작 위치, 끝 위치
	//시작위치는 액터위치, 끝 위치는 아래로 -100
	FVector StartTracetLocation = GetActorLocation();
	FVector EndTraceLocation = StartTracetLocation + FVector(0, 0, GroundLineTraceDistance);

	FHitResult HitResult; //히트결과 저장

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this); //자기자신 무시

	//바닥 감지용 라인트레이스 쏘기
	Isfalling  = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartTracetLocation,
		EndTraceLocation,
		ECC_Visibility,
		Params
	);

	//디버그용 라인 그리기
	if(IsActiveGroundLineTrace)
		DrawDebugLine(GetWorld(), StartTracetLocation, EndTraceLocation, Isfalling ? FColor::Green : FColor::Red, false, 2.0f, 0, 1.0f);
}

void AABFPhsicsGunnerBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	//Super::SetupPlayerInputComponent(PlayerInputComponent);

	//인풋액션 바인드
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AABFPhsicsGunnerBase::Look);
		EnhancedInputComponent->BindAction(LeftHandHold, ETriggerEvent::Triggered, this, &AABFPhsicsGunnerBase::HandHoldOffset);
		EnhancedInputComponent->BindAction(LeftHandHold, ETriggerEvent::Started, this, &AABFPhsicsGunnerBase::HandHoldOStart);
		EnhancedInputComponent->BindAction(LeftHandHold, ETriggerEvent::Completed, this, &AABFPhsicsGunnerBase::HandHoldOEnd);
	}

}

//인풋액션 바인드용 함수
void AABFPhsicsGunnerBase::Look(const FInputActionValue& value)
{
	FVector2D LookAxisVector = value.Get<FVector2D>(); //XY값
	if (Controller != nullptr) //컨트롤러가 있으면
	{
		//시야회전
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}
void AABFPhsicsGunnerBase::Move(const FInputActionValue& value)
{

}
void AABFPhsicsGunnerBase::HandHoldOffset(const FInputActionValue& value)
{

}

void AABFPhsicsGunnerBase::HandHoldOStart(const FInputActionValue& value)
{
	//시작위치 설정 (현제 캐릭터의 위치에서 카메라의 200 앞 방향으로 위치한 곳)
	FVector StartLocation = GetActorLocation() + (FollowCamera->GetForwardVector() * 200);

	//현제 핸드 로케이션 가져오기

}

void AABFPhsicsGunnerBase::HandHoldOEnd(const FInputActionValue& value)
{
}
