// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "NetImguiModule.h"
#include "CoreMinimal.h"
#include <Interfaces/IPluginManager.h>
#include "Misc/App.h"
#include "Misc/CoreDelegates.h"

// Binary Font converted to c data array (using Dear Imgui 'binary_to_compressed_c.cpp')
#include "Fonts/Roboto_Medium.cpp"
#include "Fonts/Cousine_Regular.cpp"
#include "Fonts/Droid_Sans.cpp"
#include "Fonts/Karla_Regular.cpp"
#include "Fonts/Proggy_Tiny.cpp"

#if NETIMGUI_ENABLED
#include "ThirdParty/NetImgui/NetImgui_Api.h"
//----------------------------------------------------------------------------
#include "ImguiUnrealCommand.h"
#if IMGUI_UNREAL_COMMAND_ENABLED
static UECommandImgui::CommandContext* spUECommandContext = nullptr;
#endif
//----------------------------------------------------------------------------
#endif

#define LOCTEXT_NAMESPACE "FNetImguiModule"
IMPLEMENT_MODULE(FNetImguiModule, NetImgui)

// define the symbol for Clang compiler
constexpr char FNetImguiModule::kModuleName[];

#if NETIMGUI_ENABLED

//=================================================================================================
// If engine was launched with "-netimguiserver [hostname]", try connecting directly 
// to the NetImguiServer application at the provided address.
// 
// [hostname] can be an ip address, a window pc hostname, etc...
// 
// You can also change the default port by appending ":[port number]"
// The default NetImguiServer port is 8888 (unless modified in NetImgui.Build.cs)
// 
//-------------------------------------------------------------------------------------------------
// COMMANDLINE EXAMPLES
// "-netimguiserver localhost"		Try connecting to NetImguiServer at 'localhost : 8888'
// "-netimguiserver 192.168.1.2"	Try connecting to NetImguiServer at '192.168.1.2 : 8888'
// "-netimguiserver 192.168.1.2:60"	Try connecting to NetImguiServer at '192.168.1.2 : 60'
//=================================================================================================
void TryConnectingToServer(const FString& sessionName)
{	
	FString hostname;
	if (FParse::Value(FCommandLine::Get(), TEXT("netimguiserver"), hostname))
	{
		int32_t customPort = NETIMGUI_CONNECTPORT;
		if (hostname.IsEmpty()) {
			hostname = "localhost";
		}
		else {
			int pos = hostname.Find(TEXT(":"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
			if( pos > 0 ){
				FString portNumber = hostname.Right(hostname.Len()-pos-1);
				hostname.LeftInline(pos);
				customPort = FCString::Atoi(portNumber.GetCharArray().GetData());
				customPort = (customPort == 0) ? NETIMGUI_CONNECTPORT : customPort;	// Restore Port Number if integer conversion failed
			}
		}
		NetImgui::ConnectToApp(TCHAR_TO_ANSI(sessionName.GetCharArray().GetData()), TCHAR_TO_ANSI(hostname.GetCharArray().GetData()), customPort);
	}
}

//=================================================================================================
// If the plugin was not able to reach the NetImguiServer (not requested, or bad address),
// starts listening for the NetImguiServer to try connecting to it instead.
// 
// Will start waiting for a connection on this port by default (based on engine type)
// 
// Here are the default values (unless modified in NetImgui.Build.cs)
// NETIMGUI_LISTENPORT_GAME				= 8889
// NETIMGUI_LISTENPORT_EDITOR			= 8890
// NETIMGUI_LISTENPORT_DEDICATED_SERVER	= 8891
// 
// User can request a specidic listening port by using commandline option "-netimguiport [Port Number]"
// 
//-------------------------------------------------------------------------------------------------
// COMMANDLINE EXAMPLES
// (empty)					Game will waits for connection on Default Port
// "-netimguiport 10000"	Game will waits for connection on Port '10000'
//=================================================================================================
void TryListeningForServer(const FString& sessionName)
{
	if( !NetImgui::IsConnectionPending() && !NetImgui::IsConnected() )
	{
		uint32_t customPort = 0;
		if ( !FParse::Value(FCommandLine::Get(), TEXT("netimguiport"), customPort) )
		{
			customPort =	IsRunningDedicatedServer()	? NETIMGUI_LISTENPORT_DEDICATED_SERVER :
							FApp::IsGame()				? NETIMGUI_LISTENPORT_GAME :
														  NETIMGUI_LISTENPORT_EDITOR;
		}
		NetImgui::ConnectFromApp(TCHAR_TO_ANSI(sessionName.GetCharArray().GetData()), customPort);
	}
}

#endif

void FNetImguiModule::StartupModule()
{
#if NETIMGUI_ENABLED
	NetImgui::Startup();
	mpContext = ImGui::CreateContext();
	ImGui::SetCurrentContext(mpContext);
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	//---------------------------------------------------------------------------------------------
	// Load our Font (Must be loaded in same order as FNetImguiModule::eFont enum)
	ImFontConfig Config;
	io.Fonts->AddFontDefault();
	FPlatformString::Strcpy(Config.Name, sizeof(Config.Name), "Roboto Medium, 16px");
    io.Fonts->AddFontFromMemoryCompressedTTF(Roboto_Medium_compressed_data,		Roboto_Medium_compressed_size,		16.0f, &Config);
	FPlatformString::Strcpy(Config.Name, sizeof(Config.Name), "Cousine Regular, 15px");
    io.Fonts->AddFontFromMemoryCompressedTTF(Cousine_Regular_compressed_data,	Cousine_Regular_compressed_size,	15.0f, &Config);
	FPlatformString::Strcpy(Config.Name, sizeof(Config.Name), "Karla Regular, 16px");
	io.Fonts->AddFontFromMemoryCompressedTTF(Karla_Regular_compressed_data,		Karla_Regular_compressed_size,		16.0f, &Config);
	FPlatformString::Strcpy(Config.Name, sizeof(Config.Name), "Droid Sans, 16px");
    io.Fonts->AddFontFromMemoryCompressedTTF(Droid_Sans_compressed_data,		Droid_Sans_compressed_size,			16.0f, &Config);
	FPlatformString::Strcpy(Config.Name, sizeof(Config.Name), "Proggy Tiny, 10px");
	io.Fonts->AddFontFromMemoryCompressedTTF(Proggy_Tiny_compressed_data,		Proggy_Tiny_compressed_size,		10.0f, &Config);

	// ... add extra fonts here (and add extra entry in 'FNetImguiModule::eFont' enum)

	io.Fonts->Build();
	//---------------------------------------------------------------------------------------------


	//---------------------------------------------------------------------------------------------
	// Setup connection to wait for netImgui server to reach us
	// Note:	The default behaviour is for the Game Client to wait for connection from the NetImgui Server
	FString sessionName = FString::Format(TEXT("{0}-{1}"), { FApp::GetProjectName(), FPlatformProcess::ComputerName() });
	TryConnectingToServer(sessionName);	// Try connecting to NetImguiServer
	TryListeningForServer(sessionName);	// If failed connecting, start listening for the NetImguiServer
	//---------------------------------------------------------------------------------------------

	mUpdateCallback		= FCoreDelegates::OnEndFrame.AddRaw(this, &FNetImguiModule::Update);

//----------------------------------------------------------------------------
#if IMGUI_UNREAL_COMMAND_ENABLED
	
	spUECommandContext	= UECommandImgui::Create(); // Create a new Imgui Command Window
	// Commented code demonstrating how to add/modify Presets
	// Could also modify the list of 'Default Presets' directly (UECommandImgui::sDefaultPresets)
	//UECommandImgui::AddPresetFilters(spUECommandContext, TEXT("ExamplePreset"), {"ai.Debug", "fx.Dump"});
	//UECommandImgui::AddPresetCommands(spUECommandContext, TEXT("ExamplePreset"), {"Stat Unit", "Stat Fps"});
#endif
//----------------------------------------------------------------------------
#endif
}

void FNetImguiModule::ShutdownModule()
{
#if NETIMGUI_ENABLED
	FCoreDelegates::OnEndFrame.Remove(mUpdateCallback);
	mUpdateCallback.Reset();
	if (NetImgui::IsDrawing())
		NetImgui::EndFrame();
	NetImgui::Shutdown();

	ImGui::DestroyContext(mpContext);
	mpContext = nullptr;
//----------------------------------------------------------------------------
#if IMGUI_UNREAL_COMMAND_ENABLED
	UECommandImgui::Destroy(spUECommandContext);
#endif
//----------------------------------------------------------------------------
#endif
}

void FNetImguiModule::Update()
{
#if NETIMGUI_ENABLED
	if( NetImgui::IsDrawing() )
		NetImgui::EndFrame();

#if NETIMGUI_FRAMESKIP_ENABLED //Not interested in drawing Dear ImGui Content, until connection established
	if( NetImgui::IsConnected() )
#endif
	{
		NetImgui::NewFrame(NETIMGUI_FRAMESKIP_ENABLED);
		if (NetImgui::IsDrawingRemote()) {
	//----------------------------------------------------------------------------
	#if IMGUI_UNREAL_COMMAND_ENABLED
			// Add Main Menu entry to toggle Unreal Command Window visibility
			if (ImGui::BeginMainMenuBar()) {
				ImGui::MenuItem("Unreal-Commands", nullptr, &UECommandImgui::IsVisible(spUECommandContext) );
				ImGui::EndMainMenuBar();
			}

			// Always try displaying the 'Unreal Command Imgui' Window (handle Window visibily internally)
			UECommandImgui::Show(spUECommandContext);
	#endif
	//----------------------------------------------------------------------------
		}
	
	}
#endif
}

void FNetImguiModule::setDefaultFont(eFont font)
{
#if NETIMGUI_ENABLED
	check(font < eFont::_Count);
	ImFont* pFont				= font < eFont::_Count ? ImGui::GetIO().Fonts->Fonts[static_cast<int>(font)] : nullptr;
	ImGui::GetIO().FontDefault = pFont ? pFont : ImGui::GetIO().FontDefault;
#endif
}

void FNetImguiModule::pushFont(eFont font)
{
#if NETIMGUI_ENABLED
	check(font < eFont::_Count);
	ImFont* pFont = font < eFont::_Count ? ImGui::GetIO().Fonts->Fonts[static_cast<int>(font)] : nullptr;
	ImGui::PushFont(pFont ? pFont : ImGui::GetFont());
#endif
}

void FNetImguiModule::popFont()
{
#if NETIMGUI_ENABLED
	ImGui::PopFont();
#endif
}

bool FNetImguiModule::isDrawing()
{
#if NETIMGUI_ENABLED
	return NetImgui::IsDrawing();
#else
	return false;
#endif
}

#undef LOCTEXT_NAMESPACE
