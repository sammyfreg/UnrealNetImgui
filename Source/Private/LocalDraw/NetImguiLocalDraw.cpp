// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "NetImguiModule.h"
#include "CoreMinimal.h"

#if NETIMGUI_LOCALDRAW_ENABLED || 1//SF

#include "Engine/Engine.h"
#include "NetImguiLocalDraw.h"
#include "SceneView.h"

#include "SceneInterface.h"
#include "SystemSettings.h"
#include "Engine/GameViewportClient.h"
#include "Slate/SceneViewport.h"
#include "Framework/Application/SlateApplication.h" //SF
#include "Framework/Application/SlateUser.h" //SF
#include "ThirdParty/NetImgui/NetImgui_Api.h"

#if WITH_EDITOR
#include "Slate/SceneViewport.h"
#include "LevelEditorViewport.h"
#include "SLevelViewport.h"
#include "LevelEditor.h"
#endif

//=================================================================================================
// NetImgui to Unreal Keys mapping
//=================================================================================================
TMap<FKey, ImGuiKey> GUnrealKeyToImguiMap;

//=================================================================================================
// NETIMGUI INPUT PROCESSOR
//-------------------------------------------------------------------------------------------------
// Intercept a few inputs before Slate, to shift control to NetImgui when needed
// Main key forwarding to ImGui is done by the SNetImguiWidget itself, but this is needed
// to intercept the NetImgui Activation keys/controls combination
//=================================================================================================
class FNetImguiInputProcessor : public IInputProcessor
{
public:
							FNetImguiInputProcessor(FNetImguiLocalDraw* inOwner):LocalDrawOwner(inOwner){};
	virtual const TCHAR*	GetDebugName() const { return TEXT("FNetImguiInputProcessor"); }
	virtual bool			HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		// Update our list of currently pressed keys
		static const UNetImguiSettings* NetImguiSettings	= GetDefault<UNetImguiSettings>();
		static const TArray<FKey>* ToggleKeysConfigs[]		= {&NetImguiSettings->ToggleKeys1, &NetImguiSettings->ToggleKeys2, &NetImguiSettings->ToggleKeys3};
		if( !InKeyEvent.IsRepeat() )
		{
			
			static_assert(UE_ARRAY_COUNT(ToggleKeysConfigs) == UE_ARRAY_COUNT(KeydownMask));
			for(int ConfigIndex(0); ConfigIndex < UE_ARRAY_COUNT(ToggleKeysConfigs); ++ConfigIndex)
			{
				const TArray<FKey>* ToggleKeys = ToggleKeysConfigs[ConfigIndex];
				if( ToggleKeys && ToggleKeys->Num() > 0 )
				{
					// Save the keypress into a mask, when it fit one of our wanted input
					for(int i(0); i< FMath::Min(32, (*ToggleKeys).Num()); ++i)
					{
						if( InKeyEvent.GetKey() == (*ToggleKeys)[i] )
						{
							KeydownMask[ConfigIndex] |= 1 << i;
							KeydownFrame = 0;
						}
					}
					// Check if we have all required input to toggle Imgui
					bool IsNetImguiToggle = (KeydownMask[ConfigIndex] == (1<<(*ToggleKeys).Num())-1);
					if( IsNetImguiToggle )
					{
						LocalDrawOwner->ToggleActiveWidgetInput();
						return true;
					}
				}
			}
		}
		return false;
	}

	// Reset Keydown mask after X frames without keypress,
	// since we do not reliably receive all KeyUp events to unset our mask 
	// (could be intercepted by another input pre processor)
	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor)
	{
		KeydownFrame++;
		if ( KeydownFrame == 30 )
		{
			KeydownMask[0] = KeydownMask[1] = KeydownMask[2] = 0;
		}
	}
protected:
	FNetImguiLocalDraw* LocalDrawOwner;
	uint32 KeydownMask[3] = {};			// Bitfield of valid toggle key currently pressed
	uint32 KeydownFrame;				// Counter used to reset recorded keydowns after x Frames
};

