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
>>5.1 [Items](#items)  
>>>5.1.1 [Sub Section](#concepts-asc-rm)  
>
>>5.2 [Affixes](#affixes)  
>>>5.2.1 [Sub Section](#concepts-asc-rm)  
>
>>5.3 [Item Drop Actor](#item-drop-actor)  
>>5.4 [Item Dropper Component](#item-dropper-component)  
>>5.5 [Item Inventory Component](#item-inventory-component)  
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

It demonstrates the following concepts:
* List
* Concepts
* Here

**[⬆ Back to Top](#table-of-contents)**

<a name="setup"></a>
## 3. Setting Up a Project Using the Generic Itemization Plugin

Setup of the Generic Itemization Plugin is extremely simple. It does not require any additional code in order to get running as it provides an end to end experience for the vast majority of its functionality out of the box.

Simply add the Plugin to your Project and ensure that it is enabled. If you are using it via C++ do not forget to add it to your projects `.Build.cs` dependencies.

Once it is integrated as explained above, you are free to move ahead and add Items and Affixes, drop Items from Actors and manage them as necessary for your game.

**[⬆ Back to Top](#table-of-contents)**

<a name="class-layout"></a>
## 4. Class Layout

The Class Layout of the entire Generic Itemization Plugin is displayed below. It shows the relationships between different classes and struct types. As well as how some of their properties relate to one another. It also shows all of the functions available to each class.

![Item System Layout](https://fissureentertainment.com/devilsd/UnrealEngine/GenericItemization/Documentation/ItemSystemTransparent.png)

You will want to view it in full as a separate window in order to make out its details.

This overview of the structure of the Plugin may become useful for you later as you continue through this documentation.

**[⬆ Back to Top](#table-of-contents)**

<a name="concepts"></a>
## 5. Generic Itemization Concepts

#### Sections

> 5.1 [Items](#items)  
> 5.2 [Affixes](#affixes)  
> 5.3 [ItemDrop Actor](#item-drop-actor)  
> 5.4 [Item Dropper Component](#item-dropper-component)  
> 5.5 [Item Inventory Component](#item-inventory-component)  

<a name="items"></a>
### 5.1 Items

TODO

**[⬆ Back to Top](#table-of-contents)**

<a name="affixes"></a>
### 5.2 Affixes

TODO

**[⬆ Back to Top](#table-of-contents)**

<a name="item-drop-actor"></a>
### 5.3 ItemDrop Actor

TODO

**[⬆ Back to Top](#table-of-contents)**

<a name="item-dropper-component"></a>
### 5.4 Item Dropper Component

TODO

**[⬆ Back to Top](#table-of-contents)**

<a name="item-inventory-component"></a>
### 5.5 Item Inventory Component

TODO

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
