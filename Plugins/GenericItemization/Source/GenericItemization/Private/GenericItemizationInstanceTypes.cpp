#include "GenericItemizationInstanceTypes.h"
#include "ItemManagement/ItemInventoryComponent.h"

FItemInstance::FItemInstance()
{
	ItemId = FGuid::NewGuid();
	ItemSeed = -1;
	ItemLevel = -1;
	AffixLevel = -1;
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

void FFastItemInstancesContainer::GetItemInstances(TArray<FInstancedStruct>& OutItemInstances) const
{
	OutItemInstances.Empty(ItemInstances.Num());

	for (const FFastItemInstance& ItemInstance : ItemInstances)
	{
		OutItemInstances.Add(ItemInstance.ItemInstance);
	}
}

bool FFastItemInstancesContainer::GetFastItemInstance(const FGuid& ItemInstance, FFastItemInstance& OutFastItemInstance) const
{
	for (const FFastItemInstance& FastItemInstance : ItemInstances)
	{
		if (FastItemInstance.ItemInstance.IsValid() && FastItemInstance.ItemInstance.GetPtr<FItemInstance>() && FastItemInstance.ItemInstance.GetPtr<FItemInstance>()->ItemId == ItemInstance)
		{
			OutFastItemInstance = FastItemInstance;
			return true;
		}
	}

	return false;
}

int32 FFastItemInstancesContainer::GetNum() const
{
	return ItemInstances.Num();
}
