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
#endif

#define LOCTEXT_NAMESPACE "FNetImguiModule"
IMPLEMENT_MODULE(FNetImguiModule, NetImgui)

#if NETIMGUI_ENABLED
uint32_t GetClientPort()
{
	if (IsRunningDedicatedServer())
	{
		return NETIMGUI_LISTENPORT_DEDICATED_SERVER;
	}
	else if (FApp::IsGame())
	{
		return NETIMGUI_LISTENPORT_GAME;
	}
	else
	{
		return NETIMGUI_LISTENPORT_EDITOR;
	}
}
#endif

void FNetImguiModule::StartupModule()
{
#if NETIMGUI_ENABLED
	NetImgui::Startup();
		
	ImGui::SetCurrentContext(ImGui::CreateContext());
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
	// Note:	The default behaviour is for the Game Client to wait for connection from the NetImgui Server.
	//			It is possible to connect directly to the NetImgui Server insted, using 'NetImgui::ConnectToApp'
	FString sessionName = FString::Format(TEXT("{0}-{1}"), { FApp::GetProjectName(), FPlatformProcess::ComputerName() });
	NetImgui::ConnectFromApp(TCHAR_TO_ANSI(sessionName.GetCharArray().GetData()), GetClientPort());
	//---------------------------------------------------------------------------------------------

	FCoreDelegates::OnEndFrame.AddRaw(this, &FNetImguiModule::Update);
#endif
}

void FNetImguiModule::ShutdownModule()
{
#if NETIMGUI_ENABLED
	if (NetImgui::IsDrawing())
		NetImgui::EndFrame();
	NetImgui::Shutdown(true);

	ImGui::DestroyContext(ImGui::GetCurrentContext());
#endif
}

void FNetImguiModule::Update()
{
#if NETIMGUI_ENABLED
	if( NetImgui::IsDrawing() )
		NetImgui::EndFrame();

#if NETIMGUI_FRAMESKIP_ENABLED //Not interested in drawing menu until connection established
	if( NetImgui::IsConnected() )
#endif
	{
		NetImgui::NewFrame(NETIMGUI_FRAMESKIP_ENABLED);
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
