#pragma once

#include "CoreMinimal.h"

namespace PauseUIStyle
{
	// 시안 CSS rgba(r,g,b,a) → UMG 리니어 색. 알파는 그대로 넘기므로 리니어 합성 보정이 필요하면 호출부에서 낮춘다.
	inline FLinearColor SRGBA(uint8 R, uint8 G, uint8 B, float A = 1.f)
	{
		FLinearColor Color = FLinearColor::FromSRGBColor(FColor(R, G, B));
		Color.A = A;
		return Color;
	}
}
