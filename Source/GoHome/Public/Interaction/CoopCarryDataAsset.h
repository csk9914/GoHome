

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CoopCarryDataAsset.generated.h"

class UStaticMesh;

// 협동 운반 오브젝트 종류별 정보. 정산 가치/메쉬/표시 이름을 데이터로 관리.
UCLASS(BlueprintType)
class GOHOME_API UCoopCarryDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CoopCarry")
	FText DisplayName;

	// 납품 시 정산되는 가치. 0이면 정산 대상 아님(퍼즐용 소품 등으로 확장 가능).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CoopCarry")
	float Value = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CoopCarry")
	TObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CoopCarry")
	FVector Scale = FVector::OneVector;

};
