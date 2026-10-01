#include "UI/Nameplate/PlayerNameplateWidget.h"

#include "Components/TextBlock.h"

bool UPlayerNameplateWidget::SetPlayerName(const FString& PlayerName)
{
	UTextBlock* NameText = Cast<UTextBlock>(GetWidgetFromName(TEXT("NameText")));
	if (!NameText)
	{
		return false;
	}

	NameText->SetText(FText::FromString(PlayerName));
	SetVisibility(PlayerName.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	return true;
}
