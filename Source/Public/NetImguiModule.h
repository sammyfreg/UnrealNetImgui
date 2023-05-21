// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <Modules/ModuleManager.h>

#ifndef NETIMGUI_ENABLED
	#define NETIMGUI_ENABLED 0

#elif NETIMGUI_ENABLED

//=================================================================================================
// Additional Header includes
// Note: The following 'Defines' are optional features enabled by user in NetImgui.Build.cs
//=================================================================================================
#include "imgui.h"

/// See https://github.com/epezent/implot for more info
#if NETIMGUI_IMPLOT_ENABLED
	#include "implot.h"
#endif

// See https://github.com/thedmd/imgui-node-editor for more info
#if NETIMGUI_NODE_EDITOR_ENABLED
	#include "imgui_node_editor.h"
#endif

// Note1:	Active Icon Fonts can be toggled in 'NetImgui.Build.cs'
// 
// Note2:	'Icon Game Kenney' can always be active. However, only one from 
//			'Font Awesome' or 'MaterialDesign Icon' can be active at a time
// 
// Note3:	For Icon usage example, please take a look at NetImguiDemoActor.cpp

// List of defines to easily use Icons available in 'Kenney's Game Icons'
// For list available icons, see: https://kenney.nl/assets/game-icons and https://kenney.nl/assets/game-icons-expansion
#if NETIMGUI_FONT_ICON_GAMEKENNEY
	#include "Fonts/IconFontCppHeader/IconsKenney.h"
#endif

// List of defines to easily use Icons available in 'Font Awesome Icons' (only the 'free' subset is made available)
// For list of available icons, see: https://fontawesome.com/v6/search?m=free (Regular/Solid/Brands)
#if NETIMGUI_FONT_ICON_AWESOME
	#include "Fonts/IconFontCppHeader/IconsFontAwesome6.h"
	#include "Fonts/IconFontCppHeader/IconsFontAwesome6Brands.h"
#endif

// List of defines to easily use Icons available in 'Google's Material Design Icons'
// For list of available icons, see: https://fonts.google.com/icons
#if NETIMGUI_FONT_ICON_MATERIALDESIGN
	#include "Fonts/IconFontCppHeader/IconsMaterialDesign.h"
#endif

#endif //NETIMGUI_ENABLED


//=================================================================================================
// NETIMGUI Module
//=================================================================================================
class FNetImguiModule : public IModuleInterface
{
public:	
	/**
	 * Singleton-like access to this module's interface. This is just for convenience!
	 * Beware of calling this during the shutdown phase, though. Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline FNetImguiModule&	Get() {
		// Avoid lookup by finding the element once per 'Frame/Dll' and storing the pointer
		static FNetImguiModule* spLoadedModule = nullptr;
		static uint64 sLastFrame = 0;
		if( !spLoadedModule || sLastFrame != GFrameCounter ){
			spLoadedModule = static_cast<FNetImguiModule*>(&FModuleManager::LoadModuleChecked<FNetImguiModule>("NetImgui"));
		}
		return *spLoadedModule;
	}

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
		kCousineFixed16,	// Fixed size font + Japanese Mincho font + 'Kenney Game Icons' + 'Font Awesome Icons' or 'Material Design Icons'
		kCousineFixed20,	//  "
		kCousineFixed24,	//  "
		kDroidSans,			// TrueType font + 'Kenney Game Icons' + 'Font Awesome Icons' or 'Material Design Icons'
		kKarlaRegular,		// TrueType font + 'Kenney Game Icons' + 'Font Awesome Icons' or 'Material Design Icons'
		kProggyTiny,		// Tiny debug font
		kRobotoMedium16,	// TrueType font + 'Kenney Game Icons' + 'Font Awesome Icons' or 'Material Design Icons'
		kIcons32,			// Big Icons font with 'Kenney Game Icons' + 'Font Awesome Icons' or 'Material Design Icons'
		kIcons64,			//  "
	#if NETIMGUI_FONT_JAPANESE
		kJapanese32,		// Japanese Mincho font + 'Kenney Game Icons' + 'Font Awesome Icons' or 'Material Design Icons'
	#endif
		
		//... Your own font can be added here and loaded in 'FNetImguiModule::StartupModule()' in same order
		//... Feel free to also add/remove font size for your convenience
		_Count,
	};

	virtual void					SetDefaultFont(eFont font);
	virtual void					PushFont(eFont font);
	virtual void					PopFont();
	
	/**
	* Tell us if the plugin has a working connection established with NetImgui remote server
	* 
	* @return True if the module is connected to a NetImgui remote server
	*/
	virtual bool					IsConnected()const;

	/**	
	* Use this method when drawing Dear ImGui content on the gamethread.
	* It is not required when drawing is happening inside a 'OnDrawImgui' callback.
	* 
	* With 'FrameSkip' enabled, there are frames where we are not waiting on new 
	* Dear Imgui drawing, and attempting to do so will result in an error.
	* 
	* @return True if the module is expecting some Dear ImGui draws this frame
	*/
	inline bool						IsDrawing()
	{
		checkSlow(IsInGameThread());
		if ( isDrawing() )
		{
			// HotReload note: When a dll is reloaded, original dll is still loaded and all 'Dear ImGui' 
			// functions are still pointing to it when called from outside this dll. Only this module object
			// is recreated. This means that the game code will call original dll but this module object 
			// will use reloaded dll ImGui functions. To prevent issue with destroyed context, we are 
			// making sure that the original dll knows about this module's newly created context here.
			ImGui::SetCurrentContext(Get().mpContext);
			return true;
		}
		return false;
	}

	/**
	* Add your Dear ImGui drawing callbacks to this emitter
	** Note: If NetImgui module is reloaded, you will lose your callbacks
	*/
	FSimpleMulticastDelegate		OnDrawImgui;

protected:	
	virtual bool					isDrawing()const;
	void							Update();
	FDelegateHandle					mUpdateCallback;
	ImGuiContext*					mpContext = nullptr;
#if NETIMGUI_IMPLOT_ENABLED
	ImPlotContext*					mpImPlotContext = nullptr;
#endif
#endif //NETIMGUI_ENABLED
};

#if NETIMGUI_ENABLED
struct NetImguiScopedFont
{
	inline NetImguiScopedFont(FNetImguiModule::eFont font){ FNetImguiModule::Get().PushFont(font); }
	inline ~NetImguiScopedFont(){ FNetImguiModule::Get().PopFont(); }
};
#endif
