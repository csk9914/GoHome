
#include "Item/ItemSpawnPoint.h"
#include "Components/BillboardComponent.h"


AItemSpawnPoint::AItemSpawnPoint()
{
 
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

#if WITH_EDITORONLY_DATA
	EditorIcon = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("EditorIcon"));
	if (EditorIcon)
	{
		EditorIcon->SetupAttachment(RootComponent);
	}
#endif
}
