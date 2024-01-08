#pragma once

#include "NetImgui_Shared.h"
#include "NetImgui_CmdPackets.h"

//=============================================================================
// Forward Declares
//=============================================================================
namespace NetImgui { namespace Internal { namespace Network { struct SocketInfo; } } }

namespace NetImgui { namespace Internal { namespace Client
{

//=============================================================================
// Keep a list of textures used by Imgui, needed by server
//=============================================================================
struct ClientTexture
{
	inline void	Set( CmdTexture* pCmdTexture );
	inline bool	IsValid()const;	
	CmdTexture* mpCmdTexture= nullptr;
	bool		mbSent		= false;
	uint8_t		mPadding[7]	= {};
};

//=============================================================================
// Keeps a list of ImGui context values NetImgui overrides (to restore)
//=============================================================================
struct SavedImguiContext
{
	void					Save(ImGuiContext* copyFrom);
	void					Restore(ImGuiContext* copyTo);	
	const char*				mBackendPlatformName						= nullptr;
	const char*				mBackendRendererName						= nullptr;
	const char*				(*mGetClipboardTextFn)(void*)				= nullptr;
    void					(*mSetClipboardTextFn)(void*, const char*)	= nullptr;
	void*					mClipboardUserData							= nullptr;
    void*					mImeWindowHandle							= nullptr;
	float					mFontGlobalScale							= 1.f;
	float					mFontGeneratedSize							= 0.f;
	ImGuiBackendFlags		mBackendFlags								= 0;
	ImGuiConfigFlags		mConfigFlags								= 0;	
	bool					mDrawMouse									= false;
	bool					mSavedContext								= false;
	char					mPadding1[2]								= {};
	int						mKeyMap[ImGuiKey_COUNT]						= {};
	char					mPadding2[8 - (sizeof(mKeyMap) % 8)]		={};	
};

//=============================================================================
// Keep all Client infos needed for communication with server
//=============================================================================
struct ClientInfo
{
	using VecTexture	= ImVector<ClientTexture>;
	using BufferKeys	= Ringbuffer<uint16_t, 1024>;
	using Time			= std::chrono::time_point<std::chrono::high_resolution_clock>;

	struct InputState
	{
		uint64_t						mInputDownMask[(CmdInput::ImGuiKey_COUNT+63)/64] = {};
		float							mInputAnalog[CmdInput::kAnalog_Count] = {};
		uint64_t						mMouseDownMask				= 0;
		float							mMouseWheelVertPrev			= 0.f;
		float							mMouseWheelHorizPrev		= 0.f;
	};
										ClientInfo();
										~ClientInfo();
	void								ContextInitialize();
	void								ContextOverride();
	void								ContextRestore();
	void								ContextRemoveHooks();
	inline bool							IsContextOverriden()const;

	std::atomic<Network::SocketInfo*>	mpSocketPending;						// Hold socket info until communication is established
	std::atomic<Network::SocketInfo*>	mpSocketComs;							// Socket used for communications with server
	std::atomic<Network::SocketInfo*>	mpSocketListen;							// Socket used to wait for communication request from server
	VecTexture							mTextures;								// List if textures created by this client (used un main thread)
	char								mName[64]					= {};
	uint64_t							mFrameIndex					= 0;		// Incremented everytime we send a DrawFrame Command	
	CmdTexture*							mTexturesPending[16];
	ExchangePtr<CmdDrawFrame>			mPendingFrameOut;
	ExchangePtr<CmdBackground>			mPendingBackgroundOut;
	ExchangePtr<CmdInput>				mPendingInputIn;
	ExchangePtr<CmdClipboard>			mPendingClipboardIn;					// Clipboard content received from Server and waiting to be taken by client
	ExchangePtr<CmdClipboard>			mPendingClipboardOut;					// Clipboard content copied on Client and waiting to be sent to Server
	ImGuiContext*						mpContext					= nullptr;	// Context that the remote drawing should use (either the one active when connection request happened, or a clone)
	CmdInput*							mpCmdInputPending			= nullptr;	// Last Input Command from server, waiting to be processed by client
	CmdClipboard*						mpCmdClipboard				= nullptr;	// Last received clipboad command
	CmdDrawFrame*						mpCmdDrawLast				= nullptr;	// Last sent Draw Command. Used by data compression, to generate delta between previous and current frame
	CmdBackground						mBGSetting;								// Current value assigned to background appearance by user
	CmdBackground						mBGSettingSent;							// Last sent value to remote server
	BufferKeys							mPendingKeyIn;							// Keys pressed received. Results of 2 CmdInputs are concatenated if received before being processed
	ImVec2								mSavedDisplaySize			= {0, 0};	// Save original display size on 'NewFrame' and restore it on 'EndFrame' (making sure size is still valid after a disconnect)
	const void*							mpFontTextureData			= nullptr;	// Last font texture data send to server (used to detect if font was changed)
	ImTextureID							mFontTextureID;
	SavedImguiContext					mSavedContextValues;
	Time								mTimeTracking;							// Used to update Dear ImGui time delta on remote context
	std::atomic_uint32_t				mTexturesPendingSent;
	std::atomic_uint32_t				mTexturesPendingCreated;
	
