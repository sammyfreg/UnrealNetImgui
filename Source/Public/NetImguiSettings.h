// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/UnrealType.h"
#include "Engine/DeveloperSettings.h"
#include "NetImguiSettings.generated.h"

UENUM()
enum class ENetImguiVisibility : uint8
{
	Disabled		UMETA(Tooltip="Always invisible in all viewports."),
	Always			UMETA(Tooltip="Always visible in all viewports."),
	Focused			UMETA(Tooltip="Visible in the focused viewport."),
	HasInput		UMETA(Tooltip="Visible in viewports with enabled Dear Imgui input (using ToggleKeys or by software)."),
};

UCLASS(config = Engine, defaultconfig, meta = (DisplayName = "NetImgui"), MinimalAPI)
class UNetImguiSettings : public UDeveloperSettings
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
	bool Show = true;

	//============================================================================
	// REMOTE DEAR IMGUI SETTINGS
	//============================================================================
	UPROPERTY(config, EditAnywhere, Category = "Remote", meta = (
		DisplayName = "Show Dear ImGui remotely",
		ToolTip = "Enable drawing Dear ImGui content on the NetImgui remote server.",
		ConfigRestartRequired = false))
	bool RemoteShow = true;

	UPROPERTY(config, EditAnywhere, Category = "Remote", meta = (
		DisplayName = "Hide Local Dear ImGui",
		ToolTip = "Hides Local Dear ImGui content when connected to the NetImgui remote server.",
		ConfigRestartRequired = false))
	bool RemoteHideLocal = true;

	UPROPERTY(config, EditAnywhere, Category = "Remote", meta = (
		DisplayName = "Remote Server Name -optional",
		ToolTip = "When a value is assigned, will try to connect to the NetImgui Server at this address",
		ConfigRestartRequired = false))
	FString RemoteServerName;

	UPROPERTY(config, EditAnywhere, Category = "Remote", meta = (
		DisplayName = "Remote Server Port -optional",
		ToolTip = "When a value is assigned, will try to connect to the NetImgui Server at this Name:Port",
		ClampMin = 1, ClampMax = 65535,
		ConfigRestartRequired = false))
	uint32 RemoteServerPort;

	UPROPERTY(config, EditAnywhere, Category = "Remote", meta = (
		DisplayName = "Listening Port",
		//ToolTip = "//SF TODO Finalize this... TCP/IP Port number used to listen for a connection from the NetImGui Server. A connection can either be established by waiting for the server to reach us on this port or the game can try reaching the Server directy.",
		ClampMin = 1, ClampMax = 65535,
		ConfigRestartRequired = false))
	uint32 RemoteClientPort;
	
	//============================================================================
	// LOCAL DEAR IMGUI SETTINGS
	//============================================================================
	UPROPERTY(config, EditAnywhere, Category = "Local", meta = (
		DisplayName = "Respect 'On Screen Debug' ShowFlag",
		ToolTip = "When this option is enabled, turning off this flag will also hide the Dear ImGui content",
		ConfigRestartRequired = false))
	bool LocalUseOnScreenDebugFlag = true;

	UPROPERTY(config, EditAnywhere, Category = "Local", meta = (
		DisplayName = "Game View input locked",
		ToolTip = "Exclusive input handling with enabled. Input can freely moved between Slate and Dear ImGui when disabled.",
		ConfigRestartRequired = false))
	bool LocalInputLockGame = true;

	UPROPERTY(config, EditAnywhere, Category = "Local", meta = (
		DisplayName = "Show Dear ImGui content in game view",
		ToolTip = "When the Dear ImGui content is visible in a game viewport.",
		ConfigRestartRequired = false))
	ENetImguiVisibility LocalVisibilityGame = ENetImguiVisibility::Always;

	UPROPERTY(config, EditAnywhere, Category = "Local", meta = (
		DisplayName = "Editor view input locked",
		ToolTip = "Exclusive input handling with enabled. Input can freely moved between Slate and Dear ImGui when disabled.",
		ConfigRestartRequired = false))
	bool LocalInputLockEditor = true;

	UPROPERTY(config, EditAnywhere, Category = "Local", meta = (
		DisplayName = "Show Dear ImGui content in editor view",
		ToolTip = "When the Dear ImGui content is visible in an editor viewport.",
		ConfigRestartRequired = false))
	ENetImguiVisibility LocalVisibilityEditor = ENetImguiVisibility::Always;

	UPROPERTY(config, EditAnywhere, Category="Local", meta = (
		DisplayName = "Dear ImGui Input Toggle Keys (Option 1)",
		ToolTip = "Keys needed to toggle the Input focus in the Game's Viewport and Editor's Viewport. All keys in the array must be pressed to toggle the activation (up to 3 keys combination can be configured).",
		ConfigRestartRequired = false))
	TArray<FKey> ToggleKeys1;

	UPROPERTY(config, EditAnywhere, Category="Local", meta = (
		DisplayName = "Dear ImGui Input Toggle Keys (Option 2)",
		ToolTip = "Keys needed to toggle the Input focus in the Game's Viewport and Editor's Viewport. All keys in the array must be pressed to toggle the activation (up to 3 keys combination can be configured).",
		ConfigRestartRequired = false))
	TArray<FKey> ToggleKeys2;

	UPROPERTY(config, EditAnywhere, Category="Local", meta = (
		DisplayName = "Dear ImGui Input Toggle Keys (Option 3)",
		ToolTip = "Keys needed to toggle the Input focus in the Game's Viewport and Editor's Viewport. All keys in the array must be pressed to toggle the activation (up to 3 keys combination can be configured).",
		ConfigRestartRequired = false))
	TArray<FKey> ToggleKeys3;

public:
	UNetImguiSettings();
};
