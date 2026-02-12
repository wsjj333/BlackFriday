#include "Component/BFNetworkPhysicsComponent.h"
#include "Component/BFPhysicsMovementComponent.h"
#include "Components/PrimitiveComponent.h"

/*
 * =========================
 * 클라이언트 → 로컬 입력 저장
 * =========================
 * 이 함수들은 "네트워크"와 직접 통신하지 않는다.
 * 오직 로컬(자기 PC)에서 발생한 입력을 임시로 저장하는 역할만 한다.
 * 
 * 실제 서버 전송은 BuildInputPacket()에서 이루어진다.
 */

// 이동 입력 (WASD, 스틱 등)
void UBFNetworkPhysicsComponent::SetMoveInput(FVector2D Move)
{
	// FVector2D에서 X는 좌/우, Y는 앞/뒤이므로
	// Z축이 없는 FVector처럼 사용하기 위해 X, Y값을 바꿔줌
	LocalMove = FVector2D(Move.Y, Move.X);
}

// 시야 회전(Yaw) 입력
void UBFNetworkPhysicsComponent::SetControlYawDegrees(float YawDegrees)
{
	LocalYaw = YawDegrees;
}

// 점프 버튼 입력
void UBFNetworkPhysicsComponent::SetJumpHeld(bool bHeld)
{
	bLocalJumpHeld = bHeld;

	// 점프는 "눌렀다"는 사실이 한 프레임만으로 사라지면 안 되므로
	// 한 번이라도 눌렸다면 서버로 확실히 전달되도록 래치(latch) 처리
	if (bHeld)
	{
		bJumpHoldLatched = true;
	}
}

/*
 * =========================
 * 입력 패킷 생성 (클라이언트)
 * =========================
 * 로컬 입력을 "네트워크 전송용 구조체"로 변환한다.
 * 
 * 포인트:
 * - float → int16 변환 (대역폭 절약)
 * - 클라이언트 프레임 번호 포함 (순서 보장용)
 */
FBFMoveInputNet UBFNetworkPhysicsComponent::BuildInputPacket() const
{
	FBFMoveInputNet Packet;

	// 이동 입력: -1~1 범위를 int16 범위(-32767~32767)로 압축
	Packet.MoveX = static_cast<int16>(
		FMath::Clamp(LocalMove.X, -1.f, 1.f) * 32767.f
	);
	Packet.MoveY = static_cast<int16>(
		FMath::Clamp(LocalMove.Y, -1.f, 1.f) * 32767.f
	);

	// 회전값(Yaw): 소수점 손실을 줄이기 위해 *100 해서 정수화
	Packet.ControlYaw100 = static_cast<int16>(
		FRotator::NormalizeAxis(LocalYaw) * 100.f
	);

	// 점프 버튼 플래그 (현재 0x01만 사용)
	if (bLocalJumpHeld || bJumpHoldLatched)
	{
		Packet.Buttons |= 0x01;
	}

	// 클라이언트 입력 프레임 번호
	// 서버에서 "이 입력이 최신인지" 판단할 때 사용
	Packet.ClientFrame = ClientFrameCounter;

	return Packet;
}

/*
 * =========================
 * 프레임 번호 비교 유틸
 * =========================
 * uint16은 65535 이후 다시 0으로 돌아간다.
 * 단순 비교(<, >)를 쓰면 오동작하므로
 * "랩어라운드"를 고려한 비교 함수를 사용한다.
 */
namespace
{
	bool IsNewerFrame(uint16 A, uint16 B)
	{
		// A - B 가 절반 범위(32768) 이내면 A가 더 최신
		return static_cast<uint16>(A - B) < 32768;
	}
}

/*
 * =========================
 * 서버: 클라이언트 입력 수신
 * =========================
 * 이 함수는 서버에서만 실행된다.
 * 클라이언트가 RPC로 입력을 보내면 여기로 도착한다.
 */
