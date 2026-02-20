// Fill out your copyright notice in the Description page of Project Settings.

#include "Component/Cart/BFCartInventoryComponent.h"
#include "Data/BFItemData.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"

UBFCartInventoryComponent::UBFCartInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UBFCartInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UBFCartInventoryComponent, ItemRowNames);
}

void UBFCartInventoryComponent::AddItem(FName ItemRowName)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (ItemRowName == NAME_None) return;

	ItemRowNames.Add(ItemRowName);
	OnRep_ItemRowNames();

	UE_LOG(LogTemp, Log, TEXT("[BFCartInventory] Item added: %s (Total: %d)"), *ItemRowName.ToString(), ItemRowNames.Num());
}

bool UBFCartInventoryComponent::RemoveItem(FName ItemRowName)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;

	int32 Index = ItemRowNames.Find(ItemRowName);
	if (Index == INDEX_NONE) return false;

	ItemRowNames.RemoveAt(Index);
	OnRep_ItemRowNames();
	return true;
}

void UBFCartInventoryComponent::ClearInventory()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	ItemRowNames.Empty();
	OnRep_ItemRowNames();
}

float UBFCartInventoryComponent::GetTotalPrice() const
{
	if (!ItemDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFCartInventory] GetTotalPrice - ItemDataTable is not set!"));
		return 0.0f;
	}

	float Total = 0.0f;
	for (const FName& RowName : ItemRowNames)
	{
		if (const FBFItemData* ItemData = ItemDataTable->FindRow<FBFItemData>(RowName, TEXT("GetTotalPrice"), false))
		{
			Total += ItemData->ItemPrice;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[BFCartInventory] Item row not found: %s"), *RowName.ToString());
		}
	}
	return Total;
}

void UBFCartInventoryComponent::OnRep_ItemRowNames()
{
	OnInventoryChanged.Broadcast();
}
