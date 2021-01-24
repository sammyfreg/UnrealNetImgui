// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

using System.Collections.Generic;
using System.IO;
using UnrealBuildTool;

//=================================================================================================
// netImgui Plugin Build Setup
//-------------------------------------------------------------------------------------------------
// Plugin exposing Dear ImGui library for drawing 2D menus. These menus are displayed and 
// controlled from an external application but processed from this engine code. 
//
// Works on various platform supported by UE4
//
// Note:	Displaying Dear ImGui menus directly in this engine, can be done by using the
//			plugin UnrealImgui instead (https://github.com/segross/UnrealImGui). It was designed 
//			local display of Imgui content but also support netImgui for remote display.
//-------------------------------------------------------------------------------------------------
//
// Dear ImGui Library	: v1.80.0	(https://github.com/ocornut/imgui)
// NetImGui Library		: v1.3.0	(https://github.com/sammyfreg/netImgui)
//=================================================================================================

public class NetImgui : ModuleRules
{
#if WITH_FORWARDED_MODULE_RULES_CTOR
	public NetImgui(ReadOnlyTargetRules Target) : base(Target)
#else
	public NetImgui(TargetInfo Target)
#endif
	{
#if WITH_FORWARDED_MODULE_RULES_CTOR
		bool bBuildEditor = Target.bBuildEditor;
#else
		bool bBuildEditor = (Target.Type == TargetRules.TargetType.Editor);
#endif
#if !UE_4_19_OR_LATER
		List<string> PrivateDefinitions = Definitions;
		List<string> PublicDefinitions = Definitions;
#endif

		//---------------------------------------------------------------------
		// Settings configuration 
		//---------------------------------------------------------------------
		// Toggle NetImgui support here
		bool bUseNetImgui			= true;

		// When true, only redraw Dear ImGui when needed, saving processing. 
		// When true, must use "NetImgui::IsDrawing()" before emiting ImGui draws
		bool bSupportFrameSkip		= true;

		// Com Port used by Game exe to wait for a connection from netImgui Server (8889 by default)
		// NetImgui Server will try to find running game client on this port and connect to them
		// Note:	Server will find client running on same PC, on this port
		//			To find client on remote connection (running on console, smartphone, other PC, ...)
		//			you will need to add their IP in the Server Client list.
		//			Alternatively, you can modify the connection code in 'FNetImguiModule::StartupModule()'
		//			to let the client connect directly to NetImGui server using 'NetImgui::ConnectToApp(ServerIP)'
		string kGameListenPort		= "(NetImgui::kDefaultClientPort)";

		// Com Port used by Editor exe to wait for a connection from netImgui Server (8890 by default)
		// NetImgui Server will try to find running editor client on this port and connect to them
		string kEditorListenPort	= "(NetImgui::kDefaultClientPort+1)";
		//---------------------------------------------------------------------

		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "../ThirdParty/ImGuiLib/Source"));
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "../ThirdParty/NetImguiLib/Source"));

		PublicDependencyModuleNames.AddRange( new string[] { "Core", "Projects"} );
		PrivateDependencyModuleNames.AddRange( new string[] { "CoreUObject", "Engine", "Sockets" } );

		// Developer modules are automatically loaded only in editor builds but can be stripped out from other builds.
		// Enable runtime loader, if you want this module to be automatically loaded in runtime builds (monolithic).
		//bool bEnableRuntimeLoader = true;
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		//bUseUnity = true; // Disable to test build

#if UE_4_24_OR_LATER
		bLegacyPublicIncludePaths = false;
		ShadowVariableWarningLevel = WarningLevel.Error;
		bTreatAsEngineModule = true;
#endif

		//---------------------------------------------------------------------
		// Setup Environment to build with/without netImgui
		//---------------------------------------------------------------------
		PublicDefinitions.Add(string.Format("NETIMGUI_ENABLED={0}", bUseNetImgui ? 1 : 0));
		PublicDefinitions.Add(string.Format("NETIMGUI_USE_FRAMESKIP={0}", bSupportFrameSkip ? 1 : 0));
		PublicDefinitions.Add("IMGUI_API=DLLEXPORT");
		PublicDefinitions.Add("NETIMGUI_LISTENPORT_GAME=" + kGameListenPort);
		PublicDefinitions.Add("NETIMGUI_LISTENPORT_EDITOR=" + kEditorListenPort);
		PrivateDefinitions.Add("NETIMGUI_WINSOCKET_ENABLED=0");      // Using Unreal sockets, no need for built-in sockets
		PrivateDefinitions.Add("NETIMGUI_POSIX_SOCKETS_ENABLED=0");  // Using Unreal sockets, no need for built-in sockets
	}
}
