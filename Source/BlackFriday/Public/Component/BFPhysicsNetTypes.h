#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "BFPhysicsNetTypes.generated.h"

USTRUCT(BlueprintType)
struct FBFMoveInputNet
{
	GENERATED_BODY();

	// 입력 양자화
	UPROPERTY() int16 MoveX = 0;
	UPROPERTY() int16 MoveY = 0;

	UPROPERTY() uint8 Buttons = 0;

	UPROPERTY() int16 ControlYaw100 = 0;

	UPROPERTY() uint16 ClientFrame = 0;
};

USTRUCT()
struct FBFPhysicsState
{
	GENERATED_BODY()

	UPROPERTY() FVector_NetQuantize100 Pos = FVector::ZeroVector;
	UPROPERTY() FRotator Rot = FRotator::ZeroRotator;

	UPROPERTY() FVector_NetQuantize10 LinVel = FVector::ZeroVector;
	UPROPERTY() FVector_NetQuantize10 AngVelDeg = FVector::ZeroVector;

	UPROPERTY() float ServerTime = 0.f;
};

inline int16 BF_PackAxis(float Axis)
{
	const float Clamped = FMath::Clamp(Axis, -1.f, 1.f);
	return (int16)FMath::RoundToInt(Clamped * 32767.f);
}

inline float BF_UnpackAxis(int16 Packed)
{
	return FMath::Clamp((float)Packed / 32767.f, -1.f, 1.f);
}

inline int16 BF_PackYaw100(float YawDegrees)
{
	float Norm = FRotator::NormalizeAxis(YawDegrees);
	return (int16)FMath::RoundToInt(Norm * 100.f);
}

inline float BF_UnpackYaw100(int16 PackedYaw100)
{
	return (float)PackedYaw100 / 100.f;
}
