// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "NetImguiModule.h"
#include "CoreMinimal.h"

#if NETIMGUI_ENABLED

//#include "LevelEditorViewport.h"
//#include "DebugRenderSceneProxy.h"
#include "Engine/Texture2D.h"
//#include "BatchedElements.h"
//#include "MeshBatch.h"
//#include "RawIndexBuffer.h"
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
#include "ClearQuad.h" //SF TEST
#include "MeshPassProcessor.h"
#include "SceneInterface.h"
#include "Engine/GameViewportClient.h"
#include "Slate/Public/Framework/Application/SlateApplication.h"
#include "Slate/Public/Framework/Application/IInputProcessor.h"

#pragma optimize("", off) //SF


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

class FImGuiInputProcessor : public IInputProcessor
{
public:
	FImGuiInputProcessor(){};
	virtual ~FImGuiInputProcessor(){}

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) {
	};

	/** Key down input */
	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) { return CaptureKeyboard; }

	/** Key up input */
	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) { return CaptureKeyboard; }

	/** Analog axis input */
	virtual bool HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent) { return CaptureKeyboard; }

	/** Mouse movement input */
	virtual bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) { return CaptureMouse; }

	/** Mouse button press */
	virtual bool HandleMouseButtonDownEvent( FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) { return CaptureMouse; }

	/** Mouse button release */
	virtual bool HandleMouseButtonUpEvent( FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) { return CaptureMouse; }

	/** Mouse button double clicked. */
	virtual bool HandleMouseButtonDoubleClickEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) { return CaptureMouse; }

	/** Mouse wheel input */
	virtual bool HandleMouseWheelOrGestureEvent(FSlateApplication& SlateApp, const FPointerEvent& InWheelEvent, const FPointerEvent* InGestureEvent) { return CaptureMouse; }

	/** Called when a motion-driven device has new input */
	virtual bool HandleMotionDetectedEvent(FSlateApplication& SlateApp, const FMotionEvent& MotionEvent) { return false; };

	/** Debug name for logging purposes */
	virtual const TCHAR* GetDebugName() const { return TEXT("NetImguiInput"); }

	bool CaptureMouse = false;
	bool CaptureKeyboard = false;
};
TSharedPtr<FImGuiInputProcessor> GImGuiInputProcessorRef;
//FImGuiInputProcessor GImGuiInputProcessor; //SF TEMP global

//FCanvasTriangleRendererItem
class FCanvasImguiRendererItem : public FCanvasBaseRenderItem
{
public:	
					FCanvasImguiRendererItem(FTextureRHIRef fontTexture, FTextureRHIRef blackTexture, ImGuiContext* pContext);
	virtual bool 	Render_RenderThread(FCanvasRenderContext& RenderContext, FMeshPassProcessorRenderState& DrawRenderState, const FCanvas* Canvas) override;
	virtual bool 	Render_GameThread(const FCanvas* Canvas, FCanvasRenderThreadScope& RenderScope) override;
	inline bool 	IsValid() { return mIsValid;  }
private:
	LocalImguiData mLocalDrawData;
	FTextureRHIRef mFontTexture; 
	FTextureRHIRef mBlackTexture;
	bool mIsValid = false;
};

