#pragma once

//@SFatnassi TODO: Implement a remote input control of local imgui?

#if NETIMGUI_ENABLED || 1//SF

#if NETIMGUI_LOCALDRAW_ENABLED || 1//SF
#include "CoreMinimal.h"
#include "Containers/ResourceArray.h"	// FResourceBulkDataInterface
#include "RHIFwd.h"						// FTextureRHIRef
#include "Framework/Application/IInputProcessor.h"
#include "LocalDraw/NetImguiWidget.h"

//-------------------------------------------------------------------------------------------------
// Handling of Unreal to ImGui key mapping
//-------------------------------------------------------------------------------------------------
extern TMap<FKey, ImGuiKey> GUnrealKeyToImguiMap;
inline ImGuiKey UnrealToImguiKey(FKey key)		{ const ImGuiKey* imguiKey = GUnrealKeyToImguiMap.Find(key); return imguiKey ? *imguiKey : ImGuiKey_None; }
inline int UnrealToImguiMouseButton(FKey key)	{ ImGuiKey mouseKey = UnrealToImguiKey(key); return mouseKey != ImGuiKey_None ? (int)mouseKey - (int)ImGuiKey_MouseLeft : -1; }


//-------------------------------------------------------------------------------------------------
// Main interface handling netimgui widgets / input for local drawing
//-------------------------------------------------------------------------------------------------
class FNetImguiLocalDraw
{
public:
	FNetImguiLocalDraw();
	~FNetImguiLocalDraw();

	void						Update();
	void						ToggleActiveWidgetInput();
	void						DisableAllWidgetActivation();
	TSharedPtr<SNetImguiWidget> GetActiveViewportWidget();
	bool 						IsInputActive(const ImGuiContext* ImContext);

	//---------------------------------------------------------------------------------------------
	// User configurable callback to let the plugin know which viewport should
	// display local Dear Imgui content (when enabled). 
	// 
	// Assigning 'nullptr' will reset to the default behaviour :
	//  Enabled on Game, PIE and Editor perspective viewport
	//---------------------------------------------------------------------------------------------
#if WITH_EDITOR
	void SetWantImguiInEditorViewFN(const FWantImguiInEditorViewFN& callback);
	bool WantImguiInView(const SLevelViewport*, bool HasViewportFocus)const;
#endif
	void SetWantImguiInGameViewFN(const FWantImguiInGameViewFN& callback);
	bool WantImguiInView(const UGameViewportClient* inGameClient, bool HasInputFocus)const;

	//---------------------------------------------------------------------------------------------
	// Handling of the LocalFont texture generation and update
	//---------------------------------------------------------------------------------------------
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
	struct FLocalFontSuport
	{
		~FLocalFontSuport(){ Terminate(); }
		void Initialize();
		void Terminate();
		void Update(float wantedFontDPIScale);
		FTextureRHIRef 	TextureRef;
		FFontBulkData	TextureUpdateData;
		ImFontAtlas*	FontAtlas = nullptr;
		float			FontDPIScale = 0.f;
	};
private:
	//---------------------------------------------------------------------------------------------
	// Internal managements of LocalDrawing
	//---------------------------------------------------------------------------------------------
	void						CreateFontTexture(FRHICommandListImmediate& RHICmdList);
	TSharedPtr<SNetImguiWidget> GetNetImguiWidget(const FName& inClientName);
	TSharedPtr<SNetImguiWidget> GetOrCreateNetImguiWidget(const FName& inClientName);

	FTextureRHIRef 								BlackTexture; //SF TODO move to render file?
	FLocalFontSuport							LocalFontSupport;
	TMap<FName, TSharedPtr<SNetImguiWidget>> 	WidgetsMap; //SF change to unique?
	TMap<const ImGuiContext*, TWeakPtr<SNetImguiWidget>> 	WidgetsMap;
	TSharedPtr<IInputProcessor>					InputProcessor;
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
	bool IsInputActive(const ImGuiContext*){ return false };
};

#endif

#endif 
