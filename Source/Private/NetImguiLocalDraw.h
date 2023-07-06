#pragma once


#if NETIMGUI_ENABLED 

#if NETIMGUI_LOCALDRAW_ENABLED || 1
#include "CoreMinimal.h"
#include "Debug/DebugDrawService.h"
#include "Engine/Canvas.h"
#include "RendererInterface.h" //SF
#include "Containers/DynamicRHIResourceArray.h" //SF
//#include "Containers/ResourceArray.h" //SF


class NetImguiLocalDrawSupport
{
public:
	void Initialize();
	void Terminate();
	void InterceptInput();

private:
	struct ViewportAssociation {
		ImGuiContext* ImguiContext;
		ImVec2 ViewportPos;
		double LastUpdateTime;
	};
	struct FFontBulkData : public FResourceBulkDataInterface
	{
		void Init(const void* InData, uint32 InWidth, uint32 InHeight) 
		{ 
			Width = InWidth;
			Height = InHeight;
			Data.Append((uint8*)InData, Width*Height*4);
		}
		virtual const void* GetResourceBulkData() const override { return &Data[0]; }
		virtual uint32 GetResourceBulkDataSize() const override { return Width*Height; }
		virtual void Discard() override { Data.Reset(); Height = Width = 0; }
		TArray<uint8> Data;
		uint32 Width = 0;
		uint32 Height = 0;
	};
	
	
	void DrawOnCanvas(UCanvas* Canvas, APlayerController*);
	void CreateFontTexture(FRHICommandListImmediate& RHICmdList);

	uint32 				ActiveContextIndex;
	//FDelegateHandle 	OnDrawImguiHandle;
	FDelegateHandle 	OnDrawCanvasImguiHandle;
	FFontBulkData 		FontTextureData;
	FTextureRHIRef 		FontTexture;
	FTextureRHIRef 		BlackTexture;	
	TMap<void*, ViewportAssociation> ViewportMap;
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
