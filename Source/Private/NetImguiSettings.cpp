// Copyright Epic Games, Inc. All Rights Reserved.

#include "NetImguiSettings.h"
#include "UObject/UnrealType.h"

//-----------------------------------------------------------------------------
// Global
//-----------------------------------------------------------------------------
// Show
TAutoConsoleVariable<int32> CVarNetImguiShow(
	TEXT("netimgui.show"),
	1,
	TEXT("0 - Disabled ")
	TEXT("1 - Enabled (Default)"));

#if 0
//-----------------------------------------------------------------------------
// Remote Connection settings
//-----------------------------------------------------------------------------
// Remote Show
TAutoConsoleVariable<uint32> CVarNetImguiRemoteShow(
	TEXT("netimgui.remote.show"),
	1,
	TEXT("0 - Disabled")
	TEXT("1 - Enabled (Default)"));

// Remote ServerName
TAutoConsoleVariable<FString> CVarNetImguiRemoteServerName(
	TEXT("netimgui.remote.servername"),
	"",
	TEXT(""));

// Remote ServerPort
TAutoConsoleVariable<uint32> CVarNetImguiRemoteServerPort(
	TEXT("netimgui.remote.serverport"),
	"",
	TEXT(""));

// Remote ClientPort
TAutoConsoleVariable<uint32> CVarNetImguiRemoteClientPort(
	TEXT("netimgui.remote.clientport"),
	"",
	TEXT(""));

// Remote HideLocal
TAutoConsoleVariable<uint32> CVarNetImguiRemoteHideLocal(
	TEXT("netimgui.remote.hidelocal"),
	1,
	TEXT("0 - Disabled")
	TEXT("1 - Enabled (Default)"));

//-----------------------------------------------------------------------------
// Local Drawing settings
//-----------------------------------------------------------------------------
// Local ShowGame
TAutoConsoleVariable<uint32> CVarNetImguiLocalShowGame(
	TEXT("netimgui.local.showgame"),
	(int32)ENetImguiGameVisibility::ActiveInput,
	TEXT("0 - Disabled")
	TEXT("1 - Always")
	TEXT("2 - When input is enabled (Default)"));

// Local ShowEditor
TAutoConsoleVariable<uint32> CVarNetImguiLocalShowEditor(
	TEXT("netimgui.local.showeditor"),
	(int32)ENetImguiEditorVisibility::ViewFocused,
	TEXT("0 - Disabled")
	TEXT("1 - Always")
	TEXT("2 - When viewport has focus (Default)"));
#endif

UNetImguiSettings::UNetImguiSettings()
{
	Show						= true;

	RemoteShow					= true;
	RemoteHideLocal				= false;
	RemoteServerName			= TEXT("");
	RemoteServerPort			= NETIMGUI_CONNECTPORT;
	RemoteClientPort			= NETIMGUI_LISTENPORT_GAME;
	
	LocalUseOnScreenDebugFlag	= true;
	LocalVisibilityGame			= ENetImguiVisibility::Always;
	LocalVisibilityEditor		= ENetImguiVisibility::Always;
}

void UNetImguiSettings::PostInitProperties()
{
	Super::PostInitProperties();
#if WITH_EDITOR
	if (IsTemplate())
	{
		ImportConsoleVariableValues();
	}
#endif
}

FName UNetImguiSettings::GetCategoryName() const
{
	return FName(TEXT("Plugins"));
}

#if WITH_EDITOR
void UNetImguiSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (PropertyChangedEvent.Property)
	{
		ExportValuesToConsoleVariables(PropertyChangedEvent.Property);
	}
}
#endif
