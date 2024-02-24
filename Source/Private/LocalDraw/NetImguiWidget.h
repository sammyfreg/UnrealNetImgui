#pragma once


#if NETIMGUI_LOCALDRAW_ENABLED

#include "Widgets/SLeafWidget.h"
class SNetImguiWidget : public SLeafWidget 
{
public:
	SLATE_BEGIN_ARGS(SNetImguiWidget){}
		SLATE_ARGUMENT(FName, ClientName)
		SLATE_ARGUMENT(ImFontAtlas*, FontAtlas)
	SLATE_END_ARGS()

	virtual 				~SNetImguiWidget();	
	void 					Update(class UGameViewportClient* gameViewport, bool inVisible);
	bool					ToggleActivation();
	float					GetDPIScale() const;
	inline bool				IsActivated() const { return Activated; }
	

	// Widget/Leaftwidget Interface
	void 					Construct(const FArguments& InArgs);
	virtual FReply 			OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply 			OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply 			OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply 			OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply 			OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent) override;
	virtual FReply 			OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual bool 			SupportsKeyboardFocus() const override { return true; }
	virtual void 			Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime);
	virtual int32 			OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D		ComputeDesiredSize(float) const override { return FVector2D(0.0f, 0.0f); }
	virtual FCursorReply	OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;
	inline ImGuiContext*	GetContext(){ return ImguiContext; }

protected:
	float					GetDrawVerticalOffset() const;

	TSharedPtr<class FNetImguiSlateElement, ESPMode::ThreadSafe> NetImguiDrawers[3];
	const UGameViewportClient* ParentGameViewport 	= nullptr;
	mutable int DrawCounter							= 0;
	mutable FVector4f ImguiParameters 				= FVector4f(1, 0, 0, 0);
	ImGuiContext* ImguiContext 						= nullptr;
	bool Activated									= false;
	float FontScale 								= 1.f;
	FName ClientNameID;
    TArray<char> ClientName;
	TArray<char> ClientIniName;
	friend class FNetImguiLocalDraw;

#if WITH_EDITOR
	const SLevelViewport* ParentEditorViewport		= nullptr;
public:
	void Update(class SLevelViewport* inLevelViewport, bool inVisible);
#endif
};

#endif // NETIMGUI_LOCALDRAW_ENABLED
