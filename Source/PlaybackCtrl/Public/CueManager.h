// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CueManager.generated.h"

UCLASS()
class PLAYBACKCTRL_API ACueManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACueManager();
    
    void GetAllBlueprintSubclasses(TArray< TAssetSubclassOf< ACueActor > >& Subclasses, FName BaseClassName, bool bAllowAbstract, FString const& Path, FString ClassName);
    
    void SpawnCues(UWorld *world);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
