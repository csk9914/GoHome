

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
	
	// 서버 권한 전용. 로비/탐사 어느 쪽 상황에서 불려도 안전하도록 내부에서 HasAuthority 가드.
	UFUNCTION(BlueprintCallable, Category = "Docking Door")
	void SetOpen(bool bNewOpen);

	UFUNCTION(BlueprintPure, Category = "Docking Door")
	bool IsOpen() const {return bOpen;};
	
protected:
	UFUNCTION()
	void OnRep_bOpen();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	UPROPERTY(BlueprintAssignable, Category = "Docking Door")
	FOnDoorStateChanged OnDoorStateChanged;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_bOpen)
	bool bOpen = false;
};
