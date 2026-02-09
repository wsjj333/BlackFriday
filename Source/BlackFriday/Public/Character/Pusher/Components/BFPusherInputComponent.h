// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/BFInputSink.h"
#include "BFPusherInputComponent.generated.h"

class ABFPusher;
struct FInputActionValue;
class UInputAction;
class UInputMappingContext;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLACKFRIDAY_API UBFPusherInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UBFPusherInputComponent();
	
	/** ABFPusher::SetupPlayerInputComponent에서 호출 */
	void BindInput(UInputComponent* PlayerInputComponent);
	
	void EnsureMappingContext();
	
	// Sink 주입: Owner가 조립 시 호출
	void SetInputSink(const TScriptInterface<IBFInputSink>& InInputSink);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
	// ----- Input Assets -----
	UPROPERTY(EditDefaultsOnly, Category="BF|Control")
	TObjectPtr<UInputMappingContext> PusherMappingContext;

	UPROPERTY(EditDefaultsOnly, Category="BF|Control")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category="BF|Control")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category="BF|Control")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, Category="BF|Control")
	TObjectPtr<UInputAction> DriveModeAction;
	
	UPROPERTY(EditDefaultsOnly, Category="BF|Control")
	TObjectPtr<UInputAction> AccelerationAction;
	
	UPROPERTY(EditDefaultsOnly, Category="BF|Control")
	TObjectPtr<UInputAction> SteerAction;
	
	UPROPERTY(EditDefaultsOnly, Category="BF|Control")
	TObjectPtr<UInputAction> DriftAction;
	
private:
	// ----- Owner 캐시 -----
	UPROPERTY(Transient)
	TObjectPtr<ABFPusher> OwnerPusher;

	ABFPusher* GetOwnerPusher();
	
	// ----- Bound Functions -----
	void OnMoveInputTriggered(const FInputActionValue& Value);
	void OnMoveInputEnded();
	void HandleLookInput(const FInputActionValue& Value);
	void OnJumpPressed(const FInputActionValue& Value);
	void OnJumpReleased(const FInputActionValue& Value);
	void OnToggleDriveModePressed(const FInputActionValue& Value);
	
	// 카트 입력 라우팅용(기존에 EnhancedInput이 CartDrivingComp에 직접 바인딩하던 것)
	void OnAccelTriggered(const FInputActionValue& Value);
	void OnAccelEnded(const FInputActionValue& Value);
	void OnSteerTriggered(const FInputActionValue& Value);
	void OnSteerEnded(const FInputActionValue& Value);
	void OnDriftStarted(const FInputActionValue& Value);
	void OnDriftEnded(const FInputActionValue& Value);

	// 로컬만 MappingContext 추가(기존 BeginPlay 로직을 InputComp로 이동)
	void AddMappingContextIfLocal();
	
	// UInterface를 안전하게 들고 있게 해줌(GC 안전)
	UPROPERTY()
	TScriptInterface<IBFInputSink> InputSink;
};
