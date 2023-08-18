// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "NetImguiModule.h"
#include "CoreMinimal.h"

#if NETIMGUI_LOCALDRAW_ENABLED || 1 //SF To Figure out

#include "NetImguiLocalDraw.h"

#include "SceneView.h"
#include "SceneInterface.h"
#include "Engine/GameViewportClient.h"

#if WITH_EDITOR
#include "Slate/SceneViewport.h"
#include "LevelEditorViewport.h"
#include "SLevelViewport.h"
#include "LevelEditor.h"
#endif

#pragma optimize("", off) //SF

TMap<FKey, ImGuiKey> GUnrealKeyToImguiMap;
struct UnrealToImguiKeyPair { FKey UnrealKey; ImGuiKey ImguiKey; };
static const UnrealToImguiKeyPair KeysMapping[] = {
	// Mouse buttons
	{EKeys::LeftMouseButton, ImGuiKey_MouseLeft},
	{EKeys::RightMouseButton, ImGuiKey_MouseRight},
	{EKeys::MiddleMouseButton, ImGuiKey_MouseMiddle},
	{EKeys::ThumbMouseButton, ImGuiKey_MouseX1},
	{EKeys::ThumbMouseButton2, ImGuiKey_MouseX2},

	//SF TODO test/finalize key remapping
	// Keyboard keys
	{EKeys::Tab, ImGuiKey_Tab},					{EKeys::Left, ImGuiKey_LeftArrow},			{EKeys::Right, ImGuiKey_RightArrow},
	{EKeys::Up, ImGuiKey_UpArrow},				{EKeys::Down, ImGuiKey_DownArrow},			{EKeys::PageUp, ImGuiKey_PageUp},
	{EKeys::PageDown, ImGuiKey_PageDown},		{EKeys::Home, ImGuiKey_Home},				{EKeys::End, ImGuiKey_End},
	{EKeys::Insert, ImGuiKey_Insert},			{EKeys::Delete, ImGuiKey_Delete},			{EKeys::BackSpace, ImGuiKey_Backspace},
	{EKeys::SpaceBar, ImGuiKey_Space}, 			{EKeys::Enter, ImGuiKey_Enter}, 			{EKeys::Escape, ImGuiKey_Escape},
 
	{EKeys::LeftControl, ImGuiKey_LeftCtrl}, 	{EKeys::LeftShift, ImGuiKey_LeftShift}, 	{EKeys::LeftAlt, ImGuiKey_LeftAlt},
	{EKeys::LeftCommand, ImGuiKey_LeftSuper},	{EKeys::RightControl, ImGuiKey_RightCtrl},	{EKeys::RightShift, ImGuiKey_RightShift},
	{EKeys::RightAlt, ImGuiKey_RightAlt}, 		{EKeys::RightCommand, ImGuiKey_RightSuper},
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
	{EKeys::BackSpace, ImGuiKey_Backslash},		{EKeys::RightBracket, ImGuiKey_RightBracket}, 	{EKeys::A_AccentGrave, ImGuiKey_GraveAccent},
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
};

// Intercept a few input before Slate, to shift control to NetImgui when needed
class FNetImguiInputProcessor : public IInputProcessor//, public TSharedFromThis<FNetImguiInputProcessor>
{
public:
	FNetImguiInputProcessor(NetImguiLocalDrawSupport* inOwner):mpOwner(inOwner){};
	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor){};
	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{	
		/*
		if( GEngine->GameViewport ){
			UGameViewportClient* pGameClient = GEngine->GameViewport.Get();
			FName clientName	= pGameClient->bIsPlayInEditorViewport ? TEXT("GamePIE") : TEXT("Game");
			TSharedPtr<SNetImguiWidget>* netimguiWidget = mpOwner->GetOrCreateNetImguiWidget(clientName, false, false);
			if( netimguiWidget && (*netimguiWidget).IsValid()){
				FSlateApplication::Get().SetAllUserFocus(*netimguiWidget, EFocusCause::SetDirectly);
			}
		}
		*/
		return false;
	}
protected:
	NetImguiLocalDrawSupport* mpOwner;
};

TSharedPtr<SNetImguiWidget>* NetImguiLocalDrawSupport::GetOrCreateNetImguiWidget(const FName& inClientName, bool inIsEditorViewport, bool inAutoCreate )
{
	TSharedPtr<SNetImguiWidget>* netImguiWidget = WidgetsMap.Find(inClientName);
	if( netImguiWidget == nullptr && inAutoCreate ){
		TSharedPtr<SNetImguiWidget> newNetImguiWidget;
		SAssignNew(newNetImguiWidget, SNetImguiWidget)
			.ClientName(inClientName)
			.IsEditorWindow(inIsEditorViewport);
		netImguiWidget = &WidgetsMap.Add(inClientName, newNetImguiWidget);
	}
	return netImguiWidget;
}

void NetImguiLocalDrawSupport::Initialize()
{
	ImGuiIO& io	= ImGui::GetIO();
	uint8_t* pPixelData(nullptr);
	int width(0), height(0);
	io.Fonts->GetTexDataAsRGBA32(&pPixelData, &width, &height); //SF TODO handle R8 & RGBA32 ?
	FontTextureData.Init(pPixelData, width, height);
	//UGameViewportClient::OnViewportCreated().AddRaw(this, &NetImguiLocalDrawSupport::OnGameViewportCreated);
	
	//---------------------------------------------------------------------------------------------
	// Initialize the Unreal to DearImgui key mapping once
	//---------------------------------------------------------------------------------------------
	for (size_t i(0); i<UE_ARRAY_COUNT(KeysMapping); ++i) {
		GUnrealKeyToImguiMap.Add(KeysMapping[i].UnrealKey, KeysMapping[i].ImguiKey);
	}
}

