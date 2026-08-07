

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 상호작용 가능한 오브젝트가 구현한다 (AItemActorBase 등).
 */
class GOHOME_API IInteractable
{
	GENERATED_BODY()

public:
	virtual bool CanInteract(APlayerController* PlayerController) const = 0;
	virtual void OnInteract(APlayerController* PlayerController) = 0;
};
