// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once


#include "Engine/DeveloperSettings.h"
#include "NetImguiSettings.generated.h"

//SF #include "HAL/IConsoleManager.h"
//NETIMGUI_API extern TAutoConsoleVariable<int32> CVarNetImguiShow;


UENUM()
enum class ENetImguiVisibility : uint8
{
	Disabled		UMETA(Tooltip="Not visible in any viewport."),
	Always			UMETA(Tooltip="Always visible in all viewports."),
	Focused			UMETA(Tooltip="Visible in the viewport that has the focus."),
	Activated		UMETA(Tooltip="Visible in viewports in which Dear ImGui was activated with the ToggleKeys (or by software)."),
};

UCLASS(config = Engine, defaultconfig, meta = (DisplayName = "NetImgui"))
class NETIMGUI_API UNetImguiSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	//============================================================================
	// GLOBAL PLUGIN SETTINGS
	//============================================================================
	UPROPERTY(config, EditAnywhere, Category = "All", meta = (
		ConsoleVariable = "netimgui.show", DisplayName = "Show Dear ImGui content (master toggle)",
		ToolTip = "Control Dear ImGui visibility for everything.",
		ConfigRestartRequired = false))
	uint32 Show : 1;

	//============================================================================
	// REMOTE DEAR IMGUI SETTINGS
	//============================================================================
	UPROPERTY(config, EditAnywhere, Category = "Remote", meta = (
		DisplayName = "Show Dear ImGui remotely",
		ToolTip = "Enable or disable drawing Dear ImGui content for the NetImgui remote server.",
		ConfigRestartRequired = false))
	uint32 RemoteShow : 1;

	UPROPERTY(config, EditAnywhere, Category = "Remote", meta = (
		DisplayName = "Hide local Dear ImGui",
		ToolTip = "If we should disable the Dear ImGui local drawing when we are also drawing for the remote NetImgui connection.",
		ConfigRestartRequired = false))
	uint32 RemoteHideLocal : 1;

	UPROPERTY(config, EditAnywhere, Category = "Remote", meta = (
		DisplayName = "Remote Server Name [optional]",
		ToolTip = "When a value is assigned, will try to connect to the NetImgui Server at this address",
		ConfigRestartRequired = false))
	FString RemoteServerName;

	UPROPERTY(config, EditAnywhere, Category = "Remote", meta = (
		DisplayName = "Remote Server Port [optional]",
		ToolTip = "When a value is assigned, will try to connect to the NetImgui Server at this Name:Port",
		ClampMin = 1, ClampMax = 65535,
		ConfigRestartRequired = false))
	uint32 RemoteServerPort;

	UPROPERTY(config, EditAnywhere, Category = "Remote", meta = (
		DisplayName = "Game Port",
		ToolTip = "Port opened when this game/editor waits for a connection from the NetImGui Server. A connection can either be established by waiting for the server to reach us on this port or the game can try reaching the Server directy.",
		ClampMin = 1, ClampMax = 65535,
		ConfigRestartRequired = false))
	uint32 RemoteClientPort;
	
	//============================================================================
	// LOCAL DEAR IMGUI SETTINGS
	//============================================================================
	UPROPERTY(config, EditAnywhere, Category = "Local", meta = (
		DisplayName = "Use 'On Screen Debug' showflag",
		ToolTip = "When this option is enabled, Dear ImGui content won't be visible when the ShowFlag is disabled",
		ConfigRestartRequired = false))
	uint32 LocalUseOnScreenDebugFlag : 1;

	UPROPERTY(config, EditAnywhere, Category = "Local", meta = (
		DisplayName = "Show Dear ImGui content in game view",
		ToolTip = "When we should enable drawing Dear ImGui content inside a game viewport.",
		ConfigRestartRequired = false))
	ENetImguiVisibility LocalVisibilityGame;

	UPROPERTY(config, EditAnywhere, Category = "Local", meta = (
		DisplayName = "Show Dear ImGui content in editor view",
		ToolTip = "When we should enable drawing Dear ImGui content inside an editor viewport.",
		ConfigRestartRequired = false))
	ENetImguiVisibility LocalVisibilityEditor;
	
	UPROPERTY(config, EditAnywhere, Category="Local", meta = (
		DisplayName = "Dear ImGui Toggle Keys (Option 1)",
		ToolTip = "Keys needed to toggle the (A) Input focus in the Game's Viewport and (B) Visibility in the Editor's Viewport. All keys in the array must be held down to activate."))
	TArray<FKey> ToggleKeys1;

	UPROPERTY(config, EditAnywhere, Category="Local", meta = (
		DisplayName = "Dear ImGui Toggle Keys (Option 2)",
		ToolTip = "Keys needed to toggle the (A) Input focus in the Game's Viewport and (B) Visibility in the Editor's Viewport. All keys in the array must be held down to activate."))
	TArray<FKey> ToggleKeys2;

	UPROPERTY(config, EditAnywhere, Category="Local", meta = (
		DisplayName = "Dear ImGui Toggle Keys (Option 3)",
		ToolTip = "Keys needed to toggle the (A) Input focus in the Game's Viewport and (B) Visibility in the Editor's Viewport. All keys in the array must be held down to activate."))
	TArray<FKey> ToggleKeys3;

public:
	UNetImguiSettings();
	virtual void PostInitProperties() override;
	virtual FName GetCategoryName() const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override; 
#endif
};
