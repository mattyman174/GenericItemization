# Generic Itemization Plugin
<a name="top"></a>
### Foreword
The underpinning motivation behind the development of this Plugin and its subsequent free availability to the community was borne out of a desire to simply explore and understand the limitations of the [Instanced Struct](#instanced-structs) framework and especially its usefulness towards DataTables. While also providing a useful implementation of Itemization utilizing these features that you might find in a traditional ARPG (similar to Diablo or Path of Exile).

Added in Unreal Engine 5.0 and better developed in later engine versions, Instanced Structs work similarly as instanced `UObject*` properties but are `USTRUCT`s. DataTables do not support the usage of instanced `UObject*` properties and thus the introduction of Instanced Structs brings much needed flexibility to DataTables. This plugin will explore that new relationship.

This Project and Plugin are a work in progress and as stated before, my own exploration into Instanced Structs. I make no guarantee of the accuracy of information or the approach being close to best practice. I will update this plugin as I learn more and get a better understanding of how to make use of Instanced Structs, especially in conjunction with Blueprint.

If you find the Generic Itemization Plugin useful to you, please [consider making a donation](https://donate.stripe.com/3cs2c15SIbfXeoo5kk), although I provide it for free and with no expectations, I have put considerable time into this work and appreciate any contributions.

You can find my contact information on my website:
[https://fissureentertainment.com/](https://fissureentertainment.com/)

<a name="table-of-contents"></a>
## Table of Contents

>1. [Intro to Instanced Structs and the Generic Itemization Plugin](#intro)  
>2. [Sample Project](#sp)  
>3. [Setting Up a Project Using the Generic Itemization Plugin](#setup)  
>4. [Class Layout](#class-layout)  
>5. [Generic Itemization Concepts](#concepts)  
>>5.1 [Drop Tables](#drop-tables)
>>>5.1.1 [Drop Table Types](#drop-table-types)
>
>>5.2 [Items](#items)  
>>5.3 [Affixes](#affixes)  
>>5.4 [Item Drop Actor](#item-drop-actor)  
>>5.5 [Item Dropper Component](#item-dropper-component)  
>>5.6 [Item Inventory Component](#item-inventory-component)  
>6. [Other Resources](#other-resources)  
         
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

Simply add the Plugin to your C++ Project, ensure that it is enabled and compile. Do not forget to add it to your projects `.Build.cs` dependencies.

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
> 5.4 [ItemDrop Actor](#item-drop-actor)  
> 5.5 [Item Dropper Component](#item-dropper-component)  
> 5.6 [Item Inventory Component](#item-inventory-component)  

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

* Item Definition
* ItemDefinitionCollection
* ItemDropTableCollection

Each ItemDropTable entry type has a `PickChance` property that describes the probability of that entry being selected for against all of the other entries that may exist along side it. This also includes the `NoPickChance` which is added as the first entry in the pool of selectable entries during the selection process.

#### Item Definition Entry

The `ItemDefinition` entry type is a pointer to another entry in a DataTable that contains ItemDefinitions. It allows you to describe the selection of a single Item that you may want to be selected for specifically in a higher level of the DropTable, where you may want to override particular elements of its selection, such as its `PickChance` in relation to other entries.

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

TODO

**[⬆ Back to Top](#table-of-contents)**

<a name="affixes"></a>
### 5.3 Affixes

TODO

**[⬆ Back to Top](#table-of-contents)**

<a name="item-drop-actor"></a>
### 5.4 ItemDrop Actor

`ItemDrop` Actors are essentially a wrapper around an `ItemInstance` that allow it to exist within the world. It provides the ability to visualize the `ItemInstance` and they are used in conjunction with the `ItemDropperComponent`.

It provides some convenience functions for accessing the `ItemInstances` and sliced versions of the `ItemInstance` struct and `ItemDefinition` struct of the `ItemInstance` it is representing.

It has support for the `ItemInstance` being a replicated property as well. This means that the `ItemDrop` Actor can be replicated and managed across the Network, passing along its `ItemInstance` in the process for all Clients to see.

The `ItemInstance` property on the `ItemDrop` Actor is set by the `ItemDropperComponent` when it is spawned in the world and is unchanged after that.

**[⬆ Back to Top](#table-of-contents)**

<a name="item-dropper-component"></a>
### 5.5 Item Dropper Component

The `ItemDropperComponent` is a component that sits on an Actor to facilitate the entry point to dropping `ItemDrop` Actors that represent `ItemInstances` within the world, for that Actor from a specified DropTable.

It implements a single function `UItemDropperComponent::DropItems` which can be called from C++ or Blueprint to drop Items from its specified DropTable. This function is also where you pass through context information that surrounds and supports any `ItemInstances` that comes from this `ItemDropperComponent`. The Sample Project passes in the Items Level and the Magic Find value to use during the Item Instancing Process.

It also has an `ItemDropClass` property that allows you to specify which `ItemDrop` Actor type you want it to spawn. This is useful for overriding the visual representation of an `ItemInstance` within the world. The Sample Project overrides this to provide an appropriate visualization of `ItemInstances` that have been dropped.

![Item Dropper Component](https://fissureentertainment.com/devilsd/UnrealEngine/GenericItemization/Documentation/ItemDropperComponent.JPG)

**[⬆ Back to Top](#table-of-contents)**

<a name="item-inventory-component"></a>
### 5.6 Item Inventory Component

The `UItemInventoryComponent` class is currently not implemented but is a planned feature for a future update. Its purpose would be to provide a mechanism to store a number of `ItemInstances` that might have come from, for example, `ItemDrop` Actors that were dropped within the world.

**[⬆ Back to Top](#table-of-contents)**

<a name="other-resources"></a>
## 6. Other Resources

The best documentation for understanding the Instanced Structs is by far the source code of the Plugin itself.
[Struct Utils Plugin Source Code](https://github.com/EpicGames/UnrealEngine/tree/release/Engine/Plugins/Experimental/StructUtils)

The Class Layout was created in the Online UML Diagramming tool [Draw.io](https://www.drawio.com/)

If you found the Generic Itemization Plugin useful to you, please [consider making a donation](https://donate.stripe.com/3cs2c15SIbfXeoo5kk), although I provide it for free and with no expectations, I have put considerable time into this work and appreciate any contributions.

You can find my contact information on my website:
[https://fissureentertainment.com/](https://fissureentertainment.com/)

**[⬆ Back to Top](#table-of-contents)**
