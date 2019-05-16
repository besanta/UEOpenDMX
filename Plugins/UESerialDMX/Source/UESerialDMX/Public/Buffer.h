#pragma once



#include "CoreMinimal.h"
#include "TimerManager.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

#include "UESerialDMX.h"

#include "Serial.h"
#include "Buffer.generated.h"


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

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDMXListenerDelegate);

UCLASS(BlueprintType, Category = "Open Lighting Architecture")
class UDMXBuffer : public UObject
{
	GENERATED_BODY()
public:
	UDMXBuffer();
	~UDMXBuffer();

	UFUNCTION(BlueprintCallable, Category = "Open Lighting Architecture")
	bool Write(int32 Channel, uint8 Value);

	UFUNCTION()
	int32 GetMaxChannel();

private:
	int CurrentMaxChannel;

public:
	TArray<uint8> Data;
};

UCLASS(BlueprintType, Abstract)
class UDMXDevice : public UObject
{
	GENERATED_BODY()
public:

	UObject* contextObject;

	class UWorld* GetWorld() const override
	{
		return GEngine->GetWorldFromContextObject(contextObject, EGetWorldErrorMode::LogAndReturnNull);
	}

	UDMXDevice();

	void Tick();

	UPROPERTY(BlueprintReadWrite)
	uint8 Label;

	UPROPERTY(BlueprintReadOnly)
	int32 Universe;
	
	UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
	UDMXBuffer* Buffer;

	UFUNCTION(BlueprintPure)
	virtual bool IsConnected() { check(0 && "You must override this"); return false; }

	UPROPERTY(BlueprintAssignable)
	FDMXListenerDelegate OnDMXBufferReceived;

	UFUNCTION()
	virtual void WriteDMXBuffer() { check(0 && "You must override this"); }
	UFUNCTION()
	virtual void TryReadDMXBuffer() { check(0 && "You must override this"); }

	UFUNCTION(BlueprintCallable)
	virtual void Close() {check(0 && "You must override this");}

	UFUNCTION(BlueprintPure)
	UDMXBuffer* GetBuffer();
protected:
	FTimerHandle TimerHandle;
	int32 LastSendTime;

	bool bAllowListening : 1;
};

UCLASS(BlueprintType)
class UDMXSerialDevice : public UDMXDevice
{
	GENERATED_BODY()

public :
	UDMXSerialDevice();
	~UDMXSerialDevice();

	UFUNCTION(BlueprintCallable, Category = "Open Lighting Architecture")
	bool Open(int32 Port = 2, bool AllowListening = false);
	/**
	* Close and end the communication with the serial port. If not open, do nothing.
	*/
	void Close() override;
	bool IsConnected() override;

	void WriteDMXBuffer() override;
	void TryReadDMXBuffer() override;

	UFUNCTION()
	void Flush();

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Create DMX Serial Device", HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"), Category = "Open Lighting Architecture")
		static UDMXDevice* CreateSerialDevice(UDMXBuffer* nBuffer, int32 Port = 2, bool AutoConnect = true, bool AllowListening = false);

private:
	static int32 BaudRate;

	int32 ReadNewMaxChannel;
	TArray<uint8> ReadNewData;
protected:
	EDMXState State;
	EDMXState ReadState;

	UPROPERTY()
	USerial* Serial;
};