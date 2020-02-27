// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlaybackCtrlInterface.h"
#include "OscDataElemStruct.h"

#include "PlaybackCtrlComponent.generated.h"

// declare the OnOscReceived event type
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FComponentOscReceivedSignature, const FName &, Address, const TArray<FOscDataElemStruct> &, Data, const FString &, SenderIp);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PLAYBACKCTRL_API UPlaybackCtrlComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, Category=PlaybackCtrl)
    FString AddressFilter;

    UPROPERTY(BlueprintAssignable, Category=PlaybackCrtl)
    FComponentOscReceivedSignature OnOscReceived;

public:

    UPlaybackCtrlComponent();

    /// Hot reload constructor
    UPlaybackCtrlComponent(FVTableHelper &helper);

    const FString & GetAddressFilter() const
    {
        return AddressFilter;
    }

    void SendEvent(const FName & Address, const TArray<FOscDataElemStruct> & Data, const FString & SenderIp)
    {
        OnOscReceived.Broadcast(Address, Data, SenderIp);
    }

private:
    void OnRegister() override;

    void OnUnregister() override;

private:
    BasicCueReceiver<UPlaybackCtrlComponent> listener_;
};
