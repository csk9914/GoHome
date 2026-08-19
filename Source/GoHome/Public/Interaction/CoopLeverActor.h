

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "CoopLeverActor.generated.h"

class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLeverActiveChanged, bool, bNewActive);

// 2인 협동 게이트용 레버 하나.
// 상호작용하면 일정 시간 Active 상태를 유지하다 자동으로 꺼짐.
// 두 레버가 동시에 Active여야 하는 판정은 UMultiActorGateComponent가 담당(이 액터는 자기 상태만 안다).

UCLASS()
class GOHOME_API ACoopLeverActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:

	ACoopLeverActor();

	virtual bool CanInteract(APawn* InstigatorPawn) const override;

	virtual void OnInteract(APawn* InstigatorPawn) override;

	virtual FText GetInteractionPromptText_Implementation() const override;


	UFUNCTION(BlueprintPure, Category = "Lever")
	bool IsActive() const { return bIsActive; }

	UPROPERTY(BlueprintAssignable, Category = "Lever")
	FOnLeverActiveChanged OnLeverActiveChanged;


protected:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, Category = "Lever")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	// 활성화 후 재상호작용 없이 버티는 시간(초).
	// 이 시간이 지나면 자동으로 꺼짐.
	// 두 레버를 번갈아 계속 눌러줘야 유지됨.
	UPROPERTY(EditAnywhere, Category = "Lever", meta = (ClampMin = "0.1"))
	float ActiveDuration = 5.f;

	UPROPERTY(ReplicatedUsing = OnRep_IsActive)
	bool bIsActive = false;

	UFUNCTION()
	void OnRep_IsActive();

private:

	void Deactivate();

	FTimerHandle DeactivateTimerHandle;


};
