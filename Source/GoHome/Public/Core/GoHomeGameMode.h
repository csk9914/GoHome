

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GoHomeGameMode.generated.h"

/**
 * 접속 종료(Logout) 처리 전용. GameState의 OnPlayerRemovedFromParty로 합류시켜
 * 사망(OnDeath)과 접속 종료 두 경로 모두 생존자 수 집계가 어긋나지 않게 한다.
 */
UCLASS()
class GOHOME_API AGoHomeGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual void Logout(AController* Exiting) override;
};
