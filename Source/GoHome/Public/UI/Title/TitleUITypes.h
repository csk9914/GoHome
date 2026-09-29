#pragma once

#include "CoreMinimal.h"
#include "TitleUITypes.generated.h"

UENUM(BlueprintType)
enum class ETitleMenuAction : uint8
{
	Host,
	Find,
	Settings,
	Quit,
};
