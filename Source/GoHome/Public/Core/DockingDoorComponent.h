

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DockingDoorComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDoorStateChanged, bool, bOpen);

/**
 * 서버 권위 도킹 문 개폐 상태. AI는 이 컴포넌트의 공개 상태만 구독한다 (GameState 전체 참조 금지).
 */
UCLASS(ClassGroup = (Core), meta = (BlueprintSpawnableComponent))
class GOHOME_API UDockingDoorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDockingDoorComponent();

	UFUNCTION(BlueprintPure, Category = "Docking Door")
	bool IsOpen() const;

	UPROPERTY(BlueprintAssignable, Category = "Docking Door")
	FOnDoorStateChanged OnDoorStateChanged;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_bOpen)
	bool bOpen = false;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_bOpen();
};
