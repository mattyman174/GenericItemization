#include "GenericItemizationInstanceTypes.h"
#include "ItemManagement/ItemInventoryComponent.h"

FItemInstance::FItemInstance()
{
	ItemId = FGuid::NewGuid();
	ItemSeed = -1;
	ItemLevel = -1;
	AffixLevel = -1;
	StackCount = 1;
}

bool FItemInstance::HasAnyAffixOfType(const FGameplayTag& AffixType) const
{
	for (const TInstancedStruct<FAffixInstance>& AffixInstance : Affixes)
	{
		if (AffixInstance.IsValid() && AffixInstance.Get().AffixDefinition.IsValid())
		{
			if (AffixType.MatchesTag(AffixInstance.Get().AffixDefinition.Get().AffixType))
			{
				return true;
			}
		}
	}

	return false;
}

bool FItemInstance::IsValid() const
{
	return ItemSeed != -1;
}

void FFastItemInstancesContainer::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
	for (const int32& Index : AddedIndices)
	{
		const FFastItemInstance& FastItemInstance = ItemInstances[Index];
		const FInstancedStruct& PostAddItemInstance = FastItemInstance.ItemInstance;
		if (Owner)
		{
			Owner->OnAddedItemInstance(FastItemInstance);
		}
	}
}

void FFastItemInstancesContainer::PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
{
	for (const int32& Index : ChangedIndices)
	{
		const FFastItemInstance& FastItemInstance = ItemInstances[Index];
		const FInstancedStruct& PostChangeItemInstance = FastItemInstance.ItemInstance;
		if (Owner)
		{
			Owner->OnChangedItemInstance(FastItemInstance);
		}
	}
}

void FFastItemInstancesContainer::PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
	for (const int32& Index : RemovedIndices)
	{
		const FFastItemInstance& FastItemInstance = ItemInstances[Index];
		const FInstancedStruct& PreRemoveItemInstance = FastItemInstance.ItemInstance;
		if (Owner)
		{
			Owner->OnRemovedItemInstance(FastItemInstance);
		}
	}
}

void FFastItemInstancesContainer::Register(UItemInventoryComponent* InOwner)
{
	if (Owner != InOwner && InOwner != nullptr)
	{
		Owner = InOwner;
		bOwnerIsNetAuthority = Owner->HasAuthority();
	}
}

void FFastItemInstancesContainer::AddItemInstance(const FInstancedStruct& ItemInstance, const FInstancedStruct& UserContextData)
{
	if (!Owner)
	{
		return;
	}

	FFastItemInstance FastItemInstance;
	FastItemInstance.ItemInstance = ItemInstance;
	FastItemInstance.UserContextData = UserContextData;

	ItemInstances.Add(FastItemInstance);
	MarkItemDirty(FastItemInstance);

	Owner->OnAddedItemInstance(FastItemInstance);
}

bool FFastItemInstancesContainer::RemoveItemInstance(const FGuid& ItemInstance)
{
	for (int32 i = ItemInstances.Num() - 1; i >= 0; --i)
	{
		const FItemInstance* ItemInstancePtr = ItemInstances[i].ItemInstance.GetPtr<FItemInstance>();
		if (ItemInstancePtr && ItemInstancePtr->IsValid() && ItemInstancePtr->ItemId == ItemInstance)
		{
			FFastItemInstance OldItemInstance = ItemInstances[i];
			ItemInstances.RemoveAt(i);
			MarkArrayDirty();

			Owner->OnRemovedItemInstance(OldItemInstance);
			return true;
		}
	}

	return false;
}

TArray<FInstancedStruct> FFastItemInstancesContainer::GetItemInstances() const
{
	TArray<FInstancedStruct> Result;
	Result.Reserve(ItemInstances.Num());

	for (const FFastItemInstance& ItemInstance : ItemInstances)
	{
		Result.Add(ItemInstance.ItemInstance);
	}

	return Result;
}

FFastItemInstance FFastItemInstancesContainer::GetItemInstance(const FGuid& Item, bool& bSuccessful) const
{
	for (const FFastItemInstance& FastItemInstance : ItemInstances)
	{
		const FItemInstance* ItemInstancePtr = FastItemInstance.ItemInstance.GetPtr<FItemInstance>();
		if (ItemInstancePtr && ItemInstancePtr->ItemId == Item)
		{
			bSuccessful = true;
			return FastItemInstance;
		}
	}

	bSuccessful = false;
	return FFastItemInstance();
}

FFastItemInstance* FFastItemInstancesContainer::GetMutableItemInstance(const FGuid& Item)
{
	for (FFastItemInstance& ItemInstance : ItemInstances)
	{
		const FItemInstance* ItemInstancePtr = ItemInstance.ItemInstance.GetPtr<FItemInstance>();
		if (ItemInstancePtr && ItemInstancePtr->ItemId == Item)
		{
			MarkItemDirty(ItemInstance);
			return &ItemInstance;
		}
	}

	return nullptr;
}

int32 FFastItemInstancesContainer::GetNum() const
{
	return ItemInstances.Num();
}