//=================================================================================================
// TOGGLE ACTIVE WIDGET INPUT
//-------------------------------------------------------------------------------------------------
// Find active viewport and toggle its NetImgui activation (visibility and input)
//=================================================================================================
void FNetImguiLocalDraw::ToggleActiveWidgetInput()
{
	TSharedPtr<SNetImguiWidget> NetImguiWidget = GetActiveViewportWidget();
	if( NetImguiWidget.IsValid() )
	{
		bool WantWidget = WantImguiInView(NetImguiWidget->ParentGameViewport, true);
	#if WITH_EDITOR
		WantWidget |= WantImguiInView(NetImguiWidget->ParentEditorViewport, true);
	#endif
		NetImguiWidget->ToggleInput(WantWidget);
	}
}

//=================================================================================================
// DISABLE ALL WIDGET ACTIVATION
//-------------------------------------------------------------------------------------------------
// As name imply, find all NetImgui Widgets and deactivate them
//=================================================================================================
void FNetImguiLocalDraw::DisableAllWidgetActivation()
{
	for (auto& NetImguiWidgetIt : WidgetsMap)
	{
		if (NetImguiWidgetIt.Value.IsValid()) {
			NetImguiWidgetIt.Value->ToggleInput(false);
		}
	}
}

//=================================================================================================
// GET NETIMGUI WIDGET
//-------------------------------------------------------------------------------------------------
// Find the NetImgui Slate Widget (if available) associated with a Game/Level Viewport
//=================================================================================================
//SF TODO Use viewclient fname for everything (instead of editor configkey) ?
TSharedPtr<SNetImguiWidget> FNetImguiLocalDraw::GetNetImguiWidget(const FName& inClientName)
{
	TSharedPtr<SNetImguiWidget>* NetImguiWidgetPtr	= WidgetsMap.Find(inClientName);
	return NetImguiWidgetPtr ? *NetImguiWidgetPtr : nullptr;
}

//=================================================================================================
// CREATE NETIMGUI WIDGET
//-------------------------------------------------------------------------------------------------
// Create a new NetImgui Widget if not already created
//=================================================================================================
TSharedPtr<SNetImguiWidget> FNetImguiLocalDraw::GetOrCreateNetImguiWidget(const FName& inClientName)
{
	TSharedPtr<SNetImguiWidget> NetImguiWidget = GetNetImguiWidget(inClientName);
	if( !NetImguiWidget.IsValid() ) {
		SAssignNew(NetImguiWidget, SNetImguiWidget)
			.ClientName(inClientName)
			.FontAtlas(LocalFontSupport.FontAtlas);
		WidgetsMap.Add(inClientName, NetImguiWidget);
	}
	return NetImguiWidget;
}

//=================================================================================================
// UPDATE
//-------------------------------------------------------------------------------------------------
// Make sure each Game/Level Viewport has a valid NetImguiWidget, and update their visibility
// (Dear ImGui actual drawing is done in the Widget 'SNetImguiWidget::Tick')
//=================================================================================================
void FNetImguiLocalDraw::Update()
{
	const FNetImguiModule& NetImguiModule	= FNetImguiModule::Get();
	float FontDPIScaleMax					= 0.f;

	//---------------------------------------------------------------------------------------------
	// Makes sure there's a valid Input interceptor
	//---------------------------------------------------------------------------------------------
	if( !InputProcessor.IsValid() )
	{
		InputProcessor = MakeShareable(new FNetImguiInputProcessor(this));
		FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor);
	}
	
	//---------------------------------------------------------------------------------------------
	// Create 1 SNetImguiWidget per GameView and update it
	//---------------------------------------------------------------------------------------------
	UGameViewportClient* GameViewportClient = GEngine->GameViewport;
	while( GameViewportClient )
	{
		TSharedPtr<SNetImguiWidget> NetImguiWidget	= GetOrCreateNetImguiWidget(GameViewportClient->GetFName());
		bool IsFocused								= NetImguiWidget == FSlateApplication::Get().GetUserFocusedWidget(0);
		bool WantImgui								= WantImguiInView(GameViewportClient, IsFocused);
		NetImguiWidget->Update(GameViewportClient, WantImgui);
		FontDPIScaleMax								= WantImgui ? FMath::Max(NetImguiWidget->GetDPIScale(), FontDPIScaleMax) : FontDPIScaleMax;
		GameViewportClient							= GEngine->GetNextPIEViewport(GameViewportClient);
		GameViewportClient							= GameViewportClient == GEngine->GameViewport ? nullptr : GameViewportClient; // Detect loopback
	}

	//---------------------------------------------------------------------------------------------
	// Create 1 SNetImguiWidget per Editor Viewport and update it
	//---------------------------------------------------------------------------------------------
