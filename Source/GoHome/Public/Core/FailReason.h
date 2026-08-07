

#pragma once

#include "CoreMinimal.h"
#include "FailReason.generated.h"

UENUM(BlueprintType)
enum class EFailReason : uint8
{
	AllPlayersDead,
	TimeExpired,
	DockThreatened
};
