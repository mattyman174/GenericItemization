// Copyright Fissure Entertainment, Pty Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GenericItemizationTypes.h"
#include "Engine/NetSerialization.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "GenericItemizationTableTypes.h"
#include "ItemManagement/ItemSocketSettings.h"
#include "StructView.h"
#include "GenericItemizationInstanceTypes.generated.h"

/************************************************************************/
/* Affixes
/************************************************************************/

/**
 * An actual instance of an Affix that exists on an FItemInstance.
 */
USTRUCT(BlueprintType)
struct FAffixInstance
{
    GENERATED_BODY()

public:

    /* Was this AffixInstance from a predefined Affix on the ItemDefinition for the ItemInstance its applied to. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bPredefinedAffix = false;

    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

    const TInstancedStruct<FAffixDefinition>& GetAffixDefinition() const { return AffixDefinition; }
    void SetAffixDefinition(const FDataTableRowHandle& Handle);

protected:

    /* The static data that describes this Affix. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, NotReplicated, meta = (AllowPrivateAccess))
    TInstancedStruct<FAffixDefinition> AffixDefinition;

    /* Handle to the actual AffixDefinition, this is serialized instead of the AffixDefinition itself. */
    UPROPERTY()
    FDataTableRowHandle AffixDefinitionHandle;

};

template<>
struct TStructOpsTypeTraits<FAffixInstance> : public TStructOpsTypeTraitsBase2<FAffixInstance>
{
    enum
    {
        WithNetSerializer = true,
    };
};

/************************************************************************/
/* Items
/************************************************************************/

/**
 * Contains information about the Context around which an ItemInstance is called to be generated.
 */
USTRUCT(BlueprintType)
struct FItemInstancingContext
{
    GENERATED_BODY()

public:

    /* The overall level that the ItemInstance will be. This goes on to affect the types of and magnitude of attributes the ItemInstance can have among other things. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (UIMin = "0", ClampMin = "0"))
    int32 ItemLevel = 1;

    /* The amount of Magic Find bonus was snapshot in this context, this contributes to the tendency to find certain Item Quality Types. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (UIMin = "0", ClampMin = "0"))
    int32 MagicFind = 1;

    /* Mutators that were applied to this Instancing Context during the Item Instancing Process. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TMap<FGameplayTag, FItemDropTableMutator> Mutators;

    /* The DropTable that might have been involved in the Pick for the ItemInstance being generated. */
    const FItemDropTableCollectionEntry* DropTable;

};

/**
 * Facilitates nesting an ItemInstance inside of another ItemInstance.
 */
USTRUCT(BlueprintType)
struct FItemSocketInstance
{
    GENERATED_BODY()

public:

    friend struct FSetSocketInstanceSocketDefinition;
    friend class UItemInventoryComponent;

    FItemSocketInstance();

    /* The unique id of this SocketInstance. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid SocketId;

    /* Whether or not this SocketInstance currently has a socketed Item. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsEmpty;

    /* Handle of the actual SocketDefinition, this is serialized instead of the SocketDefinition itself. */
    UPROPERTY()
    FGuid SocketDefinitionHandle;

    const TInstancedStruct<FItemSocketDefinition>& GetSocketDefinition() const { return SocketDefinition; }
    const FConstStructView GetSocketedItem() const;

protected:

    /* The static data that describes this Socket. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, NotReplicated, meta = (AllowPrivateAccess))
    TInstancedStruct<FItemSocketDefinition> SocketDefinition;

    /* The ItemInstance that might be socketed into this SocketInstance. */
    UPROPERTY(BlueprintReadOnly, meta = (BaseStruct = "/Script/GenericItemization.ItemInstance", AllowPrivateAccess))
    FInstancedStruct SocketedItemInstance;

};

/**
 * An actual instance of an Item that was generated.
 */
USTRUCT(BlueprintType)
struct FItemInstance
{
    GENERATED_BODY()

public:

    FItemInstance();

