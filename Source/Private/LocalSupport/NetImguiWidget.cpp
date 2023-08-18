// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "NetImguiModule.h"
#include "CoreMinimal.h"

#if NETIMGUI_LOCALDRAW_ENABLED || 1 //SF Find solution to this. Using  1 to see code in editor
#include "Engine/GameViewportClient.h"
#include "Slate/Public/Framework/Application/SlateApplication.h"
#include "NetImguiRender.h"

#pragma optimize("", off) //SF


SNetImguiWidget::~SNetImguiWidget()
{
	ImGui::DestroyContext(ImguiContext);
};

void SNetImguiWidget::Update(SLevelViewport* inLevelViewport, bool inVisible, bool isFocused, bool inHighlight, float inDPIScale)
{
	check(IsEditorWindow && inLevelViewport);
	if( ParentEditorViewport != inLevelViewport ){
		ParentEditorViewport = inLevelViewport;
		inLevelViewport->AddOverlayWidget(SharedThis(this));
	}
	Update(inVisible, isFocused, inHighlight, inDPIScale);
}

void SNetImguiWidget::Update(UGameViewportClient* inGameViewport, bool inVisible, bool isFocused, bool inHighlight, float inDPIScale)
{
	check(!IsEditorWindow && inGameViewport);
	if( ParentGameViewport != inGameViewport ){
		constexpr int32 IMGUI_WIDGET_Z_ORDER = 10000;
		ParentGameViewport = inGameViewport;
		inGameViewport->AddViewportWidgetContent(SharedThis(this), IMGUI_WIDGET_Z_ORDER);
	}	
	Update(inVisible, isFocused, inHighlight, inDPIScale);
}

void SNetImguiWidget::Update(bool inVisible, bool isFocused, bool inHighlight, float inDPIScale)
{
	inVisible 				&= !HideUnfocusedWindow || isFocused;
	DPIScale				= inDPIScale;
	EVisibility visibility	= GetVisibility();
	if( inVisible && !visibility.IsVisible() ){
		SetVisibility(EVisibility::Visible);
	}
	else if( !inVisible && visibility.IsVisible())
	{
		SetVisibility(EVisibility::Hidden);
	}
}

void SNetImguiWidget::Construct(const FArguments& InArgs)
{
	IsEditorWindow			= InArgs._IsEditorWindow;
	ImguiContext			= ImGui::CreateContext(ImGui::GetIO().Fonts);
	VerticalDisplayOffset	= IsEditorWindow ? 32.f : 0.f;	
	ClientNameID			= InArgs._ClientName;
	HideUnfocusedWindow		= IsEditorWindow; //SF Add config param?
	FString stringName 		= InArgs._ClientName.ToString();
	FString iniName 		= stringName + TEXT("_Imgui.ini");
	ClientName.SetNum(stringName.Len()+1);
	FTCHARToUTF8_Convert::Convert(ClientName.GetData(), ClientName.Num(), *stringName, stringName.Len()+1);
	ClientIniName.SetNum(iniName.Len()+1);
	FTCHARToUTF8_Convert::Convert(ClientIniName.GetData(), ClientIniName.Num(), *iniName, iniName.Len()+1);

	ScopedContext scopedContext(ImguiContext);
	ImGui::GetIO().IniFilename = ClientIniName.GetData();
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
	ImGui::GetIO().MouseDrawCursor = false;
	NetImguiDrawers[0] = MakeShareable(new FNetImguiSlateElement());
	NetImguiDrawers[1] = MakeShareable(new FNetImguiSlateElement());
	NetImguiDrawers[2] = MakeShareable(new FNetImguiSlateElement());
}

FCursorReply SNetImguiWidget::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	ScopedContext scopedContext(ImguiContext);
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

FReply SNetImguiWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	ScopedContext scopedContext(ImguiContext);
	int imguiMouse = UnrealToImguiMouseButton(MouseEvent.GetEffectingButton());
	if (imguiMouse != -1) {
		ImGui::GetIO().AddMouseButtonEvent(imguiMouse, true);
		return ImGui::GetIO().WantCaptureMouse ? FReply::Handled().LockMouseToWidget(SharedThis(this)) : FReply::Unhandled();
	}
	return FReply::Unhandled();
}

FReply SNetImguiWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	ScopedContext scopedContext(ImguiContext);
	int imguiMouse = UnrealToImguiMouseButton(MouseEvent.GetEffectingButton());
	if (imguiMouse != -1) {
		ImGui::GetIO().AddMouseButtonEvent(imguiMouse, false);
		return ImGui::GetIO().WantCaptureMouse ? FReply::Handled().ReleaseMouseLock() : FReply::Unhandled();
	}
	return FReply::Unhandled();
}

