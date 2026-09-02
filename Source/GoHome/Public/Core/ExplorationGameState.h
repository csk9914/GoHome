// 

#pragma once

#include "CoreMinimal.h"
#include "GoHomeGameState.h"
#include "ExplorationGameState.generated.h"

class UHealthComponent;

/**
 * 
 */

UCLASS()
class GOHOME_API AExplorationGameState : public AGoHomeGameState
{
	GENERATED_BODY()

public:
	AExplorationGameState();
	
	void SetExpeditionDeadline(float InDeadlineServerTime);

	// 위젯이 매 틱 호출해 카운트다운 표시 (서버, 클라)
	UFUNCTION(BlueprintPure, Category = "Expedition")
	float GetRemainingSeconds() const;
	
	UFUNCTION(BlueprintPure, Category = "Expedition")
	bool HasTimeLimit()const {return ExpeditionDeadline>0.f;}
	
protected:
	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
private:
	UPROPERTY(Replicated)
	float ExpeditionDeadline = 0.f;
	
};
