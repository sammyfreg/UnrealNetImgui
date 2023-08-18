// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#if 0
#include "NetImguiModule.h"
#include "CoreMinimal.h"

#if NETIMGUI_LOCALDRAW_ENABLED || 1 //SF Find solution to this. Using  1 to see code in editor

#include "StaticMeshResources.h"
#include "CanvasRender.h"
#include "SceneView.h"
#include "UnrealClient.h"
#include "EngineGlobals.h"
#include "RenderGraphUtils.h"
#include "ShaderParameterUtils.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "SceneView.h"
#include "SceneRenderTargetParameters.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "RHIStaticStates.h"
#include "MeshPassProcessor.h"
#include "SceneInterface.h"
#include "Engine/GameViewportClient.h"
#include "Slate/Public/Framework/Application/SlateApplication.h"
#include "Slate/SceneViewport.h"
#include "Widgets/SLeafWidget.h"

#if WITH_EDITOR
#include "Editor.h"
#include "LevelEditorViewport.h"
#include "SLevelViewport.h"
#include "LevelEditor.h"
#endif

#include "../NetImguiLocalDraw.h"

#pragma optimize("", off) //SF

FNetImguiInputProcessor::FNetImguiInputProcessor(class NetImguiLocalDrawSupport* owner)
: Owner(owner)
{
	struct UnrealToImguiKeyPair { FKey UnrealKey; ImGuiKey ImguiKey; };
	static const UnrealToImguiKeyPair KeysMapping[] = {
		// Mouse buttons
		{EKeys::LeftMouseButton, ImGuiKey_MouseLeft},
		{EKeys::RightMouseButton, ImGuiKey_MouseRight},
		{EKeys::MiddleMouseButton, ImGuiKey_MouseMiddle},
		{EKeys::ThumbMouseButton, ImGuiKey_MouseX1},
		{EKeys::ThumbMouseButton2, ImGuiKey_MouseX2},

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
		{EKeys::Zero, ImGuiKey_0}, {EKeys::One, ImGuiKey_1}, {EKeys::Two, ImGuiKey_2},   {EKeys::Three, ImGuiKey_3}, {EKeys::Four, ImGuiKey_4}, 
		{EKeys::Five, ImGuiKey_5}, {EKeys::Six, ImGuiKey_6}, {EKeys::Seven, ImGuiKey_7}, {EKeys::Eight, ImGuiKey_8}, {EKeys::Nine, ImGuiKey_9},

		{EKeys::A, ImGuiKey_A}, {EKeys::B, ImGuiKey_B}, {EKeys::C, ImGuiKey_C}, {EKeys::D, ImGuiKey_D}, {EKeys::E, ImGuiKey_E}, {EKeys::F, ImGuiKey_F}, 
		{EKeys::G, ImGuiKey_G}, {EKeys::H, ImGuiKey_H}, {EKeys::I, ImGuiKey_I}, {EKeys::J, ImGuiKey_J},	{EKeys::K, ImGuiKey_K}, {EKeys::L, ImGuiKey_L}, 
		{EKeys::M, ImGuiKey_M}, {EKeys::N, ImGuiKey_N}, {EKeys::O, ImGuiKey_O}, {EKeys::P, ImGuiKey_P}, {EKeys::Q, ImGuiKey_Q}, {EKeys::R, ImGuiKey_R}, 
		{EKeys::S, ImGuiKey_S}, {EKeys::T, ImGuiKey_T},	{EKeys::U, ImGuiKey_U}, {EKeys::V, ImGuiKey_V}, {EKeys::W, ImGuiKey_W}, {EKeys::X, ImGuiKey_X}, 
		{EKeys::Y, ImGuiKey_Y}, {EKeys::Z, ImGuiKey_Z},

		{EKeys::F1, ImGuiKey_F1}, {EKeys::F2, ImGuiKey_F2}, {EKeys::F3, ImGuiKey_F3}, {EKeys::F4, ImGuiKey_F4}, {EKeys::F5, ImGuiKey_F5}, 
		{EKeys::F6, ImGuiKey_F6}, {EKeys::F7, ImGuiKey_F7}, {EKeys::F8, ImGuiKey_F8}, {EKeys::F9, ImGuiKey_F9}, {EKeys::F10, ImGuiKey_F10}, 
		{EKeys::F11, ImGuiKey_F11}, {EKeys::F12, ImGuiKey_F12},
		/*
			ImGuiKey_Apostrophe,        // '
			ImGuiKey_Comma,             // ,
			ImGuiKey_Minus,             // -
			ImGuiKey_Period,            // .
			ImGuiKey_Slash,             // /
			ImGuiKey_Semicolon,         // ;
			ImGuiKey_Equal,             // =
			ImGuiKey_LeftBracket,       // [
			ImGuiKey_Backslash,         // \ (this text inhibit multiline comment caused by backslash)
			ImGuiKey_RightBracket,      // ]
			ImGuiKey_GraveAccent,       // `
			ImGuiKey_CapsLock,
			ImGuiKey_ScrollLock,
			ImGuiKey_NumLock,
			ImGuiKey_PrintScreen,
			ImGuiKey_Pause,
			ImGuiKey_Keypad0, ImGuiKey_Keypad1, ImGuiKey_Keypad2, ImGuiKey_Keypad3, ImGuiKey_Keypad4,
			ImGuiKey_Keypad5, ImGuiKey_Keypad6, ImGuiKey_Keypad7, ImGuiKey_Keypad8, ImGuiKey_Keypad9,
			ImGuiKey_KeypadDecimal,
			ImGuiKey_KeypadDivide,
			ImGuiKey_KeypadMultiply,
			ImGuiKey_KeypadSubtract,
			ImGuiKey_KeypadAdd,
			ImGuiKey_KeypadEnter,
			ImGuiKey_KeypadEqual,
			*/
	};

	for (size_t i(0); i<UE_ARRAY_COUNT(KeysMapping); ++i) {
		UnrealKeyToImguiMap.Add(KeysMapping[i].UnrealKey, KeysMapping[i].ImguiKey);
	}
}
void FNetImguiInputProcessor::Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor)
{
#if 0
	for (auto& pair : Owner->WidgetsMap) {
		ScopedContext scopedContext(pair.Value->ImguiContext);
		if (ImGui::GetIO().WantCaptureMouse){
			ImGuiMouseCursor cursor = ImGui::GetMouseCursor();
			switch (cursor) {
				case ImGuiMouseCursor_Arrow: Cursor->SetType(EMouseCursor::Type::Default); break;
				case ImGuiMouseCursor_TextInput: Cursor->SetType(EMouseCursor::Type::TextEditBeam); break;		// When hovering over InputText, etc.
				case ImGuiMouseCursor_ResizeAll: Cursor->SetType(EMouseCursor::Type::CardinalCross); break;		// (Unused by Dear ImGui functions)
				case ImGuiMouseCursor_ResizeNS: Cursor->SetType(EMouseCursor::Type::ResizeUpDown); break;		// When hovering over a horizontal border
				case ImGuiMouseCursor_ResizeEW: Cursor->SetType(EMouseCursor::Type::ResizeLeftRight); break;	// When hovering over a vertical border or a column
				case ImGuiMouseCursor_ResizeNESW: Cursor->SetType(EMouseCursor::Type::ResizeSouthWest); break;	// When hovering over the bottom-left corner of a window
				case ImGuiMouseCursor_ResizeNWSE: Cursor->SetType(EMouseCursor::Type::ResizeSouthEast); break;	// When hovering over the bottom-right corner of a window
				case ImGuiMouseCursor_Hand: Cursor->SetType(EMouseCursor::Type::Hand); break;             		// (Unused by Dear ImGui functions. Use for e.g. hyperlinks)
				case ImGuiMouseCursor_NotAllowed: Cursor->SetType(EMouseCursor::Type::SlashedCircle); break;	// When hovering something with disallowed interaction. Usually a crossed circle.
				default: Cursor->SetType(EMouseCursor::Type::Default); break;
			}
		}
		if (ImGui::GetIO().WantCaptureKeyboard){
			SlateApp.SetKeyboardFocus(pair.Value, EFocusCause::SetDirectly);
		}
	}
#endif
}

