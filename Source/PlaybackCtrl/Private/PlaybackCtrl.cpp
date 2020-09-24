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
#include <mutex>

#define STRINGIZE_VERSION(v) STRINGIZE_TOKEN(v)
#define STRINGIZE_TOKEN(t) #t
#define PLUGIN_VERSION STRINGIZE_VERSION(GIT_DESCRIBE)

#define MODULE_NAME "PlaybackCtrl"
#define LOCTEXT_NAMESPACE "FPlaybackCtrlModule"

#include "OscReceiverInterface.h"
#include "PlaybackCtrlInterface.h"
#include "OscDispatcher.h"

#include "Engine/ObjectLibrary.h"
#include <AssetRegistryModule.h>
#include <ARFilter.h>
#include <UObject/Class.h>
#include <UObject/UObjectIterator.h>

using namespace std;
using namespace std::placeholders;

typedef function<void(const FName & Address, const TArray<FOscDataElemStruct> & Data, const FString & SenderIp)> OnOscMessage;
static FPlaybackCtrlModule* SharedInstance;

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
    SharedInstance = this;
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
        if ((addressParts[0] != TEXT("HighCastle") && addressParts[0] != TEXT("Invencion")) || addressParts.Num() < 5)
        {
            DLOG_PLUGIN_DEBUG("Message doesn't meet address naming requirements.");
        }
        else
        {
            for(auto receiver : _receivers)
            {
                receiver->SendEvent(Address, Data, SenderIp);
            }
        }
    }
    else
        DLOG_PLUGIN_DEBUG("Message address is incorrect.");

}

void FPlaybackCtrlModule::onPostWorldInitialization (UWorld *world)
{
    static once_flag flag;
    call_once(flag, [&](){
        oscDispatcherRegister(world);
        SpawnCues(world);
    });
}


// added for components to register
void FPlaybackCtrlModule::RegisterReceiver(IPlaybackCtrlInterface * receiver)
{
    FScopeLock ScopeLock(&_receiversMutex);
    _receivers.AddUnique(receiver);
    UE_LOG(LogTemp, Log, TEXT("added receiver in OnRegister"));

}

void FPlaybackCtrlModule::UnregisterReceiver(IPlaybackCtrlInterface * receiver)
{
    FScopeLock ScopeLock(&_receiversMutex);
    _receivers.Remove(receiver);
}

FPlaybackCtrlModule* FPlaybackCtrlModule::GetSharedInstance()
{
    return SharedInstance;
}

void FPlaybackCtrlModule::SpawnCues(UWorld *world)
{
    // The cues will be spawned in every loaded level
    // TODO: possibly implement any logic that excludes temp levels
    TArray<TAssetSubclassOf<ACueActor>> ToSpawn;
    FString CueClassName = "Class'/Script/PlaybackCtrl.CueActor'";
    GetAllBlueprintSubclasses(ToSpawn, FName("ACueActor"), false, TEXT("/Game"), CueClassName);
    //DLOG_TRACE("I got {} CueActor results to spawn in the Cue Manager", ToSpawn.Num());
    for (auto& Cue : ToSpawn)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
//        FString namestring = "Hope" + FString::FromInt(count);
        SpawnParams.Name = FName(*Cue.GetAssetName());
        //DLOG_TRACE("my name is {}", TCHAR_TO_ANSI(*Cue.GetAssetName()));
        ACueActor* ActorRef = world->SpawnActor<ACueActor>(Cue.Get(), FVector(0,0,0), FRotator(0,0,0), SpawnParams);
    }
    
//    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
//        TArray<FAssetData> AssetData;
//    //    const UClass* Class = AActor::StaticClass();
//        AssetRegistryModule.Get().GetAllAssets(AssetData);
//
//        for (auto& G : AssetData)
//        {
//            DLOG_INFO("MY FULL NAME IS {}", TCHAR_TO_ANSI(*G.GetFullName()));
//            UE_LOG(LogTemp, Log, TEXT("MY FULL NAME IS %s"), *G.GetFullName());
//        }
}

