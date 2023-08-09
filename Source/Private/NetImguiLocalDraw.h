#pragma once


#if NETIMGUI_ENABLED 

#if NETIMGUI_LOCALDRAW_ENABLED || 1
#include "CoreMinimal.h"
#include "Debug/DebugDrawService.h"
#include "Engine/Canvas.h"
#include "Slate/Public/Framework/Application/IInputProcessor.h"
#include "InputCoreTypes.h" // FKey
#include "RendererInterface.h" //SF
#include "Containers/DynamicRHIResourceArray.h" //SF


class FNetImguiInputProcessor : public IInputProcessor
{
public:
	FNetImguiInputProcessor(class NetImguiLocalDrawSupport* owner);
	//virtual ~FNetImguiInputProcessor(){}
	//virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent);
	//virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent);
	//virtual bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) { return false; }
	//virtual bool HandleMotionDetectedEvent(FSlateApplication& SlateApp, const FMotionEvent& MotionEvent) { return false; };
	//virtual bool HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent) { return false; }
	virtual const TCHAR* GetDebugName() const { return TEXT("NetImguiInput"); }

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor);
	virtual bool HandleMouseButtonDownEvent( FSlateApplication& SlateApp, const FPointerEvent& MouseEvent);
	virtual bool HandleMouseButtonUpEvent( FSlateApplication& SlateApp, const FPointerEvent& MouseEvent);
	virtual bool HandleMouseButtonDoubleClickEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent);
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
	//bool HandleKey(const FKeyEvent& InKeyEvent, bool InKeyDown);
	bool HandleMouse(const FPointerEvent& InMouseEvent, bool InMouseDown);
	class NetImguiLocalDrawSupport* Owner = nullptr;
	uint32 PassThroughMouseMask = 0;
	TMap<FKey, ImGuiKey> UnrealKeyToImguiMap;
	EMouseCursor::Type MouseCursorOverride;
};

class NetImguiLocalDrawSupport
{
public:
	void Initialize();
	void Terminate();
	void Update();
	void InterceptInput();

private:	
	struct FFontBulkData : public FResourceBulkDataInterface
	{
		void Init(const void* InData, uint32 InWidth, uint32 InHeight);
		virtual const void* GetResourceBulkData() const override { return &Data[0]; }
		virtual uint32 GetResourceBulkDataSize() const override { return Width*Height; }
		virtual void Discard() override { Data.Reset(); Height = Width = 0; }
		TArray<uint8> Data;
		uint32 Width = 0;
		uint32 Height = 0;
	};

	void CreateFontTexture(FRHICommandListImmediate& RHICmdList);
	void AddWidgetToViewport();
	void AddWidgetToViewport(class UGameViewportClient* gameViewport);
	void AddWidgetToViewport(class SLevelViewport* editorViewport);
	
	class UGameViewportClient* GameViewport; //SF REMOVE ME
	FTextureRHIRef 			FontTexture;
	FTextureRHIRef 			BlackTexture;
	FFontBulkData 			FontTextureData;
	
	
public: //SF TEMP cleanup...
	TSharedPtr<FNetImguiInputProcessor> 			InputProcessor;
	TMap<void*, TSharedPtr<class SNetImguiWidget>> 	WidgetsMap;
	TSharedPtr<class SNetImguiWidget> FocusedWidget;
};
#else

// Local draw disabled, decalre empty class
class NetImguiLocalDrawSupport
{
public:
	void Initialize(){};	
	void Terminate(){};
	void InterceptInput(){};
};

#endif

#endif 