#if WITH_EDITOR
	FLevelEditorModule& LevelEditorModule 			= FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	TSharedPtr<ILevelEditor> LevelEditor 			= LevelEditorModule.GetFirstLevelEditor();
	TSharedPtr<SLevelViewport> ActiveLevelViewport	= LevelEditorModule.GetFirstActiveLevelViewport();
	const FViewport* activeViewport 				= ActiveLevelViewport.IsValid() ? ActiveLevelViewport->GetActiveViewport() : nullptr;
	if (LevelEditor.IsValid())
	{
		TArray<TSharedPtr<SLevelViewport>> Viewports = LevelEditor->GetViewports();
		for (const TSharedPtr<SLevelViewport>& ViewportWindow : Viewports)
		{
			bool IsActiveView							= (activeViewport == ViewportWindow->GetActiveViewport());
			bool WantImgui 								= WantImguiInView(ViewportWindow.Get(), IsActiveView);
			TSharedPtr<SNetImguiWidget> NetImguiWidget	= WantImgui ? GetOrCreateNetImguiWidget(ViewportWindow->GetConfigKey()) : GetNetImguiWidget(ViewportWindow->GetConfigKey());
			if( NetImguiWidget.IsValid() )
			{
				NetImguiWidget->Update(ViewportWindow.Get(), WantImgui);
				FontDPIScaleMax	= FMath::Max(NetImguiWidget->GetDPIScale(), FontDPIScaleMax);
			}
		}
	}
