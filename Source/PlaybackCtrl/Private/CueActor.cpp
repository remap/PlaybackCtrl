// Fill out your copyright notice in the Description page of Project Settings.


#include "CueActor.h"
#include "Kismet/GameplayStatics.h"

#include "PlaybackCtrl.h"
#include "LevelSequencePlayer.h"
#include "LevelSequenceActor.h"
#include "MovieSceneSequencePlayer.h"
#include "DDHelpers.h"
// Sets default values
ACueActor::ACueActor()
    : _listener(this)
{
    oscListener = !isDefaultObject();

    if (!isDefaultObject())
        SetActorTickEnabled(true);

    {
        DLOG_TRACE("Created Cue actor {} ({}, type {})", TCHAR_TO_ANSI(*AActor::GetDebugName(this)),
            TCHAR_TO_ANSI(*GetHumanReadableName()), TCHAR_TO_ANSI(*GetClass()->GetName()));
        UE_LOG(LogTemp, Log, TEXT("Created Cue actor %s (%s type %s)"), *AActor::GetDebugName(this),
            *GetHumanReadableName(), *GetClass()->GetName());
    }
}

ACueActor::ACueActor(FVTableHelper & helper)
    : _listener(this)
{
}

void ACueActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (cueState_ != CueActorState::None &&
        cueState_ != CueActorState::Finished)
    {
        // GetDeltaSeconds() accounts for world dilation
        // we might not need it, time dilation never changes
        // (in that case it'll be same as DeltaTime)
        float d = GetWorld()->GetDeltaSeconds();
        float stateLen = 0;

        switch (cueState_)
        {
        case CueActorState::FadeIn:
            stateLen = fadeInLen_;
            break;
        case CueActorState::Run:
            stateLen = runLen_;
            break;
        case CueActorState::FadeOut:
            stateLen = fadeOutLen_;
            break;
        default:
            break;
        }

        if (stateLen == 0)
            cueStateProgress_ = 1.;
        else
            cueStateProgress_ += d / stateLen;

        if (cueTotalLen_ == 0)
            cueProgress_ = 1;
        else
            cueProgress_ += d / cueTotalLen_;

        if (cueStateProgress_ >= 1.)
        {
            cueStateProgress_ = 1.;
            UE_LOG(LogTemp, Log, TEXT("Cue State Complete"));
            DLOG_TRACE("calling end of state delegate");
            onStateEndDelegate_.ExecuteIfBound();
        }
        if (cueProgress_ >= 1.)
            cueProgress_ = 1.;

    }
}

void ACueActor::BeginPlay()
{
    Super::BeginPlay();

    cueState_ = CueActorState::None;

    if (oscListener)
    {
        FPlaybackCtrlModule* mod = FPlaybackCtrlModule::GetSharedInstance();
        if (mod)
        {
            UE_LOG(LogTemp, Log, TEXT("CueActor %s OSC Listening..."),
                *GetHumanReadableName());
            mod->RegisterReceiver(&_listener);
        }
        else
            UE_LOG(LogTemp, Log, TEXT("no module"));

        if (!OnCueRx.Contains(this, FName("OnCueReceived")))
            OnCueRx.AddDynamic(this, &ACueActor::OnCueReceived);
        else
            UE_LOG(LogTemp, Log, TEXT("How did I get here?"));
    }

    UE_LOG(LogTemp, Log, TEXT("CueActor %s BeginPlay. Is General Purpose: %d"),
        *GetHumanReadableName(), generalPurposeCue);
}

void ACueActor::BeginDestroy()
{
    FPlaybackCtrlModule* mod = FPlaybackCtrlModule::GetSharedInstance();
    if (mod)
        mod->UnregisterReceiver(&_listener);
    else
        DLOG_TRACE("no module");

    UE_LOG(LogTemp, Log, TEXT("CueActor %s BeginDestroy"), *GetHumanReadableName());

    Super::BeginDestroy();
}

