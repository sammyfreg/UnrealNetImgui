#pragma once


#if NETIMGUI_LOCALDRAW_ENABLED || 1 //SF Find solution to this. Using  1 to see code in editor

#include "Widgets/SLeafWidget.h"

// STestFunctionWidget
class SNetImguiWidget : public SLeafWidget 
{
public:
	SLATE_BEGIN_ARGS(SNetImguiWidget){}
		SLATE_ARGUMENT(FName, ClientName)
		SLATE_ARGUMENT(bool, IsEditorWindow)
	SLATE_END_ARGS()

	virtual 		~SNetImguiWidget();
	void 			Update(class SLevelViewport* inLevelViewport, bool inVisible, bool isFocused, bool inHighlight, float inDPIScale);
	void 			Update(class UGameViewportClient* gameViewport, bool inVisible, bool isFocused, bool inHighlight, float inDPIScale);

	// Widget/Leaftwidget Interface
	void 			Construct(const FArguments& InArgs);
	virtual FReply 	OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply 	OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply 	OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply 	OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply 	OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent) override;
	virtual FReply 	OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual bool 	SupportsKeyboardFocus() const override { return true; }
	virtual void 	Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime);
	virtual int32 	OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(0.0f, 0.0f); }
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;

protected:
	void 			Update(bool inVisible, bool isFocused, bool inHighlight, float inDPIScale);
	TSharedPtr<class FNetImguiSlateElement, ESPMode::ThreadSafe> NetImguiDrawers[3];
	const SLevelViewport* ParentEditorViewport		= nullptr;
	const UGameViewportClient* ParentGameViewport 	= nullptr;
	bool IsEditorWindow 							= false;
	float VerticalDisplayOffset 					= 0.f;	// Used to avoid drawing over Editor tool icons at the top of the viewport
	mutable int DrawCounter							= 0;
	mutable FVector4f ImguiParameters 				= FVector4f(1, 0, 0, 0);
	ImGuiContext* ImguiContext 						= nullptr;
    uint32 ClientID                                 = 0;
	float FontScale 								= 1.f;
	float DPIScale									= 1.f;
	bool HideUnfocusedWindow						= true; //SF TODO constant default value
	FName ClientNameID;
    TArray<char> ClientName;
	TArray<char> ClientIniName;
};

#endif // NETIMGUI_LOCALDRAW_ENABLED