void NetImguiLocalDrawSupport::InterceptInput()
{
	if( !GImGuiInputProcessorRef.IsValid() ){
		//bool RegisterInputPreProcessor(TSharedPtr<class IInputProcessor> InputProcessor, const int32 Index = INDEX_NONE);
		GImGuiInputProcessorRef = MakeShareable(new FImGuiInputProcessor);
		FSlateApplication::Get().RegisterInputPreProcessor(GImGuiInputProcessorRef);
	}
	GImGuiInputProcessorRef->CaptureMouse = false;
	GImGuiInputProcessorRef->CaptureKeyboard = false;

	ImGuiContext* remoteContext = ImGui::GetCurrentContext();
	ImGuiIO& remoteIO			= ImGui::GetIO();
	for (auto& it : ViewportMap) {
		ViewportAssociation& viewportInfo = it.Value;
		ImGui::SetCurrentContext(viewportInfo.ImguiContext);
		ImGuiIO& localIO		= ImGui::GetIO();
		localIO.MouseDrawCursor = false;
		//if (ActiveContextIndex == 0) {
		localIO.MouseDrawCursor = true;
		//auto pos = FSlateApplication::Get().GetCursorPos();
		//localIO.AddMousePosEvent(pos.X - viewportInfo.ViewportPos.x, pos.Y-viewportInfo.ViewportPos.y);
		GImGuiInputProcessorRef->CaptureMouse = localIO.WantCaptureMouse;
		GImGuiInputProcessorRef->CaptureKeyboard = localIO.WantCaptureKeyboard;
		if( localIO.WantCaptureMouse )
			printf("Test");
			//break;
		//}
	}
	ImGui::SetCurrentContext(remoteContext);
	return; //SF
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

	TSharedPtr<SViewport> GameViewport = GEngine->GameViewport ? GEngine->GameViewport->GetGameViewportWidget() : nullptr;
	if (GameViewport.IsValid()) {
		printf("Test");// SF
	}

	TSharedPtr<SViewport> GameViewport2 = FSlateApplication::Get().GetGameViewport();
	if (GameViewport2.IsValid()) {
		printf("Test");// SF
	}
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

#if 0
	IMGUI_API void  AddKeyEvent(ImGuiKey key, bool down);                   // Queue a new key down/up event. Key should be "translated" (as in, generally ImGuiKey_A matches the key end-user would use to emit an 'A' character)
	IMGUI_API void  AddKeyAnalogEvent(ImGuiKey key, bool down, float v);    // Queue a new key down/up event for analog values (e.g. ImGuiKey_Gamepad_ values). Dead-zones should be handled by the backend.
	IMGUI_API void  AddMousePosEvent(float x, float y);                     // Queue a mouse position update. Use -FLT_MAX,-FLT_MAX to signify no mouse (e.g. app not focused and not hovered)
	IMGUI_API void  AddMouseButtonEvent(int button, bool down);             // Queue a mouse button change
	IMGUI_API void  AddMouseWheelEvent(float wheel_x, float wheel_y);       // Queue a mouse wheel update. wheel_y<0: scroll down, wheel_y>0: scroll up, wheel_x<0: scroll right, wheel_x>0: scroll left.
	IMGUI_API void  AddMouseSourceEvent(ImGuiMouseSource source);           // Queue a mouse source change (Mouse/TouchScreen/Pen)
	IMGUI_API void  AddFocusEvent(bool focused);                            // Queue a gain/loss of focus for the application (generally based on OS/platform focus of your window)
	IMGUI_API void  AddInputCharacter(unsigned int c);                      // Queue a new character input
	IMGUI_API void  AddInputCharacterUTF16(ImWchar16 c);                    // Queue a new character input from a UTF-16 character, it can be a surrogate
	IMGUI_API void  AddInputCharactersUTF8(const char* str);                // Queue a new characters input from a UTF-8 string
	// Main Input State
	// (this block used to be written by backend, since 1.87 it is best to NOT write to those directly, call the AddXXX functions above instead)
	// (reading from those variables is fair game, as they are extremely unlikely to be moving anywhere)
	ImVec2      MousePos;                           // Mouse position, in pixels. Set to ImVec2(-FLT_MAX, -FLT_MAX) if mouse is unavailable (on another screen, etc.)
	bool        MouseDown[5];                       // Mouse buttons: 0=left, 1=right, 2=middle + extras (ImGuiMouseButton_COUNT == 5). Dear ImGui mostly uses left and right buttons. Other buttons allow us to track if the mouse is being used by your application + available to user as a convenience via IsMouse** API.
	float       MouseWheel;                         // Mouse wheel Vertical: 1 unit scrolls about 5 lines text. >0 scrolls Up, <0 scrolls Down. Hold SHIFT to turn vertical scroll into horizontal scroll.
	float       MouseWheelH;                        // Mouse wheel Horizontal. >0 scrolls Left, <0 scrolls Right. Most users don't have a mouse with a horizontal wheel, may not be filled by all backends.
	ImGuiMouseSource MouseSource;                   // Mouse actual input peripheral (Mouse/TouchScreen/Pen).
	bool        KeyCtrl;                            // Keyboard modifier down: Control
	bool        KeyShift;                           // Keyboard modifier down: Shift
	bool        KeyAlt;                             // Keyboard modifier down: Alt
	bool        KeySuper;                           // Keyboard modifier down: Cmd/Super/Windows
#endif
}

void NetImguiLocalDrawSupport::DrawOnCanvas(UCanvas* Canvas, APlayerController*)
{
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
			});
		FlushRenderingCommands();
		FontTextureData.Discard();
	}
	
	//---------------------------------------------------------------------------------------------
	// Ignore some viewports 
	//---------------------------------------------------------------------------------------------
	FCanvas* canvas 			= Canvas ? Canvas->Canvas : nullptr;
	FSceneView* sceneView 		= Canvas ? Canvas->SceneView : nullptr;	
	if( !canvas || !sceneView || sceneView->bIsSceneCapture || !sceneView->Family->bRealtimeUpdate /*
		!sceneView->Family->GetIsInFocus() || !sceneView->Family->bRealtimeUpdate*/ ) {
		return;
	}
	//GetWorldContextName
	//TMap<void*, ImGuiContext*> ContextsMap2;	
	
	const UWorld* world = sceneView->Family->Scene->GetWorld();
	FScene* scene		= sceneView->Family->Scene->GetRenderScene();
	ImGuiContext* savedContext 	= ImGui::GetCurrentContext();

	ViewportAssociation* pViewportInfo = ViewportMap.Find(scene);
	if ( !pViewportInfo ) {
		pViewportInfo = &ViewportMap.Add(scene);
		pViewportInfo->ImguiContext = ImGui::CreateContext(ImGui::GetIO().Fonts);
		//SetDPIScale(InDPIScale);
	}
	if (!pViewportInfo) {
		return;
	}

	//---------------------------------------------------------------------------------------------
	// Draw the Imgui Context
	//---------------------------------------------------------------------------------------------
	//SF for(FLevelEditorViewportClient* ViewportClient : LevelViewportClients) iteration
	//SF use Canvas.ViewProjection
#if 0
	FSceneViewport::UpdateCachedCursorPos()
	FVector2D LocalPixelMousePos = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	LocalPixelMousePos.X = FMath::Clamp(LocalPixelMousePos.X * CachedGeometry.Scale, (double)TNumericLimits<int32>::Min(), (double)TNumericLimits<int32>::Max());
	LocalPixelMousePos.Y = FMath::Clamp(LocalPixelMousePos.Y * CachedGeometry.Scale, (double)TNumericLimits<int32>::Min(), (double)TNumericLimits<int32>::Max());
	CachedCursorPos = LocalPixelMousePos.IntPoint();
