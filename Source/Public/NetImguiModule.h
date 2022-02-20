// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <Modules/ModuleManager.h>

#ifndef NETIMGUI_ENABLED
	#define NETIMGUI_ENABLED 0
#endif

#if NETIMGUI_ENABLED

//=================================================================================================
// Additional Header includes
//=================================================================================================
#include "../Private/ThirdParty/DearImgui/imgui.h"

// Note1:	You can toggle which Icon Font are active, in 'NetImgui.Build.cs'
// 
// Note2:	'Icon Game Kenney' can always be active, but only one from 
//			'Font Awesome' or 'MaterialDesign Icon' can be active at a time
// 
// Note3:	For Icon usage example, please take a look at NetImguiDemoActor.cpp

// List of defines to easily use Icons available in 'Kenney's Game Icons'
// For list available icons, see: https://kenney.nl/assets/game-icons and https://kenney.nl/assets/game-icons-expansion
#if NETIMGUI_FONT_ICON_GAMEKENNEY
	#include "../Private/Fonts/IconFontCppHeader/IconsKenney.h"
#endif

// List of defines to easily use Icons available in 'Font Awesome Icons' (only the 'free' subset is made available)
// For list of available icons, see: https://fontawesome.com/v5/search?m=free (Regular/Solid)
#if NETIMGUI_FONT_ICON_AWESOME
	#include "../Private/Fonts/IconFontCppHeader/IconsFontAwesome5.h"
#endif

// List of defines to easily use Icons available in 'Google's Material Design Icons'
// For list of available icons, see: https://fonts.google.com/icons
#if NETIMGUI_FONT_ICON_MATERIALDESIGN
	#include "../Private/Fonts/IconFontCppHeader/IconsMaterialDesign.h"
#endif

// Forward declarations
class FNetImguiModule;
namespace NetImguiHelper
{
	inline bool IsDrawing();
	inline FNetImguiModule* Get();
}

#endif //NETIMGUI_ENABLED


//=================================================================================================
// NETIMGUI Module
//=================================================================================================
class FNetImguiModule : public IModuleInterface
{
public:
//Note: Avoiding higher cost of finding the module in a list, by instead relying on
//		a static pointer to loaded module. Using the pointer should be faster than 
//		using this Load/Find module.
// 
//		Since static functions are not allowed access to static pointer declared in the DLL, 
//		we are relying on a namespace with inline functions instead (after this class).
#if 0
	/**
	 * Singleton-like access to this module's interface. This is just for convenience!
	 * Beware of calling this during the shutdown phase, though. Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline FNetImguiModule&	Get() {
		return FModuleManager::LoadModuleChecked<FNetImguiModule>("NetImgui");
	}
#endif

	/**
	 * Checks to see if this module is loaded and ready. It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool				IsAvailable() { return FModuleManager::Get().IsModuleLoaded("NetImgui"); }


	/** IModuleInterface implementation */
	virtual void					StartupModule() override;
	virtual void					ShutdownModule() override;

#if NETIMGUI_ENABLED

	enum class eFont
	{
		kProggyClean,		// Built-in Dear ImGui Font
		kCousineFixed16,	// Fixed size font + 'Kenney Game Icons' + 'Font Awesome Icons' or 'Material Design Icons'
		kCousineFixed20,	//  "
		kCousineFixed24,	//  "
		kDroidSans,			// TrueType font + 'Kenney Game Icons' + 'Font Awesome Icons' or 'Material Design Icons'
		kKarlaRegular,		// TrueType font + 'Kenney Game Icons' + 'Font Awesome Icons' or 'Material Design Icons'
		kProggyTiny,		// Tiny debug font
		kRobotoMedium16,	// TrueType font + 'Kenney Game Icons' + 'Font Awesome Icons' or 'Material Design Icons'
		kIcons32,			// Big Icons font with 'Kenney Game Icons' + 'Font Awesome Icons' or 'Material Design Icons'
		kIcons64,			//  "
	#if NETIMGUI_FONT_JAPANESE
		kJapanese16,		// Japanese Mincho font + 'Kenney Game Icons' + 'Font Awesome Icons' or 'Material Design Icons'
		kJapanese24,		// "
		kJapanese32,		// "
	#endif
		
		//... Your own font can be added here and loaded in 'FNetImguiModule::StartupModule()' in same order
		//... Feel free to also add/remove font size for your convenience
		_Count,
	};

	virtual void					SetDefaultFont(eFont font);
	virtual void					PushFont(eFont font);
	virtual void					PopFont();
	virtual bool					IsConnected()const;
	
	//---------------------------------------------------------------------------------------------
	// Add your Dear ImGui drawing callbacks to this emitter
	static FSimpleMulticastDelegate OnDrawImgui;
	//---------------------------------------------------------------------------------------------

protected:	
	virtual bool					IsDrawing()const;
	void							Update();
	inline ImGuiContext*			GetContext(){ return mpContext; }
	FDelegateHandle					mUpdateCallback;
	struct ImGuiContext*			mpContext = nullptr;
	
	// Statics (stays valid between module reload)
	static FNetImguiModule*			spLoadedModule;
	static eFont					sDefaultFont;

	// Friend access
	friend bool						NetImguiHelper::IsDrawing();
	friend FNetImguiModule*			NetImguiHelper::Get();
#endif //NETIMGUI_ENABLED
};

#if NETIMGUI_ENABLED

namespace NetImguiHelper
{

/**
* Retrieve the active NetImgui module. The pointer is garanteed to be valid while IsDrawing()
* is true, or inside 'OnDrawImgui' callback.
*
* @return The Current loaded and active FNetImguiModule
*/
FNetImguiModule* Get()
{
	return FNetImguiModule::spLoadedModule;
}

/**
* Make sure that the FNetImguiModule is loaded properly, and that NetImgui currently expect
* some Dear ImGui drawing. With 'FrameSkip' enabled, there are frames where we are not waiting
* on some Dear Imgui drawing, and attempting to do so will result in an error.
*
* Note: Use this method when trying to draw Dear ImGui content anywhere in your code. It is not
*		required when drawing is happening inside a 'OnDrawImgui' callback.
* 
* @return True if the module is loaded expecting some Dear ImGui draws this frame
*/
bool IsDrawing()
{
	if ( Get() && Get()->IsDrawing() )
	{
		// HotReload note: When a dll is reloaded, original dll is still loaded and all 'Dear ImGui' 
		// functions are still pointing to it when called from outside this dll. Only this module object
		// is recreated. This means that the game code will call original dll but this module object 
		// will use reloaded dll ImGui functions. To prevent issue with destroyed context, we are 
		// making sure that the original dll knows about this module's newly created context here.
		ImGui::SetCurrentContext(Get()->GetContext());
		return true;
	}
	return false;
}

/**
* Tell us if the plugin has a working connection established with NetImgui remote server
* 
* @return True if the module is connected to a NetImgui remote server
*/
bool IsConnected()
{
	return Get() && Get()->IsConnected();
}

/**
// Helper class that change the font and automatically restore it when object is out of scope
*/
struct ScopedFont
{
	inline ScopedFont(FNetImguiModule::eFont font){ Get()->PushFont(font); }
	inline ~ScopedFont(){ Get()->PopFont(); }
};

}
#endif
