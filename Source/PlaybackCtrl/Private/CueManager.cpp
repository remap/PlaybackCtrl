// Fill out your copyright notice in the Description page of Project Settings.


#include "CueManager.h"

#include <AssetRegistryModule.h>
#include <ARFilter.h>
#include <UObject/Class.h>
#include <UObject/UObjectIterator.h>

#include "Kismet/GameplayStatics.h"
// Sets default values
ACueManager::ACueManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ACueManager::BeginPlay()
{
	Super::BeginPlay();
    
    
//    TSubclassOf<ACueActor> classToFind;
//    classToFind = ACueActor::StaticClass();
//    TArray<AActor*> foundCues;
//    UGameplayStatics::GetAllActorsOfClass(GetWorld(), classToFind, foundCues);
//
//    UE_LOG(LogTemp, Log, TEXT("I Found %i Cues"), foundCues.Num());
//    DLOG_INFO("I Found {} Cues", foundCues.Num());
    
    
    SpawnCues(GetWorld());
    
//    TArray<TAssetSubclassOf<ACueActor>> PlaybackCtrl_ClassesToSpawn;
//    FString CueClassName = "Class'/Script/PlaybackCtrl.CueActor'";
//    GetAllBlueprintSubclasses(PlaybackCtrl_ClassesToSpawn, FName("ACueActor"), false, TEXT("/Game"), CueClassName);
//    DLOG_INFO("I got {} resutls", PlaybackCtrl_ClassesToSpawn.Num());
//    int32 count = 0;
//    for (auto& Cue : PlaybackCtrl_ClassesToSpawn)
//    {
//        count += 1;
//
//        FActorSpawnParameters SpawnParams;
//        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
//        FString namestring = "Hope" + FString::FromInt(count);
//        SpawnParams.Name = FName(*namestring);
//        ACueActor* ActorRef = GetWorld()->SpawnActor<ACueActor>(Cue.Get(), GetTransform(), SpawnParams);
//        DLOG_INFO("I SPAWNEDMAYBE??");
//    }
}

void ACueManager::SpawnCues(UWorld *world)
{
    // The cues will be spawned in every loaded level
    // TODO: possibly implement any logic that excludes temp levels
    TArray<TAssetSubclassOf<ACueActor>> ToSpawn;
    FString CueClassName = "Class'/Script/PlaybackCtrl.CueActor'";
    GetAllBlueprintSubclasses(ToSpawn, FName("ACueActor"), false, TEXT("/Game"), CueClassName);
    DLOG_INFO("I got {} CueActor results to spawn in the Cue Manager", ToSpawn.Num());
    for (auto& Cue : ToSpawn)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
//        FString namestring = "Hope" + FString::FromInt(count);
        SpawnParams.Name = FName(*Cue.GetAssetName());
        DLOG_INFO("my name is {}", TCHAR_TO_ANSI(*Cue.GetAssetName()));
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

// Called every frame
void ACueManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}




void ACueManager::GetAllBlueprintSubclasses(TArray< TAssetSubclassOf< ACueActor > >& Subclasses,
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
        DLOG_INFO("Add search in {}", TCHAR_TO_ANSI(*Path));
        ContentPaths.Add(Path);
    }

    AssetRegistry.ScanPathsSynchronous(ContentPaths);
    DLOG_INFO("scan synchronous");

    //FName BaseClassName = Base->GetFName();

    // Use the asset registry to get the set of all class names deriving from Base
    TSet< FName > DerivedNames;
    {
        TArray< FName > BaseNames;
        BaseNames.Add(BaseClassName);
//        DLOG_INFO("added base class name");
//        DLOG_INFO("searching for classes derived from {}",
//                  TCHAR_TO_ANSI(*BaseClassName.ToString()));

        TSet< FName > Excluded;
        AssetRegistry.GetDerivedClassNames(BaseNames, Excluded, DerivedNames);
    }

//    DLOG_INFO("derived class names num {}", DerivedNames.Num());
//    for (auto n : DerivedNames)
//        DLOG_INFO("DERIVED NAME {}", TCHAR_TO_ANSI(*n.ToString()));

    // Set up a filter and then pull asset data for all blueprints in the specified path from the asset registry.
    // Note that this works in packaged builds too. Even though the blueprint itself cannot be loaded, its asset data
    // still exists and is tied to the UBlueprint type.

    FARFilter Filter;
    Filter.ClassNames.Add(FName("Blueprint"));
//            Filter.ClassNames.Add(BaseClassName);

    for (auto cn : Filter.ClassNames)
        DLOG_INFO(" -- filter ClassName {}", TCHAR_TO_ANSI(*cn.ToString()));

//    UBlueprint::StaticClass()->GetFName());
    Filter.bRecursiveClasses = true;
    if(!Path.IsEmpty())
    {
        Filter.PackagePaths.Add(*Path);
    }
    Filter.bRecursivePaths = true;

    TArray< FAssetData > AssetList;
    AssetRegistry.GetAssets(Filter, AssetList);

    DLOG_INFO("loaded AssetList, n items {}", AssetList.Num());

    // Iterate over retrieved blueprint assets
    for(auto const& Asset : AssetList)
    {
        DLOG_INFO("item {}",
                  TCHAR_TO_ANSI(*Asset.AssetName.ToString()));

        TArray<FName> tagKeys;
//                TArray<FName> tagValues;
        Asset.TagsAndValues.GenerateKeyArray(tagKeys);
//                Asset.TagsAndValues.GenerateValueArray(tagValues);

        DLOG_INFO("item tag keys");
        for (auto tagKey : tagKeys)
            DLOG_INFO("{}", TCHAR_TO_ANSI(*tagKey.ToString()));
//                DLOG_INFO("item tag values");
//                for (auto tagValue : tagValues)
//                    DLOG_INFO("{}", TCHAR_TO_ANSI(*tagValue.ToString()));

        // Get the the class this blueprint generates (this is stored as a full path)
        auto findResult = Asset.TagsAndValues.FindTag(GeneratedClassTag);
        FString findResultValue = Asset.TagsAndValues.FindTag(TEXT("ParentClass")).GetValue();
//                auto findResult = Asset.TagsAndValues.FindTag(TEXT("ParentClass"));

        DLOG_INFO("find result val {}", TCHAR_TO_ANSI(*findResultValue));

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

            DLOG_INFO("OBJECT PATH {} CLASS NAME {}",
                      TCHAR_TO_ANSI(*ClassObjectPath),
                      TCHAR_TO_ANSI(*ClassName));

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


