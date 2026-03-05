#include "Component/Common/BFCharacterAppearanceComponent.h"


// Engine
#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

// Character
#include "Character/Common/BFPawnBase.h"

// Data
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

	// 서버는 즉시 적용
	if (OwnerCharacter && OwnerCharacter->HasAuthority())
	{
		ApplyCharacterTypeInternal(CharacterType);
	}

	// 클라/서버 모두 현재 값 적용을 한 번 더 시도
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

void UBFCharacterAppearanceComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UBFCharacterAppearanceComponent, CharacterType);
}

void UBFCharacterAppearanceComponent::SetCharacterType(EBFCharacterType NewType)
{
	ABFPawnBase* Owner = GetOwnerCharacter();
	if (!Owner) return;

	if (!IsValidCharacterType(NewType)) return;

	// 서버는 즉시 확정 + 적용
	if (Owner->HasAuthority())
	{
		CharacterType = NewType;
		ApplyCharacterTypeInternal(CharacterType); // 서버도 동일 적용
		return;
	}

	// 클라는 “소유 로컬”에서만 서버에 요청
	if (!Owner->IsLocallyControlled())
	{
		return;
	}

	ServerSetCharacterType(NewType);
}

void UBFCharacterAppearanceComponent::ServerSetCharacterType_Implementation(EBFCharacterType NewType)
{
	if (!IsValidCharacterType(NewType)) return;

	CharacterType = NewType;
	OnRep_CharacterType();
}

void UBFCharacterAppearanceComponent::OnRep_CharacterType()
{
	ApplyCharacterTypeInternal(CharacterType);
}

void UBFCharacterAppearanceComponent::ApplyCharacterType(EBFCharacterType TypeToApply)
{
	(void)TryApplyCharacterType(TypeToApply);
}

void UBFCharacterAppearanceComponent::ApplyCharacterTypeDeferred()
{
	if (bGiveUpDeferredApply) return;
	
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
		bGiveUpDeferredApply = false;
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

		bGiveUpDeferredApply = true;
		DeferredApplyAttempts = 0;
		return;
	}

	// 다음 틱에 재시도
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			this,
			&UBFCharacterAppearanceComponent::ApplyCharacterTypeDeferred);
	}
}

void UBFCharacterAppearanceComponent::ApplyCharacterTypeInternal(EBFCharacterType Type)
{
	// 값이 바뀐 시점에 즉시 시도
	if (!TryApplyCharacterType(Type))
	{
		// 실패하면 다음 틱 재시도 (JIP/레이스 대응)
		ApplyCharacterTypeDeferred();
	}
}

bool UBFCharacterAppearanceComponent::TryApplyCharacterType(EBFCharacterType TypeToApply)
{
	if (!IsValidCharacterType(TypeToApply)) return false;

	USkeletalMeshComponent* MeshComp = ResolveMeshComponent();
	if (!MeshComp) return false;

	const TSoftObjectPtr<USkeletalMesh>* Found = FindMeshSoftPtr(TypeToApply);
	if (!Found) return false;

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
		const FString TypeName = Enum ? Enum->GetNameStringByValue(static_cast<int64>(Type)) : TEXT("InvalidEnum");

		UE_LOG(LogTemp, Warning, TEXT("[Appearance] Mesh not found for type %s(%d). MapNum=%d Owner=%s Data=%s"),
			*TypeName,
			static_cast<int32>(Type),
			AppearanceData->MeshMap.Num(),
			*GetNameSafe(GetOwner()),
			*AppearanceData->GetPathName());
		return nullptr;
	}

	return Found;
}

bool UBFCharacterAppearanceComponent::IsValidCharacterType(EBFCharacterType Type)
{
	if (Type == EBFCharacterType::None) return false;

	const UEnum* Enum = StaticEnum<EBFCharacterType>();
	if (!Enum) return false;

	return Enum->IsValidEnumValue(static_cast<int64>(Type));
}

USkeletalMeshComponent* UBFCharacterAppearanceComponent::ResolveMeshComponent() const
{
	const ABFPawnBase* Owner = GetOwnerCharacter();
	if (!Owner) return nullptr;

	return Owner->GetMesh();
}