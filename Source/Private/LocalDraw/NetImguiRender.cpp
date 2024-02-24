// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "NetImguiModule.h"
#include "CoreMinimal.h"

#if NETIMGUI_LOCALDRAW_ENABLED || 1 //SF Find solution to this. Using  1 to see code in editor

#include "NetImguiRender.h"
#include "ShaderParameterUtils.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "RHIStaticStates.h" // TStaticRasterizerState
//#include "Engine/Texture2D.h"

#include "CommonRenderResources.h"

#pragma optimize("", off) //SF

class FImGuiVertexDeclaration : public FRenderResource
{
public:
	FVertexDeclarationRHIRef VertexDeclarationRHI;
	virtual ~FImGuiVertexDeclaration() {}
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 3
	virtual void InitRHI(FRHICommandListBase& RHICmdList) override
#else
	virtual void InitRHI() override
#endif
	{
		FVertexDeclarationElementList Elements;
		uint16 Stride = sizeof(ImDrawVert);
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(ImDrawVert, pos), VET_Float2, 0, Stride));
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(ImDrawVert, uv), VET_Float2, 1, Stride));
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(ImDrawVert, col), VET_UByte4N, 2, Stride));
		VertexDeclarationRHI = PipelineStateCache::GetOrCreateVertexDeclaration(Elements);
	}

	virtual void ReleaseRHI()
	{
		VertexDeclarationRHI.SafeRelease();
	}
};
TGlobalResource<FImGuiVertexDeclaration> GImguiVertexDeclaration;

class FDearImguiVS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FDearImguiVS);
public:	
	FDearImguiVS() = default;
	FDearImguiVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
	: FGlobalShader(Initializer)
	{
		ImguiViewProjection.Bind(Initializer.ParameterMap, TEXT("ImguiViewProjection"));
		ImguiParameters.Bind(Initializer.ParameterMap, TEXT("ImguiParameters"));
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters&){ return true;}
	LAYOUT_FIELD(FShaderParameter, ImguiViewProjection)
	LAYOUT_FIELD(FShaderParameter, ImguiParameters)
};

class FDearImguiPS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FDearImguiPS);
public:
	FDearImguiPS() = default;
	FDearImguiPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
	: FGlobalShader(Initializer)
	{
		ImguiTexture.Bind(Initializer.ParameterMap, TEXT("ImguiTexture"));
		ImguiSampler.Bind(Initializer.ParameterMap, TEXT("ImguiSampler"));
	}
	
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters){return true;}
	LAYOUT_FIELD(FShaderResourceParameter, ImguiTexture);
	LAYOUT_FIELD(FShaderResourceParameter, ImguiSampler);
};

IMPLEMENT_GLOBAL_SHADER(FDearImguiVS, "/Plugin/UnrealNetimgui/Private/DearImguiShaders.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FDearImguiPS, "/Plugin/UnrealNetimgui/Private/DearImguiShaders.usf", "MainPS", SF_Pixel);

void FNetImguiSlateElement::DrawRenderThread(FRHICommandListImmediate& RHICmdList, const void* InWindowBackBuffer)
{
	SCOPED_DRAW_EVENT(RHICmdList, DrawNetImgui);
	FRHIRenderPassInfo RPInfo(*(FTexture2DRHIRef*)InWindowBackBuffer, ERenderTargetActions::Load_Store);
	RHICmdList.BeginRenderPass(RPInfo, TEXT("LocalNetImgui"));
	{
		//SF TODO Handle invalid texture with default black texture
		//SF TODO Handle UTextures
		//SF TODO Handle various texture formats
		TShaderMapRef<FDearImguiVS> VertexShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		TShaderMapRef<FDearImguiPS> PixelShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		FRHIResourceCreateInfo 		CreateInfoIdx(TEXT("FImguiIndexBuffer"), &mLocalDrawData.IdxBuffer);
		FRHIResourceCreateInfo 		CreateInfoVtx(TEXT("FImguiVertexBuffer"), &mLocalDrawData.VtxBuffer);
		FBufferRHIRef IndexBufferRHI 	= RHICmdList.CreateIndexBuffer(sizeof(ImDrawIdx), mLocalDrawData.IdxBuffer.GetResourceDataSize(), BUF_Volatile, CreateInfoIdx);
		FBufferRHIRef VertexBufferRHI 	= RHICmdList.CreateVertexBuffer(mLocalDrawData.VtxBuffer.GetResourceDataSize(), BUF_Volatile | BUF_ShaderResource, CreateInfoVtx);
		
		if (VertexShader.IsValid() && PixelShader.IsValid())
		{
			FGraphicsPipelineStateInitializer GraphicsPSOInit;
			RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
			GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None>::GetRHI();
			GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
			GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGB, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha, BO_Add, BF_One, BF_InverseSourceAlpha>::GetRHI();
			GraphicsPSOInit.PrimitiveType = PT_TriangleList;
			GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GImguiVertexDeclaration.VertexDeclarationRHI;
			GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
			GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
			SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);
			
			float L = FMath::Floor(CullingRect.Left);
			float R = FMath::Floor(CullingRect.Right);
			float T = FMath::Floor(CullingRect.Top);
			float B = FMath::Floor(CullingRect.Bottom);
			float W = R - L;
			float H = B - T;
			float scale = 1.f;
			FMatrix ProjectionMatrix(
				FVector(2.0f / W*scale,	0.f, 				0.f),
				FVector(0.f, 			-2.0f / H*scale, 	0.f),
				FVector(0.f, 			0.f, 				0.5f),
				FVector(-1.f,			1.f, 				0.5f)
			);
			RHICmdList.SetViewport(L, T, 0, R, B, 1);
			RHICmdList.SetStreamSource(0, VertexBufferRHI, 0);

			FRHIBatchedShaderParameters& BatchedParameters = RHICmdList.GetScratchShaderParameters();
			SetShaderValue(BatchedParameters, VertexShader->ImguiViewProjection, FMatrix44f(ProjectionMatrix), 0);
			SetShaderValue(BatchedParameters, VertexShader->ImguiParameters, ImguiParams, 0);
			RHICmdList.SetBatchedShaderParameters(RHICmdList.GetBoundVertexShader(), BatchedParameters);
			
			for (int i(0); i<mLocalDrawData.CmdLists.Num(); ++i)
			{
				const LocalImguiData::DrawList& cmdList = mLocalDrawData.CmdLists[i];
				for (const ImDrawCmd& cmdDraw : cmdList.CmdBuffer)
				{
					// Project scissor/clipping rectangles into framebuffer space
					// Note 'ImDrawCmd::UserCallback' not supported for now
					ImVec2 clip_min(FMath::Min(L + cmdDraw.ClipRect.x, R),
									FMath::Min(T + cmdDraw.ClipRect.y, B));
					ImVec2 clip_max(FMath::Min(L + cmdDraw.ClipRect.z, R),
									FMath::Min(T + cmdDraw.ClipRect.w, B));
					if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y || cmdDraw.UserCallback != nullptr)
						continue;

					SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), PixelShader->ImguiTexture, PixelShader->ImguiSampler, TStaticSamplerState<SF_Bilinear>::GetRHI(), mFontTexture);					
					RHICmdList.SetScissorRect(true, clip_min.x, clip_min.y, clip_max.x, clip_max.y);

					//SF 
					size_t indexMax = cmdList.IndexOffset + cmdDraw.IdxOffset + cmdDraw.ElemCount;
					size_t indexMaxSize = indexMax*sizeof(ImDrawIdx);
					size_t bufferSize = IndexBufferRHI->GetSize();
					if ( indexMax >= bufferSize) {
						printf("WTF!");
					}
					RHICmdList.DrawIndexedPrimitive(IndexBufferRHI, cmdList.VertexOffset + cmdDraw.VtxOffset, 0,
													cmdList.VertexCount, cmdList.IndexOffset + cmdDraw.IdxOffset, cmdDraw.ElemCount / 3, 1);
				}
			}
		}
	}
	RHICmdList.EndRenderPass();
}