bool FNetImguiInputProcessor::HandleMouseButton(const FPointerEvent& InMouseEvent, bool InMouseDown)
{
	bool captured(false);
	int imguiMouseBtn = UnrealToImguiMouseButton(InMouseEvent.GetEffectingButton());
	if (imguiMouseBtn != -1) {
		uint32 mouseBtnMask(1 << imguiMouseBtn);
		for (auto& pair : Owner->WidgetsMap) {
			ScopedContext scopedContext(pair.Value->ImguiContext);
			ImGui::GetIO().AddMouseButtonEvent(imguiMouseBtn, InMouseDown);
			PassThroughMouseMask |= InMouseDown && ImGui::GetIO().WantCaptureMouse && (Owner->FocusedWidget != pair.Value) ? mouseBtnMask : 0;
			captured |= ImGui::GetIO().WantCaptureMouse;
		}
		captured				&= (PassThroughMouseMask & mouseBtnMask) == 0;	// Let mouse down/up events through when clicking in new unreal window (to let it become active)
		PassThroughMouseMask	&= !InMouseDown ? ~mouseBtnMask : ~0;			// Cancel the mouse passtrough
	}
	return captured;
}

bool FNetImguiInputProcessor::HandleMouseButtonUpEvent( FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{	
	HandleMouseButton(MouseEvent, false);
	return false;
}

/*
bool FNetImguiInputProcessor::HandleMouseButtonDownEvent( FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	return HandleMouseButton(MouseEvent, true);
}

bool FNetImguiInputProcessor::HandleMouseButtonDoubleClickEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	return HandleMouseButton(MouseEvent, true);
}
*/
bool FNetImguiInputProcessor::HandleMouseWheelOrGestureEvent(FSlateApplication& SlateApp, const FPointerEvent& InWheelEvent, const FPointerEvent* InGestureEvent)
{
	bool captured(false);
	for (auto& pair : Owner->WidgetsMap) {
		ScopedContext scopedContext(pair.Value->ImguiContext);
		if (ImGui::GetIO().WantCaptureMouse) {
			ImGui::GetIO().AddMouseWheelEvent(0.f, InWheelEvent.GetWheelDelta());
			captured = true;
		}
	}
	return captured;
}
#endif // NETIMGUI_LOCALDRAW_ENABLED

#pragma optimize("", on) //SF
#endif
