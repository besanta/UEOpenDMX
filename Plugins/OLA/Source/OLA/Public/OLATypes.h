#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"

#include "OLATypes.generated.h"

class AOLAServer;

USTRUCT(BlueprintType)
struct FOLAMessage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 Channel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 Value;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 Function;


	FOLAMessage() 
	{
	}

	FOLAMessage(int8 Channel, int8 Value, int8 Function) {
		this->Channel = Channel;
		this->Value = Value;
		this->Function = Function;
	}
};

UCLASS(ClassGroup = Ola, meta = (BlueprintSpawnableComponent), Category = "Open Lighting Architecture")
class UOLAClient : public UActorComponent
{
	GENERATED_BODY()

	/** Creates a Player Compositing Target which you can modify during gameplay. */
	UFUNCTION(BlueprintCallable, Category = "Open Lighting Architecture")
	void SendDMX(uint8 Channel, uint8 Value, uint8 Function);

	/**
	 * @brief Register our interest in a universe.
	 *
	 * @param universe the id of the universe to register for.
	 */
	UFUNCTION(BlueprintCallable, Category = "Open Lighting Architecture")
	void RegisterUniverse(uint8 Universe);

	/**
	 * @brief UnRegister our interest in a universe.
	 *
	 * @param universe the id of the universe to unregister for.
	 */
	UFUNCTION(BlueprintCallable, Category = "Open Lighting Architecture")
	void UnRegisterUniverse(uint8 Universe);

	/**
	* Reports whether this Controller is connected to the OLAServer service and the OLADevice is plugged in.
	*
	* @return True, if connected; false otherwise.
	*/
	UFUNCTION(BlueprintPure, Category = "Open Lighting Architecture")
	bool IsServiceConnected() const;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 Universe;

private:
	//UPROPERTY()
	TSharedPtr<AOLAServer> UniverseServer;
};



DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMessageDelegate, FOLAMessage, Output);

UCLASS(ClassGroup = Ola, meta = (BlueprintSpawnableComponent), Category = "Open Lighting Architecture")
class UOLAVirtualDevice : public UActorComponent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintAssignable)
	FMessageDelegate OnDMXMessage;

public:
	void OnOLAMessage(FOLAMessage Message);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 Universe;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 Channel;
};

UCLASS()
class UOLABuffer : public UObject
{
	GENERATED_BODY()

	//Private UProperties
	UPROPERTY()
	TArray<FOLAMessage> Buffer;
};

UCLASS(BlueprintType)
class AOLAServer : public AActor
{
	GENERATED_BODY()
public:

	void SendDMX(FOLAMessage Message);

	UPROPERTY()
	int Universe;

private:
	TSharedPtr<UOLABuffer> Buffer;
	
};

UCLASS(Blueprintable, BlueprintType)
class AOLAGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	//AOLAGameMode();
	AOLAGameMode(const FObjectInitializer& ObjectInitializer);

	/** Transitions to calls BeginPlay on actors. */
	//UFUNCTION(BlueprintCallable, Category = Game)
	virtual void BeginPlay() override;

	//virtual void BeginPlay() override;

	//UPROPERTY(BlueprintReadOnly)
	TMap<int,TSharedPtr<AOLAServer>> Servers;
};
