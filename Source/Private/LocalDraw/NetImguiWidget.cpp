// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "NetImguiModule.h"
#include "CoreMinimal.h"

#if NETIMGUI_LOCALDRAW_ENABLED || 1 //SF
#include "Slate/Public/Framework/Application/SlateApplication.h"
#include "Slate/Public/Framework/Application/SlateUser.h"
#include "NetImguiRender.h"

#pragma optimize("", off) //SF

//=================================================================================================
// GET DRAW VERTICAL OFFSET (inline)
//-------------------------------------------------------------------------------------------------
// Used to avoid drawing over Editor tool icons at the top of the viewport
//=================================================================================================
float SNetImguiWidget::GetDrawVerticalOffset() const
{
#if WITH_EDITOR
	static const UNetImguiSettings* NetImguiSettings = GetDefault<UNetImguiSettings>();
	if( ParentEditorViewport && NetImguiSettings->LocalVisibilityEditor != ENetImguiVisibility::HasInput )
		return 32.f;
#endif
	return 0.f;
}

//=================================================================================================
// DESTRUCTOR
//-------------------------------------------------------------------------------------------------
// Free allocated Dear ImGui context
//=================================================================================================
SNetImguiWidget::~SNetImguiWidget()
{
	ImGui::DestroyContext(ImguiContext);
}

//=================================================================================================
// HAS INPUT
//-------------------------------------------------------------------------------------------------
// True if Widget is focused and receiving inputs
//=================================================================================================
bool SNetImguiWidget::HasInput() const 
{ 
	return FSlateApplication::Get().GetUserFocusedWidget(0).Get() == this;
}

//=================================================================================================
// TOGGLE INPUT
//-------------------------------------------------------------------------------------------------
// Switch focus of this Widget On/Off to enable input
// Try restoring focus to previous item when deactivating the widget
//=================================================================================================
void SNetImguiWidget::ToggleInput(bool IsWantedWidget)
{
	if (IsWantedWidget && !HasInput() )
	{
		FocusedWidgetLast	= FSlateApplication::Get().GetUserFocusedWidget(0);
		SetVisibility(EVisibility::Visible);
		FSlateApplication::Get().ResetToDefaultPointerInputSettings();
		FSlateApplication::Get().SetUserFocus(0, SharedThis(this));
	}
	else if( FocusedWidgetLast.IsValid() )
	{
		FSlateApplication::Get().SetUserFocus(0, FocusedWidgetLast.Pin(), EFocusCause::SetDirectly);
	}
}

//=================================================================================================
// GET DPI SCALE
//-------------------------------------------------------------------------------------------------
// Return the DPIScale used by associated Viewport
//=================================================================================================
float SNetImguiWidget::GetDPIScale() const
{
#if WITH_EDITOR
	return	ParentGameViewport		? ParentGameViewport->GetDPIScale()
									: ParentEditorViewport	? ParentEditorViewport->GetViewportClient()->GetDPIScale()
									: 1.f;
#else
	return	ParentGameViewport		? ParentGameViewport->GetDPIScale() : 1.f;
#endif
}

