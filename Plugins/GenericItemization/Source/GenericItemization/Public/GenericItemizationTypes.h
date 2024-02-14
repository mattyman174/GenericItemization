// Copyright Fissure Entertainment, Pty Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "InstancedStruct.h"
#include "Misc/Optional.h"
#include "GenericItemizationTypes.generated.h"

class UItemDefinitionCollectionPickFunction;
class UItemDropTableCollectionPickFunction;
class UItemInstancingFunction;

/************************************************************************/
/* Items
/************************************************************************/

/**
 * 
 */
USTRUCT(BlueprintType)
struct FItemQualityTypeBonuses
{
    GENERATED_BODY()

public:

    /**
     * How much to reduce the Factor by for this QualityType when being selected for. 
     * 
     * The default implementation of QualityType selection operates on the basis that QualityTypes are rolled for
     * separately and the first to have a Pick value of < 128 is chosen.
     * 
     * The AdjustedFactor changes the overall Pick value by that amount where 1024 forces it to 0 (will absolutely be selected, assuming a QualityType in front doesn't get selected first).
     * A value of -1024 will likely cause the QualityType to be skipped during selection.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (/*UIMin = "-1024", ClampMin = "-1024", UIMax = "1024", ClampMax = "1024"*/))
    int32 AdjustedFactor = 0;
};

/**
 * Represents the Ratio that a particular Item Quality Type can be selected for during the Item Instancing Process.
 */
USTRUCT(BlueprintType)
struct FItemQualityRatioType
{
    GENERATED_BODY()

public:

    /* The Quality Type this Ratio is representing. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Itemization.QualityType"))
    FGameplayTag QualityType;

    /* The base value for the Ratio. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (UIMin = "1", ClampMin = "1"))
    int32 Base = 1;

    /* The divisor value of the Ratio. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (UIMin = "1", ClampMin = "1"))
    int32 Divisor = 1;

    /* The factor by which the Ratio is reduced. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (UIMin = "0", ClampMin = "0"))
    int32 Factor = 0;
};

/**
 * Simply facilitates the instancing of different derived types as entries in a Drop Table with a chance to be selected.
 */
USTRUCT(BlueprintType)
struct FItemDropTableType
{
    GENERATED_BODY()

public:
    
    /* The Chance that this Type is chosen in the Item Instancing Process. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayPriority = "0", UIMin = "0", ClampMin = "0"))
    int32 PickChance = 0;

};

/**
 * Reference to a Row in a DataTable containing Item Drop Table Collection Entries.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Item Drop Table Collection"))
struct FItemDropTableCollectionRow : public FItemDropTableType
{
    GENERATED_BODY()

public:

    FItemDropTableCollectionRow();

    /* The requirements that are applied to the ItemDropTableCollections when making Picks. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BaseStruct = "/Script/GenericItemization.ItemDropTableCollectionPickRequirements"))
    FInstancedStruct PickRequirements;

    /* The Pick Function that describes how to apply the Pick Requirements when making a Pick, this operates on the CDO. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (NoClear))
    TSubclassOf<UItemDropTableCollectionPickFunction> PickFunction;
    
    /* The Item Drop Table Collection Entry we will continue to query for Items into. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (RowType = "/Script/GenericItemization.ItemDropTableCollectionEntry"))
    FDataTableRowHandle ItemDropTableCollectionRow;

};

/**
 * Reference to a Row in a DataTable containing an Item Definition Entry.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Item Definition"))
struct FItemDefinitionRow : public FItemDropTableType
{
    GENERATED_BODY()

public:
    
    /* The Item Definition Entry we will create an Item instance from. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (RowType = "/Script/GenericItemization.ItemDefinitionEntry"))
    FDataTableRowHandle ItemDefinitionRow;

};

/**
 * Base for all Item Pick Requirement types.
 */
USTRUCT()
struct FItemPickRequirements
{
    GENERATED_BODY()

public:

};

/**
 * The specific Pick Requirements for Item Drop Table Collections.
 */
USTRUCT()
struct FItemDropTableCollectionPickRequirements
{
    GENERATED_BODY()

public:

};

/**
 * The specific Pick Requirements for Items selected from an Item Definition Collection.
 */
USTRUCT(BlueprintType)
struct FItemDefinitionCollectionPickRequirements : public FItemPickRequirements
{
    GENERATED_BODY()

public:

