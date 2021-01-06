// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlaybackCtrlInterface.h"
#include "OscDataElemStruct.h"

#include "PlaybackCtrlComponent.generated.h"

// declare the OnOscReceived event type

//DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FComponentOscReceivedSignature, const TMap<FString, FString> &, AddressDict, const TMap<FString, FString> &, DataDict, const FString &, SenderIp);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FComponentCueReceivedSignature, const FName &, Address, const TArray<FOscDataElemStruct> &, Data, const FString &, SenderIp);

UCLASS( ClassGroup=PlaybackCtrl, meta=(BlueprintSpawnableComponent) )
class PLAYBACKCTRL_API UPlaybackCtrlComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, Category=PlaybackCtrl)
    FString DepartmentFilter;
    
    UPROPERTY(EditAnywhere, Category=PlaybackCtrl)
    FString BuildFilter;

    UPROPERTY(BlueprintAssignable, Category=PlaybackCrtl)
    FComponentCueReceivedSignature OnCueReceived;
    
    
    UPlaybackCtrlComponent();
    

    /// Hot reload constructor
    UPlaybackCtrlComponent(FVTableHelper &helper);
    
    const FString & GetDepartmentFilter() const
    {
        return DepartmentFilter;
    }

    const FString & GetBuildFilter() const
       {
           return BuildFilter;
       }
    
    void SendEvent(const FName & Address, const TArray<FOscDataElemStruct> & Data, const FString & SenderIp)
    {
        InvokeOnCueRxReplicated(Address, Data, SenderIp);
    }
    
    UFUNCTION( NetMulticast, Reliable )
    void InvokeOnCueRxReplicated(const FName & Address, const TArray<FOscDataElemStruct> & Data, const FString & SenderIp);
    
    FString GetListenerName() const;

private:
    void OnRegister() override;

    void OnUnregister() override;
   
    BasicCueReceiver<UPlaybackCtrlComponent> listener_;
};