#endif
	TArray< TSharedRef<SWindow> > windows = FSlateApplication::Get().GetInteractiveTopLevelWindows();
	auto posResult = windows[0]->GetPositionInScreen();

	auto mousePos = FSlateApplication::Get().GetCursorPos();
	ImVec2 mouseViewPos = ImVec2(mousePos.X - posResult.X, mousePos.Y - posResult.Y);
	//FVector mouseScreenPos = Canvas->ViewProjectionMatrix.TransformPosition(FVector(mousePos.X,mousePos.Y,0));
	//pViewportInfo->ViewportPos 		= ImVec2(Canvas->OrgX, Canvas->OrgY);
	pViewportInfo->LastUpdateTime 	= sceneView->Family->Time.GetRealTimeSeconds();
	ImGui::SetCurrentContext(pViewportInfo->ImguiContext);
	ImGuiIO& io 		= ImGui::GetIO();
	io.AddMousePosEvent(mouseViewPos.x, mouseViewPos.y);
	io.DisplaySize.x 	= Canvas->SizeX;
	io.DisplaySize.y 	= Canvas->SizeY;
	io.DeltaTime 		= sceneView->Family->Time.GetDeltaRealTimeSeconds();  //FGameTime Time = Canvas->GetTime();
	//SetFlag(IO.ConfigFlags, ImGuiConfigFlags_NavEnableKeyboard, InputState.IsKeyboardNavigationEnabled());
	//SetFlag(IO.ConfigFlags, ImGuiConfigFlags_NavEnableGamepad, InputState.IsGamepadNavigationEnabled());
	//SetFlag(IO.BackendFlags, ImGuiBackendFlags_HasGamepad, InputState.HasGamepad());
	io.FontGlobalScale 	= Canvas->GetDPIScale();
	ImGui::GetStyle().DisplaySafeAreaPadding.y = 32.f*Canvas->GetDPIScale();//SF	
	ImGui::NewFrame();
	ImGui::ShowDemoWindow(nullptr);
	ImGui::Render();
	ImGui::SetCurrentContext(savedContext);

	//---------------------------------------------------------------------------------------------
	// Add DebugDraw Item for the 'Dear ImGui' content of this viewport
	//---------------------------------------------------------------------------------------------
	FCanvasImguiRendererItem* ImguiRenderItem = new FCanvasImguiRendererItem(FontTexture, BlackTexture, pViewportInfo->ImguiContext);
	if (ImguiRenderItem->IsValid()) {
		FCanvas::FCanvasSortElement& SortElement = canvas->GetSortElement(canvas->TopDepthSortKey());
		SortElement.RenderBatchArray.Add(ImguiRenderItem);
	}
	else {
		delete ImguiRenderItem;
	}
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
		ViewProjection.Bind(Initializer.ParameterMap, TEXT("ViewProjection"));
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters&)
	{
		return true;
	}
	//private:
	LAYOUT_FIELD(FShaderParameter, ViewProjection)
};

class FDearImguiPS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FDearImguiPS);
	SHADER_USE_PARAMETER_STRUCT(FDearImguiPS, FGlobalShader)
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_TEXTURE(Texture2D, ImguiTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, ImguiSampler)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()
	
public:
		/*
		FDearImguiPS() = default;
		FDearImguiPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
		{
		ImguiTexture.Bind(Initializer.ParameterMap, TEXT("ImguiTexture"), SPF_Optional); //SF TEMP
		ImguiSampler.Bind(Initializer.ParameterMap, TEXT("ImguiSampler"), SPF_Optional);
		}
		*/
		static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

	//LAYOUT_FIELD(FShaderResourceParameter, ImguiTexture);
	//LAYOUT_FIELD(FShaderResourceParameter, ImguiSampler);
};

IMPLEMENT_GLOBAL_SHADER(FDearImguiVS, "/Plugin/UnrealNetimgui/Private/DearImguiShaders.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FDearImguiPS, "/Plugin/UnrealNetimgui/Private/DearImguiShaders.usf", "MainPS", SF_Pixel);

void NetImguiLocalDrawSupport::Initialize()
{
	//OnDrawImguiHandle 	= GetRendererModule().RegisterOverlayRenderDelegate(FPostOpaqueRenderDelegate::CreateRaw(this, &FNetImguiModule::RenderThread_RenderLocalDearImgui));
	//SF Game, TextRender, Editor
	OnDrawCanvasImguiHandle = UDebugDrawService::Register(TEXT("Rendering"), FDebugDrawDelegate::CreateRaw(this, &NetImguiLocalDrawSupport::DrawOnCanvas));
	
	ImGuiIO& io	= ImGui::GetIO();
	uint8_t* pPixelData(nullptr);
	int width(0), height(0);
	io.Fonts->GetTexDataAsRGBA32(&pPixelData, &width, &height); //SF TODO handle R8 & RGBA32 ?
	FontTextureData.Init(pPixelData, width, height);
	ActiveContextIndex = 0;
}

void NetImguiLocalDrawSupport::Terminate()
{
	UDebugDrawService::Unregister(OnDrawCanvasImguiHandle);
	FSlateApplication::Get().UnregisterInputPreProcessor(GImGuiInputProcessorRef);
	GImGuiInputProcessorRef = nullptr;
	//SF Destroy contexts
}

FCanvasImguiRendererItem::FCanvasImguiRendererItem(FTextureRHIRef fontTexture, FTextureRHIRef blackTexture, ImGuiContext* pContext)
: mFontTexture(fontTexture)
, mBlackTexture(blackTexture)
{
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
			for (int i(0); i<pDrawData->CmdListsCount; ++i) {
				ImDrawList& srcCmdList 					= *pDrawData->CmdLists[i];
				LocalImguiData::DrawList& dstCmdList 	= mLocalDrawData.CmdLists[i];
				dstCmdList.Flags 						= srcCmdList.Flags;
				dstCmdList.IndexOffset 					= mLocalDrawData.IdxBuffer.GetResourceDataSize() / sizeof(ImDrawIdx);
				dstCmdList.VertexOffset					= mLocalDrawData.VtxBuffer.GetResourceDataSize() / sizeof(ImDrawVert);
				dstCmdList.VertexCount 					= srcCmdList.VtxBuffer.size();
				dstCmdList.CmdBuffer.Append(srcCmdList.CmdBuffer.Data, srcCmdList.CmdBuffer.size());
				mLocalDrawData.IdxBuffer.Append((uint8_t*)srcCmdList.IdxBuffer.Data, srcCmdList.IdxBuffer.size_in_bytes());
				mLocalDrawData.VtxBuffer.Append(srcCmdList.VtxBuffer.Data, srcCmdList.VtxBuffer.size());
			}
		}
		mIsValid = mLocalDrawData.IdxBuffer.Num() > 0;
		ImGui::SetCurrentContext(savedContext);
	}
}

