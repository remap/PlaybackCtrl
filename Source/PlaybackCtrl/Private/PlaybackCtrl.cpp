//
// PlaybackCtrl.cpp
//
//  Generated on February 19 2020
//  Template created by Peter Gusev on 27 January 2020.
//  Copyright 2013-2019 Regents of the University of California
//

#include "PlaybackCtrl.h"
#include "logging.hpp"
#include "git-describe.h"

#define STRINGIZE_VERSION(v) STRINGIZE_TOKEN(v)
#define STRINGIZE_TOKEN(t) #t
#define PLUGIN_VERSION STRINGIZE_VERSION(GIT_DESCRIBE)

#define MODULE_NAME "PlaybackCtrl"
#define LOCTEXT_NAMESPACE "FPlaybackCtrlModule"

#include "OscReceiverInterface.h"
#include "OscDispatcher.h"

using namespace std;
using namespace std::placeholders;

typedef function<void(const FName & Address, const TArray<FOscDataElemStruct> & Data, const FString & SenderIp)> OnOscMessage;

class OscListener {
public:
    OscListener(OnOscMessage onOscMessage) :
        listener_(this),
        onOscMessage_(onOscMessage)
    {
        // register with OSC dispatcher here
        auto oscDispatcher = UOscDispatcher::Get();
        oscDispatcher->RegisterReceiver(&listener_);
    }
    
    const FString & GetAddressFilter() const
    {
        return addressFilter_;
    }

    void SendEvent(const FName & Address, const TArray<FOscDataElemStruct> & Data, const FString & SenderIp)
    {
        onOscMessage_(Address, Data, SenderIp);
    }
    
private:
    FString addressFilter_;
    BasicOscReceiver<OscListener> listener_;
    OnOscMessage onOscMessage_;
};

void FPlaybackCtrlModule::StartupModule()
{
    initModule(MODULE_NAME, PLUGIN_VERSION);

    // To log using ReLog plugin, use these macro definitions:
//     DLOG_PLUGIN_ERROR("Error message");
//     DLOG_PLUGIN_WARN("Warning message");
//     DLOG_PLUGIN_INFO("Info message");
//     DLOG_PLUGIN_DEBUG("Debug message");
//     DLOG_PLUGIN_TRACE("Trace message");

    // making sure OSC module is loaded
    FName oscModuleName(TEXT("OSC"));
    FModuleManager &manager = FModuleManager::Get();
    FModuleStatus oscModuleStatus;
    // force load DDManager module
    IModuleInterface *moduleIface = manager.LoadModule(oscModuleName);

    if (GIsServer)
     {
         DLOG_PLUGIN_DEBUG("Server");
         //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Server"));
     }
     else
     {
         DLOG_PLUGIN_DEBUG("Not server");
         //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Not server!"));
     }
    if (manager.QueryModule(oscModuleName, oscModuleStatus))
    {
        bool isLoaded = oscModuleStatus.bIsLoaded;
                        
        if (!isLoaded)
            isLoaded = (nullptr != manager.LoadModule(oscModuleName));
            
        if (isLoaded)
        {
            if (UOscDispatcher::Get())
                listener_ = new OscListener(bind(&FPlaybackCtrlModule::onOscReceived, this, _1, _2, _3));
//                listener_ = new OscListener([](){
//                    DLOG_DEBUG("hey! callback in lambda");
//                });
            else
                DLOG_PLUGIN_ERROR("OSC dispatcher is NULL");
        }
        else
            DLOG_PLUGIN_ERROR("OSC Module could not be loaded.");
    }
    else
        DLOG_PLUGIN_ERROR("OSC Module could not be found.");
    

}

void FPlaybackCtrlModule::ShutdownModule()
{
    if (listener_)
        delete listener_;
}

void FPlaybackCtrlModule::onOscReceived(const FName & Address, const TArray<FOscDataElemStruct> & Data, const FString & SenderIp)
{
    DLOG_PLUGIN_DEBUG("OSC RECEIVED IN PLAYBACK CTRL MODULE");
    
    // Parse OSC message
    // Current naming: /<project>/<build>/<dept>/<cue name>/<action>
    FString oscAddress = Address.ToString();
    TArray<FString> addressParts;
    
    oscAddress.ParseIntoArray(addressParts, TEXT("/"), true);
    if (addressParts.IsValidIndex(0))
    {
        if (addressParts[0] != TEXT("HighCastle") || addressParts.Num() < 5)
        {
            DLOG_PLUGIN_DEBUG("Message doesn't meet address naming requirements.");
        }
    }
    else
        DLOG_PLUGIN_DEBUG("Message address is incorrect.");
    
    for (auto& Str : addressParts)
    {
        DLOG_PLUGIN_DEBUG(TCHAR_TO_UTF8(*Str));
    }
    

    
    
    
    
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPlaybackCtrlModule, PlaybackCtrl)