//void FPlaybackCtrlModule::SpawnCues(UWorld *world)
//{
//    // The cues will be spawned in every loaded level
//    // TODO: possibly implement any logic that excludes temp levels
//    TArray<TAssetSubclassOf<ACueActor>> ToSpawn;
//    FString CueClassName = "Class'/Script/PlaybackCtrl.CueActor'";
//    GetAllBlueprintSubclasses(ToSpawn, FName("UBlueprint"), false, TEXT("/Game"), CueClassName);
//    DLOG_INFO("I got {} CueActor results to spawn", ToSpawn.Num());
//    for (auto& Cue : ToSpawn)
//    {
//        FActorSpawnParameters SpawnParams;
//        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
////        FString namestring = "Hope" + FString::FromInt(count);
//        SpawnParams.Name = FName(*Cue.GetAssetName());
//        DLOG_INFO("my name is {}", TCHAR_TO_ANSI(*Cue.GetAssetName()));
//        ACueActor* ActorRef = world->SpawnActor<ACueActor>(Cue.Get(), FVector(0,0,0), FRotator(0,0,0), SpawnParams);
//    }
//}

void FPlaybackCtrlModule::oscDispatcherRegister(UWorld* world)
{
    DLOG_PLUGIN_DEBUG("world init plugin debug");
    
    FName oscModuleName(TEXT("OSC"));
    FModuleManager &manager = FModuleManager::Get();
    FModuleStatus oscModuleStatus;
    // force load DDManager module
    IModuleInterface *moduleIface = manager.LoadModule(oscModuleName);
    ENetMode netMode = world->GetNetMode();
    DLOG_PLUGIN_DEBUG(netMode);
    if (netMode == NM_ListenServer || netMode == NM_DedicatedServer || netMode==NM_Standalone)
    {
        if (manager.QueryModule(oscModuleName, oscModuleStatus))
        {
            bool isLoaded = oscModuleStatus.bIsLoaded;

            if (!isLoaded)
                isLoaded = (nullptr != manager.LoadModule(oscModuleName));
            if (isLoaded)
            {
                if (UOscDispatcher::Get())
                {
                    listener_ = new OscListener(bind(&FPlaybackCtrlModule::onOscReceived, this, _1, _2, _3));
                }

                else
                    DLOG_PLUGIN_ERROR("OSC dispatcher is NULL");
                }
            else
                DLOG_PLUGIN_ERROR("OSC Module could not be loaded.");
            }
        else
            DLOG_PLUGIN_ERROR("OSC Module could not be found.");
    }
    else
        DLOG_PLUGIN_ERROR("THIS IS NOT A LISTEN OR DEDICATED SERVER");
    
    if (netMode == NM_Client)
    {
        if (listener_)
            delete listener_;
    }
}