bool FCanvasImguiRendererItem::Render_RenderThread(FCanvasRenderContext& RenderContext, FMeshPassProcessorRenderState& DrawRenderState, const FCanvas* Canvas)
{
	FRDGBuilder& GraphBuilder = RenderContext.GraphBuilder;

	//SF TEST
//	static uint32 sTestOnce = 0;
//	if (sTestOnce == GFrameNumberRenderThread)return;
//	sTestOnce = GFrameNumberRenderThread;

	auto* PassParameters = GraphBuilder.AllocParameters<FDearImguiPS::FParameters>();
	PassParameters->ImguiTexture = mFontTexture;
	PassParameters->ImguiSampler = TStaticSamplerState<SF_Bilinear>::GetRHI();
	//PassParameters->RenderTargets[0] = FRenderTargetBinding(Parameters.ColorTexture, ERenderTargetLoadAction::ELoad);
	PassParameters->RenderTargets[0] = FRenderTargetBinding(RenderContext.GetRenderTarget(), ERenderTargetLoadAction::ELoad);
	const FIntRect ViewportRect = RenderContext.GetViewportRect();
	//const FIntPoint TextureExtent = Parameters.ColorTexture->Desc.Extent;
	//	PassParameters->View 						= Parameters.View->ViewUniformBuffer;
	//	PassParameters->SceneTextures 				= Parameters.SceneTexturesUniformParams;
	//	PassParameters->IrradianceCachingParameters = Scene->IrradianceCache->IrradianceCachingParametersUniformBuffer;	
	
	//LocalImguiData*& pLocalData = mpLocalDrawDatas[GFrameNumberRenderThread % UE_ARRAY_COUNT(mpLocalDrawDatas)];
	//if (!pLocalData) return;
	
	//SF TODO collapse all in 1 buffer?
	//static TArray<FBufferRHIRef> IndexBufferRHI;
	//IndexBufferRHI.Reset();
	
	
	FRHIResourceCreateInfo CreateInfoIdx(TEXT("FImguiIndexBuffer"), &mLocalDrawData.IdxBuffer);
	FBufferRHIRef IndexBufferRHI = RHICreateIndexBuffer(sizeof(ImDrawIdx), mLocalDrawData.IdxBuffer.GetResourceDataSize(), BUF_Volatile, CreateInfoIdx);

	FRHIResourceCreateInfo CreateInfoVtx(TEXT("FImguiVertexBuffer"), &mLocalDrawData.VtxBuffer);
	FBufferRHIRef VertexBufferRHI = RHICreateVertexBuffer(mLocalDrawData.VtxBuffer.GetResourceDataSize(), BUF_Volatile | BUF_ShaderResource, CreateInfoVtx);

	//---------------------------------------------------------------------------------------------
	// Add the Imgui DrawPass
	GraphBuilder.AddPass(
		RDG_EVENT_NAME("DearImgui"),
		PassParameters,
		ERDGPassFlags::Raster,
		[this, ViewportRect, PassParameters, IndexBufferRHI, VertexBufferRHI] (FRHICommandList& RHICmdList)
		{
			TShaderMapRef<FDearImguiVS> VertexShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			TShaderMapRef<FDearImguiPS> PixelShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
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
				
				float L 	= ViewportRect.Min.X;
				float R 	= ViewportRect.Max.X;
				float T 	= ViewportRect.Min.Y;
				float B 	= ViewportRect.Max.Y;
				float W 	= ViewportRect.Max.X - ViewportRect.Min.X;
				float H 	= ViewportRect.Max.Y - ViewportRect.Min.Y;
				float scale = 1.f;
				FMatrix ProjectionMatrix(
					FVector(2.0f/W*scale,			0.f, 				0.f),
					FVector(0.f, 					-2.0f/H*scale, 		0.f),
					FVector(0.f, 					0.f, 				0.5f),
					FVector(-1.f,	1.f,0.5f)
				);
				RHICmdList.SetViewport(ViewportRect.Min.X, ViewportRect.Min.Y, 0,	ViewportRect.Max.X, ViewportRect.Max.Y, 1);
				
				ClearUnusedGraphResources(PixelShader, PassParameters);
				RHICmdList.SetStreamSource(0, VertexBufferRHI, 0);
				SetShaderValue(RHICmdList, RHICmdList.GetBoundVertexShader(), VertexShader->ViewProjection, FMatrix44f(ProjectionMatrix));
				SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), *PassParameters);
				//SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), PixelShader->ImguiTexture, PixelShader->ImguiSampler, TStaticSamplerState<SF_Linear>::GetRHI(), FontTexture);

				for (int i(0); i<mLocalDrawData.CmdLists.Num(); ++i)
				{
					const LocalImguiData::DrawList& cmdList = mLocalDrawData.CmdLists[i];
					for (const ImDrawCmd& cmdDraw : cmdList.CmdBuffer)
					{
						// Project scissor/clipping rectangles into framebuffer space
						// Note 'ImDrawCmd::UserCallback' not supported for now
						ImVec2 clip_min(L + cmdDraw.ClipRect.x, T + cmdDraw.ClipRect.y);
						ImVec2 clip_max(FMath::Min(L + cmdDraw.ClipRect.z, R), 
										FMath::Min(T + cmdDraw.ClipRect.w, B));
						if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y || cmdDraw.UserCallback != nullptr)
							continue;
						
						RHICmdList.SetScissorRect(true, clip_min.x, clip_min.y, clip_max.x, clip_max.y);
						RHICmdList.DrawIndexedPrimitive(IndexBufferRHI, cmdList.VertexOffset + cmdDraw.VtxOffset, 0, 
														cmdList.VertexCount, cmdList.IndexOffset + cmdDraw.IdxOffset,cmdDraw.ElemCount / 3, 1);
					}
				}
			}
		});	