void UBFNetworkPhysicsComponent::ServerReceiveInput_Implementation(FBFMoveInputNet Input)
{
	// 이미 받은 입력이 있고,
	// 이번 입력이 "더 오래된 입력"이라면 무시
	if (bHasRecvClientFrame && !IsNewerFrame(Input.ClientFrame, LastRecvClientFrame))
	{
		return;
	}

	// 최신 입력으로 갱신
	bHasRecvClientFrame = true;
	LastRecvClientFrame = Input.ClientFrame;
	ServerInput = Input;

	// 서버 기준 이동 컴포넌트에 입력 반영
	if (MoveComp)
	{
		// 현재 입력 상태 저장
		MoveComp->SetCurrentInput(Input);

		// 즉시 물리 계산에 반영 (지연 최소화)
		MoveComp->ApplyInputImmediately(Input);
	}

	// 실제 물리 바디가 잠들어 있다면 깨워준다
	// (입력이 있는데 물리가 멈춰 있으면 반응이 없어 보이기 때문)
	if (Prim && (Input.MoveX != 0 || Input.MoveY != 0 || (Input.Buttons & 0x01)))
	{
		if (!Prim->IsSimulatingPhysics())
		{
			Prim->SetSimulatePhysics(true);
		}

		if (Prim->GetBodyInstance())
		{
			Prim->GetBodyInstance()->WakeInstance();
		}
	}
}

/*
 * =========================
 * 서버: 입력 검증
 * =========================
 * 클라이언트는 신뢰할 수 없기 때문에
 * 서버에서 반드시 입력값을 검증해야 한다.
 */
bool UBFNetworkPhysicsComponent::ServerReceiveInput_Validate(FBFMoveInputNet Input)
{
	// 정의되지 않은 버튼 플래그 차단
	// (해킹 또는 프로토콜 오류 방지)
	constexpr uint8 ValidButtonMask = 0x01;
	if (Input.Buttons & ~ValidButtonMask)
	{
		return false;
	}

	// 이동 입력 범위 체크
	if (Input.MoveX < -32767 || Input.MoveX > 32767 ||
		Input.MoveY < -32767 || Input.MoveY > 32767)
	{
		return false;
	}

	// 회전값 범위 체크 (-180 ~ 180도)
	if (Input.ControlYaw100 < -18000 || Input.ControlYaw100 > 18000)
	{
		return false;
	}

	return true;
}

/*
 * =========================
 * 복제된 속도 조회
 * =========================
 * - AutonomousProxy(내 캐릭터): 서버가 보내준 "Owner 전용 상태"
 * - 그 외(SimulatedProxy): 일반 복제 상태
 */
FVector UBFNetworkPhysicsComponent::GetReplicatedVelocity() const
{
	if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		return RepStateOwner.LinVel;
	}

	return RepState.LinVel;
}

/*
 * =========================
 * 물리 상태 적용 + 순간이동 판정
 * =========================
 * 서버 위치와 너무 멀리 떨어져 있으면
 * 보간(Smooth)하지 않고 즉시 텔레포트한다.
 */
void UBFNetworkPhysicsComponent::ApplyStateWithTeleportCheck(const FBFPhysicsState& NewState)
{
	PrevState = TargetState;
	TargetState = NewState;
	SmoothAlpha = 0.f;

	if (Prim)
	{
		float DistSq = FVector::DistSquared(
			TargetState.Pos,
			Prim->GetComponentLocation()
		);

		// 순간이동 거리 이상 차이나면 즉시 위치 보정
		if (DistSq > TeleportDist * TeleportDist)
		{
			Prim->SetWorldLocationAndRotation(
				TargetState.Pos,
				TargetState.Rot,
				false,
				nullptr,
				ETeleportType::TeleportPhysics
			);

			// 보간 완료 처리
			SmoothAlpha = 1.0f;
		}
	}
}

/*
 * =========================
 * 서버 → 클라이언트 상태 복제 콜백
 * =========================
 */

// 일반 클라이언트(SimulatedProxy)용 상태
void UBFNetworkPhysicsComponent::OnRep_PhysicsState()
{
	ApplyStateWithTeleportCheck(RepState);
}

// 오너 클라이언트(AutonomousProxy)용 상태
void UBFNetworkPhysicsComponent::OnRep_PhysicsStateOwner()
{
	bHasOwnerState = true;
	LastOwnerState = RepStateOwner;

	// 클라이언트 예측을 쓰지 않는 경우
	// 서버 상태를 그대로 적용
	if (!bUseClientPrediction)
	{
		ApplyStateWithTeleportCheck(RepStateOwner);
	}
}
