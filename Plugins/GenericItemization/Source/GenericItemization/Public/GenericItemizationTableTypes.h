// Copyright Fissure Entertainment, Pty Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GenericItemizationTypes.h"
#include "InstancedStruct.h"
#include "GenericItemizationTableTypes.generated.h"

/************************************************************************/
/* Items
/************************************************************************/

/**
 * Simply facilitates the Instancing of FItemQualityRatioType.
 */
USTRUCT(BlueprintType)
struct FItemQualityRatioTypesTableEntry : public FTableRowBase
{
    GENERATED_BODY()

public:

    /* List of the Ratios for different Item Qualities. These determine how likely an Item is to be of a particular Quality Type. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BaseStruct = "/Script/GenericItemization.ItemQualityRatioType"))
    TArray<TInstancedStruct<FItemQualityRatioType>> ItemQualityRatios;
};

/**
 * Can be considered as a "master" table of all Item Drop Tables and their chances.
 * 
 * Also describes how many Items are selected for each Drop Table and the chance of No Pick occurring during the Item Drop Process for each Pick of those Drop Tables.
 * 
 * Rows will be referenced from outside Sources into this table to know which Item Drop Tables they should select from when executing the Item Drop Process.
 */
USTRUCT(BlueprintType)
struct FItemDropTableCollectionEntry : public FTableRowBase
{
    GENERATED_BODY()

public:

    /* How many times we Pick for Items in a single Drop process. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (UIMin = "1", ClampMin = "1"))
    int32 PickCount = 1;

    /* The Chance for NoPick to occur for each PickCount. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (UIMin = "0", ClampMin = "0"))
    int32 NoPickChance = 0;

    /* List of arbitrary Mutators from this DropTable that are appended to the ItemInstancingContext. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Mutators", Categories = "Itemization.Mutator"))
    TMap<FGameplayTag, FItemDropTableMutator> CustomMutators;

    /* Contains a list of the bonus increase to selection of a particular Quality Type of Item. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Itemization.QualityType"))
    TMap<FGameplayTag, FItemQualityTypeBonuses> QualityTypeBonuses;

    /* The different types of tables we will query for Item Drops (usually an Item Drop Table or Item Definition Collection). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BaseStruct = "/Script/GenericItemization.ItemDropTableType", ExcludeBaseStruct, ShowTreeView))
    TArray<TInstancedStruct<FItemDropTableType>> ItemDropTables;
};

/**
 * Simply facilitates the Instancing for FItemDefinition.
 */
USTRUCT(BlueprintType)
struct FItemDefinitionEntry : public FTableRowBase
{
    GENERATED_BODY()

public:

    /* The static description of an individual Item. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TInstancedStruct<FItemDefinition> ItemDefinition;
};

/************************************************************************/
/* Affixes
/************************************************************************/

/**
 * Simply facilitates the Instancing for FItemAffixCountRatioType.
 */
USTRUCT(BlueprintType)
struct FItemAffixCountRatiosTableEntry : public FTableRowBase
{
    GENERATED_BODY()

public:

    /* Describes the range of the number of Affixes of a particular type that an ItemInstance can have. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BaseStruct = "/Script/GenericItemization.ItemAffixCountRatioType"))
    TArray<TInstancedStruct<FItemAffixCountRatioType>> AffixCountRatios;
};

/**
 * Simply facilitates the Instancing for FAffixDefinition.
 */
USTRUCT(BlueprintType)
struct FAffixDefinitionEntry : public FTableRowBase
{
    GENERATED_BODY()

public:

    /* The static description of an individual Affix. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TInstancedStruct<FAffixDefinition> AffixDefinition;
};