    /* The minimum inclusive Quality Level for the ItemDefinitions we want to query from this Collection. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayPriority = "1", UIMin = "1", ClampMin = "1"))
    int32 QualityLevelMinimum = 1;

    /* The maximum inclusive Quality Level for the ItemDefinitions we want to query from this Collection. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayPriority = "1", UIMin = "1", ClampMin = "1"))
    int32 QualityLevelMaximum = 1;

};

/**
 * Contains the definitions of actual Items.
 */
USTRUCT(BlueprintType)
struct FItemDefinitionCollection : public FItemDropTableType
{
    GENERATED_BODY()

public:

    FItemDefinitionCollection();

    /* The requirements that are applied to the ItemDefinitions when making Picks from this Collection. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BaseStruct = "/Script/GenericItemization.ItemDefinitionCollectionPickRequirements"))
    FInstancedStruct PickRequirements;

    /* The Pick Function that describes how to apply the Pick Requirements when making a Pick from this Collection, this operates on the CDO. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (NoClear))
    TSubclassOf<UItemDefinitionCollectionPickFunction> PickFunction;

    /* Table of all Items contained in this Collection. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (RequiredAssetDataTags = "RowStructure=/Script/GenericItemization.ItemDefinitionEntry"))
    TObjectPtr<UDataTable> ItemDefinitions;

};

USTRUCT(BlueprintType)
struct FItemDefinitionUserData
{
    GENERATED_BODY()

public:

    /* The Category that this UserData should be assigned. Helps with identifying different data sets. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Itemization.UserData"))
    FGameplayTag Category;

    /* The actual data. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FInstancedStruct UserData;
};

/**
 * All static data that defines what an individual ItemInstance is.
 */
USTRUCT(BlueprintType)
struct FItemDefinition
{
    GENERATED_BODY()

public:

    FItemDefinition();

    /* The Instancing Function that manages how certain attributes of the ItemDefinition are embodied onto the ItemInstance that might be using it. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (NoClear))
    TSubclassOf<UItemInstancingFunction> InstancingFunction;

    /* The name of this specific Item. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText ItemName;

    /**
     * The Item Type of this particular Item. 
     * 
     * This can affect the types of Affixes that might naturally be selected for an ItemInstance thats generated from this ItemDefinition.
     * 
     * @See FAffixDefinition::OccursForItemTypes
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Itemization.ItemType"))
    FGameplayTag ItemType;

    /* Can this Item actually be Instanced. If this is false, it will be ignored during the Item Instancing Process. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bSpawnable = true;

    /* The Chance that this Item is chosen in the Item Instancing Process. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (UIMin = "0", ClampMin = "0"))
    int32 PickChance = 1;

    /**
     * The preset level of Quality for this Item, this has consequences for this particular Item being selected during the Item Instancing Process. 
     * 
     * If the Item Instancing Process is requesting only Items within a specific QualityLevel range, it may ignore this Item. 
     *
     * @See FItemDefinitionCollection::PickRequirements - they contain restrictions on the QualityLevel when making a selection.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (UIMin = "1", ClampMin = "1"))
    int32 QualityLevel = 1;

    /* True if any ItemInstances made from this ItemDefinition will be given the predefined QualityType instead of going through the QualityType selection process. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (InlineEditConditionToggle))
    bool bHasPredefinedQualityType = true;

    /* The QualityType that the Item will come predefined with, it will not go through the QualityType selection process. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bHasPredefinedQualityType", Categories = "Itemization.QualityType"))
    FGameplayTag PredefinedQualityType;

    /* Describes the Ratios that an Instance of this Item can be of a particular QualityType when it is created during the Item Instancing Process. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "!bHasPredefinedQualityType", RowType = "/Script/GenericItemization.ItemQualityRatioTypesTableEntry"))
    FDataTableRowHandle QualityTypeRatio;

    /* Describes the number of Affixes an Instance of this Item can have when it is created during the Item Instancing Process. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "!bOnlyPredefinedAffixes", RowType = "/Script/GenericItemization.ItemAffixCountRatiosTableEntry"))
    FDataTableRowHandle AffixCountRatio;

    /* This Item only comes with predefined Affixes, during the Item Instancing Process it will never generate its own at random. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bOnlyPredefinedAffixes = false;

    /* All of the Affixes that this Item comes predefined with. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (RowType = "/Script/GenericItemization.AffixDefinitionEntry"))
    TArray<FDataTableRowHandle> PredefinedAffixes;

    /* The table that contains all of the Affixes that ItemInstances of this ItemDefinition can select from. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (RequiredAssetDataTags = "RowStructure=/Script/GenericItemization.AffixDefinitionEntry"))
    TObjectPtr<UDataTable> AffixPool;

    /**
     * DEPRECATED.
     * Please use CustomUserData instead. This will be removed in a later release.
     * 
     * Arbitrary Data that can contain whatever you like. 
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "UserData_DEPRECATED", Categories = "Itemization.UserData", DeprecatedProperty, DeprecationMessage = "Please use CustomUserData instead. This will be removed in a later release."))
    TMap<FGameplayTag, FInstancedStruct> UserData;

    /* Custom Data that can contain whatever you like. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FItemDefinitionUserData> CustomUserData;
};

/************************************************************************/
/* Affixes
/************************************************************************/

/**
 * Represents the Ratio that a particular  Type can be selected for during the Item Instancing Process.
 */
USTRUCT(BlueprintType)
struct FItemAffixCountRatioType
{
    GENERATED_BODY()

public:

