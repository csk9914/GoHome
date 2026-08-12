#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CarryWeightProvider.generated.h"

// Unreal reflection wrapper for ICarryWeightProvider.
UINTERFACE(MinimalAPI)
class UCarryWeightProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 * 현재 운반 무게 상태를 제공하는 계약.
 * 산소, UI, 수류 같은 시스템은 CarryWeightComponent의 구체 타입을 몰라도
 * 이 인터페이스만 보고 현재무게/최대무게/초과무게를 읽을 수 있다.
 */
class GOHOME_API ICarryWeightProvider
{
	GENERATED_BODY()

public:
	virtual float GetCurrentCarryWeight() const = 0;
	virtual float GetMaxCarryWeight() const = 0;
	virtual float GetOverweightAmount() const = 0;
};
