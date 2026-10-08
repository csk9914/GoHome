#include "UI/Locker/SharedLockerWidget.h"

#include "UI/Locker/SharedLockerRowWidget.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Core/GoHomeGameState.h"
#include "Core/SharedLockerBackend.h"
#include "Engine/World.h"
#include "Item/ItemDataAsset.h"
#include "Shop/ItemShopCatalogDataAsset.h"
#include "Shop/ItemShopSubsystem.h"

void USharedLockerWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// UIOnly 입력 모드에서 Escape를 받으려면 포커스를 가질 수 있어야 한다.
	SetIsFocusable(true);

	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleCloseClicked);
	}
}

void USharedLockerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (AGoHomeGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AGoHomeGameState>() : nullptr)
	{
		BoundGameState = GameState;
		GameState->OnSharedLockerChanged.AddUniqueDynamic(this, &ThisClass::HandleLockerChanged);
	}

	if (ISharedLockerBackend* Backend = Cast<ISharedLockerBackend>(GetOwningPlayer()))
	{
		ResultHandle = Backend->OnSharedLockerResult().AddUObject(this, &ThisClass::HandleLockerResult);
	}

	if (ResultText)
	{
		ResultText->SetText(FText::GetEmpty());
	}

	// 바인딩 직후 한 번 당겨 온다.
	HandleLockerChanged();
}

void USharedLockerWidget::NativeDestruct()
{
	if (AGoHomeGameState* GameState = BoundGameState.Get())
	{
		GameState->OnSharedLockerChanged.RemoveAll(this);
	}
	BoundGameState.Reset();

	if (ISharedLockerBackend* Backend = Cast<ISharedLockerBackend>(GetOwningPlayer()))
	{
		Backend->OnSharedLockerResult().Remove(ResultHandle);
	}
	ResultHandle.Reset();

	Super::NativeDestruct();
}

FReply USharedLockerWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::E)
	{
		RequestClose();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void USharedLockerWidget::HandleLockerChanged()
{
	if (!EntryList)
	{
		return;
	}

	EntryList->ClearChildren();

	const AGoHomeGameState* GameState = BoundGameState.Get();
	const UItemShopSubsystem* ShopSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UItemShopSubsystem>() : nullptr;

	int32 RowCount = 0;
	if (GameState && RowClass)
	{
		for (const FSharedLockerViewEntry& Entry : GameState->GetSharedLockerItems())
		{
			if (Entry.OwnedQuantity <= 0)
			{
				continue;
			}

			// 표시 이름·아이콘은 클라에도 로드돼 있는 카탈로그에서 찾는다(없으면 ProductId).
			const FItemShopProduct* Product = ShopSubsystem ? ShopSubsystem->FindProduct(Entry.ProductId) : nullptr;
			const UItemDataAsset* ItemData = Product ? Product->ItemData.Get() : nullptr;
			const FText DisplayName = ItemData && !ItemData->DisplayName.IsEmpty()
				? ItemData->DisplayName
				: FText::FromName(Entry.ProductId);

			USharedLockerRowWidget* Row = CreateWidget<USharedLockerRowWidget>(this, RowClass);
			if (!Row)
			{
				continue;
			}

			Row->Setup(Entry, DisplayName, ItemData ? ItemData->Icon.Get() : nullptr);
			EntryList->AddChild(Row);
			++RowCount;
		}
	}

	if (EmptyText)
	{
		EmptyText->SetVisibility(RowCount == 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void USharedLockerWidget::HandleLockerResult(const FSharedLockerResult& Result)
{
	if (!ResultText)
	{
		return;
	}

	if (!Result.Message.IsEmpty())
	{
		ResultText->SetText(Result.Message);
		return;
	}

	if (Result.Result == EItemShopResult::Success)
	{
		ResultText->SetText(Result.Action == ESharedLockerAction::Withdraw
			? NSLOCTEXT("SharedLocker", "Withdrawn", "꺼냈습니다.")
			: NSLOCTEXT("SharedLocker", "Deposited", "넣었습니다."));
		return;
	}

	ResultText->SetText(NSLOCTEXT("SharedLocker", "Failed", "처리하지 못했습니다."));
}

void USharedLockerWidget::HandleCloseClicked()
{
	RequestClose();
}

void USharedLockerWidget::RequestClose()
{
	if (ISharedLockerBackend* Backend = Cast<ISharedLockerBackend>(GetOwningPlayer()))
	{
		Backend->RequestCloseSharedLocker();
	}
}
