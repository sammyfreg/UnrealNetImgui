// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "NetImguiModule.h"

#if defined(NETIMGUI_ENABLED) && NETIMGUI_ENABLED

#if PLATFORM_XBOXONE
	// Disable Win32 functions used in ImGui and not supported on XBox.
	#define IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS
	#define IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS
#endif

//=================================================================================================
// For convenience and easy access to the Dear ImGui source code, we build it as part of this module.
//=================================================================================================

#if PLATFORM_WINDOWS
#include <Windows/AllowWindowsPlatformTypes.h>
#endif

#include <imgui.cpp>
#include <imgui_demo.cpp>
#include <imgui_draw.cpp>
#include <imgui_widgets.cpp>
#include <imgui_tables.cpp>

#if PLATFORM_WINDOWS
#include <Windows/HideWindowsPlatformTypes.h>
#endif

#endif
