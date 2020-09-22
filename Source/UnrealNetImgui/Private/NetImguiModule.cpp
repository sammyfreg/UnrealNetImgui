// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "NetImguiModule.h"
#include "CoreMinimal.h"
#include <Interfaces/IPluginManager.h>

#define LOCTEXT_NAMESPACE "FNetImguiModule"
IMPLEMENT_MODULE(FNetImguiModule, NetImgui)

void FNetImguiModule::StartupModule()
{
#if NETIMGUI_ENABLED
	NetImgui::Startup();
	
	ImGui::SetCurrentContext(ImGui::CreateContext());
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontDefault();
	io.Fonts->Build();

	unsigned char* Pixels;
	int Width, Height, Bpp;
	io.Fonts->GetTexDataAsAlpha8(&Pixels, &Width, &Height, &Bpp);
	NetImgui::SendDataTexture(0, Pixels, Width, Height, NetImgui::eTexFormat::kTexFmtA8);

	// Setup connection to wait for netImgui server to reach us
	FString sessionName = FString::Format(TEXT("{0}-{1}"), { FApp::GetProjectName(), FPlatformProcess::ComputerName() });
	NetImgui::ConnectFromApp(TCHAR_TO_ANSI(sessionName.GetCharArray().GetData()), FApp::IsGame() ? NETIMGUI_LISTENPORT_GAME : NETIMGUI_LISTENPORT_EDITOR, false);

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

	NetImgui::NewFrame();	
#endif
}
#undef LOCTEXT_NAMESPACE



