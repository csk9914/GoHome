

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GoHomeSaveSubsystem.generated.h"

class UGoHomeSaveGame;
enum class EExpeditionState : uint8;

/**
 * 트래블 간 유지되는 GameInstanceSubsystem. GameState는 맵 이동마다 새로 스폰되므로
 * 이 서브시스템이 매 레벨 GameState::BeginPlay에서 OnStateChanged에 재구독한다.
 * NewState == Lobby일 때 저장하며, 구독 직후 현재 상태가 이미 Lobby면 즉시 저장한다
 * (로비 맵 재진입 시 엣지 트리거 누락 방지).
 */
UCLASS()
class GOHOME_API UGoHomeSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Save")
	TObjectPtr<UGoHomeSaveGame> SaveGame;

	UFUNCTION(BlueprintCallable, Category = "Save")
	void SaveToDisk();

protected:
	UFUNCTION()
	void OnExpeditionStateChanged(EExpeditionState NewState);
};
