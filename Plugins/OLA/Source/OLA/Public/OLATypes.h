#pragma once



#include "CoreMinimal.h"
#include "TimerManager.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"

#include "Serial.h"

#include "OLA.h"
#include "OLATypes.generated.h"

class AOLAServer;

// ENTTEC DMX USB 
//	https://dol2kh495zr52.cloudfront.net/pdf/misc/dmx_usb_pro_api_spec.pdf
// DMX512 Protocol Implementation Using MC9S08GT60 8-Bit MC
//	https://www.nxp.com/docs/en/application-note/AN3315.pdf

// State of receiving DMX Bytes
UENUM()
enum class EDMXState : uint8 {
	START = 0,  // wait for any interrupt BEFORE starting analyzig the DMX protocol.
	LABEL = 1,  // wait for a BREAK condition.
	LEN_LSB = 2,  // BREAK was detected.
	LEN_MSB = 3,  // DMX data.
	DATA = 4,  // All channels received.
	END = 5
};

#define DMX_START 0x7E
#define DMX_STOP 0xE7
#define DMX_BREAK 0x00

#define DMX_MAX_SIZE 512
#define DMX_BAUD_RATE 115200

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

	FOLAMessage(uint8 Channel, uint8 Value, uint8 Function) {
		this->Channel = Channel;
		this->Value = Value;
		this->Function = Function;
	}
};

UCLASS(ClassGroup = Ola, meta = (BlueprintSpawnableComponent), Category = "Open Lighting Architecture")
class UOLAClient : public UActorComponent
{
	GENERATED_BODY()

	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;

	/** Creates a Player Compositing Target which you can modify during gameplay. */
	UFUNCTION(BlueprintCallable, Category = "Open Lighting Architecture")
	void SendDMX(uint8 Channel, uint8 Value, uint8 Function);

	/**
	 * @brief Register our interest in a universe.
	 *
	 * @param universe the id of the universe to register for.
	 */
	UFUNCTION(BlueprintCallable, Category = "Open Lighting Architecture")
	void RegisterUniverse();

	/**
	 * @brief UnRegister our interest in a universe.
	 *
	 * @param universe the id of the universe to unregister for.
	 */
	UFUNCTION(BlueprintCallable, Category = "Open Lighting Architecture")
	void UnRegisterUniverse();

	/**
	* Reports whether this Controller is connected to the OLAServer service and the OLADevice is plugged in.
	*
	* @return True, if connected; false otherwise.
	*/
	UFUNCTION(BlueprintPure, Category = "Open Lighting Architecture")
	bool IsServiceConnected() const;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	uint8 Universe;

private:
	UPROPERTY()
	AOLAServer* UniverseServer;
	//TSharedPtr<AOLAServer> UniverseServer;
};



DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMessageDelegate, FOLAMessage, Output);

UCLASS(ClassGroup = Ola, meta = (BlueprintSpawnableComponent), Category = "Open Lighting Architecture")
class UOLAVirtualDevice : public UActorComponent
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintAssignable)
	FMessageDelegate OnDMXMessage;


	UFUNCTION()
	void MessageReceived(const FOLAMessage& Message);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 Universe;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 Channel;
};


//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDMXListenerDelegate, const TArray<uint8>&, Output);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDMXListenerDelegate);
UCLASS(BlueprintType, Category = "Open Lighting Architecture")
class UOLABuffer : public UObject
{
	GENERATED_BODY()
public:
	UOLABuffer();
	~UOLABuffer();

	/**
	* Open a serial port. Don't forget to close the port before exiting the game.
	* If this Serial instance has already an opened port,
	* return false and doesn't change the opened port number.
	* DMXUSB should receive and transmit data at the highest, most reliable speed possible
	* Recommended Arduino baud rate: 115200
	* Recommended Teensy 3 baud rate: 2000000 (2 Mb/s)
	* DMX baud rate: 250000
	* MIDI baud rate: 31250
	*
	* @param Port The serial port to open.
	* @param BaudRate BaudRate to open the serial port with.
	* @return If the serial port was successfully opened.
	*/
	UFUNCTION(BlueprintCallable, Category = "Open Lighting Architecture")
		bool Open(int32 Port = 2, bool AllowListening = false);

	static int32 BaudRate;
	/**
	* Close and end the communication with the serial port. If not open, do nothing.
	*/
	UFUNCTION(BlueprintCallable, Category = "Open Lighting Architecture")
		void Close();

	UFUNCTION(BlueprintCallable, Category = "Open Lighting Architecture")
		TArray<uint8> ReadBytes(int32 Limit = 256);

	UFUNCTION(BlueprintCallable, Category = "Open Lighting Architecture")
		bool Write(int32 Channel, uint8 Value);

	UFUNCTION(BlueprintPure)
		bool IsConnected();

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Flush Port") , Category = "Open Lighting Architecture")
		void Flush();

	UFUNCTION()
		void WriteDMXBuffer();
	UFUNCTION()
		void TryReadDMXBuffer();
private:
	UPROPERTY()
	TArray<FOLAMessage> MessageBuffer;

	TArray<uint8> Data;
	int CurrentMaxChannel;

	int32 ReadNewMaxChannel;
	TArray<uint8> ReadNewData;

	FTimerHandle TimerHandle;
	int32 LastSendTime;

	bool bAllowListening : 1;

public:
	UPROPERTY(BlueprintReadWrite)
		uint8 Label;

	UPROPERTY(BlueprintReadOnly)
	int32 Universe;

	void OnTime();

	UPROPERTY(BlueprintAssignable)
	FDMXListenerDelegate OnDMXBufferReceived;

protected:
	EDMXState State;
	EDMXState ReadState;

	UPROPERTY()
	USerial* Serial;
};

UCLASS(BlueprintType)
class AOLAServer : public AActor
{
	GENERATED_BODY()
public:

	/*AOLAServer(int UniverseID);*/

	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;

	UFUNCTION(BlueprintCallable, Category = "Open Lighting Architecture")
	bool SendDMX(FOLAMessage Message);

	UPROPERTY()
	int Universe;

public:
	UPROPERTY()
	UOLABuffer* Buffer;
	//TSharedPtr<UOLABuffer> Buffer;
	
};

UCLASS(Blueprintable, BlueprintType)
class AOLAGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	//AOLAGameMode();
	AOLAGameMode(const FObjectInitializer& ObjectInitializer);

	/** Transitions to calls BeginPlay on actors. */
	virtual void BeginPlay() override;


	AOLAServer* RegisterUniverse(int UniverseID);

	UFUNCTION(BlueprintCallable, Category="Open Lighting Architecture")
	AOLAServer* GetUniverse(int UniverseID);
private:


	//UPROPERTY(BlueprintReadOnly)
	TMap<int,AOLAServer*> Servers;
};
