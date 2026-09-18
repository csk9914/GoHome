

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SwitchboardScreenWidget.generated.h"

class AElectricSwitchboardActor;

// ElectricSwitchboard 메인화면(게이지 + 성공/실패)용 위젯.
// DangerGauge/SwitchboardState는 이미 BlueprintReadOnly라, 이 참조로 직접 읽어서 쓰면 됨.

UCLASS()
class GOHOME_API USwitchboardScreenWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	UPROPERTY(BlueprintReadOnly, Category = "Switchboard")
	TObjectPtr<AElectricSwitchboardActor> OwningSwitchboard;
};
