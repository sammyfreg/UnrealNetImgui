// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <Modules/ModuleManager.h>

#ifndef NETIMGUI_ENABLED
	#define NETIMGUI_ENABLED 0
#endif

#if NETIMGUI_ENABLED
	#include "../Private/ThirdParty/DearImgui/imgui.h"
#endif

class FNetImguiModule : public IModuleInterface
{
public:
	enum class eFont
	{
		kProggyClean,
		kCousineRegular,
		kDroidSans,
		kKarlaRegular,
		kProggyTiny,
		kRobotoMedium,

		//... Your own font can be added here and loaded in 'FNetImguiModule::StartupModule()'

		_Count,
	};

	/**
	 * Singleton-like access to this module's interface. This is just for convenience!
	 * Beware of calling this during the shutdown phase, though. Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline FNetImguiModule& Get() { return FModuleManager::LoadModuleChecked<FNetImguiModule>("NetImgui"); }

	/**
	 * Checks to see if this module is loaded and ready. It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable() { return FModuleManager::Get().IsModuleLoaded("NetImgui"); }

	/**
	 * Let us know if this module is currently ready to receives 'Dear ImGui' draw commands
	 *
	 * @return True if the module is expect draws
	 */
	static inline bool IsDrawing()
	{
	#if NETIMGUI_ENABLED
		return IsAvailable() && Get().isDrawing(); 
	#else
		return false;
	#endif
	}
	
	/**
	 * Replace the default Font
	 */
	static inline void SetDefaultFont(eFont font)
	{
	#if NETIMGUI_ENABLED
		if( IsAvailable() ){
			Get().setDefaultFont(font); 
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
		if( IsAvailable() ){
			Get().pushFont(font); 
		}
	#endif
	}

	/**
	 * Undo the last PushFont(). Returning it to previous value. 
	 */
	static inline void PopFont()
	{
	#if NETIMGUI_ENABLED
		ImGui::PopFont();
	#endif
	}

	virtual void setDefaultFont(eFont font);
	virtual void pushFont(eFont font);
	virtual void popFont();
	virtual bool isDrawing();

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	void Update();
};

#if NETIMGUI_ENABLED

/**
// Helper class that change the font and automatically restore it when object is out of scope
*/
struct NetImguiScopedFont
{
	inline NetImguiScopedFont(FNetImguiModule::eFont font){ FNetImguiModule::PushFont(font); }
	inline ~NetImguiScopedFont(){ ImGui::PopFont(); }
};

#endif
