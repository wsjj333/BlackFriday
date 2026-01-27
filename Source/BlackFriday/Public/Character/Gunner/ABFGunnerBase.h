// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/Common/BFCharacterBase.h"
#include "ABFGunnerBase.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

UCLASS(BlueprintType)
class BLACKFRIDAY_API AABFGunnerBase : public ABFCharacterBase
{
	GENERATED_BODY()
	
public:
	AABFGunnerBase();

protected:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override; // APawn interface
	virtual void BeginPlay();

protected:
	//조작
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom; //카메라 붐
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera; //카메라

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	//인풋들
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

public:
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; } // 카메라 붐 Getter
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; } //카메라 Getter

};