    /* The Unique Id of the Item. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGuid ItemId;

    /* The authoritative seed that was generated, when this Item was created during the Item Instancing Process. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 ItemSeed;

    /* A random stream initialized with the ItemSeed when this Item was created. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FRandomStream ItemStream;

    /* The calculated level, of this Item when it was created during the Item Instancing Process. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 ItemLevel;

    /* The calculated level for Affixes that can appear on this Item, this is determined by the ItemLevel and the QualityLevel. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 AffixLevel;

    /* The Quality Type of this Item, this helps determine what types of Affixes and other attributes can occur on this Item. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Itemization.QualityType"))
    FGameplayTag QualityType;

    /* All of the actual Affixes that this Item has that would apply attribute modifiers from this Item. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BaseStruct = "/Script/GenericItemization.AffixInstance"))
    TArray<TInstancedStruct<FAffixInstance>> Affixes;

    /* The Context information around which this ItemInstance was called to be generated, this is not replicated. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, NotReplicated)
    TInstancedStruct<FItemInstancingContext> InstancingContext;

    /* The number of stacks of this Item. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 StackCount;

    /* All of the actual Sockets that this Item has that other ItemInstances could be placed into. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BaseStruct = "/Script/GenericItemization.ItemSocketInstance"))
    TArray<TInstancedStruct<FItemSocketInstance>> Sockets;

    bool HasAnyAffixOfType(const FGameplayTag& AffixType) const;

    bool IsValid() const;

    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

    const TInstancedStruct<FItemDefinition>& GetItemDefinition() const { return ItemDefinition; }
    void SetItemDefinition(const FDataTableRowHandle& Handle);

    /* Adds a new SocketInstance to this ItemInstance. */
    void AddSocket(TInstancedStruct<FItemSocketInstance>& NewSocket);

    /* Returns a view into the SocketInstance of the given SocketId if it exists on this ItemInstance. */
    TOptional<const FConstStructView> GetSocket(FGuid SocketId) const;

    /* Returns true if this ItemInstance has a SocketInstance with the given SocketId .*/
    bool HasSocket(FGuid SocketId) const;

protected:

    /* The static data that describes this Item. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, NotReplicated, meta = (AllowPrivateAccess))
    TInstancedStruct<FItemDefinition> ItemDefinition;

    /* Handle to the actual ItemDefinition, this is serialized instead of the ItemDefinition itself. */
    UPROPERTY()
    FDataTableRowHandle ItemDefinitionHandle;

};

template<>
struct TStructOpsTypeTraits<FItemInstance> : public TStructOpsTypeTraitsBase2<FItemInstance>
{
    enum
    {
        WithNetSerializer = true,
    };
};

struct FSetSocketInstanceSocketDefinition
{
private:

    friend struct FItemInstance;

    FSetSocketInstanceSocketDefinition(const FItemDefinition* ItemDefinition, FItemSocketInstance* Socket)
    {
        if (ItemDefinition && !ItemDefinition->bStacksOverSockets && ItemDefinition->SocketSettings && Socket)
        {
            const UItemSocketSettings* const ItemSocketSettingsCDO = ItemDefinition->SocketSettings.GetDefaultObject();
            if (ItemSocketSettingsCDO)
            {
                for (const TInstancedStruct<FItemSocketDefinition>& SocketDefinition : ItemSocketSettingsCDO->SocketDefinitions)
                {
                    if (SocketDefinition.Get().SocketDefinitionHandle == Socket->SocketDefinitionHandle)
                    {
                        Socket->SocketDefinition = SocketDefinition;
                    }
                }
            }
        }
    }
};

USTRUCT()
struct FItemInstanceChange
{
    GENERATED_BODY()

public:
    
    /* Identifies what type of change that was being made. */
    UPROPERTY()
    FGameplayTag ChangeDescriptor;

    /* Id of this Change. This helps the Client know how far behind it is. */
    UPROPERTY()
    int32 ChangeId;

    /* Names of all of the properties that changed. */
    UPROPERTY()
    TArray<FName> ChangedProperties;

};

/**
 * FastArraySerializerItem wrapper for an ItemInstance.
 * 
 * This is needed because in order to avoid data slicing an FItemInstance as a direct FastArraySerializerItem, it needs to be handled with a FInstancedStruct.
 * FInstancedStruct does not support FastArray Serialization. Thus we need to wrap it in our own type.
 * 
 * Therefore this type should only be used by the FFastItemInstancesContainer type.
 */
USTRUCT(BlueprintType)
struct FFastItemInstance : public FFastArraySerializerItem
{
    GENERATED_BODY()

public:

    friend struct FFastItemInstancesContainer;
    friend class UItemInventoryComponent;

    void Initialize(FInstancedStruct& InItemInstance, FInstancedStruct& InUserContextData);

    //~ Begin of FFastArraySerializerItem
    void PostReplicatedAdd(const struct FFastItemInstancesContainer& InArray);
    void PostReplicatedChange(const struct FFastItemInstancesContainer& InArray);
    void PreReplicatedRemove(const struct FFastItemInstancesContainer& InArray);
    //~ End of FFastArraySerializerItem

private:

