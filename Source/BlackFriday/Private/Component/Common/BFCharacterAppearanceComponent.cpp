#include "Component/Common/BFCharacterAppearanceComponent.h"

#include "Character/Common/BFPawnBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

// DataAsset
#include "Data/BFCharacterAppearanceData.h"

UBFCharacterAppearanceComponent::UBFCharacterAppearanceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UBFCharacterAppearanceComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ABFPawnBase>(GetOwner());

	// ✅ 서버는 즉시 적용 (기존 로직 유지)
	if (OwnerCharacter && OwnerCharacter->HasAuthority())
	{
		OnRep_CharacterType();
	}

	// ✅ 클라/서버 모두 "현재값" 적용을 한 번 더 시도
	// - JIP에서 OnRep 호출 타이밍이 메시 컴포넌트/데이터 준비보다 빠를 수 있어
	// - 실패 시 다음 틱 재시도로 결국 맞춤
	ApplyCharacterTypeDeferred();
}

void UBFCharacterAppearanceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 타이머 정리
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeferredApplyHandle);
	}

	Super::EndPlay(EndPlayReason);
}

ABFPawnBase* UBFCharacterAppearanceComponent::GetOwnerCharacter() const
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
	ABFPawnBase* Owner = GetOwnerCharacter();
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
		OnRep_CharacterType();
		return;
	}

	// 클라는 “소유 로컬”에서만 서버에 요청
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
	// 값이 바뀐 시점에 즉시 시도
	if (!TryApplyCharacterType(CharacterType))
	{
		// 실패하면 다음 틱 재시도 (JIP/레이스 대응)
		ApplyCharacterTypeDeferred();
	}
}

void UBFCharacterAppearanceComponent::ApplyCharacterType(EBFCharacterType TypeToApply)
{
	(void)TryApplyCharacterType(TypeToApply);
}

void UBFCharacterAppearanceComponent::ApplyCharacterTypeDeferred()
{
	// 이미 재시도 예약된 상태면 중복 예약 방지
	if (UWorld* World = GetWorld())
	{
		if (World->GetTimerManager().IsTimerActive(DeferredApplyHandle))
		{
			return;
		}
	}

	// 즉시 한 번 시도
	if (TryApplyCharacterType(CharacterType))
	{
		DeferredApplyAttempts = 0;
		return;
	}

	// 무한 루프 방지
	DeferredApplyAttempts++;
	if (DeferredApplyAttempts > MaxDeferredApplyAttempts)
	{
		const UEnum* Enum = StaticEnum<EBFCharacterType>();
		const FString TypeName = Enum ? Enum->GetNameStringByValue((int64)CharacterType) : TEXT("InvalidEnum");

		UE_LOG(LogTemp, Warning, TEXT("[Appearance] Deferred apply exceeded. Type=%s(%d) Owner=%s AppearanceData=%s"),
			*TypeName, (int32)CharacterType, *GetNameSafe(GetOwner()), *GetNameSafe(AppearanceData));

		DeferredApplyAttempts = 0;
		return;
	}

	// 다음 틱에 재시도
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DeferredApplyHandle,
			this,
			&UBFCharacterAppearanceComponent::ApplyCharacterTypeDeferred,
			0.0f,
			false
		);
	}
}

bool UBFCharacterAppearanceComponent::TryApplyCharacterType(EBFCharacterType TypeToApply)
{
	if (!IsValidCharacterType(TypeToApply))
	{
		return false;
	}

	USkeletalMeshComponent* MeshComp = ResolveMeshComponent();
	if (!MeshComp)
	{
		return false;
	}

	const TSoftObjectPtr<USkeletalMesh>* Found = FindMeshSoftPtr(TypeToApply);
	if (!Found)
	{
		// FindMeshSoftPtr에서 상세 로그를 찍으므로 여기서는 조용히 실패 처리
		return false;
	}

	USkeletalMesh* MeshAsset = Found->LoadSynchronous();
	if (!MeshAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Appearance] Failed to load mesh. Type=%d SoftPath=%s Owner=%s"),
			(int32)TypeToApply, *Found->ToSoftObjectPath().ToString(), *GetNameSafe(GetOwner()));
		return false;
	}

	MeshComp->SetSkeletalMeshAsset(MeshAsset);
	OnAppearanceApplied.Broadcast(TypeToApply);
	return true;
}

const TSoftObjectPtr<USkeletalMesh>* UBFCharacterAppearanceComponent::FindMeshSoftPtr(EBFCharacterType Type) const
{
	if (!AppearanceData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Appearance] AppearanceData is null. Owner=%s"), *GetNameSafe(GetOwner()));
		return nullptr;
	}

	const TSoftObjectPtr<USkeletalMesh>* Found = AppearanceData->MeshMap.Find(Type);
	if (!Found)
	{
		const UEnum* Enum = StaticEnum<EBFCharacterType>();
		const FString TypeName = Enum ? Enum->GetNameStringByValue((int64)Type) : TEXT("InvalidEnum");

		UE_LOG(LogTemp, Warning, TEXT("[Appearance] Mesh not found for type %s(%d). MapNum=%d Owner=%s Data=%s"),
			*TypeName, (int32)Type, AppearanceData->MeshMap.Num(), *GetNameSafe(GetOwner()), *AppearanceData->GetPathName());
		return nullptr;
	}

	return Found;
}

bool UBFCharacterAppearanceComponent::IsValidCharacterType(EBFCharacterType Type) const
{
	if (Type == EBFCharacterType::None)
	{
		return false;
	}

	const UEnum* Enum = StaticEnum<EBFCharacterType>();
	if (!Enum)
	{
		return false;
	}

	return Enum->IsValidEnumValue(static_cast<int64>(Type));
}

USkeletalMeshComponent* UBFCharacterAppearanceComponent::ResolveMeshComponent() const
{
	const ABFPawnBase* Owner = GetOwnerCharacter();
	if (!Owner)
	{
		return nullptr;
	}

	return Owner->GetMesh();
}