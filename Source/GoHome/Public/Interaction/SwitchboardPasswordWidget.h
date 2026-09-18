

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SwitchboardPasswordWidget.generated.h"

// 입력 중인 비밀번호를 보여주는 위젯의 C++ 베이스.
// WBP가 이걸 상속해서 SetEnteredDigits를 구현.

UCLASS()
class GOHOME_API USwitchboardPasswordWidget : public UUserWidget
{
	GENERATED_BODY()
	

public:

	UFUNCTION(BlueprintImplementableEvent, Category = "Switchboard")
	void SetEnteredDigits(const TArray<int32>& Digits);
};
