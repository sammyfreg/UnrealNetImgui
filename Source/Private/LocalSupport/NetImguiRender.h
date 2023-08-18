#pragma once

#if NETIMGUI_LOCALDRAW_ENABLED || 1 //SF Find solution to this. Using  1 to see code in editor

#include "StaticMeshResources.h" // TResourceArray

struct LocalImguiData
{
	using ResArrayIndex 	= TResourceArray<uint8, INDEXBUFFER_ALIGNMENT>;
	using ResArrayVertex	= TResourceArray<ImDrawVert, VERTEXBUFFER_ALIGNMENT>;
	struct DrawList{
		TArray<ImDrawCmd>   CmdBuffer; 	// Draw commands. Typically 1 command = 1 GPU draw call, unless the command is a callback.
		ImDrawListFlags  	Flags;		// Flags, you may poke into these to adjust anti-aliasing settings per-primitive.
        uint32              IndexOffset;
		uint32 				VertexOffset;
		uint32 				VertexCount;
	};
	int             	TotalIdxCount;		// For convenience, sum of all ImDrawList's IdxBuffer.Size
	int             	TotalVtxCount;		// For convenience, sum of all ImDrawList's VtxBuffer.Size
	ImVec2          	DisplayPos;			// Top-left position of the viewport to render (== top-left of the orthogonal projection matrix to use) (== GetMainViewport()->Pos for the main viewport, == (0.0) in most single-viewport applications)
	ImVec2          	DisplaySize;		// Size of the viewport to render (== GetMainViewport()->Size for the main viewport, == io.DisplaySize in most single-viewport applications)
	ImVec2          	FramebufferScale;	// Amount of pixels for each unit of DisplaySize. Based on io.DisplayFramebufferScale. Generally (1,1) on normal display, (2,2) on OSX with Retina display.
	TArray<DrawList>	CmdLists;           // Array of ImDrawList* to render. The ImDrawList are owned by ImGuiContext and only pointed to from here.
	ResArrayIndex 		IdxBuffer;			// Index buffer. Each command consume ImDrawCmd::ElemCount of those
	ResArrayVertex 		VtxBuffer;           // Vertex buffer.
};

class FNetImguiSlateElement : public ICustomSlateElement
{
public:
					FNetImguiSlateElement(){};
	bool 			Update(FTextureRHIRef fontTexture, FTextureRHIRef blackTexture, ImGuiContext* pContext, const FSlateRect& cullingRect, const FVector4f& imguiParameters);
	virtual void 	DrawRenderThread(FRHICommandListImmediate& RHICmdList, const void* RenderTarget) override;

private:
	LocalImguiData	mLocalDrawData;
	FTextureRHIRef 	mFontTexture; 
	FTextureRHIRef 	mBlackTexture;
	FSlateRect 		CullingRect;
	FVector4f 		ImguiParams;
};

#endif // NETIMGUI_LOCALDRAW_ENABLED