void FPlaybackCtrlModule::GetAllBlueprintSubclasses(TArray< TAssetSubclassOf< ACueActor > >& Subclasses,
                    FName BaseClassName /*TSubclassOf< UObject > Base*/,
                    bool bAllowAbstract, FString const& Path, FString ClassName)
{
/*
    For blueprint classes, things are complicated by the fact that the UClass may not have been loaded into memory yet.
    The approach taken here is a bit more complicated than it has to be, but allows us to gather the list of subclasses
    without force loading anything.
    */

    static const FName GeneratedClassTag = TEXT("GeneratedClass");
    static const FName ClassFlagsTag = TEXT("ClassFlags");

//            check(Base);

    // Load the asset registry module
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked< FAssetRegistryModule >(FName("AssetRegistry"));
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
    // The asset registry is populated asynchronously at startup, so there's no guarantee it has finished.
    // This simple approach just runs a synchronous scan on the entire content directory.
    // Better solutions would be to specify only the path to where the relevant blueprints are,
    // or to register a callback with the asset registry to be notified of when it's finished populating.
    
    TArray< FString > ContentPaths;
    ContentPaths.Add(TEXT("/Game"));
    if (!Path.IsEmpty())
    {
        //DLOG_TRACE("Add search in {}", TCHAR_TO_ANSI(*Path));
        ContentPaths.Add(Path);
    }

    AssetRegistry.ScanPathsSynchronous(ContentPaths);
    //DLOG_TRACE("scan synchronous");

    //FName BaseClassName = Base->GetFName();

    // Use the asset registry to get the set of all class names deriving from Base
    TSet< FName > DerivedNames;
    {
        TArray< FName > BaseNames;
        BaseNames.Add(BaseClassName);
//        DLOG_TRACE("added base class name");
//        DLOG_TRACE("searching for classes derived from {}",
//                  TCHAR_TO_ANSI(*BaseClassName.ToString()));

        TSet< FName > Excluded;
        AssetRegistry.GetDerivedClassNames(BaseNames, Excluded, DerivedNames);
    }

//    DLOG_TRACE("derived class names num {}", DerivedNames.Num());
//    for (auto n : DerivedNames)
//        DLOG_TRACE("DERIVED NAME {}", TCHAR_TO_ANSI(*n.ToString()));

    // Set up a filter and then pull asset data for all blueprints in the specified path from the asset registry.
    // Note that this works in packaged builds too. Even though the blueprint itself cannot be loaded, its asset data
    // still exists and is tied to the UBlueprint type.

    FARFilter Filter;
    Filter.ClassNames.Add(FName("Blueprint"));
//            Filter.ClassNames.Add(BaseClassName);

    //for (auto cn : Filter.ClassNames)
        //DLOG_TRACE(" -- filter ClassName {}", TCHAR_TO_ANSI(*cn.ToString()));

//    UBlueprint::StaticClass()->GetFName());
    Filter.bRecursiveClasses = true;
    if(!Path.IsEmpty())
    {
        Filter.PackagePaths.Add(*Path);
    }
    Filter.bRecursivePaths = true;

    TArray< FAssetData > AssetList;
    AssetRegistry.GetAssets(Filter, AssetList);

    //DLOG_TRACE("loaded AssetList, n items {}", AssetList.Num());

    // Iterate over retrieved blueprint assets
    for(auto const& Asset : AssetList)
    {
        //DLOG_TRACE("item {}",
                  //TCHAR_TO_ANSI(*Asset.AssetName.ToString()));

        TArray<FName> tagKeys;
//                TArray<FName> tagValues;
        Asset.TagsAndValues.GenerateKeyArray(tagKeys);
//                Asset.TagsAndValues.GenerateValueArray(tagValues);

        //DLOG_TRACE("item tag keys");
//        for (auto tagKey : tagKeys)
//            DLOG_TRACE("{}", TCHAR_TO_ANSI(*tagKey.ToString()));
//                DLOG_TRACE("item tag values");
//                for (auto tagValue : tagValues)
//                    DLOG_TRACE("{}", TCHAR_TO_ANSI(*tagValue.ToString()));

        // Get the the class this blueprint generates (this is stored as a full path)
        auto findResult = Asset.TagsAndValues.FindTag(GeneratedClassTag);
        FString findResultValue = Asset.TagsAndValues.FindTag(TEXT("ParentClass")).GetValue();
//                auto findResult = Asset.TagsAndValues.FindTag(TEXT("ParentClass"));

//        DLOG_TRACE("find result val {}", TCHAR_TO_ANSI(*findResultValue));

        if(findResult.IsSet())
        {
            auto GeneratedClassPathPtr = findResult.GetValue();
            // Optionally ignore abstract classes
            // As of 4.12 I do not believe blueprints can be marked as abstract, but this may change so included for completeness.
//                    if(!bAllowAbstract)
//                    {
//                        auto findResult = Asset.TagsAndValues.FindTag(ClassFlagsTag);
//                        if(findResult.IsSet())
//                        {
//                            auto ClassFlagsPtr = findResult.GetValue();
//                            auto ClassFlags = FCString::Atoi(**ClassFlagsPtr);
//                            if((ClassFlags & CLASS_Abstract) != 0)
//                            {
//                                continue;
//                            }
//                        }
//                    }

            // Convert path to just the name part
            const FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(*GeneratedClassPathPtr);
            const FString ClassName = FPackageName::ObjectPathToObjectName(ClassObjectPath);

//            DLOG_TRACE("OBJECT PATH {} CLASS NAME {}",
//                      TCHAR_TO_ANSI(*ClassObjectPath),
//                      TCHAR_TO_ANSI(*ClassName));

//                     Check if this class is in the derived set
            FString CueClassName = "Class'/Script/PlaybackCtrl.CueActor'";
            if (findResultValue != CueClassName)
            {
                continue;
            }
//                    if(!DerivedNames.Contains(*CueClassName))
//                    {
//                        continue;
//                    }

            // Store using the path to the generated class
            Subclasses.Add(TAssetSubclassOf< ACueActor >(FStringAssetReference(ClassObjectPath)));
        }
    }
}

