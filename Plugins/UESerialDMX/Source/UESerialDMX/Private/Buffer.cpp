#include "Buffer.h"


#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "GameFramework/SpectatorPawn.h"


int32 UDMXSerialDevice::BaudRate(115200);

UDMXSerialDevice::UDMXSerialDevice()
	: Serial(NULL)
{
	Serial = NewObject<USerial>();
	
	State = EDMXState::START;
	Label = 6;
}
UDMXSerialDevice::~UDMXSerialDevice()
{
	Serial->Close();
	Serial = NULL;
}



bool UDMXSerialDevice::Open(int32 nPort, bool AllowListening)
{
	// DMXUSB should receive and transmit data at the highest, most reliable speed possible
	// Recommended Arduino baud rate: 115200
	// Recommended Teensy 3 baud rate: 2000000 (2 Mb/s)
	// DMX baud rate: 250000
	// MIDI baud rate: 31250
	Serial->Open(nPort, BaudRate);
	
	State = EDMXState::START;
	
	if (GetOuter())
	{
		if (auto World = GetOuter()->GetWorld())
		{
			World->GetTimerManager().SetTimer(TimerHandle, this, &UDMXSerialDevice::Tick, 0.0001, false);
		}
	}
	
	return true;
}



void UDMXSerialDevice::Close()
{
	Serial->Close();
}

bool UDMXSerialDevice::IsConnected()
{
	return Serial->IsOpened();
}

UDMXDevice::UDMXDevice()
	: Buffer(NULL)
{
	
}

UDMXBuffer * UDMXDevice::GetBuffer()
{
	return Buffer;
}

void UDMXSerialDevice::Flush()
{
	if(Serial)
		Serial->Flush();
}

UDMXDevice * UDMXSerialDevice::CreateSerialDevice(UDMXBuffer * nBuffer, int32 Port, bool AutoConnect, bool AllowListening)
{
	UDMXSerialDevice* Device = NewObject<UDMXSerialDevice>();
	if(AutoConnect && Device)
		Device->Open(Port, AllowListening);
	return Device;
}

void UDMXSerialDevice::WriteDMXBuffer()
{
	bool Wrote = true;
	int32 CurrentMaxChannel = Buffer->GetMaxChannel();
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
		if (Buffer)
		{
			TArray<uint8> DmxFrame;
			//Wrote = Serial->WriteByte(0);
			//DmxFrame.Add(0);
			for (int i = 0; i <= CurrentMaxChannel; i++)
			{
				DmxFrame.Add(Buffer->Data[i]);
			}
			//UE_LOG(LogOla, Log, TEXT("Serial Write Bytes %d"), Data.Num());
			Wrote = Serial->WriteBytes(DmxFrame);
			if (!Wrote)
			{
				UE_LOG(LogDmx, Log, TEXT("Serial Write FAILED"));
			}
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

void UDMXSerialDevice::TryReadDMXBuffer()
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
				Buffer->Data[i] = ReadNewData[i];
			}

			UE_LOG(LogDmx, Log, TEXT("MessageReceived"));
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

void UDMXDevice::Tick()
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

	
	World->GetTimerManager().SetTimer(TimerHandle, this, &UDMXDevice::Tick, 0.0001, false);
	
}

UDMXBuffer::UDMXBuffer()
{
	Data.SetNum(512);


	
	CurrentMaxChannel = 0;
}

UDMXBuffer::~UDMXBuffer()
{
}

bool UDMXBuffer::Write(int32 Channel, uint8 Value)
{

	Data[Channel] = Value;
	CurrentMaxChannel = FMath::Max(CurrentMaxChannel, Channel);
	
	return true;
}

int32 UDMXBuffer::GetMaxChannel()
{
	return CurrentMaxChannel;
}

