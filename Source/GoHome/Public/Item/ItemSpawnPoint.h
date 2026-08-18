//THE

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Item/ItemSpawnTypes.h"
#include "ItemSpawnPoint.generated.h"

class UBillboardComponent;

UCLASS()
class GOHOME_API AItemSpawnPoint : public AActor
{
	GENERATED_BODY()
	
public:	

	AItemSpawnPoint();

	UPROPERTY(EditAnywhere, Category = "Spawn")
	ESpawnDangerTier DangerTier = ESpawnDangerTier::Near;

protected:

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "Spawn")
	TObjectPtr<UBillboardComponent> EditorIcon;
#endif

};
