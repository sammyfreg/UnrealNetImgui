// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "NetImguiModule.h"
#include "CoreMinimal.h"

#if NETIMGUI_ENABLED

#include <Interfaces/IPluginManager.h>
#include "Misc/App.h"
#include "Misc/CoreDelegates.h"
#include "HAL/IConsoleManager.h"
#include "LocalDraw/NetImguiLocalDraw.h"
#include "NetImguiSettings.h"

#if NETIMGUI_FREETYPE_ENABLED
#include "misc/freetype/imgui_freetype.h"
#endif

//=================================================================================================
// Binary Font converted to c data array 
// (using Dear Imgui 'binary_to_compressed_c.cpp')
//=================================================================================================
#include "Fonts/Roboto_Medium.cpp"
#include "Fonts/Cousine_Regular.cpp"
#include "Fonts/Droid_Sans.cpp"
#include "Fonts/Karla_Regular.cpp"
#include "Fonts/Proggy_Tiny.cpp"

#if NETIMGUI_FONT_ICON_GAMEKENNEY
	#include "Fonts/FontKenney/KenneyIcon.cpp"
#endif

#if NETIMGUI_FONT_ICON_AWESOME	
	#include "Fonts/FontAwesome6/fa-solid-900.cpp"
	#include "Fonts/FontAwesome6/fa-regular-400.cpp"
	#include "Fonts/FontAwesome6/fa-brands-400.cpp"
#endif

#if NETIMGUI_FONT_ICON_MATERIALDESIGN
	#include "Fonts/FontMaterialDesign/MaterialIcons_Regular.cpp"
#endif

#if NETIMGUI_FONT_JAPANESE
	#include "Fonts/FontIPAexMincho/IPAexMincho.cpp"
#endif


//=================================================================================================
// Misc
//=================================================================================================
#include "Sample/NetImguiDemoNodeEditor.h"

#if IM_UNREAL_COMMAND_ENABLED
#include "ThirdParty/ImUnrealCommand/ImUnrealCommand.h"
static ImUnrealCommand::CommandContext* spImUnrealCommandContext = nullptr;
#endif

#pragma optimize("", off) //SF