    /* The Item Quality Type this Ratio is representing. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Itemization.QualityType"))
    FGameplayTag QualityType;

    /* The minimum of the range of Affix count. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (UIMin = "0", ClampMin = "0"))
    int32 Minimum = 1;

    /* The maximum of the range of Affix count. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (UIMin = "1", ClampMin = "1"))
    int32 Maximum = 1;

};

/**
 * How a particular Affix changes attributes when applied and to what degree.
 */
USTRUCT(BlueprintType)
struct FAffixModifier
{
    GENERATED_BODY()

public:

    /* The name of this specific Affix Modifier. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText ModName;

    /* The type of Modifier to attributes this Affix applies. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Itemization.AffixModifier"))
    FGameplayTag ModType;

    /* The minimum value of the applied Modifier Effect. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (UIMin = "0", ClampMin = "0"))
    int32 ModMinimum = 0;

    /* The maximum value of the applied Modifier Effect. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (UIMin = "0", ClampMin = "0"))
    int32 ModMaximum = 0;

};

/**
 * All static data that defines what an individual Affix is.
 */
USTRUCT(BlueprintType)
struct FAffixDefinition
{
    GENERATED_BODY()

public:

    /* The name of this specific Affix. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText AffixName;

    /* The Affix type of this particular Affix. An Item instance might want to use this to restrict multiples of the same Affix Type from appearing. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Itemization.AffixType"))
    FGameplayTag AffixType;

    /* Can this Affix actually appear on Items. If this is false, it will be ignored during the Item Instancing Process. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bSpawnable = true;

    /* The Chance that this Affix is chosen in the Item Instancing Process. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (UIMin = "0", ClampMin = "0"))
    int32 PickChance = 0;

    /* All of the ItemTypes of Items that this Affix can appear on. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Itemization.ItemType"))
    FGameplayTagContainer OccursForItemTypes;

    /* All of the QualityTypes of Items that this Affix can appear on. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Itemization.QualityType"))
    FGameplayTagContainer OccursForQualityTypes;

    /* The maximum inclusive QualityLevel of Items that this Affix can appear on. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (UIMin = "0", ClampMin = "0"))
    int32 OccursForQualityLevel = 0;

    /* The minimum value an Items Affix Level must be in order for it to be able to receive this Affix. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (UIMin = "0", ClampMin = "0"))
    int32 MinimumRequiredItemAffixLevel = 0;

    /* The maximum value an Items Affix Level can be in order for it to be able to receive this Affix. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (UIMin = "0", ClampMin = "0"))
    int32 MaximumRequiredItemAffixLevel = 0;

    /* All of the Modifiers that this Affix applies. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BaseStruct = "/Script/GenericItemization.AffixModifier"))
    TArray<TInstancedStruct<FAffixModifier>> Modifiers;

};

/************************************************************************/
/* Misc
/************************************************************************/

template<typename T> struct FPickEntry
{
    int32 PickChance;
    TOptional<T> PickType;
};