// Copyright Epic Games, Inc. All Rights Reserved.

#include "BunjilandGameMode.h"
#include "BunjilandCharacter.h"
#include "UObject/ConstructorHelpers.h"

ABunjilandGameMode::ABunjilandGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
