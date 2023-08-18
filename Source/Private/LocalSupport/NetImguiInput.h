#pragma once


#if NETIMGUI_LOCALDRAW_ENABLED || 1 //SF Find solution to this. Using  1 to see code in editor

#if 0
#include "CoreMinimal.h"
#include "Debug/DebugDrawService.h"
#include "Engine/Canvas.h"
#include "Slate/Public/Framework/Application/IInputProcessor.h"
#include "InputCoreTypes.h" // FKey

class FNetImguiInputProcessor : public IInputProcessor
{
public:
	FNetImguiInputProcessor(class NetImguiLocalDrawSupport* owner);
	virtual const TCHAR* GetDebugName() const { return TEXT("NetImguiInput"); }
	// Handle mouse events globally instead of inside the NetImguiWidget
	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor);
	//virtual bool HandleMouseButtonDownEvent( FSlateApplication& SlateApp, const FPointerEvent& MouseEvent);
	virtual bool HandleMouseButtonUpEvent( FSlateApplication& SlateApp, const FPointerEvent& MouseEvent);
	//virtual bool HandleMouseButtonDoubleClickEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent);
	virtual bool HandleMouseWheelOrGestureEvent(FSlateApplication& SlateApp, const FPointerEvent& InWheelEvent, const FPointerEvent* InGestureEvent);
	
	inline ImGuiKey UnrealToImguiKey(FKey key)const { 
		const ImGuiKey* imguiKey = UnrealKeyToImguiMap.Find(key); 
		return imguiKey ? *imguiKey : ImGuiKey_None;
	}
	inline int UnrealToImguiMouseButton(FKey key)const {
		ImGuiKey mouseKey = UnrealToImguiKey(key);
		return mouseKey != ImGuiKey_None ? (int)mouseKey - (int)ImGuiKey_MouseLeft : -1;
	}

protected:
	bool HandleMouseButton(const FPointerEvent& InMouseEvent, bool InMouseDown);
	class NetImguiLocalDrawSupport* Owner = nullptr;
	uint32 PassThroughMouseMask = 0;
	TMap<FKey, ImGuiKey> UnrealKeyToImguiMap;
	EMouseCursor::Type MouseCursorOverride;
};

#endif

#endif // NETIMGUI_LOCALDRAW_ENABLED