void ACueActor::OnCueReceived(const FName & Address, const TArray<FOscDataElemStruct> & Data, const FString & SenderIp)
{
    DLOG_MODULE_TRACE(PlaybackCtrl, "CueActor {} -- parsing cue {}",
                      TCHAR_TO_ANSI(*GetHumanReadableName()),
                      TCHAR_TO_ANSI(*Address.ToString()));
    UE_LOG(LogTemp, Log, TEXT("CueActor %s -- parsing cue %s"),
        *GetHumanReadableName(),
        *Address.ToString());

    // Parse OSC message
    // Address: Current naming: /<project>/<build>/<dept>/<cue name>/<action>
    FString oscAddress = Address.ToString();
    TArray<FString> addressParts;
    oscAddress.ParseIntoArray(addressParts, TEXT("/"), true);
    TMap<FString, FString> AddressDict;

    AddressDict.Add(TEXT("Build"), addressParts[1]);
    AddressDict.Add(TEXT("Department"), addressParts[2]);

#if 0
#if WITH_EDITOR
    AddressDict.Add(TEXT("CueName"), addressParts[3].Append("_"));
#else
    AddressDict.Add(TEXT("CueName"), addressParts[3].Append("_C"));
#endif
#else
    AddressDict.Add(TEXT("CueName"), addressParts[3]);
#endif
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

    // save cue lengths
    fadeInLen_ = getStateLength(CueActorState::FadeIn);
    fadeOutLen_ = getStateLength(CueActorState::FadeOut);
    runLen_ = getStateLength(CueActorState::Run);
    cueTotalLen_ = fadeInLen_ + fadeOutLen_ + runLen_;

    DLOG_MODULE_TRACE(PlaybackCtrl, "CueActor {} (debug name {}) received OSC message: build {} dept {} cuename {} action {}",
                      TCHAR_TO_ANSI(*GetHumanReadableName()),
                      TCHAR_TO_ANSI(*AActor::GetDebugName(this)),
                      TCHAR_TO_ANSI(*AddressDict["Build"]),
                      TCHAR_TO_ANSI(*AddressDict["Department"]),
                      TCHAR_TO_ANSI(*AddressDict["CueName"]),
                      TCHAR_TO_ANSI(*AddressDict["Action"])
                      );
    UE_LOG(LogTemp, Log, TEXT("CueActor %s (debug name %s) received OSC message: build %s dept %s cuename %s action %s"),
        *GetHumanReadableName(),
        *AActor::GetDebugName(this),
        *AddressDict["Build"],
        *AddressDict["Department"],
        *AddressDict["CueName"],
        *AddressDict["Action"]);

    // Handle pausing/resuming
    FString theAction = AddressDict["Action"].ToLower();
    if (GetHumanReadableName().Contains(AddressDict["CueName"]))
    {
        if (SequencePlayer)
        {
            if (theAction == "pause")
            {
                DLOG_MODULE_TRACE(PlaybackCtrl, "CueActor {} -- cue pause",
                                  TCHAR_TO_ANSI(*GetHumanReadableName()));
                SequencePlayer->Pause();
            }
            else if (theAction == "resume")
            {
                DLOG_MODULE_TRACE(PlaybackCtrl, "CueActor {} -- cue resume",
                                  TCHAR_TO_ANSI(*GetHumanReadableName()));
                SequencePlayer->Play();
            }
        }
        else if (theAction == "go")
        {
            DLOG_MODULE_TRACE(PlaybackCtrl, "CueActor {} -- cue go",
                              TCHAR_TO_ANSI(*GetHumanReadableName()));
            OnFadeInStart_Implementation();
        }
        else if (theAction == "reset")
        {
            DLOG_MODULE_TRACE(PlaybackCtrl, "CueActor {} -- cue reset",
                              TCHAR_TO_ANSI(*GetHumanReadableName()));
            ResetCue();
        }
    }
    else
    {
        DLOG_MODULE_TRACE(PlaybackCtrl, "CueActor {} ignoring cue {}",
                          TCHAR_TO_ANSI(*GetHumanReadableName()),
                          TCHAR_TO_ANSI(*AddressDict["CueName"]));
    }
}

void ACueActor::OnFadeInStart_Implementation()
{
    setState(CueActorState::FadeIn);

    DLOG_INFO("FadeIn Start");
    UE_LOG(LogTemp, Log, TEXT("FadeIn Start"));
    OnFadeInStart(); //for BP
    if (GetFadeInSeq())
        CueStateStart(GetFadeInSeq(), "FadeInLength", "OnFadeInEnd_Implementation");
    else
        //OnFadeInEnd_Implementation();
        onStateEndDelegate_.BindUObject(this, &ACueActor::OnFadeInEnd_Implementation);
}

void ACueActor::OnFadeInEnd_Implementation()
{
    OnFadeInEnd(); // for BP
//    DLOG_INFO("Fade in End. TIME: {}", TCHAR_TO_ANSI(*FDateTime::Now().ToString()));
//    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Fade In End Implementation"));
    SequencePlayer = nullptr;
    OnRunStart_Implementation();

    DLOG_INFO("FadeIn End");
    UE_LOG(LogTemp, Log, TEXT("FadeIn End"));
}

void ACueActor::OnRunStart_Implementation()
{
    setState(CueActorState::Run);

    DLOG_INFO("Run Start");
    UE_LOG(LogTemp, Log, TEXT("RunStart"));
    OnRunStart(); // for BP
//    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Run Start Implementation"));
    if (GetRunSeq())
        CueStateStart(GetRunSeq(), "RunLength","OnRunEnd_Implementation");
    else
    //    OnRunEnd_Implementation();
        onStateEndDelegate_.BindUObject(this, &ACueActor::OnRunEnd_Implementation);
}

