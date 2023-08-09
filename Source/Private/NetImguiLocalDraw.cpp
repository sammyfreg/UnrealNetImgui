// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "NetImguiModule.h"
#include "CoreMinimal.h"

#if NETIMGUI_ENABLED


#include "Engine/Texture2D.h"
#include "StaticMeshResources.h"
#include "CanvasRender.h"
#include "SceneView.h"
#include "UnrealClient.h"
#include "EngineGlobals.h"
#include "RenderGraphUtils.h"
#include "ShaderParameterUtils.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "SceneView.h"
#include "SceneRenderTargetParameters.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "RHIStaticStates.h"
#include "MeshPassProcessor.h"
#include "SceneInterface.h"
#include "Engine/GameViewportClient.h"
#include "Slate/Public/Framework/Application/SlateApplication.h"
#include "Slate/SceneViewport.h"
#include "Widgets/SLeafWidget.h"

#if WITH_EDITOR
#include "Editor.h"
#include "LevelEditorViewport.h"
#include "SLevelViewport.h"
#include "LevelEditor.h"
#endif


#pragma optimize("", off) //SF
class ScopedContext
{
public:
	ScopedContext(ImGuiContext* scopedContext) : SavedContext(ImGui::GetCurrentContext()) { ImGui::SetCurrentContext(scopedContext); }
	~ScopedContext() { ImGui::SetCurrentContext(SavedContext); }
	ImGuiContext* SavedContext;
};

struct LocalImguiData
{
	struct DrawList{
		TArray<ImDrawCmd>   CmdBuffer; 	// Draw commands. Typically 1 command = 1 GPU draw call, unless the command is a callback.
		ImDrawListFlags  	Flags;		// Flags, you may poke into these to adjust anti-aliasing settings per-primitive.
		uint32 				IndexOffset;
		uint32 				VertexOffset;
		uint32 				VertexCount;
	};
	int             	TotalIdxCount;          // For convenience, sum of all ImDrawList's IdxBuffer.Size
	int             	TotalVtxCount;          // For convenience, sum of all ImDrawList's VtxBuffer.Size
	ImVec2          	DisplayPos;             // Top-left position of the viewport to render (== top-left of the orthogonal projection matrix to use) (== GetMainViewport()->Pos for the main viewport, == (0.0) in most single-viewport applications)
	ImVec2          	DisplaySize;            // Size of the viewport to render (== GetMainViewport()->Size for the main viewport, == io.DisplaySize in most single-viewport applications)
	ImVec2          	FramebufferScale;       // Amount of pixels for each unit of DisplaySize. Based on io.DisplayFramebufferScale. Generally (1,1) on normal display, (2,2) on OSX with Retina display.
	TArray<DrawList>	CmdLists;               // Array of ImDrawList* to render. The ImDrawList are owned by ImGuiContext and only pointed to from here.
	TResourceArray<uint8, INDEXBUFFER_ALIGNMENT> 		IdxBuffer; 	// Index buffer. Each command consume ImDrawCmd::ElemCount of those
	TResourceArray<ImDrawVert, VERTEXBUFFER_ALIGNMENT> 	VtxBuffer; 	// Vertex buffer.
};

class FNetImguiSlateElement : public ICustomSlateElement
{
public:
	FNetImguiSlateElement(){};
	bool Update(FTextureRHIRef fontTexture, FTextureRHIRef blackTexture, ImGuiContext* pContext, const FSlateRect& cullingRect, const FVector4f& imguiParameters);
	virtual void 	DrawRenderThread(FRHICommandListImmediate& RHICmdList, const void* RenderTarget);

private:
	LocalImguiData	mLocalDrawData;
	FTextureRHIRef 	mFontTexture; 
	FTextureRHIRef 	mBlackTexture;
	FSlateRect CullingRect;
	FVector4f ImguiParams;
};

FTextureRHIRef 		GFontTexture; //SF TEMP

