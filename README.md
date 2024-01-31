# Generic Itemization Plugin
<a name="top"></a>
### Foreword

The underpinning motivation behind the development of this Plugin and its subsequent free availability to the community was borne out of a desire to simply explore and understand the limitations of the [Instanced Struct](#instanced-structs) framework and especially its usefulness towards DataTables. While also providing a useful implementation of Itemization utilizing these features that you might find in a traditional ARPG (similar to Diablo or Path of Exile).

Added in Unreal Engine 5.0 and better developed in later engine versions, Instanced Structs work similarly as instanced `UObject*` properties but are `USTRUCT`s. DataTables do not support the usage of instanced `UObject*` properties and thus the introduction of Instanced Structs brings much needed flexibility to DataTables. This plugin will explore that new relationship.

This Project and Plugin are a work in progress and as stated before, my own exploration into Instanced Structs. I make no guarantee of the accuracy of information or the approach being close to best practice. I will update this plugin as I learn more and get a better understanding of how to make use of Instanced Structs, especially in conjunction with Blueprint.

If you find the Generic Itemization Plugin useful to you, please [consider making a donation](https://donate.stripe.com/3cs2c15SIbfXeoo5kk), although I provide it for free and with no expectations, I have put considerable time into this work and appreciate any contributions.

You can find my contact information on my website:
[https://fissureentertainment.com/](https://fissureentertainment.com/)

**Latest compiled against: UE 5.3.2**

<a name="table-of-contents"></a>
## Table of Contents

>1 [Intro to Instanced Structs and the Generic Itemization Plugin](#intro)  
>2 [Sample Project](#sp)  
>3 [Setting Up a Project Using the Generic Itemization Plugin](#setup)  
>4 [Class Layout](#class-layout)  
>5 [Generic Itemization Concepts](#concepts)  
>>5.1 [Drop Tables](#drop-tables)  
>>>5.1.1 [Drop Table Types](#drop-table-types)  
>>
>>5.2 [Items](#items)  
>>>5.2.1 [Item Definition](#item-definition)  
>>>>5.2.1.1 [Item Type](#item-definition-item-type)  
>>>>5.2.1.2 [Spawnable](#item-definition-spawnable)  
>>>>5.2.1.3 [Pick Chance](#item-definition-pick-chance)  
>>>>5.2.1.4 [Quality Type](#item-definition-quality-type)  
>>>>>5.2.1.4.1 [ItemQualityRatio and Quality Type selection](#item-definition-quality-type-selection)  
>>>>
>>>>5.2.1.5 [Affix Count, Predefined Affixes and the Affix Pool](#item-definition-affixes)  
>>>>5.2.1.6 [User Data](#item-definition-user-data)  
>>>>5.2.1.7 [Item Instancing Function](#item-definition-instancing-function)  
>>>
>>>5.2.2 [Item Instance](#item-instance)  
>>
>>5.3 [Affixes](#affixes)  
>>>5.3.1 [Affix Definition](#affix-definition)
>>>>5.3.1.1 [Affix Type](#affix-definition-affix-type)  
>>>>5.3.1.2 [Spawnable](#affix-definition-spawnable)  
>>>>5.3.1.3 [Pick Chance](#affix-definition-pick-chance)  
>>>>5.3.1.4 [Occurs for, Item Type, Quality Type and Quality Level](#affix-definition-occurs-for)  
>>>>5.3.1.5 [Required Item Affix Level](#affix-definition-required-affix-level)  
>>>>5.3.1.6 [Modifiers](#affix-definition-modifiers)  
>>>
>>>5.3.2 [Affix Instance](#affix-instance)  
>>
>>5.4 [Item Instancing Process](#item-instancing-process)  
>>5.5 [Item Drop Actor](#item-drop-actor)  
>>5.6 [Item Dropper Component](#item-dropper-component)  
>>>5.6.1 [Item Instancing Context and the Context Provider Function](#item-instancing-context)  
>>
>>5.7 [Item Inventory Component](#item-inventory-component)  
>
>6 [Other Resources](#other-resources)  
         
<a name="intro"></a>
## 1. Intro to Instanced Structs and the Generic Itemization Plugin

<a name="instanced-structs"></a>
### Instanced Structs

As briefly explained above, introduced in Unreal Engine 5.0 is the [StructUtils](https://docs.unrealengine.com/5.0/en-US/API/Plugins/StructUtils/) plugin that brings with it the concept of Instanced Structs. They do exactly what their name suggests; provides the same features as instanced objects but embodied as `USTRUCT`s. This comes with several advantages, the most prominent of which is their light weight nature as compared to their counter parts in instanced `UObject*`s.

Instanced Structs are also fully polymorphic and serialized across the network. Meaning they support replicated, effectively arbitrary data. It is up to the receiver to interpret that data appropriately.

This documentation is not an exhaustive overview of Instanced Structs but I will provide explanation of my understanding of their usage throughout the document.

A quick example of what an Instanced Struct might look like can be found below.

```c++
USTRUCT(BlueprintType)
struct MY_API FMyStructParent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	int32 ParentProperty;
};

USTRUCT(BlueprintType)
struct MY_API FMyStructChild : public FMyStructParent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float ChildProperty;
};

USTRUCT(BlueprintType)
struct MY_API FMyStructChildOfChild : public FMyStructChild
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	bool ChildOfChildProperty;
};
```
Declared above are 3 regular `USTRUCT`s which have a hierarchy derived from each of the previous. A great attribute of Instanced Structs is that they do not require any extra or special code of the `USTRUCT` they are representing in order to work. Declaring the above `USTRUCT`s as Instanced Structs is as easy as the following:

```c++
UPROPERTY(EditAnywhere, meta = (BaseStruct = "MyStructParent"))
FInstancedStruct MyStructParentProperty;

UPROPERTY(EditAnywhere, meta = (ExcludeBaseStruct))
TInstancedStruct<FMyStructChild> MyStructChildProperty;

UPROPERTY(EditAnywhere, meta = (BaseStruct = "MyStructParent", ExcludeBaseStruct))
TArray<TInstancedStruct<FMyStructParent>> MyArrayOfMyStructParentProperties;
```
The Editor displays them as such.

![Instanced Struct Example in Editor](https://fissureentertainment.com/devilsd/UnrealEngine/GenericItemization/Documentation/InstancedStructExample.JPG)

Take special note of how the `BaseStruct` meta in conjunction with `FInstancedStruct` and the `TInstancedStruct` type are used interchangeably. `TInstancedStruct` is a type safe wrapper around an `FInstancedStruct` and can be used in its place without declaring the `BaseStruct` meta.

There is also an unfortunate quirk where when utilizing Instanced Structs with a `TArray` you must still include the `BaseStruct` meta in order for the Editor to understand the type correctly, as utilizing `TInstancedStruct`, while it does mention in the documentation that it will initialize the `BaseStruct` meta natively, this is not the case for when the property is exposed in Editor Details Panels via `TArray`s.

`ExcludeBaseStruct` also does as its name suggests, it will not display the `BaseStruct` as a selectable option for the Instanced Struct property within the dropdown.

Another useful meta property that is supported by Instanced Structs is `ShowTreeView`, which is not documented as working with Instanced Structs but causes them to be displayed in dropdowns in a tree view instead of a straight list.

### What is the Generic Itemization Plugin

Itemization is generally speaking the process of defining and creating Items, these Items can apply modifiers to attributes, grant new abilities to their users or affect other changes to gameplay. A typical example of what Itemization is can be found in ARPGs like Diablo or Path of Exile.

The Generic Itemization Plugin implements this process in a way that can be applied to any style of game that requires the instantiation of Items from predefined aggregated tables of [Items](#items) and [Affixes](#affixes). Some of what the Plugin provides are mechanisms for controlling things like rarity, distribution,  stats and their ranges, through Affixes as well as how those Affixes are to be applied to particular Item types.

The plugin manages these things through DataTables. A lot of DataTables. These DataTables describe things like [DropTables](#drop-tables), [ItemDefinitions](#item-definition), [AffixDefinitions](#affix-definition), [ItemQualityRatios](#item-quality-ratio) and [AffixCountRatios](#affix-count-ratio). All these DataTables provide the foundational data behind what Items are, what Affixes are, their qualities and how often they appear.

All of these aspects of Itemization and many others are customizable through the Plugin via `UObject` classes that can be overridden by Blueprints. These Blueprints expose core functions for things like how Items are selected, how they calculate certain attributes, which Affixes and how many of them an ItemInstance can have and much more.

**[⬆ Back to Top](#table-of-contents)**

<a name="sp"></a>
## 2. Sample Project

A sample third person project is included with this documentation and is designed to demonstrate the Generic Itemization Plugins flexibility and ease of use without introducing any additional code.

The goal is to keep the project simple for those that wish to take advantage of what it offers out of the box without having to dive deep into setting up the nitty gritty details. It does not focus on advanced usage such as overriding Affix Selection to implement more complex requirements that maybe desired on a per game basis.

It demonstrates the following non exhaustive list of concepts in no particular order:
* Defining Items as `ItemDefinitions` inside DataTables.
* Defining Affixes as `AffixDefinitions` inside DataTables.
* Selecting an `AffixPool` for an `ItemDefinition`.
* How to layout an `ItemQualityRatio` DataTable for Item `QualityType` determination.
* Setup of an `AffixCountRatio` DataTable to determine the number of Affixes for particular Item `QualityTypes`.
* Applications of an `AffixModifier` to `AffixDefinition` to describe what the Affix changes.
* Managing `AffixPickRequirements` to decide which Affixes can be selected by which Items.
* `UserData` for expressing additional information about ItemDefinitions we want to display to users.
* Changing `UItemInstancingContextFunction` for passing in context information for an Item being Instanced.
* Setting up `DropTables` that ultimately determine the pools of Items that can be selected for.
* Overriding the `AItemDrop` Actor to display actual instances of Items within the world.
* Usage of the `ItemDropperComponent` to actually drop Item Instances in the world.

The Sample Project has sample data taken from Diablo 2 in order to demonstrate the plugins usage, it also provides some minimal User Interface additions to make visualizing the Itemization that has been implemented.

![Sample Project Example](https://fissureentertainment.com/devilsd/UnrealEngine/GenericItemization/Documentation/SampleProject.JPG)

**[⬆ Back to Top](#table-of-contents)**

<a name="setup"></a>
## 3. Setting Up a Project Using the Generic Itemization Plugin

Setup of the Generic Itemization Plugin is extremely simple. It does not require any additional code in order to get running as it provides an end to end experience for the vast majority of its functionality out of the box.

Simply add the Plugin to your C++ or Blueprint based Project by placing the `GenericItemization` plugin folder into the `YOURPROJECT/Plugins/` folder. Do not forget to add it to your projects `.Build.cs` dependencies if you are going to utilize it in C++.

Once it is integrated as explained above, you are free to move ahead and add Items and Affixes, drop Items from Actors and manage them as necessary for your specific games needs.

**[⬆ Back to Top](#table-of-contents)**

<a name="class-layout"></a>
## 4. Class Layout

The Class Layout of the entire Generic Itemization Plugin is displayed below. It shows the relationships between different classes and struct types. As well as how some of their properties relate to one another. It also shows all of the functions available to each class.

![Item System Layout](https://fissureentertainment.com/devilsd/UnrealEngine/GenericItemization/Documentation/ItemSystem.png)

You will want to view it in full as a separate window in order to make out its details.

This overview of the structure of the Plugin may become useful for you later as you continue through this documentation.

**[⬆ Back to Top](#table-of-contents)**

<a name="concepts"></a>
## 5. Generic Itemization Concepts

#### Sections

> 5.1 [Drop Tables](#drop-tables)  
> 5.2 [Items](#items)  
> 5.3 [Affixes](#affixes)  
> 5.4 [Item Instancing Process](#item-instancing-process)  
> 5.5 [ItemDrop Actor](#item-drop-actor)  
> 5.6 [Item Dropper Component](#item-dropper-component)  
> 5.7 [Item Inventory Component](#item-inventory-component)  

<a name="drop-tables"></a>
### 5.1 Drop Tables

DropTables are the core data of what makes up the Generic Itemization plugin. They contain entries that describe how many Items can be selected for (`PickCount`) during the Item selection process, what the probability of no Item being selected for is (`NoPickChance`) and finally the actual layout of what Items or other DropTables are selectable when that entry is used during the Item selection process.

The entries in a DropTable are made up of a few different types. Which are described in [5.1.1 - DropTable Types](#drop-table-types).

The `PickCount` should be self explanatory. It is how many times we will make a selection from that entry for an Item Definition. 

The `NoPickChance` is a special property of the DropTable entries that describes the probability, for a single Pick, that nothing will be selected.

To make a simple example, if we had a DropTable entry with a `NoPickChance` of `1` and a single `ItemDropTable` entry that also has a `PickChance` of `1`. Both the `NoPick` and `ItemDropTable` entry have the same probability of being selected. They each have a 1/2 chance of being chosen. If the `NoPickChance` was `2`, then `NoPick` would have a 2/3 chance of being selected.

The above example is true by default for all selection processes throughout the entire plugin.

![Item Drop Table](https://fissureentertainment.com/devilsd/UnrealEngine/GenericItemization/Documentation/ItemDropTable.JPG)

<a name="drop-table-types"></a>
### 5.1.1 Drop Table Types

There are currently 3 different types of ItemDropTable entry types that can be selected from when composing a DropTable. Each serves a specific purpose that can be utilized to architect a DropTable that achieves any combination of outcomes for which Items will be selected for.

* `ItemDefinitionEntry`
* `ItemDefinitionCollection`
* `ItemDropTableCollection`

Each ItemDropTable entry type has a `PickChance` property that describes the probability of that entry being selected for against all of the other entries that may exist along side it. This also includes the `NoPickChance` which is added as the first entry in the pool of selectable entries during the selection process.

#### Item Definition Entry

The `ItemDefinitionEntry` type is a pointer to another entry in a DataTable that contains ItemDefinitions. It allows you to describe the selection of a single Item that you may want to be selected for specifically in a higher level of the DropTable, where you may want to override particular elements of its selection, such as its `PickChance` in relation to other entries.

![Item Definition Entry](https://fissureentertainment.com/devilsd/UnrealEngine/GenericItemization/Documentation/ItemDefinitionEntry.JPG)

#### Item Definition Collection Entry

The `ItemDefinitionCollection` entry type is a DataTable itself that contains `ItemDefinitions`, from which a selection will be made within that DataTable based on the `PickRequirements` and `PickFunction` that are described on the `ItemDefinitionCollection` entry within the DropTable. 

This types native `PickRequirements` allow you to define the `QualityLevel` range that a selection from that DataTable of `ItemDefinitions` will be made.

`PickRequirements` is an InstancedStruct that the `PickFunction` uses to determine what entries from the `ItemDefinition` pool are valid for selection. These can be overridden to provide extra requirements that you might want for your Itemization.

The `PickFunction` is an Object of `UItemDefinitionCollectionPickFunction` type and contains a single function `DoesItemDefinitionSatisfyPickRequirements` that can be overridden both in C++ and Blueprint. It is called for every `ItemDefinition` that is in the pool to compare it against the requirements.

This allows you to compose DataTables of `ItemDefinitions` with all of the Items you like, regardless of their `QualityLevel` together and still have fine grain control over which ones will be available as part of that entries selection pool.

Most entries in a DropTable will be of this type.

![Item Definition Collection Entry](https://fissureentertainment.com/devilsd/UnrealEngine/GenericItemization/Documentation/ItemDefinitionCollectionEntry.JPG)

#### Item DropTable Collection Entry

The `ItemDropTableCollection` entry is a little more complex than the others above. It has a recursive nature, in that it allows the selection of other DropTable entries with its own set of `PickRequirements` and accompanying `PickFunction`. 

By default, this type does not have any native `PickRequirements`. These can be overridden to implement any functionality you like for deciding which entries are selected for in the same way that the `ItemDefinitionCollection` entries `PickRequirements` and `PickFunction` can.

![Item Drop Table Collection Entry](https://fissureentertainment.com/devilsd/UnrealEngine/GenericItemization/Documentation/ItemDropTableCollectionEntry.JPG)

**[⬆ Back to Top](#table-of-contents)**

<a name="items"></a>
### 5.2 Items

Items are data structures which contain information that can affect gameplay which maybe presented to the Player in-game. 

The Generic Itemization Plugin is designed to strip away a lot of the game specific properties of Items that are typically found in games like Diablo and Path of Exile to give you a bare bones slate to build upon for what an Item means to you and your game. Items do however retain important components of what makes up the Itemization system as a whole, such as the concepts of rarity, distribution, quality and the idea of Affixes.

Items are the lifeblood of the Generic Itemization Plugin and of games that utilize those types of systems which you can find especially in ARPGs.

![Items](https://fissureentertainment.com/devilsd/UnrealEngine/GenericItemization/Documentation/Items.JPG)

**[⬆ Back to Top](#table-of-contents)**

<a name="item-definition"></a>
### 5.2.1 Item Definition

`ItemDefinition`s are one of the core types within the Generic Itemization plugin. It is what describes an individual Items static data. There are few parts of the plugin which do not touch an `ItemDefintion` in some way.

They are an Instanced Struct type held within DataTables. This means that you can override the default type with your own to introduce new information about what Items are to your game (`UserData` is a preferable choice to introducing new information, however there are some advantages to overriding the `ItemDefinition` itself).

![Item Definition](https://fissureentertainment.com/devilsd/UnrealEngine/GenericItemization/Documentation/ItemDefinition.JPG)

The `ItemDefinition` describes everything about an Item and also how an `ItemInstance` is to be generated from it.

**[⬆ Back to Top](#table-of-contents)**

<a name="item-definition-item-type"></a>
### 5.2.1.1 Item Type

The `ItemType` of an Item places it into a particular category that has consequences for elements of its behaviour and generation options that are available to it in the `Item Instancing Process`.

The main restriction that is imposed on an Item by its `ItemType` is the Affixes that are available to it when an `ItemInstance` of that Item is being generated. Affixes have requirements on the types of Items they can be generated on.

**[⬆ Back to Top](#table-of-contents)**

<a name="item-definition-spawnable"></a>
### 5.2.1.2 Spawnable

Because the Generic Itemization plugin is heavily reliant on data. There is a need for a mechanism to outright disable `ItemDefinition`s (and other concepts like `AffixDefinition`s) from being selectable in the `Item Instancing Process`.

The `bSpawnable` flag on the `ItemDefintion` provides this control. When this flag is set to `false`, that `ItemDefinition` cannot be selected during the `Item Instancing Process` and is effectively ignored.

This allows entries to remain within the DataTable (either because they are obsolete, outdated or otherwise) whilst still providing access to the static information they provide.

**[⬆ Back to Top](#table-of-contents)**

<a name="item-definition-pick-chance"></a>
### 5.2.1.3 Pick Chance

The `PickChance` outlined on an `ItemDefinition` is that entries probability for selection when composed into an `ItemDefinition` pool that the `Item Instancing Process` creates behind the scenes as its making selections.

**[⬆ Back to Top](#table-of-contents)**

<a name="item-definition-quality-type"></a>
### 5.2.1.4 Quality Type

The `QualityType` of an Item can be thought of as the grade of that type of Item. `ItemDefinition`s have 2 choices when defining their `QualityType`.

They can opt to have a predefined type or utilize an `ItemQualityRatio` DataTable that describes the probabilities of that Item being of a particular `QualityType`.

**[⬆ Back to Top](#table-of-contents)**

<a name="item-definition-quality-type-selection"></a>
### 5.2.1.4.1 ItemQualityRatio and Quality Type selection

If an `ItemDefinition` does not have a predefined `QualityType`, it must instead designate an `ItemQualityRatio` table. Such a table expresses all of the `QualityType`s that would be available to that `ItemDefinition` and their selection ratios.

When an `ItemInstance` is being generated from that `ItemDefinition` it will have the `UItemInstancingFunction::SelectItemQualityType` function called on its `ItemInstancingFunction` Object to manage selection of the `QualityType`. This function uses that `ItemQualityRatio` table to make the selection.

![Item Quality Ratio](https://fissureentertainment.com/devilsd/UnrealEngine/GenericItemization/Documentation/ItemQualityRatios.JPG)

The Ratios are defined in an Array. The process by which the selection is made is as follows. For each of the Ratio entries in the Array, a Suitability value is generated by taking the `Base`, `Divisor` and `Factor` values, along with the `ItemLevel` and `QualityLevel` of the `ItemInstance`. There is also a `MagicFind` term that is taken from the `ItemInstancingContext` that amplifies the probabilities of each `QualityType` being selected.

In the above example, the `QualityTypes` are laid out in a most valuable to least valuable form. With `Itemization.QualityType.Rare` being the first entry. Therefore its suitability value is generated first.

The Suitability value must be less than `128` for a `QualityType` to be successfully selected. The first `QualityType` to pass this test is the one chosen. If none pass then the last `QualityType` in the array is chosen instead.

The default formula used in the `UItemInstancingFunction::SelectItemQualityType` function for the selection of the `QualityType` looks like this:

```cpp
Suitability = (Base - ((ItemLevel - QualityLevel) / Divisor)) * 128;
```

The `MagicFind` is integrated along with the `Factor` which causes diminishing returns for that `QualityType`. 

```cpp
EffectiveMagicFind = (Factor > 0) ? (MagicFind * Factor / (MagicFind + Factor)) : MagicFind;
Suitability = Suitability * 100 / (100 + EffectiveMagicFind);
FinalSuitability  = Suitability - (Suitability / 1024);
```

With `MagicFind` integrated, the final value for Suitability is then set to a random number in the range `0 - Suitability`. This value is checked for less than `128` to which that `QualityType` would be selected if true. Otherwise the next `QualityType` is run through the same process and so on.

```cpp
FinalSuitability = ItemStream.RandHelper(FinalSuitability);
if (FinalSuitability < 128)
{
	// This QualityType is successfully selected.
	SelectedItemQualityType = ItemQualityRatioType.QualityType;
}
```

**[⬆ Back to Top](#table-of-contents)**

<a name="item-definition-affixes"></a>
### 5.2.1.5 Affix Count, Predefined Affixes and the Affix Pool

An `ItemDefintion` has the choice to define how many Affixes it will be able to generate at random from the `AffixPool`. This is managed through an `AffixCountRatio` DataTable. If this table is not defined on the `ItemDefinition` then it is assumed that Item cannot generate random Affixes. This is not the same as predefined Affixes, which are a separate property of an `ItemDefinition`, predefined Affixes are implied to be inherent to all `ItemInstances` of that `ItemDefinition` and are not randomly generated by the nature of them being predefined.

![Affix Count Ratio](https://fissureentertainment.com/devilsd/UnrealEngine/GenericItemization/Documentation/AffixCountRatios.JPG)

This DataTable outlines an Array similar to the `ItemQualityRatio` DataTable, in that each entry describes a `QualityType` and the range for the number of Affixes that it can have randomly generated for it.

It is rather simple in comparison, `UItemInstancingFunction::DetermineAffixCount` is called on the `ItemInstancingFunction` Object for that `ItemInstance` and it simply finds the entry in the `AffixCountRatio` DataTable that corresponds to its `QualityType` and takes a random value in the range `Min - Max` as shown in the example image above.

If no entry exists for that `QualityType` then it is assumed that it does not want to generate any random Affixes and will return a Affix Count of `0`.

There is also a flag on the `ItemDefinition` called `bOnlyPredefinedAffixes` which completely disables for `ItemInstances` using the `ItemDefinition` from generating any random Affixes regardless of if the `AffixCountRatio` DataTable is defined and it has a `QualityType` entry in that table. As its name suggests, only the list of `PredefinedAffixes` will be available on the `ItemInstance`.

The `AffixPool` is a property that defines a DataTable of all of the Affixes that are available to `ItemInstances` having Affixes generated for them from that `ItemDefinition`. See the section on [Affixes](#affixes) for further information on its layout and usage.

**[⬆ Back to Top](#table-of-contents)**

<a name="item-definition-user-data"></a>
### 5.2.1.6 User Data

The `UserData` property on the `ItemDefinition` is a Map of Gameplay Tags and Instanced Structs designed to provide a mechanism to associate arbitrary data with an `ItemDefinition`.

![User Data](https://fissureentertainment.com/devilsd/UnrealEngine/GenericItemization/Documentation/ItemUserData.JPG)

In the Sample Project, it is used to hold references to assets that are displayed on the User Interface, such as the `DisplayImage` of the `ItemInstance` that was generated from the `ItemDefinition`.

You can easily categories and query for specific data by making use of the Gameplay Tag key.

**[⬆ Back to Top](#table-of-contents)**

<a name="item-definition-instancing-function"></a>
### 5.2.1.7 Item Instancing Function

Each `ItemDefinition` must describe a valid `ItemInstancingFunction`. This Class type is a vital part of how `ItemInstance`s are created from `ItemDefinition`s.

It contains many functions, which can be overridden in C++ and Blueprint, that help an `ItemInstance` determine its properties from the `ItemDefinition` it is being generated from. You can look at the [Class Layout](#class-layout) for a full list of all of its functions. Many of which can be overridden to change their behaviour.

The default `UItemInstancingFunction` class is setup to manage generating an appropriate `ItemInstance` from an `ItemDefinition` and only Advanced Users that need alternative functionality should override it.

The `ItemInstancingFunction` class has its functions called from the Class Default Object of that type by the `Item Instancing Process` when necessary for generating the relevant properties of the `ItemInstance`.

It is also responsible for defining the `AffixPickFunction`, that controls how Affixes are selected from the Affix Pool for an Item when it is being generated.

**[⬆ Back to Top](#table-of-contents)**

<a name="item-instance"></a>
### 5.2.2 Item Instance

An `ItemInstance` is an actual Item that has been generated from an `ItemDefinition`. Typically it will be owned by an `ItemDrop` Actor or an `ItemInventoryComponent`, Advanced Users might want to manage it in other ways but by default only those 2 classes can own an `ItemInstance` that gets created by an `ItemDropperComponent`.

`ItemInstance`s are produced in accordance to the functions on the `ItemInstancingFunction` that is defined on the `ItemDefinition` that the `ItemInstance` is being generated from.

They contain a list of all of their Affixes as well as the `ItemInstancingContext` that was present when it was generated. They also retain the `ItemDefinition` they were generated with in order to have direct access to that static data for gameplay and other purposes.

![Item Instance](https://fissureentertainment.com/devilsd/UnrealEngine/GenericItemization/Documentation/ItemInstance.JPG)

The Struct type of an `ItemInstance` can only be overridden from the C++ function `UItemInstancingFunction::MakeItemInstance`, due to Blueprint Structs not being able to support inheritance from a base type (as `ItemInstance`s must be derived from the `FItemInstance` struct type). If you need to introduce additional functionality or properties to an `ItemInstance` you would need to manage it in C++. Alternatively utilizing Affixes might be a sufficient method depending on your needs.

**[⬆ Back to Top](#table-of-contents)**

<a name="affixes"></a>
### 5.3 Affixes

Affixes are collections of modifiers that affect changes to properties of an Item or any other gameplay element. The Generic Itemization Plugin provides a mechanism for defining and generating Affixes for Items, however it does not go into great detail on the final usage of those Affixes. That is up to the User to describe.

Games like Diablo and Path of Exile use Affixes to change the properties of an Item itself, the Player Character Attributes, Strength and Power. As well as make changes to gameplay like granting abilities or performing other action dependant operations.

![Affixes](https://fissureentertainment.com/devilsd/UnrealEngine/GenericItemization/Documentation/Affixes.JPG)

The Sample Project splits Affixes into 2 groups, `Prefixes` and `Suffixes` to demonstrate that different combinations of these can be restricted to appearing on specific Item `QualityTypes`. The Generic Itemization Plugin provides many mechanisms for managing Affixes.

**[⬆ Back to Top](#table-of-contents)**

<a name="affix-definition"></a>
### 5.3.1 Affix Definition

`AffixDefinition`s are another of the core types within the Generic Itemization plugin. It is what describes an individual Affixes static data especially including the types of Modifiers that it will apply for an Item.

They are an Instanced Struct type held within DataTables. This means that you can override the default type with your own to introduce new information about what Affixes are to your game.

![Affix Definition](https://fissureentertainment.com/devilsd/UnrealEngine/GenericItemization/Documentation/AffixDefinition.JPG)

**[⬆ Back to Top](#table-of-contents)**

<a name="affix-definition-affix-type"></a>
### 5.3.1.1 Affix Type

The `AffixType` of an Affix places it into a particular category that has consequences for elements of its behaviour and generation options that are available to it and the `ItemInstance` that it maybe being selected for in the `Item Instancing Process`.

The main restriction that is imposed on an Item by an `AffixDefinition`s `AffixType` is that Affix will not be selectable if another Affix of the same `AffixType` has already been chosen for that `ItemInstance` being generated. This restriction is imposed by the `UAffixPickFunction::GetAffixesWithMinimumNativeRequirements` function which generates the Affix Pool available for selection for an `ItemInstance` during the `Item Instancing Process` based on its requirements.

**[⬆ Back to Top](#table-of-contents)**

<a name="affix-definition-spawnable"></a>
### 5.3.1.2 Spawnable

Because the Generic Itemization plugin is heavily reliant on data. There is a need for a mechanism to outright disable `AffixDefinition`s (and other concepts like `ItemDefinition`s) from being selectable in the `Item Instancing Process`.

The `bSpawnable` flag on the `AffixDefintion` provides this control. When this flag is set to `false`, that `AffixDefinition` cannot be selected during the `Item Instancing Process` and is effectively ignored.

This allows entries to remain within the DataTable (either because they are obsolete, outdated or otherwise) whilst still providing access to the static information they provide.

**[⬆ Back to Top](#table-of-contents)**

<a name="affix-definition-pick-chance"></a>
### 5.3.1.3 Pick Chance

The `PickChance` outlined on an `AffixDefinition` is that entries probability for selection when composed into an `AffixDefinition` pool that the `Item Instancing Process` creates behind the scenes as its making selections.

**[⬆ Back to Top](#table-of-contents)**

<a name="affix-definition-occurs-for"></a>
### 5.3.1.4 Occurs for Item Types, Quality Types and Quality Level

There are several requirements that an `ItemInstance` that wants to have an Affix applied to it must meet from the Affix before it can have that Affix be considered within the Affix Pool for selection.

The native requirements from the Plugin are described in this section are mandatory to ensure that Affixes are applied correctly.

**Occurs for Item Types**

Affixes are able to manage which types of Items that they can appear on. This property is a Gameplay Tag Container that lists all of the `ItemType`s that individual Affix can be available to be selected for.

**Occurs for Quality Types**

Just with `ItemType`s, you can restrict access of an Affix to particular `QualityType`s of Items as well. In the Sample Project there are a number of `QualityType`s with different characteristics, we restrict particular Affixes from only appearing on these types and not others.

**Occurs for Quality Level**

You can also restrict an Affix from appearing on Items that do not meet a minimum `QualityLevel`. This is useful for ensuring that higher `QualityLevel` Items do not try to select from low quality Affixes if that concept is important to your game.

All of these requirements are checked and managed during the `Item Instancing Process` within the `UAffixPickFunction::GetAffixesWithMinimumNativeRequirements` function. This function specifically cannot be overridden in Blueprint, but is marked `virtual` for Advanced Users to modify within C++ if they so require. It is recommended for Blueprint Users to instead override the `UAffixPickFunction::PickAffix` function to introduce new requirements that they might need.

**[⬆ Back to Top](#table-of-contents)**

<a name="affix-definition-require-affix-level"></a>
### 5.3.1.5 Required Item Affix Level

These Minimum and Maximum Item Affix Level property requirements are relatively straight forward in that they provide control for restricting an Affix from being selectable during the `Item Instancing Process` on Items that do not meet that `AffixLevel` range. It is useful for stopping advanced Affixes from appearing on lower quality Items or inversely restricting low quality Affixes from applying to high quality Items.

**[⬆ Back to Top](#table-of-contents)**

<a name="affix-definition-modifiers"></a>
### 5.3.1.6 Modifiers

TODO

**[⬆ Back to Top](#table-of-contents)**

<a name="affix-definition-affix-type"></a>
### 5.3.1.1 Affix Type

TODO

**[⬆ Back to Top](#table-of-contents)**

<a name="affix-instance"></a>
### 5.3.2 Affix Instance

TODO

**[⬆ Back to Top](#table-of-contents)**

<a name="item-instancing-process"></a>
### 5.4 Item Instancing Process

The Generic Itemization plugin currently provides a relatively straight forward mechanism for generating Items from `ItemDefinitions` defined in DropTables. The `ItemDropperComponent`s `DropItems` function is the entry point to generating `ItemInstances` and `ItemDrop` Actors that can represent those instances within the world.

The Item Instancing Process is the steps that are taken from when the `DropItems` function is called to when it produces an `ItemDrop` Actor. The short list of what occurs during this process is as follows:

1. Build an `ItemInstancingContext`.
2. Find the number of Items to drop (the `PickCount`).
3. For each of the Picks, select an `ItemDefinition` or a `NoPick` result.
4. Calculate the Affix Level from the `ItemLevel` and `QualityLevel` on the `ItemDefinition`.
5. Randomly select the Item `QualityType` (which can be predefined) from the `ItemQualityRatio` on the `ItemDefinition`.
6. Add any predefined Affixes that are described on the `ItemDefinition`.
7. Determine the number of Affixes to randomly generate, from the `AffixCountRatio` on the `ItemDefinition`.
8. For Affix Count number of Affixes, generate them from the Affix Pool on the `ItemDefinition`.
9. Spawn an `ItemDrop` Actor and assign the `ItemInstance` to it.

**[⬆ Back to Top](#table-of-contents)**

<a name="item-drop-actor"></a>
### 5.5 ItemDrop Actor

`ItemDrop` Actors are essentially a wrapper around an `ItemInstance` that allow it to exist within the world. It provides the ability to visualize the `ItemInstance` and they are used in conjunction with the `ItemDropperComponent`.

It provides some convenience functions for accessing the `ItemInstances` and sliced versions of the `ItemInstance` struct and `ItemDefinition` struct of the `ItemInstance` it is representing.

It has support for the `ItemInstance` being a replicated property as well. This means that the `ItemDrop` Actor can be replicated and managed across the Network, passing along its `ItemInstance` in the process for all Clients to see.

The `ItemInstance` property on the `ItemDrop` Actor is set by the `ItemDropperComponent` when it is spawned in the world and is unchanged after that.

**[⬆ Back to Top](#table-of-contents)**

<a name="item-dropper-component"></a>
### 5.6 Item Dropper Component

The `ItemDropperComponent` is a component that sits on an Actor to facilitate the entry point to dropping `ItemDrop` Actors that represent `ItemInstances` within the world, for that Actor from a specified DropTable.

It implements a single function `UItemDropperComponent::DropItems` which can be called from C++ or Blueprint to drop Items from its specified DropTable. This function is also where you pass through context information that surrounds and supports any `ItemInstances` that comes from this `ItemDropperComponent`. The Sample Project passes in the Items Level and the Magic Find value to use during the Item Instancing Process.

It also has an `ItemDropClass` property that allows you to specify which `ItemDrop` Actor type you want it to spawn. This is useful for overriding the visual representation of an `ItemInstance` within the world. The Sample Project overrides this to provide an appropriate visualization of `ItemInstances` that have been dropped.

![Item Dropper Component](https://fissureentertainment.com/devilsd/UnrealEngine/GenericItemization/Documentation/ItemDropperComponent.JPG)

**[⬆ Back to Top](#table-of-contents)**

<a name="item-instancing-context"></a>
### 5.6.1 Item Instancing Context and the Context Provider Function

In order to feed additional data to the `Item Instancing Process` about information such as what `Level` and how much `MagicFind` (along with any other custom data you might need along the way) to apply to a particular Item that is being generated, the `ItemDropperComponent` provides a class property for defining the `Context Provider Function` which is an `UItemInstancingContextFunction` class type and feeds it an Instanced Struct `UserContextData` parameter through its `DropItems` function.

![Drop Items and User Context Data](https://fissureentertainment.com/devilsd/UnrealEngine/GenericItemization/Documentation/DropItemsUserContextData.JPG)

The `UItemInstancingContextFunction` provides a single function called on its Class Default Object at the start of the `Item Instancing Process` to build a new `ItemInstancingContext` that is passed along every step of the `Item Instancing Process` and is available in almost all functions that are called along the way.

![Build Item Instancing Context](https://fissureentertainment.com/devilsd/UnrealEngine/GenericItemization/Documentation/BuildItemInstancingContext.JPG)

The `ItemInstancingContext` is also copied onto every `ItemInstance` that is produced, so that Item knows the context information around when it was created.

The `ItemInstancingContextFunction` can be overridden in both C++ and Blueprint and it is recommended that you do so in order to pass through information that may affect the outcome of the generation of an `ItemInstance`. The Sample Project does this as shown above, to appropriately fill the `ItemLevel` and `MagicFind` native properties of the `ItemInstancingContent` so that Items can be generated correctly according to those values.

**[⬆ Back to Top](#table-of-contents)**

<a name="item-inventory-component"></a>
### 5.7 Item Inventory Component

The `UItemInventoryComponent` class is currently not implemented but is a planned feature for a future update. Its purpose would be to provide a mechanism to store a number of `ItemInstances` that might have come from, for example, `ItemDrop` Actors that were dropped within the world.

**[⬆ Back to Top](#table-of-contents)**

<a name="other-resources"></a>
## 6. Other Resources

The best documentation for understanding the Instanced Structs is by far the source code of the Plugin itself.
[Struct Utils Plugin Source Code](https://github.com/EpicGames/UnrealEngine/tree/release/Engine/Plugins/Experimental/StructUtils)

The Class Layout was created in the Online UML Diagramming tool [Draw.io](https://www.drawio.com/)

Special thanks to the [Unreal Source](https://unrealsource.com/) Discord community for the support.

If you found the Generic Itemization Plugin useful to you, please [consider making a donation](https://donate.stripe.com/3cs2c15SIbfXeoo5kk), although I provide it for free and with no expectations, I have put considerable time into this work and appreciate any contributions.

You can find my contact information on my website:
[https://fissureentertainment.com/](https://fissureentertainment.com/)

**[⬆ Back to Top](#table-of-contents)**
