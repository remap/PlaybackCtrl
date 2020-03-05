// Fill out your copyright notice in the Description page of Project Settings.


#include "CueActor.h"
#include "PlaybackCtrl.h"
#include "LevelSequencePlayer.h"
#include "LevelSequenceActor.h"
#include "MovieSceneSequencePlayer.h"

// Sets default values
ACueActor::ACueActor()
    : _listener(this)
{
    FPlaybackCtrlModule* mod = FPlaybackCtrlModule::GetSharedInstance();
    if (mod)
    {
        mod->RegisterReceiver(&_listener);
        UE_LOG(LogTemp, Log, TEXT("got mod"));
        DLOG_TRACE("got mod!");
    }
    else
        UE_LOG(LogTemp, Log, TEXT("no module"));
    
    OnCueRx.AddDynamic(this, &ACueActor::OnCueReceived);

}

ACueActor::ACueActor(FVTableHelper & helper)
    : _listener(this)
{
    
}
// Called when the game starts or when spawned
void ACueActor::BeginDestroy()
{
    FPlaybackCtrlModule* mod = FPlaybackCtrlModule::GetSharedInstance();
    if (mod)
        mod->UnregisterReceiver(&_listener);
    else
        DLOG_TRACE("no module");
    
    Super::BeginDestroy();
}
void ACueActor::OnCueReceived(const FName & Address, const TArray<FOscDataElemStruct> & Data, const FString & SenderIp)
{
    // Parse OSC message
    // Address: Current naming: /<project>/<build>/<dept>/<cue name>/<action>
    FString oscAddress = Address.ToString();
    TArray<FString> addressParts;
    oscAddress.ParseIntoArray(addressParts, TEXT("/"), true);
    TMap<FString, FString> AddressDict;
    
    AddressDict.Add(TEXT("Build"), addressParts[1]);
    AddressDict.Add(TEXT("Department"), addressParts[2]);
    for (int32 Index = 3; Index < addressParts.Num() -1; ++Index)
    {
        AddressDict.Add(TEXT("CueName_") + FString::FromInt(Index), addressParts[Index]);
    }
    AddressDict.Add(TEXT("Action"), addressParts.Last());
    
    // Arguments
    TMap<FString, FString> DataDict;
    TArray<FString> d;
    for (auto& elem : Data)
    {
        elem.AsStringValue().ToString().ParseIntoArray(d, TEXT("="), true);
        DataDict.Add(d[0], d[1]);
    }
    
    
    FMovieSceneSequencePlaybackSettings PlaybackSettings;
    ALevelSequenceActor* LevelSequenceActor;

    ULevelSequencePlayer* SequencePlayer;
    if (GetRunSeq())
    {
        SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(GetWorld(), GetRunSeq(), FMovieSceneSequencePlaybackSettings(), LevelSequenceActor);
        
        if (SequencePlayer)
        {
            SequencePlayer->Play();
            float secs = SequencePlayer->GetDuration().AsSeconds();
            UE_LOG(LogTemp, Log, TEXT("The duration is %f seconds"), secs);
            FScriptDelegate funcDelegate;
            funcDelegate.BindUFunction(this, "OnRunEnd");
            SequencePlayer->OnFinished.AddUnique(funcDelegate);
//            FadeOut->SequencePlayer->SetPlaybackPosition(0.0f);
//            FadeOut->SequencePlayer->Play();
        }
    }
}

void ACueActor::OnRunEnd_Implementation()
{
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("The seq finished"));
}


