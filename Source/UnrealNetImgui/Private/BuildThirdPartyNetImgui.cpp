// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "NetImguiModule.h"

//=================================================================================================
// For convenience and easy access to the netImgui source code, we build it as part of this module.
//=================================================================================================

#if defined(NETIMGUI_ENABLED) && NETIMGUI_ENABLED

#include <Private/NetImgui_Api.cpp>
#include <Private/NetImgui_Client.cpp>
#include <Private/NetImgui_CmdPackets_DrawFrame.cpp>
#include <Private/NetImgui_NetworkUE4.cpp>

#endif

