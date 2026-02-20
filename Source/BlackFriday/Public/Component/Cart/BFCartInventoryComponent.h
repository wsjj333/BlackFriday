// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BFCartInventoryComponent.generated.h"

class UDataTable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCartInventoryChanged);

/**
 * 카트 인벤토리 컴포넌트
 * 카트에 담긴 아이템(Row 이름)을 관리하고 총 결제금액을 계산합니다.
 *
 * TODO: 아이템이 카트에 담기는 방식(Overlap / PickUp 등)은 팀 논의 후 결정
 *       AddItem() 호출 시점만 구현하면 이 컴포넌트는 그대로 사용 가능
 */
UCLASS(ClassGroup=(BF), meta=(BlueprintSpawnableComponent))
class BLACKFRIDAY_API UBFCartInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBFCartInventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 아이템 추가 (서버에서만 호출)
	UFUNCTION(BlueprintCallable, Category = "BF|Inventory")
	void AddItem(FName ItemRowName);

	// 아이템 제거 - 동일 Row 이름 중 1개 제거 (서버에서만 호출)
	UFUNCTION(BlueprintCallable, Category = "BF|Inventory")
	bool RemoveItem(FName ItemRowName);

	// 인벤토리 전체 비우기 (결제 완료 후 호출)
	UFUNCTION(BlueprintCallable, Category = "BF|Inventory")
	void ClearInventory();

	// DataTable 기반 총 결제금액 계산
	UFUNCTION(BlueprintCallable, Category = "BF|Inventory")
	float GetTotalPrice() const;

	// 담긴 아이템 Row 이름 목록
	UFUNCTION(BlueprintPure, Category = "BF|Inventory")
	const TArray<FName>& GetItemRowNames() const { return ItemRowNames; }

	// 아이템 총 개수
	UFUNCTION(BlueprintPure, Category = "BF|Inventory")
	int32 GetItemCount() const { return ItemRowNames.Num(); }

	// DataTable 설정 (GameMode 또는 에디터에서 주입)
	UFUNCTION(BlueprintCallable, Category = "BF|Inventory")
	void SetItemDataTable(UDataTable* InDataTable) { ItemDataTable = InDataTable; }

	// 인벤토리 변경 시 (아이템 추가/제거/초기화) 브로드캐스트
	UPROPERTY(BlueprintAssignable, Category = "BF|Inventory")
	FOnCartInventoryChanged OnInventoryChanged;

protected:
	// 카트에 담긴 아이템 Row 이름 목록 (중복 허용 - 같은 아이템 여러 개 가능)
	UPROPERTY(ReplicatedUsing = OnRep_ItemRowNames, BlueprintReadOnly, Category = "BF|Inventory")
	TArray<FName> ItemRowNames;

	// 아이템 가격 DataTable (에디터에서 설정하거나 런타임에 주입)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BF|Inventory")
	TObjectPtr<UDataTable> ItemDataTable;

	UFUNCTION()
	void OnRep_ItemRowNames();
};
