// Copyright Fissure Entertainment, Pty Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InstancedStruct.h"
#include "GameplayTagContainer.h"
#include "ItemStackSettings.generated.h"

/**
 * Describes the requirements that Items must meet in order to successfully stack.
 */
USTRUCT(BlueprintType)
struct FItemStackingRequirements
{
    GENERATED_BODY()

public:

    /* True if stacking will ignore the QualityLevel of the Items being stacked. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bIgnoreQualityLevel = true;

	/* True if stacking will ignore the ItemLevel of the Items being stacked. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bIgnoreItemLevel = true;

	/* True if stacking will ignore the AffixLevel of the Items being stacked. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bIgnoreAffixLevel = true;

	/* True if we can stack the Items when they have Affixes. Predefined Affixes are ignored by default as they will be identical for the same Item types. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bIgnoreAffixes = true;

	/* The Item QualityTypes that the Item cannot be stacked with. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Itemization.QualityType"))
    FGameplayTagContainer DoesNotStackWithQualityTypes;

};

/**
 * Describes how an ItemInstance can be stacked. Default is not stackable.
 * 
 * Stackable Items must originate from the same ItemDefinition.
 */
UCLASS(ClassGroup = ("Generic Itemization"), Blueprintable, Abstract)
class UItemStackSettings : public UObject
{
	GENERATED_BODY()

public:

	UItemStackSettings();

	/* Can this Item stack at all. */
	bool IsStackable() const { return bStackable; }

	/* Should this Item stack an unlimited amount of times. */
	bool HasUnlimitedStacks() const { return bUnlimitedStacks; }

	/* How many times can the Item be stacked. */
	int32 GetStackLimit() const { return StackLimit; }

	/**
	 * Checks if both Items can be stacked together.
	 * 
	 * @param ItemToStackFrom		The ItemInstance that we want to stack onto ItemToStackWith.
	 * @param ItemToStackWith		The ItemInstance that ItemToStackFrom will be stacked onto.
	 * @param OutRemainder			How much of ItemToStackFrom will remain if the stacking operation was to be made.
	 * @return						True if these ItemInstances can be stacked together.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item Stack Settings")
	bool CanStackWith(const FInstancedStruct& ItemToStackFrom, const FInstancedStruct& ItemToStackWith, int32& OutRemainder) const;

protected:

	/* Can this Item stack at all. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Stack Settings")
	bool bStackable = false;

	/* Should this Item stack an unlimited amount of times. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bStackable"), Category = "Item Stack Settings")
	bool bUnlimitedStacks = false;

	/* How many times can the Item be stacked. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bStackable && !bUnlimitedStacks", UIMin = "2", ClampMin = "2"), Category = "Item Stack Settings")
	int32 StackLimit = 2;

	/* The additional requirements that Items must meet in order to successfully stack. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TInstancedStruct<FItemStackingRequirements> StackingRequirements;
};

