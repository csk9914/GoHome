//THE

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DeathNotifier.generated.h"

UINTERFACE(MinimalAPI)
class UDeathNotifier : public UInterface
{
	GENERATED_BODY()
};

// 사망 이벤트만 노출하는 오브젝트가 구현한다(UHealthComponent 등).
// Interaction/Item이 HealthComponent 내부 구조를 몰라도 사망 시점을 구독할 수 있게 이 인터페이스로만 접근한다.

class GOHOME_API IDeathNotifier
{
	GENERATED_BODY()

public:

	virtual FSimpleMulticastDelegate& GetOnDeathDelegate() = 0;

};