#endif

	LocalFontSupport.Update(FontDPIScaleMax);
}
//=================================================================================================
// CONSTRUCTOR
//=================================================================================================
FNetImguiLocalDraw::FNetImguiLocalDraw()
{
	LocalFontSupport.Initialize();
	
	// Initialize the Unreal to DearImgui key mapping once
	struct UnrealToImguiKeyPair { FKey UnrealKey; ImGuiKey ImguiKey; };
	const UnrealToImguiKeyPair KeysMapping[] = {
	// Mouse buttons
	{EKeys::LeftMouseButton, ImGuiKey_MouseLeft},	{EKeys::RightMouseButton, ImGuiKey_MouseRight},	{EKeys::MiddleMouseButton, ImGuiKey_MouseMiddle},
	{EKeys::ThumbMouseButton, ImGuiKey_MouseX1},	{EKeys::ThumbMouseButton2, ImGuiKey_MouseX2},

	//SF TODO test/finalize key remapping
	// Keyboard keys
	{EKeys::Tab, ImGuiKey_Tab},						{EKeys::Left, ImGuiKey_LeftArrow},				{EKeys::Right, ImGuiKey_RightArrow},
	{EKeys::Up, ImGuiKey_UpArrow},					{EKeys::Down, ImGuiKey_DownArrow},				{EKeys::PageUp, ImGuiKey_PageUp},
	{EKeys::PageDown, ImGuiKey_PageDown},			{EKeys::Home, ImGuiKey_Home},					{EKeys::End, ImGuiKey_End},
	{EKeys::Insert, ImGuiKey_Insert},				{EKeys::Delete, ImGuiKey_Delete},				{EKeys::BackSpace, ImGuiKey_Backspace},
	{EKeys::SpaceBar, ImGuiKey_Space}, 				{EKeys::Enter, ImGuiKey_Enter}, 				{EKeys::Escape, ImGuiKey_Escape},
 
	{EKeys::LeftControl, ImGuiKey_LeftCtrl}, 		{EKeys::LeftShift, ImGuiKey_LeftShift}, 		{EKeys::LeftAlt, ImGuiKey_LeftAlt},
	{EKeys::LeftCommand, ImGuiKey_LeftSuper},		{EKeys::RightControl, ImGuiKey_RightCtrl},		{EKeys::RightShift, ImGuiKey_RightShift},
	{EKeys::RightAlt, ImGuiKey_RightAlt}, 			{EKeys::RightCommand, ImGuiKey_RightSuper},
	//ImGuiKey_Menu
	{EKeys::Zero, ImGuiKey_0}, 	{EKeys::One, ImGuiKey_1}, 	{EKeys::Two, ImGuiKey_2},	{EKeys::Three, ImGuiKey_3},
	{EKeys::Four, ImGuiKey_4}, 	{EKeys::Five, ImGuiKey_5}, 	{EKeys::Six, ImGuiKey_6},	{EKeys::Seven, ImGuiKey_7},
	{EKeys::Eight, ImGuiKey_8}, {EKeys::Nine, ImGuiKey_9},

	{EKeys::A, ImGuiKey_A}, {EKeys::B, ImGuiKey_B}, {EKeys::C, ImGuiKey_C}, {EKeys::D, ImGuiKey_D}, {EKeys::E, ImGuiKey_E},
	{EKeys::F, ImGuiKey_F}, {EKeys::G, ImGuiKey_G}, {EKeys::H, ImGuiKey_H}, {EKeys::I, ImGuiKey_I}, {EKeys::J, ImGuiKey_J},
	{EKeys::K, ImGuiKey_K}, {EKeys::L, ImGuiKey_L}, {EKeys::M, ImGuiKey_M}, {EKeys::N, ImGuiKey_N}, {EKeys::O, ImGuiKey_O},
	{EKeys::P, ImGuiKey_P}, {EKeys::Q, ImGuiKey_Q}, {EKeys::R, ImGuiKey_R}, {EKeys::S, ImGuiKey_S}, {EKeys::T, ImGuiKey_T},
	{EKeys::U, ImGuiKey_U}, {EKeys::V, ImGuiKey_V}, {EKeys::W, ImGuiKey_W}, {EKeys::X, ImGuiKey_X}, {EKeys::Y, ImGuiKey_Y},
	{EKeys::Z, ImGuiKey_Z},

	{EKeys::F1, ImGuiKey_F1}, 	{EKeys::F2, ImGuiKey_F2}, 	{EKeys::F3, ImGuiKey_F3}, 	{EKeys::F4, ImGuiKey_F4}, 
	{EKeys::F5, ImGuiKey_F5}, 	{EKeys::F6, ImGuiKey_F6}, 	{EKeys::F7, ImGuiKey_F7},	{EKeys::F8, ImGuiKey_F8}, 
	{EKeys::F9, ImGuiKey_F9}, 	{EKeys::F10, ImGuiKey_F10}, {EKeys::F11, ImGuiKey_F11}, {EKeys::F12, ImGuiKey_F12},
	
	{EKeys::Apostrophe, ImGuiKey_Apostrophe},	{EKeys::Comma, ImGuiKey_Comma}, 				{EKeys::Period, ImGuiKey_Period},
	{EKeys::Slash, ImGuiKey_Slash}, 			{EKeys::Semicolon, ImGuiKey_Semicolon}, 		{EKeys::LeftBracket, ImGuiKey_LeftBracket},
	{EKeys::BackSpace, ImGuiKey_Backspace},		{EKeys::RightBracket, ImGuiKey_RightBracket}, 	{EKeys::A_AccentGrave, ImGuiKey_GraveAccent},
	{EKeys::CapsLock, ImGuiKey_CapsLock}, 		{EKeys::ScrollLock, ImGuiKey_ScrollLock}, 		{EKeys::NumLock, ImGuiKey_NumLock},
	{EKeys::Pause, ImGuiKey_Pause},
	
	{EKeys::NumPadZero, ImGuiKey_Keypad0}, 		{EKeys::NumPadOne, ImGuiKey_Keypad1}, 	{EKeys::NumPadTwo, ImGuiKey_Keypad2},
	{EKeys::NumPadThree, ImGuiKey_Keypad3}, 	{EKeys::NumPadFour, ImGuiKey_Keypad4}, 	{EKeys::NumPadFive, ImGuiKey_Keypad5},
	{EKeys::NumPadSix, ImGuiKey_Keypad6}, 		{EKeys::NumPadSeven, ImGuiKey_Keypad7}, {EKeys::NumPadNine, ImGuiKey_Keypad8},
	{EKeys::Decimal, ImGuiKey_KeypadDecimal},	{EKeys::Divide, ImGuiKey_KeypadDivide}, {EKeys::Multiply, ImGuiKey_KeypadMultiply},
	{EKeys::Add, ImGuiKey_KeypadAdd},
	
	// No 'numpad version' of these keys in Unreal and already added to imgui
	{EKeys::Subtract, ImGuiKey_Minus},			{EKeys::Equals, ImGuiKey_Equal},
	//{EKeys::Subtract, ImGuiKey_KeypadSubtract}, {EKeys::Enter, ImGuiKey_KeypadEnter}, {EKeys::Equals, ImGuiKey_KeypadEqual},

	// Gamepad
	{EKeys::Gamepad_LeftThumbstick, ImGuiKey_GamepadL3},			{EKeys::Gamepad_RightThumbstick, ImGuiKey_GamepadR3},
	{EKeys::Gamepad_Special_Left, ImGuiKey_GamepadBack}, //SF ????	
	{EKeys::Gamepad_Special_Right, ImGuiKey_GamepadStart}, //SF ????
	{EKeys::Gamepad_FaceButton_Bottom, ImGuiKey_GamepadFaceDown},	{EKeys::Gamepad_FaceButton_Right, ImGuiKey_GamepadFaceRight},
	{EKeys::Gamepad_FaceButton_Left, ImGuiKey_GamepadFaceLeft},		{EKeys::Gamepad_FaceButton_Top, ImGuiKey_GamepadFaceUp},
	{EKeys::Gamepad_LeftShoulder, ImGuiKey_GamepadL1},				{EKeys::Gamepad_RightShoulder, ImGuiKey_GamepadR1},
	{EKeys::Gamepad_LeftTrigger, ImGuiKey_GamepadL2},				{EKeys::Gamepad_RightTrigger, ImGuiKey_GamepadR2},
	{EKeys::Gamepad_DPad_Up, ImGuiKey_GamepadDpadUp},				{EKeys::Gamepad_DPad_Down,ImGuiKey_GamepadDpadDown},
	{EKeys::Gamepad_DPad_Right, ImGuiKey_GamepadDpadRight},			{EKeys::Gamepad_DPad_Left, ImGuiKey_GamepadDpadLeft},
	};
	
	//SF Add gamepad?

	for (size_t i(0); i<UE_ARRAY_COUNT(KeysMapping); ++i) {
		GUnrealKeyToImguiMap.Add(KeysMapping[i].UnrealKey, KeysMapping[i].ImguiKey);
	}

	SetWantImguiInGameViewFN(nullptr);
#if WITH_EDITOR
	SetWantImguiInEditorViewFN(nullptr);
#endif
}