//=================================================================================================
// TICK
//-------------------------------------------------------------------------------------------------
// Called every frame to draw the Dear ImGui content
//=================================================================================================
void SNetImguiWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if( GetVisibility().IsVisible() && ImGui::GetIO().Fonts->IsBuilt() )
	{
		NetImguiScopedContext ScopedContext(ImguiContext);
		ImGuiIO& io		= ImGui::GetIO();
		io.DeltaTime	= InDeltaTime;
		
		//bool FSlateApplication::Get().IsMouseAttached() const { return PlatformApplication.IsValid() ? PlatformApplication->IsMouseAttached() : false; }
		//bool FSlateApplication::Get().IsGamepadAttached() const { return PlatformApplication.IsValid() ? PlatformApplication->IsGamepadAttached() : false; }
		io.ConfigFlags		= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
		io.BackendFlags		= ImGuiBackendFlags_HasGamepad;

		if (!HasInput())
		{
			io.ClearEventsQueue();
			io.ClearInputCharacters();
			io.ClearInputKeys();
			io.AddMousePosEvent(-1.f, -1.f);
		}
		// Ignore mouse when we do not have access to it
		//if (FSlateApplication::Get().GetCursorUser().Get()->HasAnyCapture()){
		//	io.AddMousePosEvent(-1.f, -1.f);
		//}
		else{
			const FSlateRenderTransform screenToImguiCoord = AllottedGeometry.GetAccumulatedRenderTransform();
			FVector2f mousePos	= screenToImguiCoord.Inverse().TransformPoint(FSlateApplication::Get().GetCursorPos());
			mousePos.Y			-= GetDrawVerticalOffset();
			mousePos 			*= AllottedGeometry.Scale;
			io.AddMousePosEvent(mousePos.X, mousePos.Y);
		}
		
		// Configure this widget as passthrough when no mouse events are needed
		SetVisibility(io.WantCaptureMouse ? EVisibility::Visible : EVisibility::HitTestInvisible);
		
		// We share 1 Font Atlas between all local views, using the highest DPI scaling detected
		// We then size down the font drawing to match this view expected DPI
		const auto* fontUnrealData = reinterpret_cast<FNetImguiLocalDraw::FLocalFontSuport*>(ImGui::GetIO().Fonts->TexID);
		for (auto font : fontUnrealData->FontAtlas->Fonts){
			font->Scale = (GetDPIScale()*FontScale) / fontUnrealData->FontDPIScale;
		}
	
		//SetFlag(IO.ConfigFlags, ImGuiConfigFlags_NavEnableKeyboard, InputState.IsKeyboardNavigationEnabled());
		//SetFlag(IO.ConfigFlags, ImGuiConfigFlags_NavEnableGamepad, InputState.IsGamepadNavigationEnabled());
		//SetFlag(IO.BackendFlags, ImGuiBackendFlags_HasGamepad, InputState.HasGamepad());

		ImGui::NewFrame();
#if 0 //SF
		if( ImGui::BeginMainMenuBar() )
		{
			//SF Call callbacks here
			//SF test
		
			if (ImGui::BeginMenu("NetImgui")) {
				ImGui::SliderFloat("Opacity", &ImguiParameters.X, 0.1f, 1.f);
				ImGui::SliderFloat("Font scale", &FontScale, 0.5f, 2.f);
				ImGui::EndMenu();
			}
			
			if( ImGui::IsItemHovered() ){
				ImGuiWindowFlags windowFlags 	= ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMouseInputs 
												| ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
				const ImGuiViewport* viewport   = ImGui::GetMainViewport();
				float padding                   = 16.f * AllottedGeometry.Scale;
				ImVec2 highlightPos             = viewport->WorkPos + ImVec2(padding, padding);
				ImVec2 highlightSize            = viewport->WorkSize - ImVec2(padding, padding) * 2.f;
				ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(1.f,1.f,1.f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.f,1.f,1.f, 1.0f));
				ImGui::SetNextWindowPos(highlightPos, ImGuiCond_Always);
				ImGui::SetNextWindowSize(highlightSize);
				ImGui::SetNextWindowBgAlpha(0.2f);
				ImGui::Begin("Highlight", nullptr, windowFlags | ImGuiWindowFlags_Tooltip); // tooltip flag to force stay on top
				ImGui::End();
				ImGui::PopStyleColor(2);
			}
			ImGui::EndMainMenuBar();
		}
#endif //SF
		ImGui::SetNextWindowPos(ImVec2(0,0), ImGuiCond_Once);
		ImGui::ShowDemoWindow(nullptr);
		ImGui::Render();
	}
}

