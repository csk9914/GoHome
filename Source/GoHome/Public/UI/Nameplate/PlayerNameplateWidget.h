#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerNameplateWidget.generated.h"

class UTextBlock;

UCLASS()
class GOHOME_API UPlayerNameplateWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Nameplate")
	bool SetPlayerName(const FString& PlayerName);

protected:
};