#if 0
	FGameTime Time = Canvas->GetTime();
	checkSlow(Data);

	const FRenderTarget* CanvasRenderTarget = Canvas->GetRenderTarget();

	const FSceneViewFamily& ViewFamily = *RenderContext.Alloc<const FSceneViewFamily>(FSceneViewFamily::ConstructionValues(
		CanvasRenderTarget,
		nullptr,
		FEngineShowFlags(ESFIM_Game))
		.SetTime(Time)
		.SetGammaCorrection(CanvasRenderTarget->GetDisplayGamma()));

	const FIntRect ViewRect(FIntPoint(0, 0), CanvasRenderTarget->GetSizeXY());

	// make a temporary view
	FSceneViewInitOptions ViewInitOptions;
	ViewInitOptions.ViewFamily = &ViewFamily;
	ViewInitOptions.SetViewRectangle(ViewRect);
	ViewInitOptions.ViewOrigin = FVector::ZeroVector;
	ViewInitOptions.ViewRotationMatrix = FMatrix::Identity;
	ViewInitOptions.ProjectionMatrix = Data->Transform.GetMatrix();
	ViewInitOptions.BackgroundColor = FLinearColor::Black;
	ViewInitOptions.OverlayColor = FLinearColor::White;

	const FSceneView& View = *RenderContext.Alloc<const FSceneView>(ViewInitOptions);

	Data->RenderTriangles(RenderContext, DrawRenderState, View, Canvas->IsHitTesting());

	if (Canvas->GetAllowedModes() & FCanvas::Allow_DeleteOnRender)
	{
		RenderContext.DeferredRelease(MoveTemp(Data));
		Data = nullptr;
	}
#endif
	return true;
}

	
bool FCanvasImguiRendererItem::Render_GameThread(const FCanvas* Canvas, FCanvasRenderThreadScope& RenderScope)
{
	//SF TODO
	RenderScope.EnqueueRenderCommand(
		[this, Canvas](FCanvasRenderContext& RenderContext)
		{
			// Render_RenderThread uses its own render state
			FMeshPassProcessorRenderState DummyRenderState;
			Render_RenderThread(RenderContext, DummyRenderState, Canvas);
		}
	);
	return true;
}


#if 0

IRendererModule* CachedRendererModule = NULL;

IRendererModule& GetRendererModule()
{
	if (!CachedRendererModule)
	{
		CachedRendererModule = &FModuleManager::LoadModuleChecked<IRendererModule>(TEXT("Renderer"));
	}

	return *CachedRendererModule;
}

void ResetCachedRendererModule()
{
	CachedRendererModule = NULL;
}


class FCanvasImguiRendererItem : public FCanvasBaseRenderItem
{
public:
	/**
	* Init constructor
	*/
	FCanvasImguiRendererItem(){}
	
	/**
		* Renders the canvas item.
		* Iterates over each triangle to be rendered and draws it with its own transforms
		*
		* @param Canvas - canvas currently being rendered
		* @param RHICmdList - command list to use
		* @return true if anything rendered
		*/
	virtual bool Render_RenderThread(FCanvasRenderContext& RenderContext, FMeshPassProcessorRenderState& DrawRenderState, const FCanvas* Canvas) override
	{
		FGameTime Time = Canvas->GetTime();
		checkSlow(Data);

		const FRenderTarget* CanvasRenderTarget = Canvas->GetRenderTarget();

		const FSceneViewFamily& ViewFamily = *RenderContext.Alloc<const FSceneViewFamily>(FSceneViewFamily::ConstructionValues(
			CanvasRenderTarget,
			nullptr,
			FEngineShowFlags(ESFIM_Game))
			.SetTime(Time)
			.SetGammaCorrection(CanvasRenderTarget->GetDisplayGamma()));

		const FIntRect ViewRect(FIntPoint(0, 0), CanvasRenderTarget->GetSizeXY());

		// make a temporary view
		FSceneViewInitOptions ViewInitOptions;
		ViewInitOptions.ViewFamily = &ViewFamily;
		ViewInitOptions.SetViewRectangle(ViewRect);
		ViewInitOptions.ViewOrigin = FVector::ZeroVector;
		ViewInitOptions.ViewRotationMatrix = FMatrix::Identity;
		ViewInitOptions.ProjectionMatrix = Data->Transform.GetMatrix();
		ViewInitOptions.BackgroundColor = FLinearColor::Black;
		ViewInitOptions.OverlayColor = FLinearColor::White;

		const FSceneView& View = *RenderContext.Alloc<const FSceneView>(ViewInitOptions);

		Data->RenderTriangles(RenderContext, DrawRenderState, View, Canvas->IsHitTesting());

		if (Canvas->GetAllowedModes() & FCanvas::Allow_DeleteOnRender)
		{
			RenderContext.DeferredRelease(MoveTemp(Data));
			Data = nullptr;
		}
		return true;
	}

	/**
		* Renders the canvas item.
		* Iterates over each triangle to be rendered and draws it with its own transforms
		*
		* @param Canvas - canvas currently being rendered
		* @return true if anything rendered
		*/
	virtual bool Render_GameThread(const FCanvas* Canvas, FCanvasRenderThreadScope& RenderScope) override
	{
		return true;
	}

private:
	class FTriangleVertexFactory : public FLocalVertexFactory
	{
	public:
		FTriangleVertexFactory(const FStaticMeshVertexBuffers* VertexBuffers, ERHIFeatureLevel::Type InFeatureLevel);
		void InitResource() override;