	bool								mbDisconnectRequest			= false;	// Waiting to Disconnect
	bool								mbClientThreadActive		= false;
	bool								mbListenThreadActive		= false;
	bool								mbHasTextureUpdate			= false;
	bool								mbIsDrawing					= false;	// We are inside a 'NetImgui::NewFrame' / 'NetImgui::EndFrame' (even if not for a remote draw)
	bool								mbIsRemoteDrawing			= false;	// True if the rendering it meant for the remote netImgui server
	bool								mbRestorePending			= false;	// Original context has had some settings overridden, original values stored in mRestoreXXX	
	bool								mbFontUploaded				= false;	// Auto detect if font was sent to server
	bool								mbInsideHook				= false;	// Currently inside ImGui hook callback
	bool								mbInsideNewEnd				= false;	// Currently inside NetImgui::NewFrame() or NetImgui::EndFrame() (prevents recusrive hook call)
	bool								mbValidDrawFrame			= false;	// If we should forward the drawdata to the server at the end of ImGui::Render()
	uint8_t								mClientCompressionMode		= eCompressionMode::kUseServerSetting;
	bool								mServerCompressionEnabled	= false;	// If Server would like compression to be enabled (mClientCompressionMode value can override this value)
	bool								mServerCompressionSkip		= false;	// Force ignore compression setting for 1 frame
	char								PADDING[1];
	FontCreateFuncPtr					mFontCreationFunction		= nullptr;	// Method to call to generate the remote ImGui font. By default, re-use the local font, but this doesn't handle native DPI scaling on remote server
	float								mFontCreationScaling		= 1.f;		// Last font scaling used when generating the NetImgui font
	InputState							mPreviousInputState;					// Keeping track of last keyboard/mouse state
	ImGuiID								mhImguiHookNewframe			= 0;
	ImGuiID								mhImguiHookEndframe			= 0;
	
	void								ProcessDrawData(const ImDrawData* pDearImguiData, ImGuiMouseCursor mouseCursor);
	void								ProcessTexturePending();
	inline bool							IsConnected()const;
	inline bool							IsConnectPending()const;
	inline bool							IsActive()const;
	inline void							KillSocketComs();						// Kill communication sockets (should only be called from communication thread)
	inline void							KillSocketListen();						// Kill connecting listening socket (should only be called from communication thread)

// Prevent warnings about implicitly created copy
protected:
	ClientInfo(const ClientInfo&)=delete;
	ClientInfo(const ClientInfo&&)=delete;
	void operator=(const ClientInfo&)=delete;
};

//=============================================================================
// Main communication thread, that should be started in its own thread
//=============================================================================
void CommunicationsClient(void* pClientVoid);
void CommunicationsHost(void* pClientVoid);

}}} //namespace NetImgui::Internal::Client

#include "NetImgui_Client.inl"
