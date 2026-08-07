

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NoiseType.generated.h"

/** 반경값은 02문서 5절 기준: Small(300~500), Medium(800~1200), Large(1500~2000), Alarm(2500~3000). */
UENUM(BlueprintType)
enum class ENoiseType : uint8
{
	Small,
	Medium,
	Large,
	Alarm
};

/**
 * GenerateNoise는 호출 즉시 서버가 반경 내 몬스터를 순회하는 동기 함수다 (이벤트/델리게이트 아님).
 * Player(이동 사운드)·Item(소음 유발형)이 발생원으로서 호출한다.
 */
UCLASS()
class GOHOME_API UGoHomeNoiseLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Noise")
	static void GenerateNoise(const UObject* WorldContextObject, FVector Location, float Radius, ENoiseType Type, AActor* Source);
};
