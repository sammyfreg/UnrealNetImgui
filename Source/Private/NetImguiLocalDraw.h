#pragma once


#if NETIMGUI_ENABLED 

#if NETIMGUI_LOCALDRAW_ENABLED || 1
#include "CoreMinimal.h"
#include "Containers/ResourceArray.h" // FResourceBulkDataInterface
#include "RHIFwd.h" // FTextureRHIRef
#include "Framework/Application/IInputProcessor.h"
#include "LocalSupport/NetImguiInput.h"
#include "LocalSupport/NetImguiWidget.h"

extern TMap<FKey, ImGuiKey> GUnrealKeyToImguiMap;

inline ImGuiKey UnrealToImguiKey(FKey key) { 
	const ImGuiKey* imguiKey = GUnrealKeyToImguiMap.Find(key); 
	return imguiKey ? *imguiKey : ImGuiKey_None;
}

inline int UnrealToImguiMouseButton(FKey key) {
	ImGuiKey mouseKey = UnrealToImguiKey(key);
	return mouseKey != ImGuiKey_None ? (int)mouseKey - (int)ImGuiKey_MouseLeft : -1;
}

class ScopedContext
{
public:
	ScopedContext(ImGuiContext* scopedContext) : SavedContext(ImGui::GetCurrentContext()) { ImGui::SetCurrentContext(scopedContext); }
	~ScopedContext() { ImGui::SetCurrentContext(SavedContext); }
	ImGuiContext* SavedContext;
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
public://SF TEMP
	TSharedPtr<SNetImguiWidget>* GetOrCreateNetImguiWidget(const FName& inClientName, bool isEditorViewport, bool inAutoCreate);
private:
	FTextureRHIRef 	FontTexture;
	FTextureRHIRef 	BlackTexture; //SF TODO move to render file?
	FFontBulkData 	FontTextureData;
	
protected:
	
	TMap<FName, TSharedPtr<SNetImguiWidget>> 	WidgetsMap;
	TMap<FKey, ImGuiKey> 						UnrealKeyToImguiMap;
	TSharedPtr<IInputProcessor>					InputProcessor;
};
#else

// Local draw disabled, decalre empty class
class NetImguiLocalDrawSupport
{
public:
	void Initialize(){};	
	void Terminate(){};
    void Update(){};
	void InterceptInput(){};
};

#endif

#endif 
