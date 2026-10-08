#include "UI/Locker/SharedLockerRowWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Core/SharedLockerBackend.h"
#include "Engine/Texture2D.h"

void USharedLockerRowWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (WithdrawButton)
	{
		WithdrawButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleWithdrawClicked);
	}
	if (DepositButton)
	{
		DepositButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleDepositClicked);
	}
}

void USharedLockerRowWidget::Setup(const FSharedLockerViewEntry& InEntry, const FText& InDisplayName, UTexture2D* InIcon)
{
	ProductId = InEntry.ProductId;

	if (NameText)
	{
		NameText->SetText(InDisplayName);
	}

	if (CountText)
	{
		// 보관함 안 / 팀 보유 — 차이는 누군가 꺼내 간 수량
		CountText->SetText(FText::Format(
			NSLOCTEXT("SharedLocker", "CountFormat", "보관 {0} / 보유 {1}"),
			FText::AsNumber(InEntry.StoredQuantity),
			FText::AsNumber(InEntry.OwnedQuantity)));
	}

	if (LifetimeText)
	{
		LifetimeText->SetText(InEntry.Lifetime == EItemShopItemLifetime::Permanent
			? NSLOCTEXT("SharedLocker", "Permanent", "장비")
			: NSLOCTEXT("SharedLocker", "Consumable", "소모품"));
	}

	if (IconImage)
	{
		IconImage->SetBrushFromTexture(InIcon);
		IconImage->SetVisibility(InIcon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	// 버튼 활성은 표시 힌트일 뿐 — 실제 판정은 서버가 다시 한다.
	if (WithdrawButton)
	{
		WithdrawButton->SetIsEnabled(InEntry.StoredQuantity > 0);
	}
	if (DepositButton)
	{
		DepositButton->SetIsEnabled(InEntry.OwnedQuantity > InEntry.StoredQuantity);
	}
}

void USharedLockerRowWidget::HandleWithdrawClicked()
{
	if (ISharedLockerBackend* Backend = Cast<ISharedLockerBackend>(GetOwningPlayer()))
	{
		Backend->RequestLockerWithdraw(ProductId);
	}
}

void USharedLockerRowWidget::HandleDepositClicked()
{
	if (ISharedLockerBackend* Backend = Cast<ISharedLockerBackend>(GetOwningPlayer()))
	{
		Backend->RequestLockerDeposit(ProductId);
	}
}
