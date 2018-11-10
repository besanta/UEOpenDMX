#pragma once
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "OLABlueprintLibrary.generated.h"

UCLASS()
class UOLABlueprintLibrary : public UBlueprintFunctionLibrary 
{
	GENERATED_BODY()

public:
	/** Creates a Player Compositing Target which you can modify during gameplay. */
	UFUNCTION(BlueprintCallable, Category = "Open Lighting Architecture", meta = (WorldContext = "WorldContextObject"))
	static void SendDMX(uint8 Channel, uint8 Value, uint8 Function);

};