//=================================================================================================
// FontCreationCallback
//-------------------------------------------------------------------------------------------------
// 1. Build the Font, 
// 2. Send result texture data to NetImgui remote server
// 3. Clear the local texture data, since it is un-needed (taking memory only on remote server)
//=================================================================================================
void FontCreationCallback(float PreviousDPIScale, float NewDPIScale)
{
	if (FNetImguiModule::UpdateFont(ImGui::GetIO().Fonts, PreviousDPIScale, NewDPIScale))
	{
		uint8_t* pPixelData(nullptr);
		int fontIndex = static_cast<int>(FNetImguiModule::eFont::_Default);
		int width(0), height(0);
		ImGui::GetIO().Fonts->GetTexDataAsAlpha8(&pPixelData, &width, &height);
		NetImgui::SendDataTexture(ImGui::GetIO().Fonts->TexID, pPixelData, static_cast<uint16_t>(width), static_cast<uint16_t>(height), NetImgui::eTexFormat::kTexFmtA8);
		ImGui::GetIO().Fonts->ClearTexData();	// Note: Free unneeded client texture memory. Various font size with japanese and icons can increase memory substancially(~64MB)
		ImGui::GetIO().FontDefault	= fontIndex < ImGui::GetIO().Fonts->Fonts.size() ? ImGui::GetIO().Fonts->Fonts[fontIndex] : nullptr;
	}
}

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
// "-NetImguiConnect"					Try connecting to NetImguiServer at 'localhost : (default port)'
// "-NetImguiConnect localhost"			Try connecting to NetImguiServer at 'localhost : (default port)'
// "-NetImguiConnect 192.168.1.2"		Try connecting to NetImguiServer at '192.168.1.2 : (default port)'
// "-NetImguiConnect 192.168.1.2:60"	Try connecting to NetImguiServer at '192.168.1.2 : 60'
//=================================================================================================
void TryConnectingToServer(const FString& HostnameAndPort)
{
	FString sessionName = FString::Format(TEXT("{0}-{1}"), { FApp::GetProjectName(), FPlatformProcess::ComputerName() });
	FString hostName = "localhost";
	int32_t hostPort = NETIMGUI_CONNECTPORT;

	if (!HostnameAndPort.IsEmpty()) 
	{
		int pos		= HostnameAndPort.Find(TEXT(":"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		hostName	= HostnameAndPort;
		if( pos > 0 ){
			FString portNumber = HostnameAndPort.Right(HostnameAndPort.Len()-pos-1);
			hostName.LeftInline(pos);
			hostPort = FCString::Atoi(*portNumber);
			hostPort = (hostPort == 0) ? NETIMGUI_CONNECTPORT : hostPort;	// Restore Port Number if integer conversion failed
		}
	}
	NetImgui::ConnectToApp(TCHAR_TO_ANSI(sessionName.GetCharArray().GetData()), TCHAR_TO_ANSI(*hostName), hostPort, nullptr, FontCreationCallback);
	
}

//=================================================================================================
// If the plugin was not able to reach the NetImguiServer (not requested, or bad address),
// starts waiting for a connection from the NetImguiServer instead.
// 
// Will start waiting for a connection on a default port id (based on engine type)
// (unless modified in NetImgui.Build.cs)
// NETIMGUI_LISTENPORT_GAME				= 8889
// NETIMGUI_LISTENPORT_EDITOR			= 8890
// NETIMGUI_LISTENPORT_DEDICATED_SERVER	= 8891
// 
// User can request a specific port by using commandline option "-netimguiport [Port Number]"
// 
//-------------------------------------------------------------------------------------------------
// COMMANDLINE EXAMPLES
// (empty)					Game will waits for connection on Default Port
// "-NetImguiListen"		Game will waits for connection on Default Port
// "-NetImguiListen 10000"	Game will waits for connection on Port '10000'
//=================================================================================================
void TryListeningForServer(const FString& ListeningPort)
{
	if( !NetImgui::IsConnectionPending() && !NetImgui::IsConnected() )
	{
		FString sessionName = FString::Format(TEXT("{0}-{1}"), { FApp::GetProjectName(), FPlatformProcess::ComputerName() });
		uint32_t listenPort	= FCString::Atoi(*ListeningPort);
		if( listenPort == 0 ){
			listenPort		=	IsRunningDedicatedServer()	? NETIMGUI_LISTENPORT_DEDICATED_SERVER :
								FApp::IsGame()				? NETIMGUI_LISTENPORT_GAME 
															: NETIMGUI_LISTENPORT_EDITOR;
		}
		NetImgui::ConnectFromApp(TCHAR_TO_ANSI(sessionName.GetCharArray().GetData()), listenPort, nullptr, FontCreationCallback);
	}
}

//=================================================================================================
// AddFontGroup
//-------------------------------------------------------------------------------------------------
// Add a new font entry and append some additional icons (when extraIconGlyphs=true) 
// that can be then be used with Dear ImGui. 
// 
// Your own glyph can be added to a font, either from an existing font file, or manually 
// generating them. More info can be found here :
//		https://github.com/ocornut/imgui/blob/master/docs/FONTS.md
//=================================================================================================
void AddFontGroup(FString name, ImFontAtlas* fontAtlas, float pxSize, float dpiScale, const uint32_t* pFontData, uint32_t FontDataSize, bool extraIconGlyphs, bool appendJapanese=false, const ImWchar* pGlyphRange=nullptr )
{
	ImFontConfig Config;
	dpiScale = FMath::Min(2.f, dpiScale); // Prevent increasing Font Atlas size too much

#if NETIMGUI_FREETYPE_ENABLED
	Config.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_LightHinting;	// Without this, kanji character looks wrong in smaller font size
#endif	
	float pxSizeScaled		= pxSize * dpiScale;
	name					+= FString::Printf(TEXT(" (%ipx)"), static_cast<int>(pxSize));
	FPlatformString::Strcpy(Config.Name, sizeof(Config.Name), TCHAR_TO_UTF8(name.GetCharArray().GetData()));
	fontAtlas->AddFontFromMemoryCompressedTTF(pFontData, FontDataSize, pxSizeScaled, &Config, pGlyphRange);
	
	Config.MergeMode = true;
#if NETIMGUI_FONT_JAPANESE
	if (appendJapanese) {
		Config.RasterizerMultiply = 1.5f;		// Boost kanji color intensity slightly, making them more readable
		fontAtlas->AddFontFromMemoryCompressedTTF(IPAexMincho_compressed_data,	IPAexMincho_compressed_size, pxSizeScaled, &Config, fontAtlas->GetGlyphRangesJapanese());
		Config.RasterizerMultiply = 1.f;
	}
#endif

	if( extraIconGlyphs ){
		Config.GlyphOffset.y = pxSize * 0.2f;	// Try aligning the icons a little more with the text, preventing icon to overlap previous text line

#if NETIMGUI_FONT_ICON_GAMEKENNEY
		static const ImWchar iconKenney_ranges[] = { ICON_MIN_KI, ICON_MAX_KI, 0 };
		fontAtlas->AddFontFromMemoryCompressedTTF(KenneyIcon_compressed_data, KenneyIcon_compressed_size, pxSizeScaled, &Config, iconKenney_ranges);
#endif
#if NETIMGUI_FONT_ICON_AWESOME
		static const ImWchar iconFontAwesome_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
		static const ImWchar iconFontAwesomeBrands_ranges[] = { ICON_MIN_FAB, ICON_MAX_FAB, 0 };
		fontAtlas->AddFontFromMemoryCompressedTTF(fa_solid_900_compressed_data, fa_solid_900_compressed_size, pxSizeScaled, &Config, iconFontAwesome_ranges);
		fontAtlas->AddFontFromMemoryCompressedTTF(fa_regular_400_compressed_data, fa_regular_400_compressed_size, pxSizeScaled, &Config, iconFontAwesome_ranges);
		fontAtlas->AddFontFromMemoryCompressedTTF(fa_brands_400_compressed_data, fa_brands_400_compressed_size, pxSizeScaled, &Config, iconFontAwesomeBrands_ranges);
#endif
#if NETIMGUI_FONT_ICON_MATERIALDESIGN
		static const ImWchar iconMaterialDesign_ranges[] = { ICON_MIN_MD, ICON_MAX_MD, 0 };
		fontAtlas->AddFontFromMemoryCompressedTTF(MaterialIcons_Regular_compressed_data, MaterialIcons_Regular_compressed_size, pxSizeScaled, &Config, iconMaterialDesign_ranges);
#endif
	}
}

static void CommandConnect(const TArray<FString>& Args)
{
	TryConnectingToServer(Args.Num() > 0 ? Args[0] : "");
}

static void CommandListen(const TArray<FString>& Args)
{
	TryListeningForServer(Args.Num() > 0 ? Args[0] : "");
}

static void CommandDisconnect(const TArray<FString>& Args)
{
	NetImgui::Disconnect();
}

static FAutoConsoleCommand GNetImguiConnectCmd
(
	TEXT("NetImguiConnect"),
	TEXT("Try connecting to the NetImgui Remoter server.\nNetImguiConnect [hostname/ip]:[Port]\n(Connect to localhost by default)"),
	FConsoleCommandWithArgsDelegate::CreateStatic(CommandConnect)
);

static FAutoConsoleCommand GNetImguiListenCmd
(
	TEXT("NetImguiListen"),
	TEXT("Start listening for a connection from the NetImgui Remote Server.\nNetImguiListen [Port]\n(Use default port when not specified)"),
	FConsoleCommandWithArgsDelegate::CreateStatic(CommandListen)
);

static FAutoConsoleCommand GNetImguiDisconnectCmd
(
	TEXT("NetImguiDisconnect"),
	TEXT("Stop any connection with the NetImgui Server and also stop listening for one."),
	FConsoleCommandWithArgsDelegate::CreateStatic(CommandDisconnect)
);


//=================================================================================================
// Update
//-------------------------------------------------------------------------------------------------
// Main update method of this plugin. 
//	1. Finish the previous Dear Imgui frame drawing
//	2. Start a new Dear ImGui drawing frame (when needed)
//	3. Call all listeners to draw their Dear Imgui content
// 
// Dear ImGui content can be drawn anywhere in your own code (on the gamethread) as long as 
// 'NetImguiHelper::IsDrawing()' is true. You can also add a listener to 'FNetImguiModule::OnDrawImgui'
// and draw inside the callback (without need to check for 'NetImguiHelper::IsDrawing()')
// 
// For examples on how to use UnrealNetImgui with Dear ImGui, take a look at :
//		'Plugins\NetImgui\Source\Sample\NetImguiDemoActor.cpp'
//=================================================================================================
void FNetImguiModule::Update()
{
	LocalDrawSupport->Update();
	if( NetImgui::IsDrawing() )
		NetImgui::EndFrame();

// Not interested in drawing Dear ImGui Content, until connection established
#if NETIMGUI_FRAMESKIP_ENABLED 
	if( NetImgui::IsConnected() )
#endif
	{
		NetImgui::NewFrame(NETIMGUI_FRAMESKIP_ENABLED);	
		LocalDrawSupport->InterceptRemoteInput();
		if (NetImgui::IsDrawingRemote())
		{
			//----------------------------------------------------------------------------
			// Show a simple information Window
			//----------------------------------------------------------------------------
			static bool sbShowHelp = false;
			if (ImGui::BeginMainMenuBar())
			{
				if( ImGui::BeginMenu("NetImgui") ){
					ImGui::MenuItem("Help", nullptr, &sbShowHelp);
					ImGui::EndMenu();
				}
				ImGui::EndMainMenuBar();
			}
			if (sbShowHelp) {
				static const ImVec4 kColorHighlight = ImVec4(0.1f, 0.85f, 0.1f, 1.0f);
				ImGui::SetNextWindowSize(ImVec2(600,400), ImGuiCond_FirstUseEver);
				if (ImGui::Begin("NetImgui: Help", &sbShowHelp)) {
					ImGui::TextColored(kColorHighlight, "Version :"); ImGui::SameLine();
					ImGui::TextUnformatted("NetImgui : " NETIMGUI_VERSION);
					
					ImGui::NewLine();
					ImGui::TextColored(kColorHighlight, "Low 'Dear ImGui' menus responsiveness when used with editor");
					ImGui::TextWrapped("Cause   : UE editor option to 'lower CPU cost when editor is unfocused' active");
					ImGui::TextWrapped("Solution: Disable this option. (Edit->Editor Preferences->General->Performance->Use Less CPU when in background)");

					ImGui::NewLine();
					ImGui::TextColored(kColorHighlight, "Usage example");
					ImGui::TextWrapped(	"Samples for using Dear ImGui with the NetImgui plugin, can be found in 'UnrealNetImgui/Source/Sample/NetImguiDemoActor.cpp'.  "
#if NETIMGUI_DEMO_ACTOR_ENABLED
										"The Actor can be dropped in your scene to show a demo window (NetImgui->Demo: DemoActor' to view it).  "
										"To add the actor : [Quickly add to the project] button(top of viewport, cube with '+' icon) then type 'Net Imgui Demo Actor' to find it.  "
#else
										"This Demo actor isn't enable in the build. To active it, set 'bDemoActor_Enabled' to true in 'NetImgui.Build.cs'"
#endif
										"Demo Actor source code is also is also a good demonstration of how to integrate the plugin to your project");
				}
				ImGui::End();
			}

			//----------------------------------------------------------------------------
			// Display a 'Unreal Console Command' menu entry in MainMenu bar, and the 
			// 'Unreal Console command' window itself when requested
			//----------------------------------------------------------------------------
		#if IM_UNREAL_COMMAND_ENABLED
			if (ImGui::BeginMainMenuBar()) {
				if( ImGui::BeginMenu("NetImgui") ){
					ImGui::MenuItem("Unreal-Commands", nullptr, &ImUnrealCommand::IsVisible(spImUnrealCommandContext) );
					ImGui::EndMenu();
				}
				ImGui::EndMainMenuBar();
			}

			// Always try displaying the 'Unreal Command Imgui' Window (handle Window visibility internally)
			ImUnrealCommand::Show(spImUnrealCommandContext);
		#endif

			//----------------------------------------------------------------------------
			// Display a 'Dear Imgui Demo' menu entry in MainMenu bar, and the 
			// demo window itself when requested
			//----------------------------------------------------------------------------
		#if NETIMGUI_DEMO_IMGUI_ENABLED
			static bool sbShowDemoImgui = false;
			if( sbShowDemoImgui ){
				ImGui::ShowDemoWindow(&sbShowDemoImgui);
			}
		#if NETIMGUI_IMPLOT_ENABLED
			static bool sbShowDemoImPlot = false;
			if( sbShowDemoImPlot ){
				ImPlot::ShowDemoWindow(&sbShowDemoImPlot);
			}
		#endif
		#if NETIMGUI_NODE_EDITOR_ENABLED
			static bool sbShowDemoNodeEditor = false;
			NodeEditorDemo::ShowDemo(sbShowDemoNodeEditor);
		#endif
			if (ImGui::BeginMainMenuBar())
			{
				if( ImGui::BeginMenu("NetImgui") ){
					ImGui::MenuItem("Demo: Dear Imgui", nullptr, &sbShowDemoImgui);
				#if NETIMGUI_IMPLOT_ENABLED
					ImGui::MenuItem("Demo: ImPlot", nullptr, &sbShowDemoImPlot);
				#endif
				#if NETIMGUI_NODE_EDITOR_ENABLED
					ImGui::MenuItem("Demo: Node Editor", nullptr, &sbShowDemoNodeEditor);
				#endif
					ImGui::EndMenu();
				}
				ImGui::EndMainMenuBar();
			}
		#endif	// NETIMGUI_DEMO_IMGUI_ENABLED

		
			//----------------------------------------------------------------------------
			// Ask all listener to draw their Dear ImGui content
			//----------------------------------------------------------------------------
			OnDrawImgui.Broadcast();
		}
	}
}

//=================================================================================================
// BuildFont
//-------------------------------------------------------------------------------------------------
// Regenerate the Font Atlas when not already created or the DPI scaled changed
//=================================================================================================
bool FNetImguiModule::UpdateFont(ImFontAtlas* fontAtlas, float fontDPIScalePrevious, float fontDPIScaleNeeded)
{
	if ( fontDPIScaleNeeded <= 0.f || FMath::Abs(fontDPIScalePrevious-fontDPIScaleNeeded) < 0.01f ){
		return false;
	}

	// Detect if the new DPI change the pixel size of any of our font
	bool needBuild = fontDPIScalePrevious <= 0.f || !fontAtlas->IsBuilt();
	for (int i(0); !needBuild && i < fontAtlas->Fonts.size(); ++i) {
		int pixelSizeNative	= static_cast<int>((fontAtlas->Fonts[i]->FontSize + 1.f) / fontDPIScalePrevious);
		int pixelSizeNeeded	= static_cast<int>(static_cast<float>(pixelSizeNative) * fontDPIScaleNeeded);
		needBuild			= pixelSizeNeeded != static_cast<int>(fontAtlas->Fonts[i]->FontSize);
	}

	// We need to generate the font, proceed with its creation/update
	if( needBuild )
	{
		fontAtlas->Flags			|= ImFontAtlasFlags_NoPowerOfTwoHeight;
		fontAtlas->TexDesiredWidth	= 8*1024;
		fontAtlas->Clear();

		//---------------------------------------------------------------------------------------------
		// Load our Font 
		// IMPORTANT: Must be added in same order as enum 'FNetImguiModule::eFont'
		//---------------------------------------------------------------------------------------------
		ImFontConfig fontConfig={};
		fontConfig.SizePixels = 13.f*fontDPIScaleNeeded;
		fontAtlas->AddFontDefault(&fontConfig);	// Proggy Clean

		AddFontGroup(TEXT("Cousine Fixed"),		fontAtlas, 16.f, fontDPIScaleNeeded, Cousine_Regular_compressed_data,	Cousine_Regular_compressed_size,	true, true);
		AddFontGroup(TEXT("Cousine Fixed"),		fontAtlas, 20.f, fontDPIScaleNeeded, Cousine_Regular_compressed_data,	Cousine_Regular_compressed_size,	true, true);
		AddFontGroup(TEXT("Cousine Fixed"),		fontAtlas, 24.f, fontDPIScaleNeeded, Cousine_Regular_compressed_data,	Cousine_Regular_compressed_size,	true, true);
		AddFontGroup(TEXT("Karla Regular"),		fontAtlas, 16.f, fontDPIScaleNeeded, Karla_Regular_compressed_data,		Karla_Regular_compressed_size,		true);
		AddFontGroup(TEXT("Droid Sans"),		fontAtlas, 20.f, fontDPIScaleNeeded, Droid_Sans_compressed_data,		Droid_Sans_compressed_size,			true);
		AddFontGroup(TEXT("Proggy Tiny"),		fontAtlas, 10.f, fontDPIScaleNeeded, Proggy_Tiny_compressed_data,		Proggy_Tiny_compressed_size,		false);
		AddFontGroup(TEXT("Roboto Medium"),		fontAtlas, 16.f, fontDPIScaleNeeded, Roboto_Medium_compressed_data,		Roboto_Medium_compressed_size,		true);
		AddFontGroup(TEXT("Icons"),				fontAtlas, 32.f, fontDPIScaleNeeded, Cousine_Regular_compressed_data,	Cousine_Regular_compressed_size,	true);
		AddFontGroup(TEXT("Icons"),				fontAtlas, 64.f, fontDPIScaleNeeded, Cousine_Regular_compressed_data,	Cousine_Regular_compressed_size,	true);
	#if NETIMGUI_FONT_JAPANESE
		AddFontGroup(TEXT("日本語"),				fontAtlas, 32.f, fontDPIScaleNeeded, IPAexMincho_compressed_data,		IPAexMincho_compressed_size,		true, false, fontAtlas->GetGlyphRangesJapanese());
	#endif
		// ... add extra fonts here (and add extra matching entries in 'FNetImguiModule::eFont' enum)
	

		//---------------------------------------------------------------------------------------------
		// 1. Build the Font, 
		// 2. Send result texture data to NetImgui remote server
		// 3. Clear the local texture data, since it is un-needed (taking memory only on remote server)
		//---------------------------------------------------------------------------------------------
		fontAtlas->Build();
		uint8_t* pPixelData(nullptr);
		int width(0), height(0);
		fontAtlas->GetTexDataAsAlpha8(&pPixelData, &width, &height);
		NetImgui::SendDataTexture(fontAtlas->TexID, pPixelData, static_cast<uint16_t>(width), static_cast<uint16_t>(height), NetImgui::eTexFormat::kTexFmtA8);
		fontAtlas->ClearTexData();									// Note: Free unneeded client texture memory. Various font size with japanese and icons can increase memory substancially(~64MB)
	}

	return needBuild;
}

//=================================================================================================
// SetDefaultFont
//-------------------------------------------------------------------------------------------------
// 
//=================================================================================================
void FNetImguiModule::SetDefaultFont(eFont font)
{
	check(font < eFont::_Count);
	if( ImGui::GetIO().Fonts->Fonts[static_cast<int>(font)] )
	{
		ImFont* pFont				= font < eFont::_Count ? ImGui::GetIO().Fonts->Fonts[static_cast<int>(font)] : nullptr;
		ImGui::GetIO().FontDefault	= pFont ? pFont : ImGui::GetIO().FontDefault;
	}
}

//=================================================================================================
// PushFont
//-------------------------------------------------------------------------------------------------
// 
//=================================================================================================
void FNetImguiModule::PushFont(eFont font)
{
	check(font < eFont::_Count);
	ImFont* pFont = font < eFont::_Count ? ImGui::GetIO().Fonts->Fonts[static_cast<int>(font)] : nullptr;
	ImGui::PushFont(pFont ? pFont : ImGui::GetFont());
}

//=================================================================================================
// PopFont
//-------------------------------------------------------------------------------------------------
// 
//=================================================================================================
void FNetImguiModule::PopFont()
{
	ImGui::PopFont();
}

//=================================================================================================
// isDrawing
//-------------------------------------------------------------------------------------------------
// 
//=================================================================================================
bool FNetImguiModule::isDrawing() const
{
	return NetImgui::IsDrawing();
}

//=================================================================================================
// IsConnected
//-------------------------------------------------------------------------------------------------
// 
//=================================================================================================
bool FNetImguiModule::IsConnected() const
{
	return NetImgui::IsConnected();
}

#endif // NETIMGUI_ENABLED

//=================================================================================================
// StartupModule
//-------------------------------------------------------------------------------------------------
// Initialize this module, the NetImgui client code, and Dear ImGui
// 
// Note:	By default, will wait for a connection from the NetImguiServer on the port associated
//			to the engine type (editor/game/server). You can change the listening port to your
//			desired value, by editing 'NetImgui.Build.cs', or changing the function
//			'TryListeningForServer()' or using a commandline parameter. You can also directly
//			try connecting to the server by using another commandline parameter. Please take a
//			look at 'TryConnectingToServer()' / 'TryListeningForServer()' for more informations.
//=================================================================================================
void FNetImguiModule::StartupModule()
{
#if NETIMGUI_ENABLED
	// Maps virtual shader source directory /Plugin/FX/Niagara to the plugin's actual Shaders directory.
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("Netimgui"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/UnrealNetimgui"), PluginShaderDir);
	NetImgui::Startup();
	mpContext					= ImGui::CreateContext();
	ImGuiIO& io					= ImGui::GetIO();
	io.ConfigFlags				|= ImGuiConfigFlags_DockingEnable;
	ImGui::SetCurrentContext(mpContext);

#if NETIMGUI_IMPLOT_ENABLED
	mpImPlotContext				= ImPlot::CreateContext();
#endif
	
	UpdateFont(io.Fonts, 0.f, 1.f);
	mFontDPIScale = 1.f;

	//---------------------------------------------------------------------------------------------
	// Setup connection to wait for netImgui server to reach us
	// Note:	The default behaviour is for the Game Client to wait for connection from the NetImgui Server
	//---------------------------------------------------------------------------------------------
	// Commandline request for a connectino to NetImguiServer
	FString hostNameAndPort, listenPort;
	if (FParse::Value(FCommandLine::Get(), TEXT("NetImguiConnect"), hostNameAndPort)){
		TryConnectingToServer(hostNameAndPort);
	}
	// If failed connecting, start listening for the NetImguiServer
	if (FParse::Value(FCommandLine::Get(), TEXT("NetImguiListen"), listenPort) || NETIMGUI_WAITCONNECTION_AUTO_ENABLED ){
		TryListeningForServer(listenPort);
	}
	
	//---------------------------------------------------------------------------------------------
	// Initialize the Unreal Console Command Widget
	//---------------------------------------------------------------------------------------------
#if IM_UNREAL_COMMAND_ENABLED
	spImUnrealCommandContext = ImUnrealCommand::Create(); // Create a new Imgui Command Window
	// Commented code demonstrating how to add/modify Presets
	// Could also modify the list of 'Default Presets' directly (UECommandImgui::sDefaultPresets)
	//UECommandImgui::AddPresetFilters(spUECommandContext, TEXT("ExamplePreset"), {"ai.Debug", "fx.Dump"});
	//UECommandImgui::AddPresetCommands(spUECommandContext, TEXT("ExamplePreset"), {"Stat Unit", "Stat Fps"});
#endif

	UpdateCallbackCB = FCoreDelegates::OnEndFrame.AddRaw(this, &FNetImguiModule::Update);
	LocalDrawSupport = MakeUnique<FNetImguiLocalDraw>();

#endif //NETIMGUI_ENABLED
}


//=================================================================================================
// Shutdown
//-------------------------------------------------------------------------------------------------
// Free up resources when module is unloaded
//=================================================================================================
void FNetImguiModule::ShutdownModule()
{
#if NETIMGUI_ENABLED
	FCoreDelegates::OnEndFrame.Remove(UpdateCallbackCB);
	LocalDrawSupport = nullptr;

	UpdateCallbackCB.Reset();
	if (NetImgui::IsDrawing())
		NetImgui::EndFrame();
	NetImgui::Shutdown();

#if NETIMGUI_IMPLOT_ENABLED
	ImPlot::DestroyContext(mpImPlotContext);
	mpImPlotContext = nullptr;
#endif

#if NETIMGUI_NODE_EDITOR_ENABLED
	NodeEditorDemo::Release();
#endif

	ImGui::DestroyContext(mpContext);
	mpContext = nullptr;

#if IM_UNREAL_COMMAND_ENABLED
	ImUnrealCommand::Destroy(spImUnrealCommandContext);
#endif

#endif //NETIMGUI_ENABLED
}

// Font display
#if 0
void FGbxImGuiDebuggerSamples::ShowDemoFont()
{
	ImGui::SetNextWindowPos(ImVec2(32, 48), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_Once);
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg));
	if (ImGui::Begin("Demo Font/Icons", &SampleFont_Show))
	{
		ShowDemoFont_Content();
	}
	ImGui::End();
	ImGui::PopStyleColor(2);
}

//-----------------------------------------------------------------------------
void FGbxImGuiDebuggerSamples::ShowDemoFont_Content()
{
	ImFontAtlas& FontAtlas = *ImGui::GetIO().Fonts;
	int fontSelected = static_cast<int>(SampleFont_Font);

	if (ImGui::BeginCombo("Font", FontAtlas.Fonts[fontSelected]->GetDebugName()))
	{
		for (int i = 0; i < FontAtlas.Fonts.size(); ++i)
		{
			ImGui::PushID((void*)FontAtlas.Fonts[i]);
			if (ImGui::Selectable(FontAtlas.Fonts[i]->GetDebugName(), FontAtlas.Fonts[i] == FontAtlas.Fonts[fontSelected]))
				fontSelected = i;
			ImGui::PopID();
		}
		ImGui::EndCombo();
		SampleFont_Font = static_cast<FImGuiModule::eFont>(fontSelected);
	}
	ImGui::NewLine();


	if (ImGui::BeginChild("client"))
	{
		if (ImGui::CollapsingHeader("Example text", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Filler");
			{
				ImGuiScopedFont selectedFont(SampleFont_Font);
				ImGui::TextWrapped("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
			}

			ImGui::NewLine();
			ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Icons inside text " ICON_KI_HEART);
			{
				ImGuiScopedFont selectedFont(SampleFont_Font);
				
				ImGui::TextWrapped("Mixing text and icons is Easy! Let's go crazy " ICON_KI_BUTTON_A " " ICON_KI_BUTTON_X " " ICON_KI_BUTTON_CIRCLE " " ICON_KI_BUTTON_TRIANGLE " " ICON_KI_BUTTON_R1 " " ICON_KI_BUTTON_L1 ". Just search for 'ShowDemoFont_Content()' in code to see how you can use them with a simple #Define string.");				
				ImGui::NewLine();

				const char* HeartAnimString[] = { ICON_KI_HEART, ICON_KI_HEART_HALF_O, ICON_KI_HEART_O, ICON_KI_HEART_HALF_O };
				const char* DPadAnimString[] = { ICON_KI_DPAD, ICON_KI_DPAD_TOP, ICON_KI_DPAD_RIGHT, ICON_KI_DPAD_BOTTOM, ICON_KI_DPAD_LEFT };
				uint32_t animIndex = (GFrameCounter / 32);
				ImGui::Text("With some animation: %s  %s", HeartAnimString[animIndex % UE_ARRAY_COUNT(HeartAnimString)], DPadAnimString[animIndex % UE_ARRAY_COUNT(DPadAnimString)]);
				ImGui::NewLine();

				ImGui::Text("Example: Some icon " ICON_FA_FACE_GRIN_WIDE " with text");
				ImGui::Text("Code   : ImGui::Text(\"Some icon \" ICON_FA_FACE_GRIN_WIDE \" with text\");");
				ImGui::NewLine();
				
				char website1[] = { "https://kenney.nl/assets/game-icons" };
				char website2[] = { "https://kenney.nl/assets/game-icons-expansion" };
				char website3[] = { "https://fontawesome.com/v6/search?m=free" };
				ImGui::TextWrapped("Look at 'IconsKenney.h' and 'IconsFontAwesome6.h' for a list of icons #defines and these 3 websites for more informations");
				ImGui::InputText("##website1", website1, sizeof(website1), ImGuiInputTextFlags_ReadOnly);
				ImGui::InputText("##website2", website2, sizeof(website2), ImGuiInputTextFlags_ReadOnly);
				ImGui::InputText("##website3", website3, sizeof(website3), ImGuiInputTextFlags_ReadOnly);
			}
			ImGui::NewLine();
		}
		constexpr int ICON_START_FA = 0x2013; // Skip first few font awesome characters that are regular symbols
		ShowDemoFont_Table("Characters Table : Standard", 0, 256);
		
		ImGui::NewLine();
		ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "These 2 Icons series are available");
		ShowDemoFont_Table("Characters Table : Kenny Game Icons", ICON_MIN_KI, ICON_MAX_KI);
		ShowDemoFont_Table("Characters Table : Font Awesome 6 Icons", ICON_START_FA, ICON_MAX_FA, ICON_MIN_KI, ICON_MAX_KI);
	}
	ImGui::EndChild();
}

//-----------------------------------------------------------------------------
void FGbxImGuiDebuggerSamples::ShowDemoFont_Table(const char* IconsName, uint32_t IconUnicodeFirst, uint32_t IconUnicodeLast, uint32_t ExcludeUnicodeFirst, uint32_t ExcludeUnicodeLast)
{
	const ImFont& font = *ImGui::GetFont();
	if (ImGui::CollapsingHeader(IconsName))
	{
		ImGuiScopedFont selectedFont(SampleFont_Font);
		float iconHeight = ImGui::GetFontSize() * 1.4f;
		int colCount = static_cast<int>(ImGui::GetContentRegionAvail().x / iconHeight);
		if (ImGui::BeginTable("Font Table", colCount, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_Borders | ImGuiTableFlags_NoKeepColumnsVisible))
		{
			WIDECHAR unicodeString[2] = {};
			for (uint32_t unitcode(IconUnicodeFirst); unitcode <= IconUnicodeLast; ++unitcode)
			{
				// Font Kenney character range is inside Font Awesome. 
				// Avoids displaying them 
				if (unitcode == ExcludeUnicodeFirst)
				{
					unitcode = ExcludeUnicodeLast;
					continue;
				}
				const ImFontGlyph* glyph = font.FindGlyph(static_cast<ImWchar>(unitcode));
				if (glyph && glyph != font.FallbackGlyph)
				{
					unicodeString[0]		= unitcode;
					auto strConvert			= StringCast<UTF8CHAR>(unicodeString);
					const char* utf8String	= (const char*)strConvert.Get();
					ImGui::TableNextColumn();
					ImGui::Selectable(utf8String, ImGui::IsItemHovered());
					if (ImGui::IsItemHovered())
					{
						unsigned int utf8Digits[4] = { static_cast<unsigned char>(utf8String[0]), static_cast<unsigned char>(utf8String[1]), static_cast<unsigned char>(utf8String[2]), static_cast<unsigned char>(utf8String[3]) };
						ImGuiScopedFont tooltipFont(FImGuiModule::eFont::kCousineRegular_16);
						ImGui::SetTooltip(	utf8String[1] == 0	? "Unicode:0x%04X -=- UTF8:0x[%02X]" :
											utf8String[2] == 0	? "Unicode:0x%04X -=- UTF8:0x[%02X][%02X" :
											utf8String[3] == 0	? "Unicode:0x%04X -=- UTF8:0x[%02X][%02X][%02X]" :
											utf8String[4] == 0	? "Unicode:0x%04X -=- UTF8:0x[%02X][%02X][%02X][%02X]"
																: "Unicode:0x%04X -=- UTF8:Invalid", unitcode, utf8Digits[0], utf8Digits[1], utf8Digits[2], utf8Digits[3]);
					}
				}
			}
			ImGui::EndTable();
		}
	}
}
#endif
#define LOCTEXT_NAMESPACE "FNetImguiModule"
IMPLEMENT_MODULE(FNetImguiModule, NetImgui)
//TCustomShowFlag<> ShowNetImgui(TEXT("NetImgui"), true /*DefaultEnabled*/, SFG_Developer, LOCTEXT("ShowNetImgui", "NetImgui enabled"));
//TCustomShowFlag<> ShowNetImguiLocal(TEXT("NetImguiLocal"), true /*DefaultEnabled*/, SFG_Developer, LOCTEXT("ShowNetImguiLocal", "Show Local NetImgui"));
//TCustomShowFlag<> ShowNetImguiRemote(TEXT("NetImguiRemote"), true /*DefaultEnabled*/, SFG_Developer, LOCTEXT("ShowNetImguiRemote", "Show Remote NetImgui"));

#undef LOCTEXT_NAMESPACE

#pragma optimize("", on) //SF
