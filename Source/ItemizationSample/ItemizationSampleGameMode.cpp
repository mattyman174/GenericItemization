// Copyright Epic Games, Inc. All Rights Reserved.

#include "ItemizationSampleGameMode.h"
#include "ItemizationSamplePlayerController.h"
#include "ItemizationSampleCharacter.h"
#include "UObject/ConstructorHelpers.h"

AItemizationSampleGameMode::AItemizationSampleGameMode()
{
	// use our custom PlayerController class
	PlayerControllerClass = AItemizationSamplePlayerController::StaticClass();

	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/TopDown/Blueprints/BP_TopDownCharacter"));
	if (PlayerPawnBPClass.Class != nullptr)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}

	// set default controller to our Blueprinted controller
	static ConstructorHelpers::FClassFinder<APlayerController> PlayerControllerBPClass(TEXT("/Game/TopDown/Blueprints/BP_TopDownPlayerController"));
	if(PlayerControllerBPClass.Class != NULL)
	{
		PlayerControllerClass = PlayerControllerBPClass.Class;
	}
}