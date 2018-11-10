#include "OLABlueprintLibrary.h"

void UOLABlueprintLibrary::SendDMX(uint8 Channel, uint8 Value, uint8 Function)
{

	UE_LOG(LogOla, Error, TEXT("Send DMX [Function:%d to Channel:%d Value:%d]"), Function, Channel, Value);
}