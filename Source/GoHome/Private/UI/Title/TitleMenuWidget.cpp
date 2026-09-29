#include "UI/Title/TitleMenuWidget.h"

#include "Components/PanelWidget.h"
#include "UI/Title/TitleMenuItemWidget.h"

bool UTitleMenuWidget::HasHighlightedItem() const
{
	return Items.ContainsByPredicate([](const UTitleMenuItemWidget* Item) { return Item && Item->IsHighlighted(); });
}

void UTitleMenuWidget::FocusFirstItem()
{
	if (Items.Num() > 0)
	{
		Items[0]->FocusItem();
	}
}

void UTitleMenuWidget::FocusLastItem()
{
	if (Items.Num() > 0)
	{
		Items.Last()->FocusItem();
	}
}

void UTitleMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	Items.Reset();
	if (!ItemContainer)
	{
		return;
	}

	for (UWidget* Child : ItemContainer->GetAllChildren())
	{
		if (UTitleMenuItemWidget* Item = Cast<UTitleMenuItemWidget>(Child))
		{
			Item->OnActivated.AddUObject(this, &ThisClass::HandleItemActivated);
			Items.Add(Item);
		}
	}
}

void UTitleMenuWidget::NativeDestruct()
{
	for (UTitleMenuItemWidget* Item : Items)
	{
		if (Item)
		{
			Item->OnActivated.RemoveAll(this);
		}
	}
	Items.Reset();

	Super::NativeDestruct();
}

void UTitleMenuWidget::HandleItemActivated(UTitleMenuItemWidget* Item)
{
	OnItemActivated.Broadcast(Item);
}
