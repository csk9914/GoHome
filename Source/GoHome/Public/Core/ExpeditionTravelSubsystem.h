// 

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ExpeditionTravelSubsystem.generated.h"

/**
 * 로딩 화면(LV_Loading) 경유 트래블 중, 진짜 목적지 맵 경로를 잠깐 들고 있는다.
 * GameMode는 트래블마다 새로 생성되므로, 트래블 간 유지되는 GameInstanceSubsystem에 둔다
 */
UCLASS()
class GOHOME_API UExpeditionTravelSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadWrite, Category = "Expedition")
	FString PendingDestinationMap;
};