void ACueActor::OnRunEnd_Implementation()
{
    OnRunEnd(); // for BP
    SequencePlayer = nullptr;
    OnFadeOutStart_Implementation();

    DLOG_INFO("Run End");
    UE_LOG(LogTemp, Log, TEXT("Run End"));
}

void ACueActor::OnFadeOutStart_Implementation()
{
    setState(CueActorState::FadeOut);

    DLOG_INFO("FadeOut Start");
    UE_LOG(LogTemp, Log, TEXT("FadeOut Start"));
    OnFadeOutStart(); //for BP
    if (GetFadeOutSeq())
        CueStateStart(GetFadeOutSeq(), "FadeOutLength", "OnFadeOutEnd_Implementation");
    else
        //    OnFadeOutEnd();
        onStateEndDelegate_.BindUObject(this, &ACueActor::OnFadeOutEnd_Implementation);
}

void ACueActor::OnFadeOutEnd_Implementation()
{
    OnFadeOutEnd(); //for BP
    SequencePlayer = nullptr;

    setState(CueActorState::Finished);
    onStateEndDelegate_.Unbind();

    DLOG_INFO("FadeOut End");
    UE_LOG(LogTemp, Log, TEXT("FadeOut End"));
    OnCueCompleted.Broadcast();
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
    setState(CueActorState::None);
    cueProgress_ = 0.;

}

void ACueActor::setState(CueActorState state)
{
    cueStateProgress_ = 0.;
    cueState_ = state;
}

FString ACueActor::getCueStateString() const
{
    switch (cueState_)
    {
    case CueActorState::None:
        return FString("None");
    case CueActorState::FadeIn:
        return FString("FadeIn");
    case CueActorState::Run:
        return FString("Run");
    case CueActorState::FadeOut:
        return FString("FadeOut");
    case CueActorState::Finished:
        return FString("Finished");
    default:
        break;
    }

    return FString();
}

float ACueActor::getStateLength(CueActorState state)
{
    float len = 0.;
    FString stateLengthKey;
    ULevelSequence* seq = nullptr;

    switch (state) {
    case CueActorState::FadeIn:
    {
        seq = GetFadeInSeq();
        stateLengthKey = "FadeInLength";
    }
        break;
    case CueActorState::Run:
    {
        seq = GetRunSeq();
        stateLengthKey = "RunLength";
    }
        break;
    case CueActorState::FadeOut:
    {
        seq = GetFadeOutSeq();
        stateLengthKey = "FadeOutLength";
    }
        break;

    default:
        return 0;
    }

    if (DataDict_.Contains(stateLengthKey))
        len = FCString::Atof(*DataDict_[stateLengthKey]);
    else
        len = getSequenceDurationSeconds(seq);

    return len;
}

float ACueActor::getSequenceDurationSeconds(ULevelSequence* seq) const
{
    if (!seq)
        return 0.;

    ALevelSequenceActor* LevelSequenceActor;
    ULevelSequencePlayer* tmpSeqPlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(GetWorld(),
        seq,
        FMovieSceneSequencePlaybackSettings(),
        LevelSequenceActor);

    return tmpSeqPlayer->GetDuration().AsSeconds();
}

float ACueActor::GetFloatParam(FString name) {
    float f = 1.0;
    if (DataDict_.Contains(name))
    {
        f = FCString::Atof(*DataDict_[name]);
    }
    return f;
}

FString ACueActor::GetStringParam(FString name) {
    FString s = "";
    if (DataDict_.Contains(name))
    {
        s = *DataDict_[name];
    }
    return s;
}

bool ACueActor::isDefaultObject() const
{
    return GetHumanReadableName().Contains("Default__");
}

FString ACueActor::GetListenerName() const
{
    return GetHumanReadableName();
}

void ACueActor::SpawnCue(const FName& Address, const TArray<FOscDataElemStruct>& Data, const FString& SenderIp)
{
    {
        /*AActor* UWorld::SpawnActor
        (
            UClass * Class,
            FName           InName,
            FVector const* Location,
            FRotator const* Rotation,
            AActor * Template,
            bool            bNoCollisionFail,
            bool            bRemoteOwned,
            AActor * Owner,
            APawn * Instigator,
            bool            bNoFail,
            ULevel * OverrideLevel,
            bool            bDeferConstruction
        )*/

        FTransform t = FTransform::Identity;
        auto a = UGameplayStatics::BeginDeferredActorSpawnFromClass(this, this->GetClass(), t);
        if (a)
        {
            Cast<ACueActor>(a)->oscListener = false;
            // setup cleanup callback
            Cast<ACueActor>(a)->OnCueCompleted.AddLambda([a]() {
                UE_LOG(LogTemp, Log, TEXT("Destroying spawned actor %s"),
                    *a->GetHumanReadableName());
                a->Destroy();
            });

            UGameplayStatics::FinishSpawningActor(a, t);

            // forward OSC message
            Cast<ACueActor>(a)->OnCueReceived(Address, Data, SenderIp);
        }
    }
}
