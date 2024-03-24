// Copyright Fissure Entertainment, Pty Ltd. All Rights Reserved.

#include "GenericItemizationTags.h"

namespace GenericItemizationGameplayTags
{
	GENERICITEMIZATION_API UE_DEFINE_GAMEPLAY_TAG_COMMENT(ItemType, "Itemization.ItemType", "The overarching type of a particular Item that creates logical groups of Items.");
	GENERICITEMIZATION_API UE_DEFINE_GAMEPLAY_TAG_COMMENT(QualityType, "Itemization.QualityType", "The Quality Type of an Item. The same type of Item can be of a different Quality Type.");
	GENERICITEMIZATION_API UE_DEFINE_GAMEPLAY_TAG_COMMENT(AffixType, "Itemization.AffixType", "The overarching type of a particular Affix that creates logical groups of Affixes that might do the same/similar thing.");
	GENERICITEMIZATION_API UE_DEFINE_GAMEPLAY_TAG_COMMENT(UserData, "Itemization.UserData", "Arbitrary Data that can contain whatever you like.");
	GENERICITEMIZATION_API UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mutator, "Itemization.Mutator", "Arbitrary Data that can contain whatever you like that is appended to the ItemInstancingContext.");
	GENERICITEMIZATION_API UE_DEFINE_GAMEPLAY_TAG_COMMENT(SocketType, "Itemization.SocketType", "The overarching type of an ItemSocket.");

	GENERICITEMIZATION_API UE_DEFINE_GAMEPLAY_TAG_COMMENT(ItemInstanceChange, "Itemization.ItemInstanceChange", "Describes a change to an ItemInstance.");
	GENERICITEMIZATION_API UE_DEFINE_GAMEPLAY_TAG_COMMENT(ItemInstanceChange_StackCount, "Itemization.ItemInstanceChange.StackCount", "The StackCount of an ItemInstance was changed.");
	GENERICITEMIZATION_API UE_DEFINE_GAMEPLAY_TAG_COMMENT(ItemInstanceChange_SocketChange_Socketed, "Itemization.ItemInstanceChange.SocketChange.Socketed", "An ItemInstance was Socketed into another ItemInstance.");
	GENERICITEMIZATION_API UE_DEFINE_GAMEPLAY_TAG_COMMENT(ItemInstanceChange_SocketChange_Unsocketed, "Itemization.ItemInstanceChange.SocketChange.Unsocketed", "An ItemInstance was Unsocketed from another ItemInstance.");
}