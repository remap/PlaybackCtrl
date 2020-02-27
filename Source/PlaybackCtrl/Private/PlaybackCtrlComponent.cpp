// Fill out your copyright notice in the Description page of Project Settings.
#include "PlaybackCtrlComponent.h"
#include "PlaybackCtrl.h"

UPlaybackCtrlComponent::UPlaybackCtrlComponent()
    : listener_(this)
{
    DLOG_TRACE("Start PlaybackCtrlComponent");
}

UPlaybackCtrlComponent::UPlaybackCtrlComponent(FVTableHelper & helper)
    : listener_(this)
{
    // Does not need to be a valid object.
    DLOG_TRACE("Start PlaybackCtrlComponent helper");

}


void UPlaybackCtrlComponent::OnRegister()
{
    DLOG_TRACE("Start OnRegister");
    Super::OnRegister();
//    FPlaybackCtrlModule* mod = FPlaybackCtrlModule::GetSharedInstance();
//    if (mod)
//        mod->RegisterReceiver(&_listener);
//    else
//        DLOG_TRACE("no module");
    
}

void UPlaybackCtrlComponent::OnUnregister()
{
    Super::OnUnregister();
}



