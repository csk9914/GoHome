


#include "Item/RadarItemActor.h"
#include "Item/RadarSensorComponent.h"

ARadarItemActor::ARadarItemActor()
{
	RadarSensor = CreateDefaultSubobject<URadarSensorComponent>(TEXT("RadarSensor"));
}