#pragma once


#define FC_DTRDSR       0x01
#define FC_RTSCTS       0x02
#define FC_XONXOFF      0x04
#define ASCII_BEL       0x07
#define ASCII_BS        0x08
#define ASCII_LF        0x0A
#define ASCII_CR        0x0D
#define ASCII_XON       0x11
#define ASCII_XOFF      0x13

#include "Windows/AllowWindowsPlatformTypes.h"
#include "windows.h"
#include "Windows/HideWindowsPlatformTypes.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"

//#include "Serial.h"

#include "OLA.h"
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

UCLASS(BlueprintType, Category = "Open Lighting Architecture")
class UOLABuffer : public UObject
{
	GENERATED_BODY()
public:
	UOLABuffer();
	~UOLABuffer();

	//Private UProperties
	

	/**
	* Open a serial port. Don't forget to close the port before exiting the game.
	* If this Serial instance has already an opened port,
	* return false and doesn't change the opened port number.
	*
	* @param Port The serial port to open.
	* @param BaudRate BaudRate to open the serial port with.
	* @return If the serial port was successfully opened.
	*/
	UFUNCTION(BlueprintCallable, Category = "Open Lighting Architecture")
		bool Open(int32 Port = 2, int32 BaudRate = 9600);
	/**
	* Close and end the communication with the serial port. If not open, do nothing.
	*/
	UFUNCTION(BlueprintCallable, Category = "Open Lighting Architecture")
		void Close();

	UFUNCTION(BlueprintCallable, Category = "Open Lighting Architecture")
		bool WriteBytes(TArray<uint8> Buffer);

	UFUNCTION(BlueprintPure)
		bool IsConnected();
private:
	UPROPERTY()
		TArray<FOLAMessage> MessageBuffer;

protected:
	void* m_hIDComDev;

	OVERLAPPED m_OverlappedRead, m_OverlappedWrite;

	int32 m_Port;
	int32 m_Baud;
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