//void FPlaybackCtrlModule::GetAllBlueprintSubclasses(TArray<TAssetSubclassOf<ACueActor>>& Subclasses,
//                    FName BaseClassName /*TSubclassOf< UObject > Base*/,
//                    bool bAllowAbstract, FString const& Path, FString ClassName)
//{
///*
//    For blueprint classes, things are complicated by the fact that the UClass may not have been loaded into memory yet.
//    The approach taken here is a bit more complicated than it has to be, but allows us to gather the list of subclasses
//    without force loading anything.
//    */
//
//    static const FName GeneratedClassTag = TEXT("GeneratedClass");
//    static const FName ClassFlagsTag = TEXT("ClassFlags");
//
////            check(Base);
//
//    // Load the asset registry module
//    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked< FAssetRegistryModule >(FName("AssetRegistry"));
//    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
//    // The asset registry is populated asynchronously at startup, so there's no guarantee it has finished.
//    // This simple approach just runs a synchronous scan on the entire content directory.
//    // Better solutions would be to specify only the path to where the relevant blueprints are,
//    // or to register a callback with the asset registry to be notified of when it's finished populating.
//    TArray< FString > ContentPaths;
//    ContentPaths.Add(TEXT("/Game"));
//    if (!Path.IsEmpty())
//    {
//        DLOG_INFO("Add search in {}", TCHAR_TO_ANSI(*Path));
//        ContentPaths.Add(Path);
//    }
//
//    AssetRegistry.ScanPathsSynchronous(ContentPaths);
//    DLOG_INFO("scan synchronous");
//
//    //FName BaseClassName = Base->GetFName();
//
//    // Use the asset registry to get the set of all class names deriving from Base
//    TSet< FName > DerivedNames;
//    {
//        TArray< FName > BaseNames;
//        BaseNames.Add(BaseClassName);
////        DLOG_INFO("added base class name");
////        DLOG_INFO("searching for classes derived from {}",
////                  TCHAR_TO_ANSI(*BaseClassName.ToString()));
//
//        TSet< FName > Excluded;
//        AssetRegistry.GetDerivedClassNames(BaseNames, Excluded, DerivedNames);
//    }
//
////    DLOG_INFO("derived class names num {}", DerivedNames.Num());
////    for (auto n : DerivedNames)
////        DLOG_INFO("DERIVED NAME {}", TCHAR_TO_ANSI(*n.ToString()));
//
//    // Set up a filter and then pull asset data for all blueprints in the specified path from the asset registry.
//    // Note that this works in packaged builds too. Even though the blueprint itself cannot be loaded, its asset data
//    // still exists and is tied to the UBlueprint type.
//
//    FARFilter Filter;
//    Filter.ClassNames.Add(FName("Blueprint"));
////            Filter.ClassNames.Add(BaseClassName);
//
//    for (auto cn : Filter.ClassNames)
//        DLOG_INFO(" -- filter ClassName {}", TCHAR_TO_ANSI(*cn.ToString()));
//
//    //UBlueprint::StaticClass()->GetFName());
//    Filter.bRecursiveClasses = true;
//    if(!Path.IsEmpty())
//    {
//        Filter.PackagePaths.Add(*Path);
//    }
//    Filter.bRecursivePaths = true;
//
//    TArray< FAssetData > AssetList;
//    AssetRegistry.GetAssets(Filter, AssetList);
//
//    DLOG_INFO("loaded AssetList, n items {}", AssetList.Num());
//
//    // Iterate over retrieved blueprint assets
//    for(auto const& Asset : AssetList)
//    {
//        DLOG_INFO("item {}",
//                  TCHAR_TO_ANSI(*Asset.AssetName.ToString()));
//
//        TArray<FName> tagKeys;
////                TArray<FName> tagValues;
//        Asset.TagsAndValues.GenerateKeyArray(tagKeys);
////                Asset.TagsAndValues.GenerateValueArray(tagValues);
//
//        DLOG_INFO("item tag keys");
//        for (auto tagKey : tagKeys)
//            DLOG_INFO("{}", TCHAR_TO_ANSI(*tagKey.ToString()));
////                DLOG_INFO("item tag values");
////                for (auto tagValue : tagValues)
////                    DLOG_INFO("{}", TCHAR_TO_ANSI(*tagValue.ToString()));
//
//        // Get the the class this blueprint generates (this is stored as a full path)
//        auto findResult = Asset.TagsAndValues.FindTag(GeneratedClassTag);
//        FString findResultValue = Asset.TagsAndValues.FindTag(TEXT("ParentClass")).GetValue();
////                auto findResult = Asset.TagsAndValues.FindTag(TEXT("ParentClass"));
//
//        DLOG_INFO("find result val {}", TCHAR_TO_ANSI(*findResultValue));
//
//        if(findResult.IsSet())
//        {
//            auto GeneratedClassPathPtr = findResult.GetValue();
//            // Optionally ignore abstract classes
//            // As of 4.12 I do not believe blueprints can be marked as abstract, but this may change so included for completeness.
////                    if(!bAllowAbstract)
////                    {
////                        auto findResult = Asset.TagsAndValues.FindTag(ClassFlagsTag);
////                        if(findResult.IsSet())
////                        {
////                            auto ClassFlagsPtr = findResult.GetValue();
////                            auto ClassFlags = FCString::Atoi(**ClassFlagsPtr);
////                            if((ClassFlags & CLASS_Abstract) != 0)
////                            {
////                                continue;
////                            }
////                        }
////                    }
//
//            // Convert path to just the name part
//            const FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(*GeneratedClassPathPtr);
//            const FString ClassName = FPackageName::ObjectPathToObjectName(ClassObjectPath);
//
//            DLOG_INFO("OBJECT PATH {} CLASS NAME {}",
//                      TCHAR_TO_ANSI(*ClassObjectPath),
//                      TCHAR_TO_ANSI(*ClassName));
//
////                     Check if this class is in the derived set
//            FString CueClassName = "Class'/Script/PlaybackCtrl.CueActor'";
//            if (findResultValue != CueClassName)
//            {
//                continue;
//            }
////                    if(!DerivedNames.Contains(*CueClassName))
////                    {
////                        continue;
////                    }
//
//            // Store using the path to the generated class
//            Subclasses.Add(TAssetSubclassOf< ACueActor >(FStringAssetReference(ClassObjectPath)));
//        }
//    }
//}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPlaybackCtrlModule, PlaybackCtrl)