//=================================================================================================
// ON PAINT
//-------------------------------------------------------------------------------------------------
// Called evrey frame to paint this widget. Takes the last Dear ImGui that was drawn
// and forward it to the render thread using a custom drawer for ImGui (instead of Slate)
//=================================================================================================
int32 SNetImguiWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	NetImguiScopedContext scopedContext(ImguiContext);
	FSlateRect imguiRect 		= MyCullingRect;
	imguiRect.Top 				+= GetDrawVerticalOffset() * GetDPIScale();
	ImGui::GetIO().DisplaySize	= ImVec2(imguiRect.GetSize2f().X, imguiRect.GetSize2f().Y);

	//---------------------------------------------------------------------------------------------
	// Add DebugDraw Item for the 'Dear ImGui' content of this viewport
	//---------------------------------------------------------------------------------------------
	TSharedPtr<FNetImguiSlateElement, ESPMode::ThreadSafe> NetImguiDrawer = NetImguiDrawers[DrawCounter++ % 3];
	auto* fontUnrealData = reinterpret_cast<FNetImguiLocalDraw::FLocalFontSuport*>(ImGui::GetIO().Fonts->TexID);
	//SF BLACK TEXTURE support
	if (NetImguiDrawer->Update(fontUnrealData->TextureRef, fontUnrealData->TextureRef, ImguiContext, imguiRect, ImguiParameters)) {
		FSlateDrawElement::MakeCustom(OutDrawElements, LayerId++, NetImguiDrawer);
	}
	
	return LayerId;
}

//=================================================================================================
// UPDATE (Level Editor View)
//-------------------------------------------------------------------------------------------------
// Called once per frame, to make sure the widget is assigned to the Viewport and
// adjust its visibility / dpi 
//=================================================================================================
#if WITH_EDITOR
void SNetImguiWidget::Update(SLevelViewport* LevelViewport, bool Visible)
{
	static const UNetImguiSettings* NetImguiSettings = GetDefault<UNetImguiSettings>();
	if( ParentEditorViewport != LevelViewport ){
		ParentEditorViewport	= LevelViewport;
		ParentGameViewport		= nullptr;
		LevelViewport->AddOverlayWidget(SharedThis(this));
	}
	
	// When visibility is configured to 'Activated' in plugin project settings,
	// we manage visibility here instead of 'WantImguiInView()' to make sure the widget is created,
	// otherwise, we could never activate it
	Visible &= NetImguiSettings->LocalVisibilityEditor != ENetImguiVisibility::HasInput || HasInput();
	if( Visible != GetVisibility().IsVisible() ){
		SetVisibility(Visible ? EVisibility::Visible : EVisibility::Hidden);
	}

	//SF
	FLevelEditorModule& LevelEditorModule 			= FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	TSharedPtr<SLevelViewport> ActiveLevelViewport	= LevelEditorModule.GetFirstActiveLevelViewport();
	if( ParentEditorViewport == ActiveLevelViewport.Get() && !HasInput() ) //FSlateApplication::Get().GetUserFocusedWidget(0) != SharedThis(this))
	{
		FocusedWidgetLast	= FSlateApplication::Get().GetUserFocusedWidget(0);
	}
}
#endif //WITH_EDITOR

//=================================================================================================
// UPDATE (Game View)
//-------------------------------------------------------------------------------------------------
// Called once per frame, to make sure the widget is assigned to the right Viewport and
// adjust its visibility
//=================================================================================================
void SNetImguiWidget::Update(UGameViewportClient* GameViewport, bool Visible)
{
	static const UNetImguiSettings* NetImguiSettings = GetDefault<UNetImguiSettings>();
	if( ParentGameViewport != GameViewport ){
		constexpr int32 NETIMGUI_WIDGET_Z_ORDER = 100000;
		ParentGameViewport		= GameViewport;
	#if WITH_EDITOR
		ParentEditorViewport	= nullptr;
	#endif
		GameViewport->AddViewportWidgetContent(SharedThis(this), NETIMGUI_WIDGET_Z_ORDER);
	}

	// Visibility when settings set to 'Activated' visibility mode, 
	// is managed here instead of in 'WantImguiInView()', otherwise the widget would never be created
	Visible &= NetImguiSettings->LocalVisibilityGame != ENetImguiVisibility::HasInput || HasInput();
	if( Visible != GetVisibility().IsVisible() ){
		SetVisibility(Visible ? EVisibility::Visible : EVisibility::Hidden);
	}
	
	//SF
	 if ( ParentGameViewport->IsFocused(ParentGameViewport->GetGameViewport()->GetViewport()) && !HasInput() )
	{
		FocusedWidgetLast	= FSlateApplication::Get().GetUserFocusedWidget(0);
	}
}