//=================================================================================================
// DESTRUCTOR
//=================================================================================================
FNetImguiLocalDraw::~FNetImguiLocalDraw()
{
	if( FSlateApplication::IsInitialized() ){
		FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);
	}
	LocalFontSupport.Terminate();
	WidgetsMap.Reset();
	InputProcessor = nullptr;
}

//=================================================================================================
// GET ACTIVE VIEWPORT WIDGET
//-------------------------------------------------------------------------------------------------
// Find the NetImgui Widget of the active viewport (if there's one)
//=================================================================================================
TSharedPtr<SNetImguiWidget> FNetImguiLocalDraw::GetActiveViewportWidget()
{
	TSharedPtr<SNetImguiWidget> ActiveWidget;
	
	// If a NetImguiWidget is in focus, return it
	TSharedPtr<SWidget> SlateActiveWidget = FSlateApplication::Get().GetUserFocusedWidget(0);
	for(const auto& NetImguiIt : WidgetsMap)
	{
		if (NetImguiIt.Value.Get() == SlateActiveWidget.Get())
		{
			return NetImguiIt.Value;
		}
	}

	// Iterate all Game Viewports and find first active one
	UGameViewportClient* GameViewportClient = GEngine->GameViewport;
	while( GameViewportClient )
	{
		if( GameViewportClient->IsFocused(GameViewportClient->GetGameViewport()->GetViewport()) )
		{
			return GetNetImguiWidget(GameViewportClient->GetFName());
		}
		// Go to next GameViewport and detect when we looped back to first item
		GameViewportClient	= GEngine->GetNextPIEViewport(GameViewportClient);
		GameViewportClient	= GameViewportClient == GEngine->GameViewport ? nullptr : GameViewportClient;
	}
	
#if WITH_EDITOR	
	// Find NetImgui Widget associated with active Editor Viewport
	FLevelEditorModule& LevelEditorModule 			= FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	TSharedPtr<SLevelViewport> ActiveLevelViewport	= LevelEditorModule.GetFirstActiveLevelViewport();
	if (ActiveLevelViewport.IsValid() && !ActiveLevelViewport->IsPlayInEditorViewportActive())
	{
		return GetNetImguiWidget(ActiveLevelViewport->GetConfigKey());
	}
#endif

	return nullptr;
}

