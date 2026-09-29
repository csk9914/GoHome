#pragma once

#include "CoreMinimal.h"
#include "TitleTypes.generated.h"

// 검색 결과 0개(Empty)와 온라인 서브시스템 실패(Failed)를 UI가 구분할 수 있게 분리한다.
UENUM(BlueprintType)
enum class ETitleSearchStatus : uint8
{
	Idle,
	Searching,
	Ready,
	Empty,
	Failed,
};

UENUM(BlueprintType)
enum class ETitleHostMode : uint8
{
	Continue,
	NewExpedition,
};

USTRUCT(BlueprintType)
struct FTitleSessionListing
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Title")
	FText HostName;

	// 이 스냅샷을 만든 검색 결과 배열에서의 원본 인덱스
	UPROPERTY(BlueprintReadOnly, Category = "Title")
	int32 SearchIndex = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct FTitleSearchSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Title")
	ETitleSearchStatus Status = ETitleSearchStatus::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Title")
	TArray<FTitleSessionListing> Listings;

	// 검색 완료마다 증가. 참가 요청은 이 값이 최신과 같을 때만 유효 — 갱신 사이에 인덱스가 어긋나는 것을 막는다.
	UPROPERTY(BlueprintReadOnly, Category = "Title")
	int32 Generation = 0;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FTitleSearchUpdatedEvent, const FTitleSearchSnapshot&);
DECLARE_MULTICAST_DELEGATE_OneParam(FTitleRequestCompleteEvent, bool /*bWasSuccessful*/);

struct FTitleBackendEvents
{
	FTitleSearchUpdatedEvent OnSearchUpdated;
	FTitleRequestCompleteEvent OnHostComplete;
	FTitleRequestCompleteEvent OnJoinComplete;
};
