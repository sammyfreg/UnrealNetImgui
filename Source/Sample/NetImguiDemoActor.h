//=================================================================================================
// NetImguiDemo Actor
//-------------------------------------------------------------------------------------------------
// Example of using 'NetImgui' with 'Dear ImGui' inside an Actor class. Just drop actors of this 
// class, in your scene, to see the demo 'Dear ImGui' content appear on the server.
//
// The 'Dear ImGui' draws can be done from anywhere in the engine (on the GameThread),
// and not limited to 'AActor::Tick()' or an Actor class.
// 
// !!! This class is not needed to use Dear ImGui / NetImgui, it is here as an example !!!
//=================================================================================================
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NetImguiDemoActor.generated.h"

#ifndef NETIMGUI_DEMO_ACTOR_ENABLED
	#define NETIMGUI_DEMO_ACTOR_ENABLED 0
#endif

#if !defined(NETIMGUI_ENABLED) || !NETIMGUI_ENABLED
	#undef NETIMGUI_DEMO_ACTOR_ENABLED
	#define NETIMGUI_DEMO_ACTOR_ENABLED 0
#endif

UCLASS()
class NETIMGUI_API ANetImguiDemoActor : public AActor
{
	GENERATED_BODY()
public:
	// Sets default values for this actor's properties
	ANetImguiDemoActor();

#if NETIMGUI_DEMO_ACTOR_ENABLED
	
	// Makes sure tick is called even outside of PIE
	virtual bool ShouldTickIfViewportsOnly() const override{ return true; }
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	void DrawImgui_OncePerActor();
	void DrawImgui_OncePerFrame();

#endif //#if NETIMGUI_DEMO_ACTOR_ENABLED
};
