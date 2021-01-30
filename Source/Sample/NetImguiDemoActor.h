//=================================================================================================
// NetImguiDemo Actor
//-------------------------------------------------------------------------------------------------
// Example of using 'NetImgui' with 'Dear ImGui' inside an Actor class. Just drop actors of this 
// class, in your scene, to see the demo 'Dear ImGui' content appear on the server.
//
// The 'Dear ImGui' draws can be done from anywhere in the engine (on the GameThread),
// and not limited to 'AActor::Tick()' or an Actor class.
// 
// !!! This class is not needed to use Dear ImGui / NetImgui (just an example) !!!
//=================================================================================================
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NetImguiDemoActor.generated.h"

UCLASS()
class NETIMGUI_API ANetImguiDemoActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ANetImguiDemoActor(){ PrimaryActorTick.bCanEverTick = true; }

	// Makes sure tick is called even outside of PIE
	virtual bool ShouldTickIfViewportsOnly() const override{ return true; }
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
