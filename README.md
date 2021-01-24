<p align="center" style="font-size:30px"><b>Unreal NetImgui</b></p>
<br>
<p align="center">
<img src="https://avatars3.githubusercontent.com/u/6615685?s=200&v=4" width=128 height=128>
+
<img src="https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/netImguiLogo.png" width=128 height=128>
</p>

# Summary
### [Unreal Engine 4's](https://github.com/EpicGames) support of [NetImgui 1.3](https://github.com/sammyfreg/netImgui "NetImgui").

**NetImgui** is a library to remotely display and control **Dear ImGui** menus with an associated netImgui server application. 

This plugin allows **UE4** users to remotely connect to their game and display [**Dear ImGui**](https://github.com/ocornut/imgui "Dear ImGui")'s generated menus in a separate window. The game can be running on a different computer or even a different platform such as console, cellpone, etc...

![NetImgui](https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/netImgui.png)

> **Note 1:** Allows a very simple use of **NetImgui** in **Unreal Engine 4**. To support more complex usage, with **Dear ImGui** content displayed locally on the game screen, please take a look at the excellent [**UnrealImGui**](https://github.com/segross/UnrealImGui/tree/net_imgui) plugin. It also has netImgui support integrated.

> **Note 2:** This is a useful plugin when **Dear ImGui** is not already supported in your UE4 engine codebase. Otherwise, it is possible to ignore this plugin and directly add [**NetImgui's**](https://github.com/sammyfreg/netImgui "NetImgui") client code alongside your **Dear ImGui's** code. The **Unreal 4** networking is already supported.

# Integration
 - Download and copy the **UnrealNetImgui** folder to **Unreal Engine**'s Plugin directory (`.\Engine\Plugins`)
 - Regenerate your project solution to have the new plugin included *(right-click [ProjectName].uproject-> Generate Visual Studio Project Files)*
 - In your game project `(ProjectName).Build.cs` file, add the `NetImgui` dependency to `PublicDependencyModuleNames` entries.
 - In editor, enable the plugin `2D\NetImgui`.
 - Start the (`UnrealNetImgui\NetImguiServer\NetImguiServer.exe`) application.
  - **Dear ImGui's** menu content created in your code, will be displayed and controlled in it (after establishing a connection).
  - The config file comes pre-configured with 2 clients configuration (game and editor running on the same PC). For remote PCs, game consoles or others, create a new client configuration with proper address settings.
 - Anywhere where code is running on the Game Thread, you can now make standard **Dear ImGui** drawing commands to generate your menus. 
  - The define `NETIMGUI_ENABLED` allows to selectively disable code if planning to disable **NetImgui** on certain game configuration (shipping, ...)
 - If wanting to use in editor, make sure to unselect the option `Edit->Editor Preferences->General->Performances->Use Less CPU when in Background`, otherwise framerate will be low when using the netImguiServer window.
 - By default, tne plugin is compiled with **FrameSkip** supported. This saves CPU but require a test before drawing. This means that the **Dear ImGui** functions should only be used when `FNetImguiModule::IsDrawing()` is true *(assert otherwise)*. This can be disabled with `bSupportFrameSkip` in `NetImgui.Build.cs`.
 
# Example

Code example of **Dear ImGui** to display a very basic menu from the `Tick()` method of an actor.
 - A slightly more complex sample can be found in `UnrealNetImgui\Source\Sample`
 - Information on [**Dear ImGui**](https://github.com/ocornut/imgui "Dear ImGui") menu generation, can be found reading the code of `ImGui::ShowDemoWindow()` in `UnrealNetImgui\Source\ThirdParty\ImGuiLib\Source\imgui_demo.cpp` and their webpage.

```cpp
// ...
#include <NetImguiModule.h>

// ...
void AMyImGuiActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

#if NETIMGUI_ENABLED
 	//---------------------------------------------------------------------------------------------
    // Avoid drawing ImGui menus when not expecting a new frame, reducing CPU cost.
    // Mandatary when 'bSupportFrameSkip' is emabled in 'NetImgui.Build.cs', otherwise
    // 'Dear ImGui' will assert on a missing 'ImGui::NewFrame()'
    if( FNetImguiModule::IsDrawing() )
    //---------------------------------------------------------------------------------------------
    {
        //-----------------------------------------------------------------------------------------
        // A single 'AMyImGuiActor' will display the following content
        // (could use a FCoreDelegates::OnBeginFrame delegate instead of checking frame number)
        //-----------------------------------------------------------------------------------------        
        static uint64 sLastFrame = 0;
        if( sLastFrame != GFrameCounter )
        {
            sLastFrame = GFrameCounter;
			ImGui::Begin("NetImgui Demo");
			ImGui::TextWrapped("Simple display of a text label");
			ImGui::End();
			ImGui::ShowDemoWindow();
		}
	}
#endif
}
```

# Release notes
 - Upgrade to **Dear Imgui 1.80** (table support added)
 - Upgrade to **NetImgui 1.3**
 - Added more Fonts selection
 - Improved Server UI for better handling of multiple clients
 - See [NetImgui 1.3](https://github.com/sammyfreg/netImgui "NetImgui") for more details.
 
# Credits
Sincere thanks to [Omar Cornut](https://github.com/ocornut/imgui/commits?author=ocornut) for the incredible work on [**Dear ImGui**](https://github.com/ocornut/imgui).

Code inspired by existing [**UnrealImGui**](https://github.com/segross/UnrealImGui/tree/net_imgui) plugin for Unreal 4.