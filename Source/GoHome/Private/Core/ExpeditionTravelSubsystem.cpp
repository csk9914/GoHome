// 


#include "Core/ExpeditionTravelSubsystem.h"

void UExpeditionTravelSubsystem::SetActiveZone(const UExpeditionZoneDataAsset* ZoneDataAsset)
{
	ActiveZone = const_cast<UExpeditionZoneDataAsset*>(ZoneDataAsset);
}