    /* The actual ItemInstance we are replicating. */
    UPROPERTY(BlueprintReadOnly, meta = (BaseStruct = "/Script/GenericItemization.ItemInstance", AllowPrivateAccess))
    FInstancedStruct ItemInstance;

    /* The Context Data of the ItemInstance when it was added to the Inventory. */
    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess))
    FInstancedStruct UserContextData;

    /* List of all recent changes made to the ItemInstance. */
    UPROPERTY()
    TArray<FItemInstanceChange> RecentChangesBuffer;

    /* Id of the current ChangeList. */
    UPROPERTY()
    int32 RecentChangesId;

    /* A copy of the ItemInstance which we use as a lookup for the previous values of changed properties that come in from the RecentChangesBuffer. */
    FInstancedStruct PreReplicatedChangeItemInstance;

    /* Id of the ChangeList we have executed up to. */
    int32 PreviousChangesId;

};

/**
 * FastArraySerializer container of ItemInstances.
 *  
 * This should only be used by UItemInventoryComponent. All of this could just live in UItemInventoryComponent except that we need a distinct USTRUCT to implement FFastArraySerializer.
 *
 * The preferred way to iterate through the FFastItemInstancesContainer is with CreateConstIterator/CreateIterator or stl style range iteration:
 * 
 * for (auto It = CreateConstIterator(); It; ++It) {}
 */
USTRUCT(BlueprintType)
struct FFastItemInstancesContainer : public FFastArraySerializer
{
    GENERATED_BODY()

public:

    friend class UItemInventoryComponent;

    FFastItemInstancesContainer() : 
        Owner(nullptr)
    { }

    //~ Begin of FFastArraySerializer
    void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize);
    void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);
    void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize);
    //~ End of FFastArraySerializer

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
    {
        // @TODO: Introduce per connection functionality. We will want to specify that only replication occurs to the owner or all.
        // An Inventory may only want to display its contents to a particular connection.
        return FFastArraySerializer::FastArrayDeltaSerialize<FFastItemInstance, FFastItemInstancesContainer>(ItemInstances, DeltaParams, *this);
    }

    bool HasAuthority() const
    {
        return bOwnerIsNetAuthority;
    }

    typedef TArray<FFastItemInstance>::TConstIterator TConstIterator;
    TConstIterator CreateConstIterator() const { return ItemInstances.CreateConstIterator(); }

    /* Must be called in order to correctly initialize the container! */
    void Register(UItemInventoryComponent* InOwner);

    /* Adds an Item to the container. */
    void AddItemInstance(FInstancedStruct& ItemInstance, FInstancedStruct& UserContextData);

    /* Creates a scope to make mutable changes to an ItemInstance. This should always be called if you intend to mutate an ItemInstance! */
    template<typename InstanceType>
    bool ModifyItemInstance(
        const FGuid& Item,
        const TFunctionRef<void(InstanceType* MutableItemInstance)>& MakeChanges
    );

    /* Creates a scope to make mutable changes to an ItemInstance and replicates a ChangeDescriptor. This should always be called if you intend to mutate an ItemInstance! */
    template<typename InstanceType>
    bool ModifyItemInstanceWithChangeDescriptor(
        const FGuid& Item, 
        const FGameplayTag& ChangeDescriptor, 
        TArray<FName> PendingChangeProperties, 
        const TFunctionRef<void(InstanceType* MutableItemInstance)>& MakeChanges
    );

    /* Removes an Item from the container. */
    bool RemoveItemInstance(const FGuid& Item);

    /* Returns a copy of all of the ItemInstances within the container. */
    TArray<FInstancedStruct> GetItemInstances() const;

    /* Returns a copy of the ItemInstance, if it exists. */
    const FFastItemInstance* GetItemInstance(const FGuid& Item) const;

    /* Returns the number of ItemInstances in the container. */
    int32 GetNum() const;