// STestFunctionWidget
class SNetImguiWidget : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SNetImguiWidget){}
		SLATE_ARGUMENT(const NetImguiLocalDrawSupport*, LocalDrawModule)
		SLATE_ARGUMENT(bool, IsEditorWindow)
	SLATE_END_ARGS()
	
	virtual ~SNetImguiWidget()
	{
		ImGui::DestroyContext(ImguiContext);
	};
	
	void Construct(const FArguments& InArgs)
	{
		SetVisibility(EVisibility::HitTestInvisible);
		LocalDrawModule 		= InArgs._LocalDrawModule;
		IsEditorWindow 			= InArgs._IsEditorWindow;
		ImguiContext 			= ImGui::CreateContext(ImGui::GetIO().Fonts);
		VerticalDisplayOffset	= IsEditorWindow ? 32.f : 0.f;
		
		ScopedContext scopedContext(ImguiContext);
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
		ImGui::GetIO().MouseDrawCursor = false;
		NetImguiDrawers[0] = MakeShareable(new FNetImguiSlateElement());
		NetImguiDrawers[1] = MakeShareable(new FNetImguiSlateElement());
		NetImguiDrawers[2] = MakeShareable(new FNetImguiSlateElement());
	}

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
	{
		ScopedContext scopedContext(ImguiContext);
		ImGuiKey imguiKey = LocalDrawModule->InputProcessor->UnrealToImguiKey(InKeyEvent.GetKey());
		if (imguiKey != ImGuiKey_None) {
			ImGui::GetIO().AddKeyEvent(imguiKey, true);
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

	virtual FReply OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
	{
		ScopedContext scopedContext(ImguiContext);
		ImGuiKey imguiKey = LocalDrawModule->InputProcessor->UnrealToImguiKey(InKeyEvent.GetKey());
		//if (ImGui::GetIO().WantCaptureMouse && imguiKey != ImGuiKey_COUNT) {
		if (imguiKey != ImGuiKey_None) {
			ImGui::GetIO().AddKeyEvent(imguiKey, false);
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

	virtual FReply OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent)
	{
		ScopedContext scopedContext(ImguiContext);
		if (ImGui::GetIO().WantCaptureKeyboard) {
			//SF TODO handle TCHar type
			ImGui::GetIO().AddInputCharacterUTF16(InCharacterEvent.GetCharacter());
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
	{
		ScopedContext scopedContext(ImguiContext);
		ImGuiIO& io = ImGui::GetIO();
		//SF SetVisibility(io.WantCaptureMouse ? EVisibility::Visible : EVisibility::HitTestInvisible);

		const FSlateRenderTransform screenToImguiCoord = AllottedGeometry.GetAccumulatedRenderTransform();
		FVector2f mousePos 	= screenToImguiCoord.Inverse().TransformPoint(FSlateApplication::Get().GetCursorPos());
		mousePos.Y			-= VerticalDisplayOffset;
		mousePos 			*= AllottedGeometry.Scale;
		io.AddMousePosEvent(mousePos.X, mousePos.Y);
		io.DeltaTime		= InDeltaTime; //SF sceneView->Family->Time.GetDeltaRealTimeSeconds();  //FGameTime Time = Canvas->GetTime();
		//io.FontGlobalScale = AllottedGeometry.Scale;
		//SetFlag(IO.ConfigFlags, ImGuiConfigFlags_NavEnableKeyboard, InputState.IsKeyboardNavigationEnabled());
		//SetFlag(IO.ConfigFlags, ImGuiConfigFlags_NavEnableGamepad, InputState.IsGamepadNavigationEnabled());
		//SetFlag(IO.BackendFlags, ImGuiBackendFlags_HasGamepad, InputState.HasGamepad());
		//SFio.FontGlobalScale 	= Canvas->GetDPIScale();
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		ScopedContext scopedContext(ImguiContext);
		ImGuiIO& io 			= ImGui::GetIO();
		FSlateRect imguiRect 	= MyCullingRect;
		imguiRect.Top 			+= VerticalDisplayOffset * AllottedGeometry.Scale;
		io.DisplaySize	 		= ImVec2(imguiRect.GetSize2f().X, imguiRect.GetSize2f().Y);
		
		ImGui::NewFrame();
		ImGui::GetWindowViewport()->DpiScale = AllottedGeometry.Scale;
		
		if( ImGui::BeginMainMenuBar() ){
			if (ImGui::BeginMenu("NetImgui")) {
				ImGui::SliderFloat("Opacity", &ImguiParameters.X, 0.1f, 1.f);

				static bool test(true);
				if (ImGui::MenuItem("DPI", nullptr, &test)) {
					int dpiMask = ImGuiConfigFlags_DpiEnableScaleViewports | ImGuiConfigFlags_DpiEnableScaleFonts;
					ImGui::GetIO().ConfigFlags &= ~dpiMask;
					ImGui::GetIO().ConfigFlags |= (test ? dpiMask : 0);
				}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}
		ImGui::SetNextWindowPos(ImVec2(0,0), ImGuiCond_Once);
		ImGui::ShowDemoWindow(nullptr);
		ImGui::Render();

		//---------------------------------------------------------------------------------------------
		// Add DebugDraw Item for the 'Dear ImGui' content of this viewport
		//---------------------------------------------------------------------------------------------
		TSharedPtr<FNetImguiSlateElement, ESPMode::ThreadSafe> NetImguiDrawer = NetImguiDrawers[counter++ % 3];
		if (NetImguiDrawer->Update(GFontTexture, GFontTexture, ImguiContext, imguiRect, ImguiParameters)) {
			FSlateDrawElement::MakeCustom(OutDrawElements, LayerId++, NetImguiDrawer);
		}
		
		return LayerId;
	}

	FVector2D ComputeDesiredSize(float) const override
	{
		return FVector2D(0.0f, 0.0f);
	}

	void Remove()
	{
		//SF TODO
	}

public: //SF TEMP, cleanop...
	ImGuiContext* ImguiContext = nullptr;
protected:
	TSharedPtr<FNetImguiSlateElement, ESPMode::ThreadSafe> NetImguiDrawers[3];
	const NetImguiLocalDrawSupport* LocalDrawModule = nullptr;
	bool IsEditorWindow 							= false;
	float VerticalDisplayOffset 					= 0.f;	// Used to avoid drawing over Editor tool icons at the top of the viewport
	mutable int counter 							= 0;
	mutable FVector4f ImguiParameters 				= FVector4f(1, 0, 0, 0);
};

void NetImguiLocalDrawSupport::AddWidgetToViewport()
{
	//checkf(GameViewport, TEXT("Null game viewport."));
	//checkf(FSlateApplication::IsInitialized(), TEXT("Slate should be initialized before we can add widget to game viewports."));
	if (GameViewport != GEngine->GameViewport) {
		if (GameViewport) {
			GameViewport = nullptr;
		}
		else {
			GameViewport = GEngine->GameViewport;
			AddWidgetToViewport(GEngine->GameViewport);
		}
	}
}

void NetImguiLocalDrawSupport::AddWidgetToViewport(UGameViewportClient* gameViewport)
{
	constexpr int32 IMGUI_WIDGET_Z_ORDER = 10000;
	TSharedPtr<SNetImguiWidget> netImguiWidget;
	SAssignNew(netImguiWidget, SNetImguiWidget)
		.LocalDrawModule(this)
		.IsEditorWindow(false);
	GameViewport->AddViewportWidgetContent(netImguiWidget.ToSharedRef(), IMGUI_WIDGET_Z_ORDER);
	WidgetsMap.Add(gameViewport, netImguiWidget);
}

void NetImguiLocalDrawSupport::AddWidgetToViewport(SLevelViewport* editorViewport)
{
	constexpr int32 IMGUI_WIDGET_Z_ORDER = 10000;
	TSharedPtr<SNetImguiWidget> netImguiWidget;
	SAssignNew(netImguiWidget, SNetImguiWidget)
		.LocalDrawModule(this)
		.IsEditorWindow(true);
	//netImguiWidget->editorViewportTest = editorViewport;
	editorViewport->AddOverlayWidget(netImguiWidget.ToSharedRef());
	WidgetsMap.Add(editorViewport, netImguiWidget);
}

class FImGuiVertexDeclaration : public FRenderResource
{
public:
	FVertexDeclarationRHIRef VertexDeclarationRHI;
	virtual ~FImGuiVertexDeclaration() {}
	virtual void InitRHI()
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
		FBufferRHIRef IndexBufferRHI 	= RHICreateIndexBuffer(sizeof(ImDrawIdx), mLocalDrawData.IdxBuffer.GetResourceDataSize(), BUF_Volatile, CreateInfoIdx);
		FBufferRHIRef VertexBufferRHI 	= RHICreateVertexBuffer(mLocalDrawData.VtxBuffer.GetResourceDataSize(), BUF_Volatile | BUF_ShaderResource, CreateInfoVtx);
		
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
			SetShaderValue(RHICmdList, RHICmdList.GetBoundVertexShader(), VertexShader->ImguiViewProjection, FMatrix44f(ProjectionMatrix));
			SetShaderValue(RHICmdList, RHICmdList.GetBoundVertexShader(), VertexShader->ImguiParameters, ImguiParams);
			
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

void NetImguiLocalDrawSupport::Initialize()
{
	ImGuiIO& io	= ImGui::GetIO();
	uint8_t* pPixelData(nullptr);
	int width(0), height(0);
	io.Fonts->GetTexDataAsRGBA32(&pPixelData, &width, &height); //SF TODO handle R8 & RGBA32 ?
	FontTextureData.Init(pPixelData, width, height);
	//UGameViewportClient::OnViewportCreated().AddRaw(this, &NetImguiLocalDrawSupport::OnGameViewportCreated);
}

void NetImguiLocalDrawSupport::Update()
{
#if WITH_EDITOR
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	TSharedPtr<ILevelEditor> LevelEditor = LevelEditorModule.GetFirstLevelEditor();
	FocusedWidget = nullptr; //SF set to pie game viewport
	const FViewport* editViewport 	= GCurrentLevelEditingViewportClient ? GCurrentLevelEditingViewportClient->Viewport : nullptr;
	if (LevelEditor.IsValid()){
		TArray<TSharedPtr<SLevelViewport>> Viewports = LevelEditor->GetViewports();
		for (const TSharedPtr<SLevelViewport>& ViewportWindow : Viewports){
			if (ViewportWindow.IsValid() ){
				if (!WidgetsMap.Find(ViewportWindow.Get())) {
					AddWidgetToViewport(ViewportWindow.Get());
				}
				//SF use to turnoff 2nd imgui? viewport->IsPlayInEditorViewport();
				if( editViewport == ViewportWindow->GetActiveViewport() ){
					FocusedWidget = *WidgetsMap.Find(ViewportWindow.Get());
				}
			}
		}
	}
#endif
#if 0
	class UNREALED_API UEditorEngine : public UEngine
		FViewport* UEditorEngine::GetActiveViewport();
	const TArray<class FEditorViewportClient*>& UEditorEngine::GetAllViewportClients() { return AllViewportClients; }
	FViewport* UEditorEngine::GetPIEViewport();
	TSharedPtr<class SLevelViewport> GetActiveViewport();

	void SetViewportInterface( TSharedRef<ISlateViewport> InViewportInterface );
	TWeakPtr<ISlateViewport> GetViewportInterface();

	//SF GEngine->GameViewport;
	// class ENGINE_API UGameViewportClient : public UScriptViewportClient, public FExec 
	if (UGameViewportClient* GameViewport = WorldContextObject->GetWorld()->GetGameViewport())
		FSceneViewport* GetGameViewport();
	TSharedPtr<SViewport> Viewport = GameViewport->GetGameViewportWidget();

	
	const FSceneViewport* SceneViewport = GetGameViewport();
	if (SceneViewport != nullptr)
	{
		TWeakPtr<SViewport> WeakViewportWidget = SceneViewport->GetViewportWidget();
		TSharedPtr<SViewport> ViewportWidget = WeakViewportWidget.Pin();
		return ViewportWidget;
	}
	return nullptr;
#endif
	//Move this? And remove update?
	//---------------------------------------------------------------------------------------------
	// Generate the Font Texture once
	//---------------------------------------------------------------------------------------------
	if (!FontTexture.IsValid() && FontTextureData.GetResourceBulkData() != nullptr )
	{
		ENQUEUE_RENDER_COMMAND(ImguiCreateTextures)(
			[this](FRHICommandListImmediate& RHICmdList)
			{
				FRHITextureCreateDesc Desc = FRHITextureCreateDesc::Create2D(TEXT("ImGuiFontTexture"), FontTextureData.Width, FontTextureData.Height, EPixelFormat::PF_R8G8B8A8)
					.SetClearValue(FClearValueBinding::Black)
					.SetFlags(ETextureCreateFlags::ShaderResource ) 
					//.SetFlags(ETextureCreateFlags::SRGB)
					.SetInitialState(ERHIAccess::SRVGraphics)
					.SetBulkData(&FontTextureData);
				FontTexture = RHICreateTexture(Desc);
				GFontTexture = FontTexture;
			});
		FlushRenderingCommands();
		FontTextureData.Discard();
	}

	AddWidgetToViewport();
}

void NetImguiLocalDrawSupport::Terminate()
{
	FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);
	InputProcessor = nullptr;
	//SF Destroy contexts
}

void NetImguiLocalDrawSupport::InterceptInput()
{
	if( !InputProcessor.IsValid() ){
		InputProcessor = MakeShareable(new FNetImguiInputProcessor(this));
		FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor);
	}
	
	//SF insert NetImgui internet here
	return;
#if 0
	ImGuiContext* remoteContext = ImGui::GetCurrentContext();
	ImGuiIO& remoteIO			= ImGui::GetIO();
	
	//TMap<void*, ImGuiContext*> ContextsMap2;
	for (auto& context : ContextsMap) {
		ImGui::SetCurrentContext(context.Value);
		ImGuiIO& localIO		= ImGui::GetIO();
		localIO.MouseDrawCursor = false;
		if (ActiveContextIndex == 0) {
			localIO.MouseDrawCursor = true;
			float mouseX = remoteIO.MousePos.x / remoteIO.DisplaySize.x * localIO.DisplaySize.x;
			float mouseY = remoteIO.MousePos.y / remoteIO.DisplaySize.y * localIO.DisplaySize.y;
			localIO.AddMousePosEvent(mouseX, mouseY);
			for (uint32 i = 0; i < UE_ARRAY_COUNT(remoteIO.MouseDown); ++i) {
				localIO.AddMouseButtonEvent(i, remoteIO.MouseDown[i]);
			}
			
			break;
		}
	}
	ImGui::SetCurrentContext(remoteContext);
#endif
}

void NetImguiLocalDrawSupport::FFontBulkData::Init(const void* InData, uint32 InWidth, uint32 InHeight) 
{ 
	Width = InWidth;
	Height = InHeight;
	Data.Reset();
	Data.Append((uint8*)InData, Width*Height*4);
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

FNetImguiInputProcessor::FNetImguiInputProcessor(class NetImguiLocalDrawSupport* owner)
: Owner(owner)
{
	struct UnrealToImguiKeyPair { FKey UnrealKey; ImGuiKey ImguiKey; };
	static const UnrealToImguiKeyPair KeysMapping[] = {
		// Mouse buttons
		{EKeys::LeftMouseButton, ImGuiKey_MouseLeft},
		{EKeys::RightMouseButton, ImGuiKey_MouseRight},
		{EKeys::MiddleMouseButton, ImGuiKey_MouseMiddle},
		{EKeys::ThumbMouseButton, ImGuiKey_MouseX1},
		{EKeys::ThumbMouseButton2, ImGuiKey_MouseX2},

		// Keyboard keys
		{EKeys::Tab, ImGuiKey_Tab},					{EKeys::Left, ImGuiKey_LeftArrow},			{EKeys::Right, ImGuiKey_RightArrow},
		{EKeys::Up, ImGuiKey_UpArrow},				{EKeys::Down, ImGuiKey_DownArrow},			{EKeys::PageUp, ImGuiKey_PageUp},
		{EKeys::PageDown, ImGuiKey_PageDown},		{EKeys::Home, ImGuiKey_Home},				{EKeys::End, ImGuiKey_End},
		{EKeys::Insert, ImGuiKey_Insert},			{EKeys::Delete, ImGuiKey_Delete},			{EKeys::BackSpace, ImGuiKey_Backspace},
		{EKeys::SpaceBar, ImGuiKey_Space}, 			{EKeys::Enter, ImGuiKey_Enter}, 			{EKeys::Escape, ImGuiKey_Escape},
 
		{EKeys::LeftControl, ImGuiKey_LeftCtrl}, 	{EKeys::LeftShift, ImGuiKey_LeftShift}, 	{EKeys::LeftAlt, ImGuiKey_LeftAlt},
		{EKeys::LeftCommand, ImGuiKey_LeftSuper},	{EKeys::RightControl, ImGuiKey_RightCtrl},	{EKeys::RightShift, ImGuiKey_RightShift},
		{EKeys::RightAlt, ImGuiKey_RightAlt}, 		{EKeys::RightCommand, ImGuiKey_RightSuper},
		//ImGuiKey_Menu
		{EKeys::Zero, ImGuiKey_0}, {EKeys::One, ImGuiKey_1}, {EKeys::Two, ImGuiKey_2},   {EKeys::Three, ImGuiKey_3}, {EKeys::Four, ImGuiKey_4}, 
		{EKeys::Five, ImGuiKey_5}, {EKeys::Six, ImGuiKey_6}, {EKeys::Seven, ImGuiKey_7}, {EKeys::Eight, ImGuiKey_8}, {EKeys::Nine, ImGuiKey_9},

		{EKeys::A, ImGuiKey_A}, {EKeys::B, ImGuiKey_B}, {EKeys::C, ImGuiKey_C}, {EKeys::D, ImGuiKey_D}, {EKeys::E, ImGuiKey_E}, {EKeys::F, ImGuiKey_F}, 
		{EKeys::G, ImGuiKey_G}, {EKeys::H, ImGuiKey_H}, {EKeys::I, ImGuiKey_I}, {EKeys::J, ImGuiKey_J},	{EKeys::K, ImGuiKey_K}, {EKeys::L, ImGuiKey_L}, 
		{EKeys::M, ImGuiKey_M}, {EKeys::N, ImGuiKey_N}, {EKeys::O, ImGuiKey_O}, {EKeys::P, ImGuiKey_P}, {EKeys::Q, ImGuiKey_Q}, {EKeys::R, ImGuiKey_R}, 
		{EKeys::S, ImGuiKey_S}, {EKeys::T, ImGuiKey_T},	{EKeys::U, ImGuiKey_U}, {EKeys::V, ImGuiKey_V}, {EKeys::W, ImGuiKey_W}, {EKeys::X, ImGuiKey_X}, 
		{EKeys::Y, ImGuiKey_Y}, {EKeys::Z, ImGuiKey_Z},

		{EKeys::F1, ImGuiKey_F1}, {EKeys::F2, ImGuiKey_F2}, {EKeys::F3, ImGuiKey_F3}, {EKeys::F4, ImGuiKey_F4}, {EKeys::F5, ImGuiKey_F5}, 
		{EKeys::F6, ImGuiKey_F6}, {EKeys::F7, ImGuiKey_F7}, {EKeys::F8, ImGuiKey_F8}, {EKeys::F9, ImGuiKey_F9}, {EKeys::F10, ImGuiKey_F10}, 
		{EKeys::F11, ImGuiKey_F11}, {EKeys::F12, ImGuiKey_F12},
		/*
			ImGuiKey_Apostrophe,        // '
			ImGuiKey_Comma,             // ,
			ImGuiKey_Minus,             // -
			ImGuiKey_Period,            // .
			ImGuiKey_Slash,             // /
			ImGuiKey_Semicolon,         // ;
			ImGuiKey_Equal,             // =
			ImGuiKey_LeftBracket,       // [
			ImGuiKey_Backslash,         // \ (this text inhibit multiline comment caused by backslash)
			ImGuiKey_RightBracket,      // ]
			ImGuiKey_GraveAccent,       // `
			ImGuiKey_CapsLock,
			ImGuiKey_ScrollLock,
			ImGuiKey_NumLock,
			ImGuiKey_PrintScreen,
			ImGuiKey_Pause,
			ImGuiKey_Keypad0, ImGuiKey_Keypad1, ImGuiKey_Keypad2, ImGuiKey_Keypad3, ImGuiKey_Keypad4,
			ImGuiKey_Keypad5, ImGuiKey_Keypad6, ImGuiKey_Keypad7, ImGuiKey_Keypad8, ImGuiKey_Keypad9,
			ImGuiKey_KeypadDecimal,
			ImGuiKey_KeypadDivide,
			ImGuiKey_KeypadMultiply,
			ImGuiKey_KeypadSubtract,
			ImGuiKey_KeypadAdd,
			ImGuiKey_KeypadEnter,
			ImGuiKey_KeypadEqual,
			*/
	};

	for (size_t i(0); i<UE_ARRAY_COUNT(KeysMapping); ++i) {
		UnrealKeyToImguiMap.Add(KeysMapping[i].UnrealKey, KeysMapping[i].ImguiKey);
	}
}
void FNetImguiInputProcessor::Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor)
{
	for (auto& pair : Owner->WidgetsMap) {
		ScopedContext scopedContext(pair.Value->ImguiContext);
		if (ImGui::GetIO().WantCaptureMouse){
			ImGuiMouseCursor cursor = ImGui::GetMouseCursor();
			switch (cursor) {
				case ImGuiMouseCursor_Arrow: Cursor->SetType(EMouseCursor::Type::Default); break;
				case ImGuiMouseCursor_TextInput: Cursor->SetType(EMouseCursor::Type::TextEditBeam); break;		// When hovering over InputText, etc.
				case ImGuiMouseCursor_ResizeAll: Cursor->SetType(EMouseCursor::Type::CardinalCross); break;		// (Unused by Dear ImGui functions)
				case ImGuiMouseCursor_ResizeNS: Cursor->SetType(EMouseCursor::Type::ResizeUpDown); break;		// When hovering over a horizontal border
				case ImGuiMouseCursor_ResizeEW: Cursor->SetType(EMouseCursor::Type::ResizeLeftRight); break;	// When hovering over a vertical border or a column
				case ImGuiMouseCursor_ResizeNESW: Cursor->SetType(EMouseCursor::Type::ResizeSouthWest); break;	// When hovering over the bottom-left corner of a window
				case ImGuiMouseCursor_ResizeNWSE: Cursor->SetType(EMouseCursor::Type::ResizeSouthEast); break;	// When hovering over the bottom-right corner of a window
				case ImGuiMouseCursor_Hand: Cursor->SetType(EMouseCursor::Type::Hand); break;             		// (Unused by Dear ImGui functions. Use for e.g. hyperlinks)
				case ImGuiMouseCursor_NotAllowed: Cursor->SetType(EMouseCursor::Type::SlashedCircle); break;	// When hovering something with disallowed interaction. Usually a crossed circle.
				default: Cursor->SetType(EMouseCursor::Type::Default); break;
			}
		}
		if (ImGui::GetIO().WantCaptureKeyboard){
			SlateApp.SetKeyboardFocus(pair.Value, EFocusCause::SetDirectly);
		}
	}
}

bool FNetImguiInputProcessor::HandleMouse(const FPointerEvent& InMouseEvent, bool InMouseDown)
{
	bool captured(false);
	int imguiMouseBtn = UnrealToImguiMouseButton(InMouseEvent.GetEffectingButton());
	if (imguiMouseBtn != -1) {
		uint32 mouseBtnMask(1 << imguiMouseBtn);
		for (auto& pair : Owner->WidgetsMap) {
			ScopedContext scopedContext(pair.Value->ImguiContext);
			ImGui::GetIO().AddMouseButtonEvent(imguiMouseBtn, InMouseDown);
			PassThroughMouseMask |= InMouseDown && ImGui::GetIO().WantCaptureMouse && (Owner->FocusedWidget != pair.Value) ? mouseBtnMask : 0;
			captured |= ImGui::GetIO().WantCaptureMouse;
		}
		captured				&= (PassThroughMouseMask & mouseBtnMask) == 0;	// Let mouse down/up events through when clicking in new unreal window (to let it become active)
		PassThroughMouseMask	&= !InMouseDown ? ~mouseBtnMask : ~0;			// Cancel the mouse passtrough
	}
	return captured;
}

bool FNetImguiInputProcessor::HandleMouseButtonDownEvent( FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	return HandleMouse(MouseEvent, true);
}

bool FNetImguiInputProcessor::HandleMouseButtonUpEvent( FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{	
	return HandleMouse(MouseEvent, false);
}

bool FNetImguiInputProcessor::HandleMouseButtonDoubleClickEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	return HandleMouse(MouseEvent, true);
}

bool FNetImguiInputProcessor::HandleMouseWheelOrGestureEvent(FSlateApplication& SlateApp, const FPointerEvent& InWheelEvent, const FPointerEvent* InGestureEvent)
{
	bool captured(false);
	for (auto& pair : Owner->WidgetsMap) {
		ScopedContext scopedContext(pair.Value->ImguiContext);
		if (ImGui::GetIO().WantCaptureMouse) {
			ImGui::GetIO().AddMouseWheelEvent(0.f, InWheelEvent.GetWheelDelta());
			captured = true;
		}
	}
	return captured;
}

#if 0
bool FNetImguiInputProcessor::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
return HandleKey(InKeyEvent, true);
}

bool FNetImguiInputProcessor::HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
return HandleKey(InKeyEvent, false);
}
bool FNetImguiInputProcessor::HandleKey(const FKeyEvent& InKeyEvent, bool InKeyDown)
{
	bool captured(false);
	ImGuiKey imguiKey = UnrealToImguiKey(InKeyEvent.GetKey());
	if (imguiKey != ImGuiKey_None && Owner->FocusedWidget) {
		ScopedContext scopedContext(Owner->FocusedWidget->ImguiContext);
		//if (ImGui::GetIO().WantCaptureKeyboard) { //SF Figure out key capture
		ImGui::GetIO().AddKeyEvent(imguiKey, InKeyDown);
		//captured = true;
		//}
	}
	return captured;
}
#endif
#endif //NETIMGUI_ENABLED

#pragma optimize("", on) //SF