//=================================================================================================
// CONSTRUCT
//-------------------------------------------------------------------------------------------------
// Create a new NetImgui Widget
//=================================================================================================
void SNetImguiWidget::Construct(const FArguments& InArgs)
{
	ImguiContext		= ImGui::CreateContext(InArgs._FontAtlas);
	FString stringName 	= InArgs._ClientName.ToString();
	FString iniName 	= stringName + TEXT("_Imgui.ini");
	ClientName.SetNum(stringName.Len()+1);
	FTCHARToUTF8_Convert::Convert(ClientName.GetData(), ClientName.Num(), *stringName, stringName.Len()+1);
	ClientIniName.SetNum(iniName.Len()+1);
	FTCHARToUTF8_Convert::Convert(ClientIniName.GetData(), ClientIniName.Num(), *iniName, iniName.Len()+1);

	NetImguiScopedContext scopedContext(ImguiContext);
	ImGui::GetIO().IniFilename		= ClientIniName.GetData();
	ImGui::GetIO().MouseDrawCursor	= false;
	ImGui::GetIO().ConfigFlags		= ImGuiConfigFlags_DockingEnable;
	ImGui::GetIO().BackendFlags		= 0;

	NetImguiDrawers[0] = MakeShareable(new FNetImguiSlateElement());
	NetImguiDrawers[1] = MakeShareable(new FNetImguiSlateElement());
	NetImguiDrawers[2] = MakeShareable(new FNetImguiSlateElement());
}

//=================================================================================================
// ON CURSOR QUERY
//-------------------------------------------------------------------------------------------------
// Adjust the mouse cursor to the one wanted by the Dear ImGui content
//=================================================================================================
FCursorReply SNetImguiWidget::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	NetImguiScopedContext scopedContext(ImguiContext);
	EMouseCursor::Type newCursor(EMouseCursor::Type::None);
	if (ImGui::GetIO().WantCaptureMouse){
		ImGuiMouseCursor cursor = ImGui::GetMouseCursor();
		switch (cursor) {
			case ImGuiMouseCursor_Arrow: newCursor = EMouseCursor::Type::Default; break;
			case ImGuiMouseCursor_TextInput: newCursor = EMouseCursor::Type::TextEditBeam; break;		// When hovering over InputText, etc.
			case ImGuiMouseCursor_ResizeAll: newCursor = EMouseCursor::Type::CardinalCross; break;		// (Unused by Dear ImGui functions)
			case ImGuiMouseCursor_ResizeNS: newCursor = EMouseCursor::Type::ResizeUpDown; break;		// When hovering over a horizontal border
			case ImGuiMouseCursor_ResizeEW: newCursor = EMouseCursor::Type::ResizeLeftRight; break;		// When hovering over a vertical border or a column
			case ImGuiMouseCursor_ResizeNESW: newCursor = EMouseCursor::Type::ResizeSouthWest; break;	// When hovering over the bottom-left corner of a window
			case ImGuiMouseCursor_ResizeNWSE: newCursor = EMouseCursor::Type::ResizeSouthEast; break;	// When hovering over the bottom-right corner of a window
			case ImGuiMouseCursor_Hand: newCursor = EMouseCursor::Type::Hand; break;             		// (Unused by Dear ImGui functions. Use for e.g. hyperlinks)
			case ImGuiMouseCursor_NotAllowed: newCursor = EMouseCursor::Type::SlashedCircle; break;		// When hovering something with disallowed interaction. Usually a crossed circle.
			default: newCursor = EMouseCursor::Type::Default; break;
		}
	}
	return newCursor != EMouseCursor::Type::None ? FCursorReply::Cursor(newCursor) : FCursorReply::Unhandled();
}

//=================================================================================================
// INPUT HANDLING
//-------------------------------------------------------------------------------------------------
// Receive and process various inputs
//=================================================================================================
FReply SNetImguiWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	int ImguiMouse = UnrealToImguiMouseButton(MouseEvent.GetEffectingButton());
	if (ImguiMouse != -1) 
	{
		NetImguiScopedContext scopedContext(ImguiContext);
		if( ImGui::GetIO().WantCaptureMouse )
		{
			ImGui::GetIO().AddMouseButtonEvent(ImguiMouse, true);
			return FReply::Handled().LockMouseToWidget(SharedThis(this));
		}
	}
	return FReply::Unhandled();
}