FReply SNetImguiWidget::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	ScopedContext scopedContext(ImguiContext);
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
	ScopedContext scopedContext(ImguiContext);
	ImGuiKey imguiKey = UnrealToImguiKey(InKeyEvent.GetKey());
	if (imguiKey != ImGuiKey_None) {
		ImGui::GetIO().AddKeyEvent(imguiKey, true);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply SNetImguiWidget::OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	ScopedContext scopedContext(ImguiContext);
	ImGuiKey imguiKey = UnrealToImguiKey(InKeyEvent.GetKey());
	if (imguiKey != ImGuiKey_None) {
		ImGui::GetIO().AddKeyEvent(imguiKey, false);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

//SF TODO: Handle Copy/Paste
FReply SNetImguiWidget::OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent)
{
	ScopedContext scopedContext(ImguiContext);
	if (ImGui::GetIO().WantCaptureKeyboard) {
		//SF TODO handle TCHar type base on platform...
		ImGui::GetIO().AddInputCharacterUTF16(InCharacterEvent.GetCharacter());
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

void SNetImguiWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if( GetVisibility().IsVisible() ){
		ScopedContext scopedContext(ImguiContext);
		ImGuiIO& io = ImGui::GetIO();
		SetVisibility(io.WantCaptureMouse ? EVisibility::Visible : EVisibility::HitTestInvisible);

		const FSlateRenderTransform screenToImguiCoord = AllottedGeometry.GetAccumulatedRenderTransform();
		FVector2f mousePos 	= screenToImguiCoord.Inverse().TransformPoint(FSlateApplication::Get().GetCursorPos());
		mousePos.Y			-= VerticalDisplayOffset;
		mousePos 			*= AllottedGeometry.Scale;
		io.AddMousePosEvent(mousePos.X, mousePos.Y);
		io.DeltaTime		= InDeltaTime; //SF sceneView->Family->Time.GetDeltaRealTimeSeconds();  //FGameTime Time = Canvas->GetTime();	
		io.FontGlobalScale 	= FontScale;
		//SetFlag(IO.ConfigFlags, ImGuiConfigFlags_NavEnableKeyboard, InputState.IsKeyboardNavigationEnabled());
		//SetFlag(IO.ConfigFlags, ImGuiConfigFlags_NavEnableGamepad, InputState.IsGamepadNavigationEnabled());
		//SetFlag(IO.BackendFlags, ImGuiBackendFlags_HasGamepad, InputState.HasGamepad());

		ImGui::NewFrame();
		ImGui::GetWindowViewport()->DpiScale = DPIScale;
	
		if( ImGui::BeginMainMenuBar() ){
			if (ImGui::BeginMenu("NetImgui")) {
				//SF remove?
				ImGui::MenuItem("Auto-Hide", nullptr, &HideUnfocusedWindow, true);
				if( ImGui::IsItemHovered() ){
					ImGui::SetTooltip("If we should hide the UI content when viewport lose its focus");
				}

				ImGui::SliderFloat("Opacity", &ImguiParameters.X, 0.1f, 1.f);
				ImGui::SliderFloat("Font scale", &FontScale, 0.5f, 2.f);

				//SF TODO remove this DPI toggle
				static bool test(true);
				if (ImGui::MenuItem("DPI", nullptr, &test)) {
					int dpiMask = ImGuiConfigFlags_DpiEnableScaleViewports | ImGuiConfigFlags_DpiEnableScaleFonts;
					ImGui::GetIO().ConfigFlags &= ~dpiMask;
					ImGui::GetIO().ConfigFlags |= (test ? dpiMask : 0);
				}
				ImGui::EndMenu();
			}
			//SF test
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
		ImGui::SetNextWindowPos(ImVec2(0,0), ImGuiCond_Once);
		ImGui::ShowDemoWindow(nullptr);
		ImGui::Render();
	}
}

int32 SNetImguiWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	ScopedContext scopedContext(ImguiContext);
	//const SWindow* windowDrawn	= OutDrawElements.GetPaintWindow();
	//float dpiScale				= windowDrawn ? windowDrawn->GetDPIScaleFactor() : 1.f;
	FSlateRect imguiRect 		= MyCullingRect;
	imguiRect.Top 				+= VerticalDisplayOffset * DPIScale;
	ImGui::GetIO().DisplaySize	= ImVec2(imguiRect.GetSize2f().X, imguiRect.GetSize2f().Y);

	//---------------------------------------------------------------------------------------------
	// Add DebugDraw Item for the 'Dear ImGui' content of this viewport
	//---------------------------------------------------------------------------------------------
	TSharedPtr<FNetImguiSlateElement, ESPMode::ThreadSafe> NetImguiDrawer = NetImguiDrawers[DrawCounter++ % 3];
	if (NetImguiDrawer->Update(GFontTexture, GFontTexture, ImguiContext, imguiRect, ImguiParameters)) {
		FSlateDrawElement::MakeCustom(OutDrawElements, LayerId++, NetImguiDrawer);
	}
	
	return LayerId;
}

#endif // NETIMGUI_LOCALDRAW_ENABLED

#pragma optimize("", on) //SF
