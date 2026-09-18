// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "MagicTourCameraManager.generated.h"

/**
 *  Camera manager for the third-person character.
 *  Limits min/max look pitch.
 */
UCLASS()
class AMagicTourCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()
	
public:

	/** Constructor */
	AMagicTourCameraManager();
};
