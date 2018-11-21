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
		Buffer->WriteBytes(Bytes);
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

UOLABuffer::UOLABuffer() 
	: m_hIDComDev(NULL)
	, m_Port(-1)
	, m_Baud(-1)
{
	FMemory::Memset(&m_OverlappedRead, 0, sizeof(OVERLAPPED));
	FMemory::Memset(&m_OverlappedWrite, 0, sizeof(OVERLAPPED));
}
UOLABuffer::~UOLABuffer()
{
	Close();
}

bool UOLABuffer::Open(int32 nPort, int32 nBaud)
{
	if (nPort < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid port number: %d"), nPort);
		return false;
	}
	if (m_hIDComDev)
	{
		UE_LOG(LogTemp, Warning, TEXT("Trying to use opened Serial instance to open a new one. "
			"Current open instance port: %d | Port tried: %d"), m_Port, nPort);
		return false;
	}

	FString szPort;
	if (nPort < 10)
		szPort = FString::Printf(TEXT("COM%d"), nPort);
	else
		szPort = FString::Printf(TEXT("\\\\.\\COM%d"), nPort);
	DCB dcb;

	m_hIDComDev = CreateFile(*szPort, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
	if (m_hIDComDev == NULL)
	{
		unsigned long dwError = GetLastError();
		UE_LOG(LogTemp, Error, TEXT("Failed to open port COM%d (%s). Error: %08X"), nPort, *szPort, dwError);
		return false;
	}

	FMemory::Memset(&m_OverlappedRead, 0, sizeof(OVERLAPPED));
	FMemory::Memset(&m_OverlappedWrite, 0, sizeof(OVERLAPPED));

	COMMTIMEOUTS CommTimeOuts;
	//CommTimeOuts.ReadIntervalTimeout = 10;
	CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
	CommTimeOuts.ReadTotalTimeoutConstant = 0;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
	CommTimeOuts.WriteTotalTimeoutConstant = 10;
	SetCommTimeouts(m_hIDComDev, &CommTimeOuts);

	m_OverlappedRead.hEvent = CreateEvent(NULL, true, false, NULL);
	m_OverlappedWrite.hEvent = CreateEvent(NULL, true, false, NULL);

	dcb.DCBlength = sizeof(DCB);
	GetCommState(m_hIDComDev, &dcb);
	dcb.BaudRate = nBaud;
	dcb.ByteSize = 8;

	if (!SetCommState(m_hIDComDev, &dcb) ||
		!SetupComm(m_hIDComDev, 10000, 10000) ||
		m_OverlappedRead.hEvent == NULL ||
		m_OverlappedWrite.hEvent == NULL)
	{
		unsigned long dwError = GetLastError();
		if (m_OverlappedRead.hEvent != NULL) CloseHandle(m_OverlappedRead.hEvent);
		if (m_OverlappedWrite.hEvent != NULL) CloseHandle(m_OverlappedWrite.hEvent);
		CloseHandle(m_hIDComDev);
		m_hIDComDev = NULL;
		UE_LOG(LogTemp, Error, TEXT("Failed to setup port COM%d. Error: %08X"), nPort, dwError);
		return false;
	}

	//FPlatformProcess::Sleep(0.05f);
	//AddToRoot();
	m_Port = nPort;
	m_Baud = nBaud;
	return true;
}

void UOLABuffer::Close()
{
	if (!m_hIDComDev) return;

	if (m_OverlappedRead.hEvent != NULL) CloseHandle(m_OverlappedRead.hEvent);
	if (m_OverlappedWrite.hEvent != NULL) CloseHandle(m_OverlappedWrite.hEvent);
	CloseHandle(m_hIDComDev);
	m_hIDComDev = NULL;

	//RemoveFromRoot();
}

bool UOLABuffer::IsConnected()
{
	return NULL != m_hIDComDev;
}

bool UOLABuffer::WriteBytes(TArray<uint8> Buffer)
{
	if (!m_hIDComDev) return false;

	bool bWriteStat;
	unsigned long dwBytesWritten;

	bWriteStat = WriteFile(m_hIDComDev, Buffer.GetData(), Buffer.Num(), &dwBytesWritten, &m_OverlappedWrite);
	if (!bWriteStat && (GetLastError() == ERROR_IO_PENDING))
	{
		if (WaitForSingleObject(m_OverlappedWrite.hEvent, 1000))
		{
			dwBytesWritten = 0;
			return false;
		}
		else
		{
			GetOverlappedResult(m_hIDComDev, &m_OverlappedWrite, &dwBytesWritten, false);
			m_OverlappedWrite.Offset += dwBytesWritten;
			return true;
		}
	}

	return true;
}
