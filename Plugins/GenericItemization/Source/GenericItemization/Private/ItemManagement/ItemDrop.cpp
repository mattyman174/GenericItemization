// Copyright Fissure Entertainment, Pty Ltd. All Rights Reserved.

#include "ItemManagement/ItemDrop.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

AItemDrop::AItemDrop()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bReplicates = true;
}

void AItemDrop::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;

	Params.Condition = COND_InitialOnly;
	DOREPLIFETIME_WITH_PARAMS_FAST(AItemDrop, ItemInstance, Params);
}

void AItemDrop::GetItemInstance(TInstancedStruct<FItemInstance>& OutItemInstance) const
{
	OutItemInstance = ItemInstance;
}

void AItemDrop::GetItemInstanceStruct(FItemInstance& OutItemInstanceStruct) const
{
	if (ItemInstance.IsValid())
	{
		OutItemInstanceStruct = ItemInstance.Get();
	}
}

void AItemDrop::GetItemDefinitionStruct(FItemDefinition& OutItemDefinitionStruct) const
{
	if (ItemInstance.IsValid() && ItemInstance.Get().ItemDefinition.IsValid())
	{
		OutItemDefinitionStruct = ItemInstance.Get().ItemDefinition.Get();
	}
}

bool AItemDrop::HasValidItemInstance() const
{
	return ItemInstance.IsValid() && ItemInstance.Get().IsValid();
}

bool AItemDrop::CanTakeItem_Implementation(UItemInventoryComponent* InventoryComponent) const
{
	return true;
}