//=================================================================================================
// XXXX WantImguiInGameView
//-------------------------------------------------------------------------------------------------
// Handle requests of knowing if we should use local Dear Imgui content in a game viewport.
// Default behavior is to always enabled local content.
//=================================================================================================
bool DefaultWantImguiInGameView(const UGameViewportClient& inGameClient, bool HasInputFocus)
{
	return true;
}

void FNetImguiLocalDraw::SetWantImguiInGameViewFN(const FWantImguiInGameViewFN& callback)
{	
	WantImguiInGameViewFN = callback ? callback : DefaultWantImguiInGameView;
}

bool FNetImguiLocalDraw::WantImguiInView(const UGameViewportClient* GameClient, bool HasInputFocus) const
{
	static const UNetImguiSettings* NetImguiSettings = GetDefault<UNetImguiSettings>();
	if( !GameClient ){
		return false;
	}

	bool ForceDebugOff		= GSystemSettings.GetForce0Mask().OnScreenDebug;
	bool ForceDebugOn		= GSystemSettings.GetForce1Mask().OnScreenDebug;
	bool ValidRemote		= !NetImguiSettings->RemoteHideLocal || !NetImgui::IsConnected(); //SF TODO affect only the 1 selected context?
	bool ValidVisibility	= (NetImguiSettings->LocalVisibilityGame == ENetImguiVisibility::Always) ||
							  (NetImguiSettings->LocalVisibilityGame == ENetImguiVisibility::HasInput) || // 'HasInput' visibility controlled in widget's update
							  (NetImguiSettings->LocalVisibilityGame == ENetImguiVisibility::Focused && HasInputFocus);
	bool ValidShowFlag		= !NetImguiSettings->LocalUseOnScreenDebugFlag || ForceDebugOn || (!ForceDebugOff && GameClient->EngineShowFlags.OnScreenDebug);
	
	return NetImguiSettings->Show && ValidVisibility && ValidShowFlag && ValidRemote && WantImguiInGameViewFN(*GameClient, HasInputFocus);
}

//=================================================================================================
// XXXX WantImguiInEditorView
//-------------------------------------------------------------------------------------------------
// Handle requests of knowing if we should use local Dear Imgui content in a editor viewport.
// Default behavior is to enable it on view set to perspective and without PIE current active.
//=================================================================================================
#if WITH_EDITOR
bool DefaultWantImguiInEditorView(const SLevelViewport& EditorViewport, bool HasViewportFocus)
{
	return EditorViewport.GetLevelViewportClient().IsPerspective();
}

void FNetImguiLocalDraw::SetWantImguiInEditorViewFN(const FWantImguiInEditorViewFN& Callback)
{
	WantImguiInEditorViewFN = Callback ? Callback : DefaultWantImguiInEditorView;
}

