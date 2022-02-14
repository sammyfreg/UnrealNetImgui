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


// List of defines to easily use each Icon available in 'Kenney's Game Icons'
// For list available icons, see: https://kenney.nl/assets/game-icons and https://kenney.nl/assets/game-icons-expansion
#if NETIMGUI_FONT_ICON_GAMEKENNEY	
	#include "../Private/Fonts/IconFontCppHeader/IconsKenney.h"
#endif

// List of defines to easily use each Icon available 'Font Awesome Icons' (only the 'free' subset is made available)
// For list of available icons, see: https://fontawesome.com/v5/search?m=free (Regular/Solid)
#if NETIMGUI_FONT_ICON_AWESOME	
	#include "../Private/Fonts/IconFontCppHeader/IconsFontAwesome5.h"
#endif

// List of defines to easily use each Icon available in 'Google's Material Design Icons'
// For list of available icons, see: https://fonts.google.com/icons
#if NETIMGUI_FONT_ICON_MATERIALDESIGN
	#include "../Private/Fonts/IconFontCppHeader/IconsMaterialDesign.h"
#endif

// Note1:	You can toggle which Icon Font are active, in 'NetImgui.Build.cs'
// 
// Note2:	'Icon Game Kenney' can always be active, but only one from 
//			'Font Awesome' or 'MaterialDesign Icon' can be active at a time
// 
// Note3:	For Icon usage example, please take a look at NetImguiDemoActor.cpp

#endif //NETIMGUI_ENABLED

//=================================================================================================
// NETIMGUI Module
//=================================================================================================
class FNetImguiModule : public IModuleInterface
{
public:
	static constexpr char kModuleName[] = "NetImgui";
	enum class eFont
	{
		kProggyClean,		// Built-in Dear ImGui Font
		kCousineFixed16,	// Fixed size font + 'Kenney Game Icons' + 'Font Awesome Icons' or 'Material Design Icons'
		kCousineFixed20,	//  "
		kCousineFixed24,	//  "
		//kDroidSans,		// Disabled for now, to keep font size down (can be re-enabled)
		//kKarlaRegular,	// Disabled for now, to keep font size down (can be re-enabled)
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

	/**
	 * Singleton-like access to this module's interface. This is just for convenience!
	 * Beware of calling this during the shutdown phase, though. Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline FNetImguiModule& Get() { return FModuleManager::LoadModuleChecked<FNetImguiModule>(kModuleName); }

	/**
	 * Checks to see if this module is loaded and ready. It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable() { return FModuleManager::Get().IsModuleLoaded(kModuleName); }

	/**
	 * Let us know if this module is currently ready to receives 'Dear ImGui' draw commands
	 *
	 * @return True if the module is expect draws
	 */
	static inline bool IsDrawing()
	{
	#if NETIMGUI_ENABLED
		FNetImguiModule* pNetImguiModule = FModuleManager::LoadModulePtr<FNetImguiModule>(kModuleName);
		if (pNetImguiModule != nullptr && pNetImguiModule->isDrawing()) 
		{
			// HotReload note: When a dll is reloaded, original dll is still loaded and all 'Dear ImGui' 
			// functions are still pointing to it when called from outside this dll. Only this module object
			// is recreated. This means that the game code will call original dll but this module object 
			// will use reloaded dll ImGui functions. To prevent issue with destroyed context, we are 
			// making sure that the original dll knows about this module's newly created context.
			ImGui::SetCurrentContext(pNetImguiModule->mpContext);
			return true;
		}
	#endif
		return false;
	
	}
	
	/**
	 * Replace the default Font
	 */
	static inline void SetDefaultFont(eFont font)
	{
	#if NETIMGUI_ENABLED
		FNetImguiModule* pNetImguiModule = FModuleManager::LoadModulePtr<FNetImguiModule>(kModuleName);
		if( pNetImguiModule != nullptr ){
			pNetImguiModule->setDefaultFont(font); 
		}
	#endif
	}

	/**
	 * Change the current Font used for drawing. 
	 * A matching PopFont() is expected once user is done with the Font.
	 */
	static inline void PushFont(eFont font)
	{
	#if NETIMGUI_ENABLED
		FNetImguiModule* pNetImguiModule = FModuleManager::LoadModulePtr<FNetImguiModule>(kModuleName);
		if( pNetImguiModule != nullptr ){
			pNetImguiModule->pushFont(font);
		}
	#endif
	}

	/**
	 * Undo the last PushFont(). Returning it to previous value. 
	 */
	static inline void PopFont()
	{
	#if NETIMGUI_ENABLED
		FNetImguiModule* pNetImguiModule = FModuleManager::LoadModulePtr<FNetImguiModule>(kModuleName);
		if( pNetImguiModule != nullptr ){
			pNetImguiModule->popFont(); 
		}
	#endif
	}

	virtual void setDefaultFont(eFont font);
	virtual void pushFont(eFont font);
	virtual void popFont();
	virtual bool isDrawing();

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

protected:
	void Update();
	FDelegateHandle mUpdateCallback;
	struct ImGuiContext* mpContext = nullptr;
};

#if NETIMGUI_ENABLED

/**
// Helper class that change the font and automatically restore it when object is out of scope
*/
struct NetImguiScopedFont
{
	inline NetImguiScopedFont(FNetImguiModule::eFont font){ FNetImguiModule::PushFont(font); }
	inline ~NetImguiScopedFont(){ FNetImguiModule::PopFont(); }
};

#endif
