#pragma once


#if NETIMGUI_ENABLED 

#if NETIMGUI_LOCALDRAW_ENABLED
#include "CoreMinimal.h"
#include "Containers/ResourceArray.h"	// FResourceBulkDataInterface
#include "RHIFwd.h"						// FTextureRHIRef
#include "Framework/Application/IInputProcessor.h"
#include "LocalDraw/NetImguiWidget.h"

extern TMap<FKey, ImGuiKey> GUnrealKeyToImguiMap;

inline ImGuiKey UnrealToImguiKey(FKey key) { 
	const ImGuiKey* imguiKey = GUnrealKeyToImguiMap.Find(key); 
	return imguiKey ? *imguiKey : ImGuiKey_None;
}

inline int UnrealToImguiMouseButton(FKey key) {
	ImGuiKey mouseKey = UnrealToImguiKey(key);
	return mouseKey != ImGuiKey_None ? (int)mouseKey - (int)ImGuiKey_MouseLeft : -1;
}

class FNetImguiLocalDraw
{
public:
	FNetImguiLocalDraw();
	~FNetImguiLocalDraw();
	void Update();
	void InterceptRemoteInput();
	void ToggleWidgetActivated(TSharedPtr<SNetImguiWidget> NetImguiWidget = nullptr);
	//bool IsGameInputFocused();
	//void GameInputSet(bool Enable);
	//void GameInputToggle();
	TSharedPtr<SNetImguiWidget> GetActiveViewportWidget();

	/**	
	* User configurable callback to let the plugin know which viewport should
	* display local Dear Imgui content (when enabled). 
	* 
	* Assigning 'nullptr' will reset to the default behaviour :
	* Enabled on Game, PIE and Editor perspective viewport.
	*/
#if WITH_EDITOR
	void SetWantImguiInEditorViewFN(const FWantImguiInEditorViewFN& callback);
	bool WantImguiInView(const SLevelViewport*, bool HasViewportFocus)const;
#endif
	void SetWantImguiInGameViewFN(const FWantImguiInGameViewFN& callback);
	bool WantImguiInView(const UGameViewportClient* inGameClient, bool HasInputFocus)const;

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

public://SF TEMP
	TSharedPtr<SNetImguiWidget> GetNetImguiWidget(const FName& inClientName);
	TSharedPtr<SNetImguiWidget> GetOrCreateNetImguiWidget(const FName& inClientName);

	struct FFontSuport
	{
		~FFontSuport(){ Terminate(); }
		void Initialize();
		void Terminate();
		void Update(float wantedFontDPIScale);
		FTextureRHIRef 	TextureRef;
		FFontBulkData	TextureUpdateData;
		ImFontAtlas*	FontAtlas = nullptr;
		float			FontDPIScale = 0.f;
	};

protected:
	FTextureRHIRef 								BlackTexture; //SF TODO move to render file?
	FFontSuport									FontSupport;
	TMap<FName, TSharedPtr<SNetImguiWidget>> 	WidgetsMap;
	TMap<FKey, ImGuiKey> 						UnrealKeyToImguiMap;
	TSharedPtr<IInputProcessor>					InputProcessor;
	TWeakPtr<SWidget>							FocusedWidgetLast;		// Keep track of the last non NetImguiWidget focused, to restore input to it when toggled
	TWeakPtr<SNetImguiWidget>					FocusedWidgetNetImgui;
	FWantImguiInGameViewFN						WantImguiInGameViewFN;
#if WITH_EDITOR
	FWantImguiInEditorViewFN					WantImguiInEditorViewFN;
#endif
};
#else

// Local draw disabled, decalre empty class
class FNetImguiLocalDraw
{
public:
	void Update(){};
	void InterceptRemoteInput(){};
};

#endif

#endif 
