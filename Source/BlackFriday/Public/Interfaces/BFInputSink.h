#pragma once

#include "Component/BFPhysicsNetTypes.h"
#include "UObject/Interface.h"
#include "BFInputSink.generated.h"

UINTERFACE(BlueprintType)
class UBFInputSink : public UInterface
{
	GENERATED_BODY()
};

class IBFInputSink
{
	GENERATED_BODY()

public:
	// "점프 버튼을 누르고 있는지" 상태 전달 (Pressed/Released)
	virtual void SetMoveInput(FVector2D Move) = 0;
	virtual void SetControlYawDegrees(float YawDegrees) = 0;
	virtual void SetJumpHeld(bool bHeld) = 0;
	virtual void ServerReceiveInput(FBFMoveInputNet Input) = 0;
};