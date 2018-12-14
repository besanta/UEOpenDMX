#include "OLATypes.h"


#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "GameFramework/SpectatorPawn.h"

void UOLAClient::BeginPlay()
{
	Super::BeginPlay();
	RegisterUniverse();
}
void UOLAClient::BeginDestroy()
{
	Super::BeginDestroy();
	/*if (UniverseServer.IsValid()) {
		UniverseServer.Reset();
	}*/
}

void UOLAClient::SendDMX(uint8 Channel, uint8 Value, uint8 Function)
{
	UE_LOG(LogOla, Log, TEXT("Send DMX [Function:%d to Channel:%d Value:%d] in Universe %d"), Function, Channel, Value, this->Universe);

	if (!IsBeingDestroyed()) {

		if (!IsServiceConnected())
		{
			RegisterUniverse();
		}
		else if (UniverseServer->IsValidLowLevel())
		{
			UniverseServer->SendDMX(FOLAMessage(Channel, Value, Function));
		}
	}
}

void UOLAClient::RegisterUniverse()
{
	//UE_LOG(LogOla, Log, TEXT("Register To Universe %d]"), this->Universe);
	//for (TActorIterator <AOLAServer> ServerItr(GetWorld()); ServerItr; ++ServerItr)
	//{
	//	if (this->Universe == ServerItr->Universe) {
	//		UniverseServer = MakeShareable(*ServerItr);
	//	}
	//}

	//if (!UniverseServer.IsValid()) {

	//}

	if (GetWorld())
	{
		if (AOLAGameMode* GameMode = Cast<AOLAGameMode>(GetWorld()->GetAuthGameMode()))
		{
			UniverseServer = GameMode->RegisterUniverse(this->Universe);
		}
	}
	
}

void UOLAClient::UnRegisterUniverse()
{
	UE_LOG(LogOla, Log, TEXT("UnRegister Universe %d]"), this->Universe);
	//UniverseServer.Reset();
}

bool UOLAClient::IsServiceConnected() const
{
	return UniverseServer != NULL;
}

//
//AOLAServer::AOLAServer(int UniverseID)
//{
//	this->Universe = UniverseID;
//}

void AOLAServer::BeginPlay()
{
	Super::BeginPlay();
	Buffer = NewObject<UOLABuffer>(this);
	if(Buffer)
		Buffer->Open(Universe);
}

void AOLAServer::BeginDestroy()
{
	Super::BeginDestroy();
	if(Buffer->IsValidLowLevel())
		Buffer->Close();
}

bool AOLAServer::SendDMX(FOLAMessage Message)
{
	UE_LOG(LogOla, Log, TEXT("DMX SEND [%d, %d, %d]"), Message.Channel, Message.Function, Message.Value);
	for (TActorIterator <AActor> ActorItr(GetWorld()); ActorItr; ++ActorItr)
	{
		if (ActorItr->IsValidLowLevel()) {
			TInlineComponentArray<UOLAVirtualDevice*> ActorComponents;
			ActorItr->GetComponents(ActorComponents);

			for (UOLAVirtualDevice* Component : ActorComponents)
			{
				if (Component->Channel == Message.Channel
					&& Component->Universe == this->Universe)
				{
					Component->MessageReceived(Message);
					//Component->OnDMXMessage.Broadcast(Message);
				}
			}
		}
	}

	if (Buffer)
	{
		TArray<uint8> Bytes = {Message.Channel, Message.Function, Message.Value};

		//Bytes.Append(reinterpret_cast<const uint8*>(&Message), 3);
		Buffer->Write(Message.Channel, Message.Value);
	}
	return true;
}

/// Virtual DEVICE

void UOLAVirtualDevice::MessageReceived(const FOLAMessage& Message)
{
	UE_LOG(LogOla, Log, TEXT("MessageReceived") );
	OnDMXMessage.Broadcast(Message);
}


///GAME MODE

AOLAGameMode::AOLAGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefaultPawnClass = ASpectatorPawn::StaticClass();
	//UE_LOG(LogOla, Log, TEXT("STart Server") );
}
void AOLAGameMode::BeginPlay()
{
	Super::BeginPlay();
}

