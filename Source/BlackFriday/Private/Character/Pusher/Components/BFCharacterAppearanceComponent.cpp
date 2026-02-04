#include "Character/Pusher/Components/BFCharacterAppearanceComponent.h"

#include "Character/Common/BFCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"

UBFCharacterAppearanceComponent::UBFCharacterAppearanceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UBFCharacterAppearanceComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ABFCharacterBase>(GetOwner());

	// 서버가 소스: 서버는 BeginPlay 시점에 즉시 적용(클라는 OnRep로 적용)
	if (OwnerCharacter && OwnerCharacter->HasAuthority())
	{
		OnRep_CharacterType();
	}
}

ABFCharacterBase* UBFCharacterAppearanceComponent::GetOwnerCharacter() const
{
	return OwnerCharacter.Get();
}

void UBFCharacterAppearanceComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UBFCharacterAppearanceComponent, CharacterType);
}

// -------------------- Public API --------------------

void UBFCharacterAppearanceComponent::SetCharacterType(EBFCharacterType NewType)
{
	ABFCharacterBase* Owner = GetOwnerCharacter();
	if (!Owner)
	{
		return;
	}

	if (!IsValidCharacterType(NewType))
	{
		return;
	}

	// 서버는 즉시 확정 + 적용
	if (Owner->HasAuthority())
	{
		CharacterType = NewType;
		OnRep_CharacterType(); // 서버도 즉시 반영하고 싶으면 직접 호출
		return;
	}

	// 클라는 “소유 로컬”에서만 서버에 요청(원격 시뮬 프록시에서 RPC 방지)
	if (!Owner->IsLocallyControlled())
	{
		return;
	}

	ServerSetCharacterType(NewType);
}

// -------------------- RPC --------------------

void UBFCharacterAppearanceComponent::ServerSetCharacterType_Implementation(EBFCharacterType NewType)
{
	if (!IsValidCharacterType(NewType))
	{
		return;
	}

	CharacterType = NewType;
	OnRep_CharacterType();
}

// -------------------- OnRep / Apply --------------------

void UBFCharacterAppearanceComponent::OnRep_CharacterType()
{
	ApplyCharacterType(CharacterType);
}

void UBFCharacterAppearanceComponent::ApplyCharacterType(EBFCharacterType TypeToApply)
{
	if (!IsValidCharacterType(TypeToApply))
	{
		return;
	}

	USkeletalMeshComponent* MeshComp = ResolveMeshComponent();
	if (!MeshComp)
	{
		return;
	}

	TSoftObjectPtr<USkeletalMesh>* Found = CharacterMeshMap.Find(TypeToApply);
	if (!Found)
	{
		return;
	}

	USkeletalMesh* MeshAsset = Found->LoadSynchronous();
	if (!MeshAsset)
	{
		return;
	}

	MeshComp->SetSkeletalMeshAsset(MeshAsset);
	
	// 외형 적용 완료 이벤트 (서버/클라 모두)
	OnAppearanceApplied.Broadcast(TypeToApply);
}

bool UBFCharacterAppearanceComponent::IsValidCharacterType(EBFCharacterType Type) const
{
	if (Type == EBFCharacterType::None)
	{
		return false;
	}

	// Enum 유효성 체크(네 기존 코드 유지)
	const UEnum* Enum = StaticEnum<EBFCharacterType>();
	if (!Enum)
	{
		return false;
	}

	return Enum->IsValidEnumValue(static_cast<int64>(Type));
}

USkeletalMeshComponent* UBFCharacterAppearanceComponent::ResolveMeshComponent() const
{
	const ABFCharacterBase* Owner = GetOwnerCharacter();
	if (!Owner)
	{
		return nullptr;
	}

	// ABFCharacterBase가 ACharacter 기반이면 GetMesh() 사용 가능
	return Owner->GetMesh();
}
