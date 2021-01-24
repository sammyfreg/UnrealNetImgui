//=================================================================================================
// NetImgui Demo Actor
//-------------------------------------------------------------------------------------------------
// Example of using 'NetImgui' with 'Dear ImGui' inside an Actor class. 
//
// The sources files are not compiled nor needed by the 'UnrealNetImgui' Plugin, it is only here 
// to help integrating it to your own codebase. 
//
// The 'Dear ImGui' draws can be done from anywhere in the engine, on the GameThread, not limited 
// to 'AActor::Tick()' or an Actor class.
//=================================================================================================
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NetImguiActor.generated.h"

UCLASS()
class MYPROJECT_API ANetImguiActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ANetImguiActor(){ PrimaryActorTick.bCanEverTick = true; }

	// Makes sure tick is called even outside of PIE
	virtual bool ShouldTickIfViewportsOnly() const override{ return true; }
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