	private:
		const FStaticMeshVertexBuffers* VertexBuffers;
	};

	class FRenderData
	{
	public:
		FRenderData(ERHIFeatureLevel::Type InFeatureLevel,
		            const FMaterialRenderProxy* InMaterialRenderProxy,
		            const FCanvas::FTransformEntry& InTransform)
		: MaterialRenderProxy(InMaterialRenderProxy)
		, Transform(InTransform)
		, VertexFactory(&StaticMeshVertexBuffers, InFeatureLevel)
		{}

		FORCEINLINE int32 AddTriangle(const FCanvasUVTri& Tri, FHitProxyId HitProxyId)
		{
			FTriangleInst NewTri = { Tri, HitProxyId };
			return Triangles.Add(NewTri);
		};

		FORCEINLINE void AddReserveTriangles(int32 NumTriangles)
		{
			Triangles.Reserve(Triangles.Num() + NumTriangles);
		}

		FORCEINLINE void ReserveTriangles(int32 NumTriangles)
		{
			Triangles.Reserve(NumTriangles);
		}

		void RenderTriangles(
			FCanvasRenderContext& RenderContext,
			FMeshPassProcessorRenderState& DrawRenderState,
			const FSceneView& View,
			bool bIsHitTesting);

		const FMaterialRenderProxy* const MaterialRenderProxy;
		const FCanvas::FTransformEntry Transform;

		uint32 GetNumVertices() const { return 0; }; //SF
		uint32 GetNumIndices() const { return 0;  }; //SF

	private:
		FMeshBatch* AllocTriangleMeshBatch(FCanvasRenderContext& InRenderContext, FHitProxyId InHitProxyId);
		void InitTriangleMesh(const FSceneView& View);
		void ReleaseTriangleMesh();

		FRawIndexBuffer16or32 IndexBuffer;
		FStaticMeshVertexBuffers StaticMeshVertexBuffers;
		FTriangleVertexFactory VertexFactory;

		struct FTriangleInst
		{
			FCanvasUVTri Tri;
			FHitProxyId HitProxyId;
		};
		TArray<FTriangleInst> Triangles;
	};

	/**
		* Render data which is allocated when a new FCanvasTriangleRendererItem is added for rendering.
		* This data is only freed on the rendering thread once the item has finished rendering
		*/
	TSharedPtr<FRenderData> Data;

	//const bool bFreezeTime;
};

void FCanvasImguiRendererItem::FRenderData::InitTriangleMesh(const FSceneView& View)
{
	const uint32 NumIndices = GetNumIndices();
	const uint32 NumVertices = GetNumIndices();
	StaticMeshVertexBuffers.PositionVertexBuffer.Init(NumVertices);
	StaticMeshVertexBuffers.StaticMeshVertexBuffer.Init(NumVertices, 1);
	StaticMeshVertexBuffers.ColorVertexBuffer.Init(NumVertices);

	IndexBuffer.Indices.SetNum(NumIndices);
	// Make sure the index buffer is using the appropriate size :
	IndexBuffer.ForceUse32Bit(NumVertices > MAX_uint16);

	for (int32 i = 0; i < Triangles.Num(); i++)
	{
		const uint32 StartIndex = i * 3;

		/** The use of an index buffer here is actually necessary to workaround an issue with BaseVertexIndex, DrawPrimitive, and manual vertex fetch.
			*  In short, DrawIndexedPrimitive with StartIndex map SV_VertexId to the correct location, but DrawPrimitive with BaseVertexIndex will not.
			*/
		IndexBuffer.Indices[StartIndex + 0] = StartIndex + 0;
		IndexBuffer.Indices[StartIndex + 1] = StartIndex + 1;
		IndexBuffer.Indices[StartIndex + 2] = StartIndex + 2;

		const FCanvasUVTri& Tri = Triangles[i].Tri;

		// create verts. Notice the order is (1, 0, 2)
		StaticMeshVertexBuffers.PositionVertexBuffer.VertexPosition(StartIndex + 0) = FVector3f(Tri.V1_Pos.X, Tri.V1_Pos.Y, 0.0f);
		StaticMeshVertexBuffers.PositionVertexBuffer.VertexPosition(StartIndex + 1) = FVector3f(Tri.V0_Pos.X, Tri.V0_Pos.Y, 0.0f);
		StaticMeshVertexBuffers.PositionVertexBuffer.VertexPosition(StartIndex + 2) = FVector3f(Tri.V2_Pos.X, Tri.V2_Pos.Y, 0.0f);

		StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexTangents(StartIndex + 0, FVector3f(1.0f, 0.0f, 0.0f), FVector3f(0.0f, 1.0f, 0.0f), FVector3f(0.0f, 0.0f, 1.0f));
		StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexTangents(StartIndex + 1, FVector3f(1.0f, 0.0f, 0.0f), FVector3f(0.0f, 1.0f, 0.0f), FVector3f(0.0f, 0.0f, 1.0f));
		StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexTangents(StartIndex + 2, FVector3f(1.0f, 0.0f, 0.0f), FVector3f(0.0f, 1.0f, 0.0f), FVector3f(0.0f, 0.0f, 1.0f));

		StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexUV(StartIndex + 0, 0, FVector2f(Tri.V1_UV.X, Tri.V1_UV.Y));
		StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexUV(StartIndex + 1, 0, FVector2f(Tri.V0_UV.X, Tri.V0_UV.Y));
		StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexUV(StartIndex + 2, 0, FVector2f(Tri.V2_UV.X, Tri.V2_UV.Y));

		StaticMeshVertexBuffers.ColorVertexBuffer.VertexColor(StartIndex + 0) = Tri.V1_Color.ToFColor(true);
		StaticMeshVertexBuffers.ColorVertexBuffer.VertexColor(StartIndex + 1) = Tri.V0_Color.ToFColor(true);
		StaticMeshVertexBuffers.ColorVertexBuffer.VertexColor(StartIndex + 2) = Tri.V2_Color.ToFColor(true);
	}

	StaticMeshVertexBuffers.PositionVertexBuffer.InitResource();
	StaticMeshVertexBuffers.StaticMeshVertexBuffer.InitResource();
	StaticMeshVertexBuffers.ColorVertexBuffer.InitResource();
	IndexBuffer.InitResource();
	VertexFactory.InitResource();
};