void NetImguiLocalDrawSupport::Update()
{
	const FNetImguiModule& netImguiModule = FNetImguiModule::Get();
	
	// Makes sure there's a valid SNetImguiWidget for Game View
	if( GEngine->GameViewport ){
		FName clientName	= GEngine->GameViewport->bIsPlayInEditorViewport ? TEXT("GamePIE") : TEXT("Game");
		bool wantImgui 		= netImguiModule.WantImguiInView(GEngine->GameViewport);
		TSharedPtr<SNetImguiWidget>* netimguiWidget = GetOrCreateNetImguiWidget(clientName, false, wantImgui);
		if( netimguiWidget ){
			bool isFocused = true; //SF TODO
			(*netimguiWidget)->Update(GEngine->GameViewport, wantImgui, isFocused, false, GEngine->GameViewport->GetDPIScale());
		}

		UGameViewportClient* pGameClient = GEngine->GameViewport.Get();
		bool isCap = pGameClient->IsInPermanentCapture();
		isCap &= true;
	}

	
#if WITH_EDITOR
	// Makes sure there's a valid SNetImguiWidget for Editor Viewport
	FLevelEditorModule& LevelEditorModule 			= FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	TSharedPtr<ILevelEditor> LevelEditor 			= LevelEditorModule.GetFirstLevelEditor();
	TSharedPtr<IAssetViewport> ActiveLevelViewport 	= LevelEditorModule.GetFirstActiveViewport();
	const FViewport* activeViewpport 				= ActiveLevelViewport.IsValid() ? ActiveLevelViewport->GetActiveViewport() : nullptr;
	if (LevelEditor.IsValid()){
		TArray<TSharedPtr<SLevelViewport>> Viewports = LevelEditor->GetViewports();
		for (const TSharedPtr<SLevelViewport>& ViewportWindow : Viewports){
			FName clientName	= ViewportWindow->GetConfigKey();
			bool wantImgui 		= netImguiModule.WantImguiInView(ViewportWindow.Get());
			TSharedPtr<SNetImguiWidget>* netimguiWidget = GetOrCreateNetImguiWidget(clientName, true, wantImgui);
			if( netimguiWidget ){
				bool isFocused = activeViewpport == ViewportWindow->GetActiveViewport();
				(*netimguiWidget)->Update(ViewportWindow.Get(), wantImgui, isFocused, false, ViewportWindow->GetViewportClient()->GetDPIScale());
			}
		}
	}
#endif

	//Move this? And remove update?
	//---------------------------------------------------------------------------------------------
	// Generate the Font Texture once
	//---------------------------------------------------------------------------------------------
	if (!FontTexture.IsValid() && FontTextureData.GetResourceBulkData() != nullptr )
	{
		ENQUEUE_RENDER_COMMAND(ImguiCreateTextures)(
			[this](FRHICommandListImmediate& RHICmdList)
			{
				FRHITextureCreateDesc Desc = FRHITextureCreateDesc::Create2D(TEXT("ImGuiFontTexture"), FontTextureData.Width, FontTextureData.Height, EPixelFormat::PF_R8G8B8A8)
					.SetClearValue(FClearValueBinding::Black)
					.SetFlags(ETextureCreateFlags::ShaderResource ) 
					//.SetFlags(ETextureCreateFlags::SRGB)
					.SetInitialState(ERHIAccess::SRVGraphics)
					.SetBulkData(&FontTextureData);
				FontTexture = RHICreateTexture(Desc);
				GFontTexture = FontTexture;
			});
		FlushRenderingCommands();
		FontTextureData.Discard();
	}
}

void NetImguiLocalDrawSupport::Terminate()
{
	FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);
	//InputProcessor = nullptr;
	//SF Destroy contexts
}

void NetImguiLocalDrawSupport::InterceptInput()
{
	
	if( !InputProcessor.IsValid() ){
		InputProcessor = MakeShareable(new FNetImguiInputProcessor(this));
		FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor);
	}
	
	//SF insert NetImgui internet here
	return;
#if 0
	ImGuiContext* remoteContext = ImGui::GetCurrentContext();
	ImGuiIO& remoteIO			= ImGui::GetIO();
	
	//TMap<void*, ImGuiContext*> ContextsMap2;
	for (auto& context : ContextsMap) {
		ImGui::SetCurrentContext(context.Value);
		ImGuiIO& localIO		= ImGui::GetIO();
		localIO.MouseDrawCursor = false;
		if (ActiveContextIndex == 0) {
			localIO.MouseDrawCursor = true;
			float mouseX = remoteIO.MousePos.x / remoteIO.DisplaySize.x * localIO.DisplaySize.x;
			float mouseY = remoteIO.MousePos.y / remoteIO.DisplaySize.y * localIO.DisplaySize.y;
			localIO.AddMousePosEvent(mouseX, mouseY);
			for (uint32 i = 0; i < UE_ARRAY_COUNT(remoteIO.MouseDown); ++i) {
				localIO.AddMouseButtonEvent(i, remoteIO.MouseDown[i]);
			}
			
			break;
		}
	}
	ImGui::SetCurrentContext(remoteContext);
#endif
}

void NetImguiLocalDrawSupport::FFontBulkData::Init(const void* InData, uint32 InWidth, uint32 InHeight) 
{ 
	Width = InWidth;
	Height = InHeight;
	Data.Reset();
	Data.Append((uint8*)InData, Width*Height*4);
}

#endif //NETIMGUI_ENABLED

#pragma optimize("", on) //SF
