<p align="center" style="font-size:30px"><b>Unreal NetImgui</b></p>
<br>
<p align="center">
<img src="https://avatars3.githubusercontent.com/u/6615685?s=200&v=4" width=128 height=128>
+
<img src="https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/netImguiLogo.png" width=128 height=128>
</p>

# Summary
### [Unreal Engine 4's](https://github.com/EpicGames) support of [NetImgui 1.6](https://github.com/sammyfreg/netImgui "NetImgui")

**NetImgui** is a library to remotely display and control **Dear ImGui** menus with a connected NetImgui server application. 

This plugin allows **UE4** users to remotely connect to their game and display [**Dear ImGui**](https://github.com/ocornut/imgui "Dear ImGui")'s generated menus in a separate window. The game can be running on a different computer or even a different platform such as console, cellpone, etc...

![NetImgui](https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/UnrealCommands.gif)

> **Note 1:** Allows a very simple use of **NetImgui** in **Unreal Engine 4**. To support more complex usage, with **Dear ImGui** content displayed locally on the game screen, please take a look at the excellent [**UnrealImGui**](https://github.com/segross/UnrealImGui/tree/net_imgui) plugin. It also has NetImgui support integrated.

> **Note 2:** This is a useful plugin when **Dear ImGui** is not already supported in your UE4 engine codebase. Otherwise, it is possible to ignore this plugin and directly add [**NetImgui's**](https://github.com/sammyfreg/netImgui "NetImgui") client code alongside your **Dear ImGui's** code. 

# Unreal Commands
Integrated in the plugin, is the ***Imgui Unreal Commands*** functionalities. Allows user to quickly browse and execute the various Unreal Commands that are already available in the Console, but with a nicer interface. 

<p align="center">
<iframe name='SimpleVideoPlayer' width='630px' height='375px' scrolling='no' src='https://simplevideoplayer.bubbleapps.io/player?video=1028677.7160696657' marginwidth='0px' marginheight='0px' frameborder='0' id='simplevideoplayer' allowfullscreen> </iframe>
</p>

 - **Note :**
  - The *Imgui Unreal Commands* functionality can easily be added in other projects (without  **UnrealNetImgui** dependency).
  -Copy `Source\Private\ImguiUnrealCommand.cpp + .h` to your own project
  -Follow usage found in `Source\Private\NetImguiModule.cpp` (inside IMGUI_UNREAL_COMMAND_ENABLED defines)

# Integration
 1. Download and copy the **UnrealNetImgui** folder to **Unreal Engine**'s Plugin directory (`.\Engine\Plugins`)
 1. Regenerate your project solution to have the new plugin included *(right-click [ProjectName].uproject-> Generate Visual Studio Project Files)*
 1. In your game project `(ProjectName).Build.cs` file, add the `NetImgui` dependency to `PublicDependencyModuleNames` entries.
 1. In editor, enable the plugin `2D\NetImgui`.
 1. Start the (`UnrealNetImgui\NetImguiServer\NetImguiServer.exe`) application.
  - **Dear ImGui's** menu content created in your code, will be displayed and controlled in it (after a connection is established).
  - The client list comes pre-configured with 2 clients configuration (game and editor running on the same PC). For remote PCs, game consoles or others, create a new client configuration with proper address settings.
 1. Any code running on the Game Thread, can now make standard **Dear ImGui** drawing commands to generate your menus.
  - The define `NETIMGUI_ENABLED` allows to selectively disable code if planning to disable **NetImgui** on certain game configurations (shipping, ...)
  - By default, the plugin is compiled with **FrameSkip** support. This saves CPU but require a test before drawing. This means that the **Dear ImGui** functions should only be used when `FNetImguiModule::IsDrawing()` is true *(assert otherwise)*. This can be disabled with `bFrameSkip_Enabled` in `NetImgui.Build.cs`.
 1. If wanting to use in editor, make sure to unselect the option `Edit->Editor Preferences->General->Performances->Use Less CPU when in Background`, otherwise framerate will be low when focus is on the NetImguiServer window instead of the Unreal Editor.
# Example

Code example of **Dear ImGui** to display a very basic menu from the `Tick()` method of an actor.
 - A slightly more complex sample can be found in `UnrealNetImgui\Source\Sample`
 - The sample code is the actor `Net Imgui Demo Actor` that can be dropped in any of your scenes.
 - Information on [**Dear ImGui**](https://github.com/ocornut/imgui "Dear ImGui") menu generation, can be found reading the code of `ImGui::ShowDemoWindow()` in `UnrealNetImgui\Source\Private\ThirdParty\DearImgui\imgui_demo.cpp` and their webpage.

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

# Release notes 1.4
 - Upgraded to [**NetImgui 1.6**](https://github.com/sammyfreg/netImgui "NetImgui") *(more details in link)*
 - NetImgui Server keyboard Input fixes
 - Added ***Imgui Unreal Commands*** support (browse and execute Unreal Commands)
 
# Release notes (older)
 - Upgraded to **Dear Imgui 1.83** *(docking branch)*
 - Upgraded to [**NetImgui 1.5**](https://github.com/sammyfreg/netImgui "NetImgui") *(more details in link)*
 - Tested on **Unreal 4.26** *(other versions should be supported without issues)*
 - **NetImgui Server** now requires less CPU/GPU

# Credits
Sincere thanks to [Omar Cornut](https://github.com/ocornut/imgui/commits?author=ocornut) for the incredible work on [**Dear ImGui**](https://github.com/ocornut/imgui).

Code inspired by existing [**UnrealImGui**](https://github.com/segross/UnrealImGui/tree/net_imgui) plugin for Unreal 4.