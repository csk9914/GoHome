

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RewardEntranceActor.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UItemDataAsset;
class UNiagaraSystem;
class USoundBase;

UENUM(BlueprintType)
enum class ERewardGrade : uint8
{
	High,  // 퍼즐 성공 + 차단기 조작 - 최고 보상.
	Risky  // 퍼즐 실패 - 일반 오브젝트보단 낫지만 High보단 낮은 보상 (고위험 상태에서 획득).
};

UENUM(BlueprintType)
enum class EEntranceState : uint8
{
	Sealed,
	OpenedHigh,
	OpenedRisky
};

// 등급별 보상 구성. Pool에서 Count개를 (중복 허용) 무작위로 뽑아 스폰한다.
USTRUCT(BlueprintType)
struct FRewardTierConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	TArray<TObjectPtr<UItemDataAsset>> Pool;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward", meta = (ClampMin = "0"))
	int32 Count = 1;
};

// 배전반 퍼즐 결과로 열리는 보상 입구.
// 권위: EntranceState 하나가 "열림 여부"의 유일한 진실 - 서버만 ServerOpen으로 바꾸고, 클라는 OnRep으로 연출만 재생.
// 보상 아이템은 열리는 순간 서버가 스폰 (등급은 결과가 나와야 확정되므로)

UCLASS()
class GOHOME_API ARewardEntranceActor : public AActor
{
	GENERATED_BODY()
	
public:	

	ARewardEntranceActor();


	bool IsOpened() const { return EntranceState != EEntranceState::Sealed; }

	// 서버 전용. Sealed일 때만 1회 동작(이미 열렸으면 무시 -> 차단기 중복 조작에도 안전).
	void ServerOpen(ERewardGrade Grade);

protected:

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 입구를 막는 문/셔터. 틈(slit)이 뚫린 메시를 넣고, 콜리전은 틈까지 막히게 단순 박스로 - 빛만 새어 나오게.
	UPROPERTY(VisibleAnywhere, Category = "RewardEntrance")
	TObjectPtr<UStaticMeshComponent> BlockerMesh;

	// 틈으로 새어 나오는 빛. 위치는 BP에서 문 안쪽에 배치. 컴포넌트의 Intensity가 배율 1.0의 기준.
	UPROPERTY(VisibleAnywhere, Category = "RewardEntrance")
	TObjectPtr<UPointLightComponent> GapGlow;

	// --- 보상 (LD가 입구마다 조정) ---
	UPROPERTY(EditAnywhere, Category = "RewardEntrance|Reward")
	FRewardTierConfig HighReward;

	UPROPERTY(EditAnywhere, Category = "RewardEntrance|Reward")
	FRewardTierConfig RiskyReward;

	// --- 글로우: 봉인 = 중립, High = 금색 강하게, Risky = 붉은 경고색 ---
	UPROPERTY(EditAnywhere, Category = "RewardEntrance|Glow")
	FLinearColor SealedGlowColor = FLinearColor(0.6f, 0.9f, 1.f);

	UPROPERTY(EditAnywhere, Category = "RewardEntrance|Glow")
	float SealedGlowScale = 1.f;

	UPROPERTY(EditAnywhere, Category = "RewardEntrance|Glow")
	FLinearColor HighGlowColor = FLinearColor(1.f, 0.75f, 0.2f);

	UPROPERTY(EditAnywhere, Category = "RewardEntrance|Glow")
	float HighGlowScale = 4.f;

	UPROPERTY(EditAnywhere, Category = "RewardEntrance|Glow")
	FLinearColor RiskyGlowColor = FLinearColor(1.f, 0.15f, 0.1f);

	UPROPERTY(EditAnywhere, Category = "RewardEntrance|Glow")
	float RiskyGlowScale = 2.f;

	// --- 코스메틱 (비어 있으면 재생 안 함) ---
	UPROPERTY(EditAnywhere, Category = "RewardEntrance|FX")
	TObjectPtr<UNiagaraSystem> HighOpenEffect;

	UPROPERTY(EditAnywhere, Category = "RewardEntrance|FX")
	TObjectPtr<USoundBase> HighOpenSound;

	UPROPERTY(EditAnywhere, Category = "RewardEntrance|FX")
	TObjectPtr<UNiagaraSystem> RiskyOpenEffect;

	UPROPERTY(EditAnywhere, Category = "RewardEntrance|FX")
	TObjectPtr<USoundBase> RiskyOpenSound;

private:	

	UPROPERTY(ReplicatedUsing = OnRep_EntranceState)
	EEntranceState EntranceState = EEntranceState::Sealed;

	UFUNCTION()
	void OnRep_EntranceState();

	// bPlayEffects = false면 상태 반영(글로우/블로커)만 (늦게 접속한 클라의 동기화용).
	void ApplyEntranceState(bool bPlayEffects);

	// 서버 전용. "Reward0", "Reward1"... 이름의 씬 컴포넌트 위치에 Pool에서 뽑은 아이템을 스폰.
	void SpawnRewards(const FRewardTierConfig& Config);

	// GapGlow의 원본 Intensity (첫 ApplyEntranceState에서 캡처). -1 = 아직 캡처 전.
	float BaseGlowIntensity = -1.f;

};
