#include "OLATypes.h"

#include "EngineUtils.h"
#include "GameFramework/SpectatorPawn.h"

void UOLAClient::SendDMX(uint8 Channel, uint8 Value, uint8 Function)
{
	UE_LOG(LogOla, Log, TEXT("Send DMX [Function:%d to Channel:%d Value:%d]"), Function, Channel, Value);

	for (TActorIterator <AOLAServer> ActorItr(GetWorld()); ActorItr; ++ActorItr)
	{
		ActorItr->SendDMX(FOLAMessage(Channel, Value, Function));
	}
}

void UOLAClient::RegisterUniverse(uint8 Universe)
{
	UE_LOG(LogOla, Log, TEXT("Register Universe %d]"), Universe);
}

void UOLAClient::UnRegisterUniverse(uint8 Universe)
{
	UE_LOG(LogOla, Log, TEXT("UnsRegister Universe %d]"), Universe);
}

bool UOLAClient::IsServiceConnected() const
{
	return false;
}

AOLAGameMode::AOLAGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefaultPawnClass = ASpectatorPawn::StaticClass();
	//UE_LOG(LogOla, Log, TEXT("STart Server") );
	
}
void AOLAGameMode::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogOla, Log, TEXT("Start OLA Server on Universe %d"), 1);

	if (auto World = GetWorld())
	{
		FActorSpawnParameters SpawnInfo;
		World->SpawnActor<AOLAServer>(SpawnInfo);
	}
}

void AOLAServer::SendDMX(FOLAMessage Message)
{
	for (TActorIterator <AActor> ActorItr(GetWorld()); ActorItr; ++ActorItr)
	{
		TInlineComponentArray<UOLAVirtualDevice*> ActorComponents;
		ActorItr->GetComponents(ActorComponents);

		for (UOLAVirtualDevice* Component : ActorComponents)
		{
			if (Component->Channel == Message.Channel
				&& Component->Universe == this->Universe)
			{
				Component->OnOLAMessage(Message);
			}
		}
	}
}

void UOLAVirtualDevice::OnOLAMessage(FOLAMessage Message)
{
	OnDMXMessage.Broadcast(Message);
}