void FCanvasImguiRendererItem::FRenderData::ReleaseTriangleMesh()
{
	VertexFactory.ReleaseResource();
	IndexBuffer.ReleaseResource();
	StaticMeshVertexBuffers.PositionVertexBuffer.ReleaseResource();
	StaticMeshVertexBuffers.StaticMeshVertexBuffer.ReleaseResource();
	StaticMeshVertexBuffers.ColorVertexBuffer.ReleaseResource();
}

FMeshBatch* FCanvasImguiRendererItem::FRenderData::AllocTriangleMeshBatch(FCanvasRenderContext& InRenderContext, FHitProxyId InHitProxyId)
{
	FMeshBatch* MeshBatch = InRenderContext.Alloc<FMeshBatch>();

	MeshBatch->VertexFactory = &VertexFactory;
	MeshBatch->MaterialRenderProxy = MaterialRenderProxy;
	MeshBatch->ReverseCulling = false;
	MeshBatch->bDisableBackfaceCulling = true;
	MeshBatch->Type = PT_TriangleList;
	MeshBatch->DepthPriorityGroup = SDPG_Foreground;
	MeshBatch->BatchHitProxyId = InHitProxyId;

	FMeshBatchElement& MeshBatchElement = MeshBatch->Elements[0];
	MeshBatchElement.IndexBuffer = &IndexBuffer;
	MeshBatchElement.FirstIndex = 0;
	MeshBatchElement.NumPrimitives = 0;
	MeshBatchElement.MinVertexIndex = 0;
	MeshBatchElement.MaxVertexIndex = GetNumVertices() - 1;
	MeshBatchElement.PrimitiveUniformBufferResource = &GIdentityPrimitiveUniformBuffer;

	return MeshBatch;
}


void FCanvasImguiRendererItem::FRenderData::RenderTriangles(
	FCanvasRenderContext& RenderContext,
	FMeshPassProcessorRenderState& DrawRenderState,
	const FSceneView& View,
	bool bIsHitTesting)
{

	check(IsInRenderingThread());

	if (Triangles.Num() == 0)
	{
		return;
	}

//	RDG_GPU_STAT_SCOPE(RenderContext.GraphBuilder, CanvasDrawTriangles);
//	RDG_EVENT_SCOPE(RenderContext.GraphBuilder, "%s", *MaterialRenderProxy->GetIncompleteMaterialWithFallback(GMaxRHIFeatureLevel).GetFriendlyName());
//	TRACE_CPUPROFILER_EVENT_SCOPE(CanvasDrawTriangles);
//	QUICK_SCOPE_CYCLE_COUNTER(STAT_CanvasDrawTriangles)

	IRendererModule& RendererModule = GetRendererModule();

	InitTriangleMesh(View);


	// We know we have at least 1 triangle so prep up a new batch right away : 
	FMeshBatch* CurrentMeshBatch = AllocTriangleMeshBatch(RenderContext, Triangles[0].HitProxyId);
	check (CurrentMeshBatch->Elements[0].FirstIndex == 0); // The first batch should always start at the first index 

	for (int32 TriIdx = 0; TriIdx < Triangles.Num(); TriIdx++)
	{
		const FTriangleInst& Tri = Triangles[TriIdx];

		// We only need a new batch when the hit proxy id changes : 
		if (CurrentMeshBatch->BatchHitProxyId != Tri.HitProxyId)
		{
			// Flush the current batch before allocating a new one: 
			RendererModule.DrawTileMesh(RenderContext, DrawRenderState, View, *CurrentMeshBatch, bIsHitTesting, CurrentMeshBatch->BatchHitProxyId);

			CurrentMeshBatch = AllocTriangleMeshBatch(RenderContext, Tri.HitProxyId);
			CurrentMeshBatch->Elements[0].FirstIndex = 3 * TriIdx;
		}

		// Add 1 triangle to the batch :
		++CurrentMeshBatch->Elements[0].NumPrimitives;
	}

	// Flush the final batch: 
	check(CurrentMeshBatch != nullptr);
	RendererModule.DrawTileMesh(RenderContext, DrawRenderState, View, *CurrentMeshBatch, bIsHitTesting, CurrentMeshBatch->BatchHitProxyId);

	AddPass(RenderContext.GraphBuilder, RDG_EVENT_NAME("ReleaseTriangleMesh"), [this](FRHICommandListImmediate&)
	        {
	        ReleaseTriangleMesh();
			});
}

