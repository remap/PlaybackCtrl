// Fill out your copyright notice in the Description page of Project Settings.


#include "CueActor.h"
#include "PlaybackCtrl.h"
#include "LevelSequencePlayer.h"
#include "LevelSequenceActor.h"
#include "MovieSceneSequencePlayer.h"
#include "DDHelpers.h"
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

void ACueActor::BeginPlay()
{
    Super::BeginPlay();
}

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
    AddressDict.Add(TEXT("CueName"), addressParts[3].Append("_C"));
    // use this block if there will be additional
    //  components in the naming hierarchy
//    for (int32 Index = 3; Index < addressParts.Num() -1; ++Index)
//    {
//        AddressDict.Add(TEXT("CueName_") + FString::FromInt(Index), addressParts[Index]);
//    }
    AddressDict.Add(TEXT("Action"), addressParts.Last());

    // Arguments
    TMap<FString, FString> DataDict;
    TArray<FString> d;
    for (auto& elem : Data)
    {
        elem.AsStringValue().ToString().ParseIntoArray(d, TEXT("="), true);
        DataDict.Add(d[0], d[1]);
    }
    DataDict_ = DataDict;
    
    
    // Handle pausing/resuming
    FString theAction = AddressDict["Action"].ToLower();
    if (AddressDict["CueName"] == GetHumanReadableName())
    {
        if (SequencePlayer)
        {
            if (theAction == "pause")
                SequencePlayer->Pause();
            else if (theAction == "resume")
                SequencePlayer->Play();
        }
        else if (theAction == "go")
            OnFadeInStart_Implementation();
        else if (theAction == "reset")
            ResetCue();
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("This cue isn't for %s"), *GetHumanReadableName());
    }
}

void ACueActor::OnFadeInStart_Implementation()
{
    OnFadeInStart(); //for BP
    if (GetFadeInSeq())
        CueStateStart(GetFadeInSeq(), "FadeInLength", "OnFadeInEnd_Implementation");
    else
        OnFadeInEnd_Implementation();
}

void ACueActor::OnFadeInEnd_Implementation()
{
    OnFadeInEnd(); // for BP
//    DLOG_INFO("Fade in End. TIME: {}", TCHAR_TO_ANSI(*FDateTime::Now().ToString()));
//    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Fade In End Implementation"));
    SequencePlayer = nullptr;
    OnRunStart_Implementation();
    
}


void ACueActor::OnRunStart_Implementation()
{
    DLOG_INFO("Run Start");
    UE_LOG(LogTemp, Log, TEXT("RunStart"));
    OnRunStart(); // for BP
//    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Run Start Implementation"));
    if (GetRunSeq())
        CueStateStart(GetRunSeq(), "RunLength","OnRunEnd_Implementation");
    else
        OnRunEnd_Implementation();
}

void ACueActor::OnRunEnd_Implementation()
{
    OnRunEnd(); // for BP
    SequencePlayer = nullptr;
    OnFadeOutStart_Implementation();
}

void ACueActor::OnFadeOutStart_Implementation()
{
    OnFadeOutStart(); //for BP
    if (GetFadeOutSeq())
        CueStateStart(GetFadeOutSeq(), "FadeOutLength","OnFadeOutEnd_Implementation");
    else
        OnFadeOutEnd();
}

void ACueActor::OnFadeOutEnd_Implementation()
{
    OnFadeOutEnd(); //for BP
    SequencePlayer = nullptr;
}

void ACueActor::CueStateStart(ULevelSequence* Seq, FString CueStateLength, FName EndCueState)
{
    UE_LOG(LogTemp, Log, TEXT("CueStateStart"));
    if (SequencePlayer == nullptr)
    {
        ALevelSequenceActor* LevelSequenceActor;
        SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(GetWorld(), Seq, FMovieSceneSequencePlaybackSettings(), LevelSequenceActor);

        if (DataDict_.Contains(CueStateLength))
        {
            float l = FCString::Atof(*DataDict_[CueStateLength]);
            float d = SequencePlayer->GetDuration().AsSeconds();
            SequencePlayer->SetPlayRate(d/l);
        }
        FScriptDelegate funcDelegate;
        funcDelegate.BindUFunction(this, EndCueState);
        SequencePlayer->OnFinished.AddUnique(funcDelegate);
    }
            
    if (SequencePlayer)
    {
//        FScriptDelegate funcDelegate;
//        funcDelegate.BindUFunction(this, EndCueState);
//        SequencePlayer->OnFinished.AddUnique(funcDelegate);
        SequencePlayer->PlayToFrame(0);
        SequencePlayer->Play();
        UE_LOG(LogTemp, Log, TEXT("pressed Play"));
    }
}

void ACueActor::ResetCue()
{
    ULevelSequence* Seq;
    if (GetFadeInSeq())
        Seq = GetFadeInSeq();
    else if (GetRunSeq())
        Seq = GetRunSeq();
    else if (GetFadeOutSeq())
        Seq = GetFadeOutSeq();
    else
        return;
        
    ALevelSequenceActor* LevelSequenceActor;
    SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(GetWorld(), Seq, FMovieSceneSequencePlaybackSettings(), LevelSequenceActor);
    SequencePlayer->PlayToFrame(0);
    SequencePlayer = nullptr;
//    SequencePlayer->Play();
//    SequencePlayer->Pause();
    
}



