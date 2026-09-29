#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/TitleTypes.h"
#include "TitleSessionRowWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;

/** 세션 목록 한 행. 클릭/Enter = 선택. hover는 키보드 포커스로 합친다(메뉴 항목과 같은 규칙). */
UCLASS(Abstract)
class GOHOME_API UTitleSessionRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnRowClicked, UTitleSessionRowWidget* /*Row*/);
	FOnRowClicked OnRowClicked;

	void SetListing(const FTitleSessionListing& InListing, int32 DisplayNumber);
	void SetSelected(bool bInSelected);
	void SetInteractive(bool bInteractive);

	const FTitleSessionListing& GetListing() const { return Listing; }
	UWidget* GetFocusTarget() const;

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RowButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NumberText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HostNameText;

	// 선택/hover 시 표시되는 좌→우 워시와 오른쪽 화살표
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> SelectedWash;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ArrowText;

private:
	UFUNCTION()
	void HandleClicked();

	UFUNCTION()
	void HandleHovered();

	UFUNCTION()
	void HandleUnhovered();

	void RefreshVisual();

	FTitleSessionListing Listing;
	bool bSelected = false;
	bool bHovered = false;
};
