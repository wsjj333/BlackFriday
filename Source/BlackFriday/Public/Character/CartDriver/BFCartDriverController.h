#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BFCartDriverController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

UCLASS()
class BLACKFRIDAY_API ABFCartDriverController : public APlayerController
{
	GENERATED_BODY()

public:
	ABFCartDriverController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	// ===== Enhanced Input Assets (에디터에서 할당) =====
	UPROPERTY(EditDefaultsOnly, Category="Input|Enhanced")
	UInputMappingContext* DefaultMappingContext = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Input|Enhanced")
	int32 DefaultMappingPriority = 0;

	UPROPERTY(EditDefaultsOnly, Category="Input|Enhanced")
	UInputAction* IA_Move = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Input|Enhanced")
	UInputAction* IA_Look = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Input|Enhanced")
	UInputAction* IA_Drift = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Input|Enhanced")
	UInputAction* IA_Interact = nullptr;

private:
	// ===== Bind callbacks =====
	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);

	void OnDriftStarted(const FInputActionValue& Value);
	void OnDriftCompleted(const FInputActionValue& Value);

	void OnInteractStarted(const FInputActionValue& Value);
};