AOLAServer* AOLAGameMode::RegisterUniverse(int UniverseID) 
{
	AOLAServer* newUniverse = NULL;
	UE_LOG(LogOla, Log, TEXT("Start OLA Server on Universe %d"), UniverseID);

	if (auto foundUniverse = Servers.Find(UniverseID))
	{
		newUniverse = *foundUniverse;
	}
	else if (auto World = GetWorld())
	{
		FActorSpawnParameters SpawnInfo;
		//newUniverse = World->SpawnActor<AOLAServer>(SpawnInfo);
		//newUniverse->Universe = UniverseID;


		FTransform Transform;
		newUniverse = Cast<AOLAServer>(UGameplayStatics::BeginDeferredActorSpawnFromClass(this, AOLAServer::StaticClass(), Transform));
		if (newUniverse != nullptr)
		{
			//newUniverse->Init(Universe);
			newUniverse->Universe = UniverseID;
			//newUniverse->Buffer = MakeShareable( NewObject<UOLABuffer>());
			UGameplayStatics::FinishSpawningActor(newUniverse, Transform);
		}

		
		Servers.Add(UniverseID, newUniverse);
	}

	return newUniverse;
}

AOLAServer* AOLAGameMode::GetUniverse(int UniverseID)
{
	if (auto foundUniverse = Servers.Find(UniverseID))
	{
		return *foundUniverse;
	}
	return NULL;
}


int32 UOLABuffer::BaudRate(115200);

UOLABuffer::UOLABuffer() 
	: Serial(NULL)
{
	Serial = NewObject<USerial>();
	Data.SetNum(512);
	State = EDMXState::START;
	Label = 6;
}
UOLABuffer::~UOLABuffer()
{
	Serial->Close();
	Serial = NULL;
}

bool UOLABuffer::Open(int32 nPort, bool AllowListening)
{
	// DMXUSB should receive and transmit data at the highest, most reliable speed possible
	// Recommended Arduino baud rate: 115200
	// Recommended Teensy 3 baud rate: 2000000 (2 Mb/s)
	// DMX baud rate: 250000
	// MIDI baud rate: 31250
	Serial->Open(nPort, BaudRate);
	
	State = EDMXState::START;
	bAllowListening = AllowListening;
	CurrentMaxChannel = 0;
	if (GetOuter())
	{
		if (auto World = GetOuter()->GetWorld())
		{
			World->GetTimerManager().SetTimer(TimerHandle, this, &UOLABuffer::OnTime, 0.0001, false);
		}
	}
	
	return true;
}



void UOLABuffer::Close()
{
	Serial->Close();
}

bool UOLABuffer::IsConnected()
{
	return Serial->IsOpened();
}

void UOLABuffer::Flush()
{
	if(Serial)
		Serial->Flush();
}

void UOLABuffer::WriteDMXBuffer()
{
	bool Wrote = true;
	switch (State) {

	case EDMXState::START:
	{
		Wrote = Serial->WriteByte(DMX_START);
		State = EDMXState::LABEL;
		//World->GetTimerManager().SetTimer(TimerHandle, this, &UOLABuffer::OnTime, 0, false);
		break;
	}
	case EDMXState::LABEL:
	{
		//Wrote = Serial->WriteByte(0);
		Wrote = Serial->WriteByte(Label);
		State = EDMXState::LEN_LSB;
		//World->GetTimerManager().SetTimer(TimerHandle, this, &UOLABuffer::OnTime, 0, false);
		break;
	}
	case EDMXState::LEN_LSB:
	{
		uint8 LSB = (CurrentMaxChannel + 1) & 0x00FF;
		Wrote = Serial->WriteByte(LSB);
		State = EDMXState::LEN_MSB;
		//World->GetTimerManager().SetTimer(TimerHandle, this, &UOLABuffer::OnTime, 0, false);
		break;
	}
	case EDMXState::LEN_MSB:
	{
		uint8 MSB = (CurrentMaxChannel + 1) >> CHAR_BIT;
		Wrote = Serial->WriteByte(MSB);
		State = EDMXState::DATA;
		//World->GetTimerManager().SetTimer(TimerHandle, this, &UOLABuffer::OnTime, 0, false);
		break;
	}
	case EDMXState::DATA:
	{
		TArray<uint8> DmxFrame;
		//Wrote = Serial->WriteByte(0);
		//DmxFrame.Add(0);
		for (int i = 0; i <= CurrentMaxChannel; i++)
		{
			DmxFrame.Add(Data[i]);
		}



		//UE_LOG(LogOla, Log, TEXT("Serial Write Bytes %d"), Data.Num());
		Wrote = Serial->WriteBytes(DmxFrame);
		if (!Wrote)
		{
			UE_LOG(LogOla, Log, TEXT("Serial Write FAILED"));
		}
		State = EDMXState::END;
		//World->GetTimerManager().SetTimer(TimerHandle, this, &UOLABuffer::OnTime, 0, false);
		break;
	}
	case EDMXState::END:
	{
		Wrote = Serial->WriteByte(DMX_STOP);
		State = EDMXState::START;
		//World->GetTimerManager().SetTimer(TimerHandle, this, &UOLABuffer::OnTime, 0, false);
		break;
	}
	default:
		State = EDMXState::START;
		//World->GetTimerManager().SetTimer(TimerHandle, this, &UOLABuffer::OnTime, 0, false);
		break;
	}
}

