// Copyright Fissure Entertainment, Pty Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InstancedStruct.h"
#include "GenericItemizationTypes.h"
#include "GenericItemizationTableTypes.h"
#include "GenericItemizationPickFunctions.generated.h"

class UDataTable;

/************************************************************************/
/* Items
/************************************************************************/

/**
 * Base class for all Item Pick Functions.
 */
UCLASS(ClassGroup = ("Generic Itemization"), Abstract, Blueprintable)
class GENERICITEMIZATION_API UItemPickFunction : public UObject
{
	GENERATED_BODY()

public:

	/**
	 * Picks a single Item based on the PickRequirements.
	 * NOTE: Derivations must provide the Items as a property on the ItemPickFunction class that can be read by the function.
	 * 
	 * Returns false if no Item was selected. 
	 */
	UFUNCTION(BlueprintNativeEvent)
	bool PickItem(const FInstancedStruct& PickRequirements, const FInstancedStruct& ItemInstancingContext, FInstancedStruct& OutItem, FDataTableRowHandle& OutItemHandle) const;
};

/**
 * Pick Function type when used with Item Drop Table Collections.
 */
UCLASS(ClassGroup = ("Generic Itemization"), Blueprintable, BlueprintType)
class GENERICITEMIZATION_API UItemDropTableCollectionPickFunction : public UItemPickFunction
{
	GENERATED_BODY()

public:

	virtual bool PickItem_Implementation(const FInstancedStruct& PickRequirements, const FInstancedStruct& ItemInstancingContext, FInstancedStruct& OutItem, FDataTableRowHandle& OutItemHandle) const override;

	/* The Drop Table Collection Entry we will make a selection from. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (DisplayPriority = "1", RowType = "/Script/GenericItemization.ItemDropTableCollectionEntry"))
	FDataTableRowHandle ItemDropTableCollectionEntry;

	/* True if the NoPickChance should be included in the pick entries. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bIncludeNoPick;

protected:

	/* Checks if the passed in ItemDropTableCollection satisfies the PickRequirements. */
	UFUNCTION(BlueprintNativeEvent)
	bool DoesItemDropTableCollectionSatisfyPickRequirements(const FInstancedStruct& PickRequirements, const FInstancedStruct& ItemInstancingContext, const FInstancedStruct& ItemDropTableCollection) const;
};

/**
 * Pick Function type when used with Item Definition Collections.
 */
UCLASS(ClassGroup = ("Generic Itemization"), Blueprintable, BlueprintType)
class GENERICITEMIZATION_API UItemDefinitionCollectionPickFunction : public UItemPickFunction
{
	GENERATED_BODY()

public:

	virtual bool PickItem_Implementation(const FInstancedStruct& PickRequirements, const FInstancedStruct& ItemInstancingContext, FInstancedStruct& OutItem, FDataTableRowHandle& OutItemHandle) const override;

	/* The Definitions of all Items we can select from. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (DisplayPriority = "1", RequiredAssetDataTags = "RowStructure=/Script/GenericItemization.ItemDefinitionEntry"))
	TObjectPtr<UDataTable> ItemDefinitions;

protected:

	/* Checks if the passed in ItemDefinition satisfies the PickRequirements. */
	UFUNCTION(BlueprintNativeEvent)
	bool DoesItemDefinitionSatisfyPickRequirements(const FInstancedStruct& PickRequirements, const FInstancedStruct& ItemInstancingContext, const FInstancedStruct& ItemDefinition) const;
};

/************************************************************************/
/* Affixes
/************************************************************************/

/**
 * Base class for all Affix Pick Functions.
 */
UCLASS(ClassGroup = ("Generic Itemization"), Abstract, Blueprintable)
class GENERICITEMIZATION_API UAffixPickFunction : public UObject
{
	GENERATED_BODY()

public:

	/**
	 * Picks a single Affix from the AffixPool. 
	 * The default implementation takes into account all of the requirements set out on the AffixDefinition and any Affixes already applied to the ItemInstance.
	 * 
	 * Returns false if no Affix was selected. 
	 */
	UFUNCTION(BlueprintNativeEvent)
	bool PickAffix(const FInstancedStruct& ItemInstance, const FInstancedStruct& ItemInstancingContext, FDataTableRowHandle& OutAffixHandle) const;

	UFUNCTION(BlueprintCallable)
	virtual bool GetAffixesWithMinimumNativeRequirements(const FInstancedStruct& ItemInstance, const FInstancedStruct& ItemInstancingContext, TArray<FDataTableRowHandle>& OutAffixHandles) const;

	/* The Definitions of all Affixes we can select from. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (DisplayPriority = "1", RequiredAssetDataTags = "RowStructure=/Script/GenericItemization.AffixDefinitionEntry"))
	TObjectPtr<UDataTable> AffixPool;
};