bool FNetImguiSlateElement::Update(FTextureRHIRef fontTexture, FTextureRHIRef blackTexture, ImGuiContext* pContext, const FSlateRect& cullingRect, const FVector4f& imguiParameters )
{
	ImguiParams = imguiParameters;
	CullingRect = cullingRect;
	mFontTexture = fontTexture;
	mBlackTexture = blackTexture;
	if( pContext ){
		ImGuiContext* savedContext = ImGui::GetCurrentContext();
		ImGui::SetCurrentContext(pContext);
		ImDrawData* pDrawData = ImGui::GetDrawData();
		if (pDrawData && pDrawData->Valid) {
			mLocalDrawData.TotalIdxCount		= pDrawData->TotalIdxCount;
			mLocalDrawData.TotalVtxCount		= pDrawData->TotalVtxCount;
			mLocalDrawData.DisplayPos			= pDrawData->DisplayPos;
			mLocalDrawData.DisplaySize			= pDrawData->DisplaySize;
			mLocalDrawData.FramebufferScale		= pDrawData->FramebufferScale;
			mLocalDrawData.CmdLists.SetNum(pDrawData->CmdListsCount);
			mLocalDrawData.IdxBuffer.Reset();
			mLocalDrawData.VtxBuffer.Reset();
			for (int i(0); i<pDrawData->CmdListsCount; ++i) {
				ImDrawList& srcCmdList 					= *pDrawData->CmdLists[i];
				LocalImguiData::DrawList& dstCmdList 	= mLocalDrawData.CmdLists[i];
				dstCmdList.CmdBuffer.Reset();
				dstCmdList.Flags 						= srcCmdList.Flags;
				dstCmdList.IndexOffset 					= mLocalDrawData.IdxBuffer.GetResourceDataSize() / sizeof(ImDrawIdx);
				dstCmdList.VertexOffset					= mLocalDrawData.VtxBuffer.GetResourceDataSize() / sizeof(ImDrawVert);
				dstCmdList.VertexCount 					= srcCmdList.VtxBuffer.size();
				dstCmdList.CmdBuffer.Append(srcCmdList.CmdBuffer.Data, srcCmdList.CmdBuffer.size());
				mLocalDrawData.IdxBuffer.Append((uint8_t*)srcCmdList.IdxBuffer.Data, srcCmdList.IdxBuffer.size_in_bytes());
				mLocalDrawData.VtxBuffer.Append(srcCmdList.VtxBuffer.Data, srcCmdList.VtxBuffer.size());
			}
		}		
		ImGui::SetCurrentContext(savedContext);
		return mLocalDrawData.IdxBuffer.Num() > 0;
	}
	return false;
}

#endif // NETIMGUI_LOCALDRAW_ENABLED

#pragma optimize("", on) //SF
