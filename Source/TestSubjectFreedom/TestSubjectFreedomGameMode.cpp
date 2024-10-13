// Copyright Epic Games, Inc. All Rights Reserved.

#include "TestSubjectFreedomGameMode.h"
#include "TestSubjectFreedomCharacter.h"
#include "UObject/ConstructorHelpers.h"

ATestSubjectFreedomGameMode::ATestSubjectFreedomGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