private:

	UPROPERTY()
	TArray<FFastItemInstance> ItemInstances;

	UPROPERTY(NotReplicated, Transient)
	TObjectPtr<UItemInventoryComponent> Owner;

    bool bOwnerIsNetAuthority;

    /* Called when an ItemInstance was changed. Calls, DiffItemInstanceChanges and updates any cached state for the changed ItemInstance. */
    void OnItemInstanceChanged(FFastItemInstance& ChangedItemInstance);

    /* Diffs the RecentChangesBuffer for the ItemInstance and emits any actual changes that took place to the ItemInventoryComponent. */
    void DiffItemInstanceChanges(FFastItemInstance& ChangedItemInstance);

    /**
     * DO NOT USE DIRECTLY
     * STL-like iterators to enable range-based for loop support.
     */

    /* Private as we want to manage mutable access. */
    typedef TArray<FFastItemInstance>::TIterator TIterator;
    TIterator CreateIterator() { return ItemInstances.CreateIterator(); }

    FORCEINLINE friend TIterator begin(FFastItemInstancesContainer& Container) { return Container.CreateIterator(); }
    FORCEINLINE friend TIterator end(FFastItemInstancesContainer& Container) { return TIterator(Container.ItemInstances, Container.ItemInstances.Num()); }

    FORCEINLINE friend TConstIterator begin(const FFastItemInstancesContainer& Container) { return Container.CreateConstIterator(); }
    FORCEINLINE friend TConstIterator end(const FFastItemInstancesContainer& Container) { return TConstIterator(Container.ItemInstances, Container.ItemInstances.Num()); }

};

template<typename InstanceType>
bool FFastItemInstancesContainer::ModifyItemInstance(const FGuid& Item, const TFunctionRef<void(InstanceType* MutableItemInstance)>& MakeChanges)
{
    static_assert(std::is_same_v<InstanceType, FItemInstance> ||
        TIsDerivedFrom<InstanceType, FItemInstance>::IsDerived, "Changes can only be made on FItemInstance types.");

    for (FFastItemInstance& ItemInstance : ItemInstances)
    {
        const FItemInstance* ItemInstancePtr = ItemInstance.ItemInstance.GetPtr<FItemInstance>();
        if (ItemInstancePtr && ItemInstancePtr->ItemId == Item)
        {
            // Update our cached state.
            ItemInstance.PreReplicatedChangeItemInstance.InitializeAs(ItemInstance.ItemInstance.GetScriptStruct(), ItemInstance.ItemInstance.GetMemory());

            // Commit the changes to the actual ItemInstance being requested.
            MakeChanges(ItemInstance.ItemInstance.GetMutablePtr<InstanceType>());

            if (HasAuthority())
            {
                OnItemInstanceChanged(ItemInstance);
                MarkItemDirty(ItemInstance);
            }

            return true;
        }
    }

    return false;
}

template<typename InstanceType>
bool FFastItemInstancesContainer::ModifyItemInstanceWithChangeDescriptor(const FGuid& Item, const FGameplayTag& ChangeDescriptor, TArray<FName> PendingChangeProperties, const TFunctionRef<void(InstanceType* MutableItemInstance)>& MakeChanges)
{
    static_assert(std::is_same_v<InstanceType, FItemInstance> ||
        TIsDerivedFrom<InstanceType, FItemInstance>::IsDerived, "Changes can only be made on FItemInstance types.");

    for (FFastItemInstance& ItemInstance : ItemInstances)
    {
        const FItemInstance* ItemInstancePtr = ItemInstance.ItemInstance.GetPtr<FItemInstance>();
        if (ItemInstancePtr && ItemInstancePtr->ItemId == Item)
        {
            // Update our cached state.
            ItemInstance.PreReplicatedChangeItemInstance.InitializeAs(ItemInstance.ItemInstance.GetScriptStruct(), ItemInstance.ItemInstance.GetMemory());

            // This is a new Change, update the ID.
            ItemInstance.RecentChangesId++;

            // Push the change descriptor onto the ItemInstance.
            // This is replicated to the Client so it can perform the same diff operation.
            FItemInstanceChange NewChange;
            NewChange.ChangeDescriptor = ChangeDescriptor;
            NewChange.ChangedProperties.Append(PendingChangeProperties);
            NewChange.ChangeId = ItemInstance.RecentChangesId;
            ItemInstance.RecentChangesBuffer.Add(NewChange);

            // Commit the changes to the actual ItemInstance being requested.
            // We then diff these against the PreReplicatedChangeItemInstance.
            MakeChanges(ItemInstance.ItemInstance.GetMutablePtr<InstanceType>());

            if(HasAuthority())
			{
                OnItemInstanceChanged(ItemInstance);
				MarkItemDirty(ItemInstance);
			}

            return true;
        }
    }

    return false;
}

template<>
struct TStructOpsTypeTraits<FFastItemInstancesContainer> : public TStructOpsTypeTraitsBase2<FFastItemInstancesContainer>
{
    enum
    {
        WithNetDeltaSerializer = true
    };
};