bool FNetImguiLocalDraw::WantImguiInView(const SLevelViewport* EditorViewport, bool HasViewportFocus)const
{
	static const UNetImguiSettings* NetImguiSettings = GetDefault<UNetImguiSettings>();
	if(!EditorViewport || EditorViewport->HasPlayInEditorViewport()){
		return false;
	}

	bool ForceDebugOff		= GSystemSettings.GetForce0Mask().OnScreenDebug;
	bool ForceDebugOn		= GSystemSettings.GetForce1Mask().OnScreenDebug;
	bool ValidRemote		= !NetImguiSettings->RemoteHideLocal || !NetImgui::IsConnected(); //SF TODO affect only the 1 selected context?
	bool ValidVisibility	= (NetImguiSettings->LocalVisibilityEditor == ENetImguiVisibility::Always) ||
							  (NetImguiSettings->LocalVisibilityEditor == ENetImguiVisibility::HasInput) || // 'HasInput' visibility controlled in widget's update
							  (NetImguiSettings->LocalVisibilityEditor == ENetImguiVisibility::Focused && HasViewportFocus);
	bool ValidShowFlag		= !NetImguiSettings->LocalUseOnScreenDebugFlag || ForceDebugOn || (!ForceDebugOff && EditorViewport->GetLevelViewportClient().EngineShowFlags.OnScreenDebug);
	return NetImguiSettings->Show && ValidVisibility && ValidShowFlag && ValidRemote && WantImguiInEditorViewFN(*EditorViewport, HasViewportFocus);
}
#endif // #if WITH_EDITOR

//=================================================================================================
// FONT BULK DATA CLASS
//-------------------------------------------------------------------------------------------------
// Used to send font texture update data to render thread
//=================================================================================================
void FNetImguiLocalDraw::FFontBulkData::Init(const void* InData, uint32 InWidth, uint32 InHeight) 
{ 
	Width = InWidth;
	Height = InHeight;
	Data.Reset();
	Data.Append((uint8*)InData, Width*Height);
}

void FNetImguiLocalDraw::FLocalFontSuport::Initialize()
{
	Terminate();
	FontAtlas = IM_NEW(ImFontAtlas);
}

void FNetImguiLocalDraw::FLocalFontSuport::Terminate()
{
	FontDPIScale	= 0.f;
	TextureRef		= nullptr;
	TextureUpdateData.Discard();
	if( FontAtlas ){
		IM_DELETE(FontAtlas);
		FontAtlas = nullptr;
	}
}

void FNetImguiLocalDraw::FLocalFontSuport::Update(float wantedFontDPIScale)
{
	if ( FNetImguiModule::UpdateFont(FontAtlas, FontDPIScale, wantedFontDPIScale) )
	{
		FontDPIScale = wantedFontDPIScale;
		uint8_t* pPixelData(nullptr);
		int width(0), height(0);
		FontAtlas->GetTexDataAsAlpha8(&pPixelData, &width, &height); //SF TODO handle R8 & RGBA32 ?
		TextureUpdateData.Init(pPixelData, width, height);
		// Note: Free unneeded client texture memory (after localdraw was able to process it).
		// Various font size with japanese and icons can increase memory substancially(~64MB)
		FontAtlas->ClearTexData();

		ENQUEUE_RENDER_COMMAND(ImguiCreateTextures)(
			[this](FRHICommandListImmediate& RHICmdList)
			{
				FRHITextureCreateDesc Desc = FRHITextureCreateDesc::Create2D(TEXT("ImGuiFontTexture"), TextureUpdateData.Width, TextureUpdateData.Height, EPixelFormat::PF_R8)
					.SetClearValue(FClearValueBinding::Black)
					.SetFlags(ETextureCreateFlags::ShaderResource)
					.SetInitialState(ERHIAccess::SRVGraphics)
					.SetBulkData(&TextureUpdateData);
					//.SetFlags(ETextureCreateFlags::SRGB)
				TextureRef = RHICreateTexture(Desc);
			});
		FlushRenderingCommands();
		TextureUpdateData.Discard();
		FontAtlas->SetTexID(reinterpret_cast<void*>(this));
	}
}

#endif //NETIMGUI_ENABLED

