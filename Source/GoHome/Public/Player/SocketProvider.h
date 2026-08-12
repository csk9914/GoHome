// THE

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SocketProvider.generated.h"

UINTERFACE(MinimalAPI)
class USocketProvider : public UInterface
{
	GENERATED_BODY()
};


// 오른손/왼손 소켓 이름을 노출하는 오브젝트가 구현한다(AGoHomeCharacter 등).
// Interaction의 소켓 어태치 로직이 Character 내부 구조를 몰라도 되게 이 인터페이스로만 접근한다.

class GOHOME_API ISocketProvider
{
	GENERATED_BODY()

public:
	virtual FName GetRightHandSocketName() const = 0;
	virtual FName GetLeftHandSocketName() const = 0;
};
