#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/BFInputSink.h"
#include "BFPusherInputComponent.generated.h"

class ABFPusher;
class UInputAction;
class UInputComponent;
class UInputMappingContext;

struct FInputActionValue;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLACKFRIDAY_API UBFPusherInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// ----- Constructor -----
	UBFPusherInputComponent();
	
	// ----- Public API -----
	void BindInput(UInputComponent* PlayerInputComponent);
	
	void EnsureMappingContext();
	
	/** Sink 주입: Owner가 조립 시 호출 */
	void SetInputSink(const TScriptInterface<IBFInputSink>& InInputSink);

protected:
	// ----- UE Lifecycle -----
	virtual void BeginPlay() override;
	
private:
	// ----- Owner Cache -----
	ABFPusher* GetOwnerPusher();
	
	// ----- Mapping Context -----
	void AddMappingContextIfLocal();
	
	// ----- Bound callbacks: Character -----
	void OnMoveTriggered(const FInputActionValue& Value);
	void OnMoveEnded(const FInputActionValue& Value);
	void OnJumpPressed(const FInputActionValue& Value);
	void OnJumpReleased(const FInputActionValue& Value);
	void OnLookTriggered(const FInputActionValue& Value);
	
	// ----- Bound callbacks: Vehicle -----
	void OnToggleDriveModePressed(const FInputActionValue& Value);
	void OnAccelTriggered(const FInputActionValue& Value);
	void OnAccelEnded(const FInputActionValue& Value);
	void OnSteerTriggered(const FInputActionValue& Value);
	void OnSteerEnded(const FInputActionValue& Value);
	void OnDriftStarted(const FInputActionValue& Value);
	void OnDriftEnded(const FInputActionValue& Value);
	void OnRecoverCart(const FInputActionValue& Value);

private:
	// ----- Runtime refs -----
	UPROPERTY(Transient)
	TObjectPtr<ABFPusher> OwnerPusher;
	
	UPROPERTY()
	TScriptInterface<IBFInputSink> InputSink;
	
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
	
	UPROPERTY(EditDefaultsOnly, Category="BF|Control")
	TObjectPtr<UInputAction> CartRecoverAction;
};