FReply SNetImguiWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	int ImguiMouse = UnrealToImguiMouseButton(MouseEvent.GetEffectingButton());
	if (ImguiMouse != -1) 
	{
		//SF improve the release
		NetImguiScopedContext scopedContext(ImguiContext);
		ImGui::GetIO().AddMouseButtonEvent(ImguiMouse, false);
		return ImGui::GetIO().WantCaptureMouse ? FReply::Handled().ReleaseMouseLock() : FReply::Unhandled();
	}
	return FReply::Unhandled();
}

FReply SNetImguiWidget::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	NetImguiScopedContext scopedContext(ImguiContext);
	if (ImGui::GetIO().WantCaptureMouse) {
		ImGui::GetIO().AddMouseWheelEvent(0.f, MouseEvent.GetWheelDelta());
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply SNetImguiWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	//SF Handle Copy/Paste?
	//FGenericPlatformMisc::ClipboardCopy
	ImGuiKey imguiKey = UnrealToImguiKey(InKeyEvent.GetKey());
	if (imguiKey != ImGuiKey_None) {
		NetImguiScopedContext scopedContext(ImguiContext);
		ImGui::GetIO().AddKeyEvent(imguiKey, true);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply SNetImguiWidget::OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	ImGuiKey imguiKey = UnrealToImguiKey(InKeyEvent.GetKey());
	if (imguiKey != ImGuiKey_None) {
		NetImguiScopedContext scopedContext(ImguiContext);
		ImGui::GetIO().AddKeyEvent(imguiKey, false);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

//SF TODO: Handle Copy/Paste
FReply SNetImguiWidget::OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent)
{
	NetImguiScopedContext scopedContext(ImguiContext);
	if (ImGui::GetIO().WantCaptureKeyboard) {
		//SF TODO handle TCHar type base on platform...
		ImGui::GetIO().AddInputCharacterUTF16(InCharacterEvent.GetCharacter());
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply SNetImguiWidget::OnAnalogValueChanged(const FGeometry& MyGeometry, const FAnalogInputEvent& InAnalogInputEvent)
{
	auto KeyDetails		= EKeys::GetKeyDetails(InAnalogInputEvent.GetKey()); //SF
	float AnalogValue	= InAnalogInputEvent.GetAnalogValue();
	ImGuiKey GamepadKey = InAnalogInputEvent.GetKey() == EKeys::Gamepad_LeftX ? AnalogValue > 0 ? ImGuiKey_GamepadLStickRight : ImGuiKey_GamepadLStickLeft 
						: InAnalogInputEvent.GetKey() == EKeys::Gamepad_LeftY ? AnalogValue > 0 ? ImGuiKey_GamepadLStickUp : ImGuiKey_GamepadLStickDown 
						: InAnalogInputEvent.GetKey() == EKeys::Gamepad_RightX ? AnalogValue > 0 ? ImGuiKey_GamepadRStickRight : ImGuiKey_GamepadRStickLeft 
						: InAnalogInputEvent.GetKey() == EKeys::Gamepad_RightY ? AnalogValue > 0 ? ImGuiKey_GamepadRStickUp : ImGuiKey_GamepadRStickDown
						: InAnalogInputEvent.GetKey() == EKeys::Gamepad_LeftTriggerAxis ? ImGuiKey_GamepadL2
						: InAnalogInputEvent.GetKey() == EKeys::Gamepad_RightTriggerAxis ? ImGuiKey_GamepadR2
						: ImGuiKey_None;
	
	if (GamepadKey != ImGuiKey_None)
	{
		ImGui::GetIO().AddKeyAnalogEvent(GamepadKey, true, FMath::Abs(AnalogValue));
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

#endif // NETIMGUI_LOCALDRAW_ENABLED

#pragma optimize("", on) //SF
