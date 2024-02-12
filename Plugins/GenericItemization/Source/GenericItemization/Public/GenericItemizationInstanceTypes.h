// Copyright Fissure Entertainment, Pty Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GenericItemizationTypes.h"
#include "Engine/NetSerialization.h"
#include "Net/Serialization/FastArraySerializer.h"
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

    /* The static data that describes this Affix. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TInstancedStruct<FAffixDefinition> AffixDefinition;

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

    /* The static data that describes this Item. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TInstancedStruct<FItemDefinition> ItemDefinition;

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

    /* The Context information around which this ItemInstance was called to be generated. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TInstancedStruct<FItemInstancingContext> InstancingContext;

    bool HasAnyAffixOfType(const FGameplayTag& AffixType) const;

    bool IsValid() const;

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

    //~ Begin of FFastArraySerializerItem
    void PostReplicatedAdd(const struct FFastItemInstancesContainer& InArray) { }
    void PostReplicatedChange(const struct FFastItemInstancesContainer& InArray) { }
    void PreReplicatedRemove(const struct FFastItemInstancesContainer& InArray) { }
    //~ End of FFastArraySerializerItem

private:

    /* The actual ItemInstance we are replicating. */
    UPROPERTY(BlueprintReadOnly, meta = (BaseStruct = "/Script/GenericItemization.ItemInstance", AllowPrivateAccess))
    FInstancedStruct ItemInstance;

    /* The Context Data of the ItemInstance when it was added to the Inventory. */
    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess))
    FInstancedStruct UserContextData;

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
    void AddItemInstance(const FInstancedStruct& ItemInstance, const FInstancedStruct& UserContextData);

    /* Removes an Item from the container. */
    bool RemoveItemInstance(const FGuid& ItemInstance);

    /* Returns all of the ItemInstances within the container. */
    void GetItemInstances(TArray<FInstancedStruct>& OutItemInstances) const;

    bool GetFastItemInstance(const FGuid& ItemInstance, FFastItemInstance& OutFastItemInstance) const;

    /* Returns the number of ItemInstances in the container. */
    int32 GetNum() const;

private:

	UPROPERTY()
	TArray<FFastItemInstance> ItemInstances;

	UPROPERTY(NotReplicated, Transient)
	TObjectPtr<UItemInventoryComponent> Owner;

    bool bOwnerIsNetAuthority;

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

template<>
struct TStructOpsTypeTraits<FFastItemInstancesContainer> : public TStructOpsTypeTraitsBase2<FFastItemInstancesContainer>
{
    enum
    {
        WithNetDeltaSerializer = true
    };
};