void FNetImguiModule::DrawOnCanvas(UCanvas* Canvas, APlayerController*)
{
	ImGui::SetCurrentContext(mpContext);
	ImDrawData* pDrawData 	= ImGui::GetDrawData();	//SF TODO create a 'mpContext' drawdata copy to be safe?
	FCanvas* canvas 		= Canvas ? Canvas->Canvas : nullptr;
	if (!canvas || !pDrawData) {
		return;
	}

	FCanvas::FCanvasSortElement& SortElement = canvas->GetSortElement(canvas->TopDepthSortKey());
	//INC_DWORD_STAT(STAT_Canvas_NumBatchesCreated);
	FCanvasImguiRendererItem* RenderBatch = new FCanvasImguiRendererItem();
	SortElement.RenderBatchArray.Add(RenderBatch);

	//SCOPE_CYCLE_COUNTER(STAT_Canvas_TileTextureItemTime);
	//ImTextureID textureIdCurrent 	= reinterpret_cast<ImTextureID>(0xFFFFFFFFFFFFFFFF);
	FHitProxyId HitProxyId 				= Canvas->Canvas->GetHitProxyId();
	ESimpleElementBlendMode BlendMode = ESimpleElementBlendMode::SE_BLEND_Translucent;
	// Draw every DearImgui element
	for (int cmdListIdx(0); cmdListIdx < pDrawData->CmdListsCount; cmdListIdx++) {
		const ImDrawList& cmdList 			= *pDrawData->CmdLists[cmdListIdx];
		const FTexture* texture				= (FTexture*)Canvas->DefaultTexture->GetResource();
		FBatchedElements* BatchedElements 	= Canvas->Canvas->GetBatchedElements(FCanvas::ET_Triangle, nullptr, texture, BlendMode);
		int32 debugTotal 					= 0;
		BatchedElements->AddReserveVertices(cmdList.VtxBuffer.size());
		BatchedElements->AddReserveTriangles(cmdList.IdxBuffer.size()/3,texture, BlendMode);
		for (int i(0); i<cmdList.VtxBuffer.size(); ++i) {
			int32 index = BatchedElements->AddVertexf(
				FVector4f(cmdList.VtxBuffer[i].pos.x, cmdList.VtxBuffer[i].pos.y, 0, 1), 
				FVector2f{cmdList.VtxBuffer[i].uv.x, cmdList.VtxBuffer[i].uv.y}, 
				FLinearColor(FColor(cmdList.VtxBuffer[i].col)), 
				HitProxyId);
			debugTotal += index;
		}
		printf("total: %i", debugTotal);
		
		int idxOffset(0);
		for(int cmdIdx(0); cmdIdx < cmdList.CmdBuffer.size(); ++cmdIdx)
		{
			const ImDrawCmd& cmd 	= cmdList.CmdBuffer[cmdIdx];
			if( cmd.UserCallback == nullptr ){
				for (unsigned int tri(0); tri<cmd.ElemCount/3; ++tri) {
					BatchedElements->AddTriangle(cmd.IdxOffset+cmdList.IdxBuffer[idxOffset+0], 
					                             cmd.IdxOffset+cmdList.IdxBuffer[idxOffset+1], 
					                             cmd.IdxOffset+cmdList.IdxBuffer[idxOffset+2], 
					                             texture, BlendMode);
					idxOffset += 3;
				}

				/*
					pOutDraws[drawCount].mVtxOffset		= pCmd->VtxOffset;
					pOutDraws[drawCount].mIdxOffset		= pCmd->IdxOffset;
					pOutDraws[drawCount].mTextureId		= TextureCastFromID(pCmd->TextureId);
					pOutDraws[drawCount].mIdxCount		= pCmd->ElemCount;
					pOutDraws[drawCount].mClipRect[0]	= pCmd->ClipRect.x;
					pOutDraws[drawCount].mClipRect[1]	= pCmd->ClipRect.y;
					pOutDraws[drawCount].mClipRect[2]	= pCmd->ClipRect.z;
					pOutDraws[drawCount].mClipRect[3]	= pCmd->ClipRect.w;
					++drawCount;
					*/
			}
		}
	}

	Canvas->K2_DrawTexture(
		nullptr, //UTexture* RenderTexture, 
		FVector2D(128,128), //FVector2D ScreenPosition, 
		FVector2D(256,256), //FVector2D ScreenSize, 
		FVector2D(0,0), //FVector2D CoordinatePosition, 
		FVector2D(1,1),	//FVector2D CoordinateSize=FVector2D::UnitVector, 
		FLinearColor::White, 
		EBlendMode::BLEND_Translucent 
		//float Rotation=0.f, 
		//FVector2D PivotPoint=FVector2D(0.5f,0.5f));
	);
	//FViewport* pViewPort = GEditor->GetActiveViewport();
	//Canvas->SceneView == 
	const FColor OldDrawColor = Canvas->DrawColor;
	const FFontRenderInfo FontRenderInfo = Canvas->CreateFontRenderInfo(true, false);
	const FFontRenderInfo FontRenderInfoWithShadow = Canvas->CreateFontRenderInfo(true, true);

	Canvas->SetDrawColor(FColor::White);

	UFont* RenderFont = GEngine->GetSmallFont();

	const FSceneView* View = Canvas->SceneView;
	/*
		for (auto It = Texts.CreateConstIterator(); It; ++It)
		{
		if (FDebugRenderSceneProxy::PointInView(It->Location, View))
		{
		const FVector ScreenLoc = Canvas->Project(It->Location);
		const FFontRenderInfo& FontInfo = TextWithoutShadowDistance >= 0 ? (FDebugRenderSceneProxy::PointInRange(It->Location, View, TextWithoutShadowDistance) ? FontRenderInfoWithShadow : FontRenderInfo) : FontRenderInfo;
		Canvas->SetDrawColor(It->Color);
		Canvas->DrawText(RenderFont, It->Text, ScreenLoc.X, ScreenLoc.Y, 1, 1, FontInfo);
		}
		}*/
	
	//const FVector ScreenLoc = Canvas->Project(It->Location);
	const FFontRenderInfo& FontInfo = FontRenderInfoWithShadow; //TextWithoutShadowDistance >= 0 ? (FDebugRenderSceneProxy::PointInRange(It->Location, View, TextWithoutShadowDistance) ? FontRenderInfoWithShadow : FontRenderInfo) : FontRenderInfo;
	//Canvas->SetDrawColor(It->Color);
	Canvas->DrawText(RenderFont, TEXT("Test-Game"), 64, 64, 1, 1, FontInfo);
	Canvas->SetDrawColor(OldDrawColor);
}
#endif

#endif //NETIMGUI_ENABLED

#pragma optimize("", on) //SF