void UOLABuffer::TryReadDMXBuffer()
{
	bool Wrote = true;
	bool Success = true;
	uint8 Read = Serial->ReadByte(Success);

	switch (ReadState) {

	case EDMXState::START:
	{
		if (Read == DMX_START)
		{
			ReadState = EDMXState::LABEL;
		}
		break;
	}
	case EDMXState::LABEL:
	{
		Label = Read;
		ReadState = EDMXState::LEN_LSB;
		break;
	}
	case EDMXState::LEN_LSB:
	{
		ReadNewMaxChannel = Read;
		ReadState = EDMXState::LEN_MSB;
		break;
	}
	case EDMXState::LEN_MSB:
	{
		ReadNewMaxChannel |= Read << CHAR_BIT;
		ReadNewData.Reserve(ReadNewMaxChannel);
		ReadState = EDMXState::DATA;
		break;
	}
	case EDMXState::DATA:
	{
		ReadNewData = Serial->ReadBytes(ReadNewMaxChannel);
		//ReadNewData.Add(Read);
		ReadState = EDMXState::END;
		break;
	}
	case EDMXState::END:
	{
		if (DMX_STOP == Read)
		{
			for (int i = 0; i < ReadNewData.Num(); i++)
			{
				Data[i] = ReadNewData[i];
			}

			UE_LOG(LogOla, Log, TEXT("MessageReceived"));
			OnDMXBufferReceived.Broadcast(/*Data*/);
		}
		ReadState = EDMXState::START;
		break;
	}
	default:
		ReadState = EDMXState::START;
		break;
	}
}

void UOLABuffer::OnTime()
{
	UWorld* World = NULL;
	if (GetOuter())
	{
		World = GetOuter()->GetWorld();
			
	}

	const int32 MaxDelay = 1;

	unsigned long Now = World->GetUnpausedTimeSeconds();
	float DeltaTime = Now - LastSendTime;

	if (IsConnected())
	{
		if (true || DeltaTime >= MaxDelay)
		{
			WriteDMXBuffer();
			LastSendTime = World->GetUnpausedTimeSeconds();

			if (bAllowListening)
			{
				TryReadDMXBuffer();
			}
		}
	}

	//bool Success = true;
	//do
	//{
	//	FString Read = Serial->ReadString(Success);
	//	if (Read.Len() == 0) Success = false;
	//	if (Success)
	//	{
	//		UE_LOG(LogOla, Log, TEXT("Serial Read : %s"), *Read);
	//	}
	//} while (Success);
	//

	
	World->GetTimerManager().SetTimer(TimerHandle, this, &UOLABuffer::OnTime, 0.0001, false);
	
}

bool UOLABuffer::Write(int32 Channel, uint8 Value)
{

	Data[Channel] = Value;
	CurrentMaxChannel = FMath::Max(CurrentMaxChannel, Channel);
	
	return true;
}

TArray<uint8> UOLABuffer::ReadBytes(int32 Limit)
{
	
	return TArray<uint8>();
}
