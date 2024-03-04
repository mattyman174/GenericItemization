#include "GenericItemizationInstanceTypes.h"
#include "ItemManagement/ItemInventoryComponent.h"
#include "Engine/DataTable.h"

/************************************************************************/
/* Affixes
/************************************************************************/

bool FAffixInstance::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	Ar << bPredefinedAffix;
	Ar << AffixDefinitionHandle.DataTable;
	Ar << AffixDefinitionHandle.RowName;

	if (Ar.IsLoading())
	{
		SetAffixDefinition(AffixDefinitionHandle);
	}

	bOutSuccess = true;
	return true;
}

void FAffixInstance::SetAffixDefinition(const FDataTableRowHandle& Handle)
{
	AffixDefinitionHandle = Handle;
	if (!AffixDefinitionHandle.IsNull())
	{
		const FAffixDefinitionEntry* AffixDefinitionEntry = AffixDefinitionHandle.GetRow<FAffixDefinitionEntry>(FString());
		if (AffixDefinitionEntry)
		{
			AffixDefinition = AffixDefinitionEntry->AffixDefinition;
		}
	}
}

/************************************************************************/
/* Items
/************************************************************************/

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
		if (AffixInstance.IsValid() && AffixInstance.Get().GetAffixDefinition().IsValid())
		{
			if (AffixType.MatchesTag(AffixInstance.Get().GetAffixDefinition().Get().AffixType))
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

void FFastItemInstance::Initialize(FInstancedStruct& InItemInstance, FInstancedStruct& InUserContextData)
{
	ItemInstance = InItemInstance;
	UserContextData = InUserContextData;

	// Make a copy of the ItemInstance for change comparison.
	PreReplicatedChangeItemInstance.InitializeAs(ItemInstance.GetScriptStruct(), ItemInstance.GetMemory());
}

void FFastItemInstance::PostReplicatedAdd(const struct FFastItemInstancesContainer& InArray)
{
	// Update our cached state.
	PreReplicatedChangeItemInstance.InitializeAs(ItemInstance.GetScriptStruct(), ItemInstance.GetMemory());
	PreviousChangesId = RecentChangesId;
}

void FFastItemInstance::PostReplicatedChange(const struct FFastItemInstancesContainer& InArray)
{

}

void FFastItemInstance::PreReplicatedRemove(const struct FFastItemInstancesContainer& InArray)
{

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
		FFastItemInstance& FastItemInstance = ItemInstances[Index];
		OnItemInstanceChanged(FastItemInstance);
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

void FFastItemInstancesContainer::AddItemInstance(FInstancedStruct& ItemInstance, FInstancedStruct& UserContextData)
{
	if (!Owner)
	{
		return;
	}

	FFastItemInstance FastItemInstance;
	FastItemInstance.Initialize(ItemInstance, UserContextData);
	ItemInstances.Add(FastItemInstance);

	if (HasAuthority())
	{
		Owner->OnAddedItemInstance(FastItemInstance);
		MarkItemDirty(FastItemInstance);
	}
}

void FFastItemInstancesContainer::OnItemInstanceChanged(FFastItemInstance& ChangedItemInstance)
{
	DiffItemInstanceChanges(ChangedItemInstance);

	if (Owner)
	{
		Owner->OnChangedItemInstance(ChangedItemInstance);
	}

	// Update our cached state.
	ChangedItemInstance.PreReplicatedChangeItemInstance.InitializeAs(ChangedItemInstance.ItemInstance.GetScriptStruct(), ChangedItemInstance.ItemInstance.GetMemory());
	ChangedItemInstance.PreviousChangesId = ChangedItemInstance.RecentChangesId;
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

			if(HasAuthority())
			{
				Owner->OnRemovedItemInstance(OldItemInstance);
				MarkArrayDirty();
			}

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

const FFastItemInstance* FFastItemInstancesContainer::GetItemInstance(const FGuid& Item) const
{
	for (const FFastItemInstance& FastItemInstance : ItemInstances)
	{
		const FItemInstance* ItemInstancePtr = FastItemInstance.ItemInstance.GetPtr<FItemInstance>();
		if (ItemInstancePtr && ItemInstancePtr->ItemId == Item)
		{
			return &FastItemInstance;
		}
	}

	return nullptr;
}

int32 FFastItemInstancesContainer::GetNum() const
{
	return ItemInstances.Num();
}

void FFastItemInstancesContainer::DiffItemInstanceChanges(FFastItemInstance& ChangedItemInstance)
{
	if(IsValid(Owner))
	{
		for (const FItemInstanceChange& RecentChange : ChangedItemInstance.RecentChangesBuffer)
		{
			if (RecentChange.ChangeId >= ChangedItemInstance.PreviousChangesId)
			{
				for (const FName& PropertyName : RecentChange.ChangedProperties)
				{
					const FProperty* OldProperty = nullptr;
					const void* OldPropertyData = nullptr;
					const bool bFoundOldProperty = ChangedItemInstance.PreReplicatedChangeItemInstance.FindInnerPropertyInstance(PropertyName, OldProperty, OldPropertyData);

					const FProperty* NewProperty = nullptr;
					const void* NewPropertyData = nullptr;
					const bool bFoundNewProperty = ChangedItemInstance.ItemInstance.FindInnerPropertyInstance(PropertyName, NewProperty, NewPropertyData);

					if (bFoundOldProperty && bFoundNewProperty)
					{
						const void* OldPropertyValue = OldProperty->ContainerPtrToValuePtr<const void>(OldPropertyData);
						const void* NewPropertyValue = NewProperty->ContainerPtrToValuePtr<const void>(NewPropertyData);
						Owner->OnItemInstancePropertyValueChanged_Internal(ChangedItemInstance, RecentChange.ChangeDescriptor, RecentChange.ChangeId, PropertyName, OldPropertyValue, NewPropertyValue);
					}
				}
			}
		}
	}
}
