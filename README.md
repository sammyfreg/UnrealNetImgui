<p align="center" style="font-size:30px"><b>Unreal NetImgui</b></p>
<br>
<p align="center" style="text-align:center;">
<img src="https://avatars3.githubusercontent.com/u/6615685?s=200&v=4" width=128 height=128>
<font size="8"> + Dear ImGui + </font>
<img src="https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/netImguiLogo.png" width=128 height=128>
</p>

# Summary
### [Unreal Engine 4's](https://github.com/EpicGames) support of [NetImgui](https://github.com/sammyfreg/netImgui "NetImgui")

**UnrealNetImgui** is a plugin adding remote debug GUI interface to **Unreal Engine**, using the [**Dear ImGui**](https://github.com/ocornut/imgui "Dear ImGui") paired with [**NetImgui**](https://github.com/sammyfreg/netImgui). Allows **UE4** users to remotely display and control some custom GUI on the dedicated **NetImgui server** application. This is really convenient in the situation of your game beeing run on hardware with limited inputs/display such as a gaming console and smartphones. Also useful to have access to a debug menu without cluttering your game screen.

![NetImgui](https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/UnrealNetImgui.gif)

> **Note 1:** Allows a simple use of **Dear ImGui** in **Unreal Engine 4**. To support more complex scenario with GUI content displayed locally on the game screen, please take a look at the excellent [**UnrealImGui**](https://github.com/segross/UnrealImGui/tree/net_imgui) plugin. It also has NetImgui support integrated in the **net_imgui branch**.

> **Note 2:** This is a useful plugin when **Dear ImGui** is not already supported in your UE4 engine codebase. Otherwise, it is possible to ignore this plugin and directly add [**NetImgui's**](https://github.com/sammyfreg/netImgui "NetImgui") client code alongside your **Dear ImGui's** code, with minimal integration time required. You can refer to this plugin for implementation details.

# Fonts and Icons
The plugin comes packaged with various latin fonts, a Japanese Mincho font, [Kenney's Gaming Icons](https://kenney.nl/assets/game-icons "gaming icons") and [Font Awesome](https://fontawesome.com "Font Awesome") (the free subset) for a nice selection of useful icons. The screenshot above shows a small subset of available icons. Mixing latin text, kanjis and icons is kept straightforward using utf8 strings.

# Connection to NetImgui Server
There are mutliple ways of connecting your game to the **NetImguiServer**.

**Default :**
- Launch your Editor or Game and if this is on your local PC, a connection should be automatically be established.
- For a game running on remote hardware, you will need to add the connection information in the NetImgui Server's clients list.

![NetImgui](https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/NetImguiServer_AddClient.gif)

**Optional :**
(works as launch command line option and Unreal Commands)
**Command Name | Parameter | Description**
--- | --- | ---
**NetImguiConnect** | HostName/IP:[Port]  | Try reaching the NetImgui Server Application directly.
**NetImguiListen** | [Port] | Start waiting for a connection from the NetImgui Server application (if not already connected or waiting).
**NetImguiDisconnect** | None | Disconnect from the NetImgui Server and stop waiting for a connection.
*Note :* The Port parameter is optional, it will use default values unless specified.
# Unreal Commands
Integrated in the plugin, is the ***Imgui Unreal Commands*** functionalities. Allows user to quickly browse and execute the various Unreal Commands that are already available in the Console, but with a nicer interface. 

![NetImgui](https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/UnrealCommandsFull.gif)
![NetImgui](https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/UnrealCommands.gif)
**[[Demonstration Video]](https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/UnrealCommands.mp4 "[Demonstration Video]")**
 
 - **Note :**
  - The *Imgui Unreal Commands* support is early release.
  - **I am interested in hearing back from people, to know what 'Preset' should comes by default**
  - The *Imgui Unreal Commands* functionality can easily be added in other projects (without  **UnrealNetImgui** dependency).
  -Copy `Source\Private\ImguiUnrealCommand.cpp + .h` to your own project
  -Follow usage found in `Source\Private\NetImguiModule.cpp` (inside IMGUI_UNREAL_COMMAND_ENABLED defines)

# Integration
 1. Download and copy the **UnrealNetImgui** folder to **Unreal Engine**'s Plugin directory (`.\Engine\Plugins`)
 1. Regenerate your project solution to have the new plugin included *(right-click [ProjectName].uproject-> Generate Visual Studio Project Files)*
 1. In your game project `(ProjectName).Build.cs` file, add the `NetImgui` dependency to `PublicDependencyModuleNames` entries.
 1. In editor, enable the plugin `2D\NetImgui`.
 1. Start the `UnrealNetImgui\NetImguiServer\NetImguiServer.exe` application.
  - **Dear ImGui's** menu content created in your code, will be displayed and controlled in it (after a connection is established).
  - The client list comes pre-configured with 3 clients configuration (game, editor, server) that will be automatically connected to when detected. For remote PCs, game consoles or others, create a new client configuration with proper address settings.
 1. You can now invoke **Dear ImGui** drawing functions to generate your GUI every frame.
  - Any code running on the Game Thread can now invoke make drawing calls (as long as  `NetImguiHelper::IsDrawing()` is true)
  - You can also add a callback to `FNetImguiModule::OnDrawImgui` to be invoked by **UnrealNetImgui** when some drawing is expected.
  - The define `NETIMGUI_ENABLED` allows to selectively disable code if planning to remove **NetImgui** on certain game configurations (shipping, ...)
 1. The Unreal build file `NetImgui.Build.cs` contains many option to toggle features/fonts.
 1. When using this plugin in the Editor, unselect the option `Edit->Editor Preferences->General->Performances->Use Less CPU when in Background`, otherwise framerate will be low when focus is on the NetImguiServer window instead of the Unreal Editor.
# Example

Code example of **Dear ImGui** to display a very basic menu from the `Tick()` method of an actor.
 - A more detailed sample can be found in `UnrealNetImgui\Source\Sample\NetImguiDemoActor.*`.
 - The sample actor `NetImguiDemoActor` can be dropped in any of your scenes.
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
    //---------------------------------------------------------------------------------------------
    if( NetImguiHelper::IsDrawing() )
    {
        //-----------------------------------------------------------------------------------------
        // First 'ANetImguiDemoActor' actor will display the following content
        // (could use 'FNetImguiModule::OnDrawImgui' callback delegate instead)
        //-----------------------------------------------------------------------------------------
        static uint64 sLastFrame = 0;
        if( sLastFrame != GFrameCounter )
        {
            sLastFrame = GFrameCounter;
            ImGui::Begin("NetImgui Demo");
            ImGui::TextWrapped("Simple display of a text label");
            ImGui::TextUnformatted("I " ICON_KI_HEART " icons in my text."); // Display 'I (HeartIcon) icons in my text
            ImGui::End();
            ImGui::ShowDemoWindow(); // Show Dear ImGui demo window
        }

        //-----------------------------------------------------------------------------------------
        // Every 'ANetImguiDemoActor' display the following content
        //-----------------------------------------------------------------------------------------
        FString windowName = FString::Format(TEXT("DemoActor: {0}"), {GetName()});
        ImGui::SetNextWindowSize(ImVec2(400.f, 200.f), ImGuiCond_Once);
        if (ImGui::Begin(TCHAR_TO_UTF8(*windowName)))
        {
            ImGui::Text("Name: ");
            ImGui::SameLine(64.f);
            ImGui::TextUnformatted(TCHAR_TO_UTF8(*GetName()));

            FVector pos = GetTransform().GetLocation();
            ImGui::Text( "Pos: ");
            ImGui::SameLine(64.f);
            ImGui::Text("(%.02f, %.02f, %.02f)", pos.X, pos.Y, pos.Z);
        }
        ImGui::End();
    }
#endif
}
```
# Release notes 1.8
 - Added Japanese Font
 - Added Kenney's gaming icons
 - Added Font Awesome's icons
 - Added Font Material Design icons
 - Added FreeType font rendering support (for sharper text)
 - Added the delegate `FNetImguiModule::OnDrawImgui` to listen to for drawing
 - Removed module lookup when calling `FNetImguiModule::Get()`
 - Upgraded to [**NetImgui 1.7.5**](https://github.com/sammyfreg/netImgui/releases/tag/v1.7.5) *(more details in link)*
 
# Release notes (older)
 - Upgraded to [**NetImgui 1.6**](https://github.com/sammyfreg/netImgui/releases/tag/v1.6.0) *(more details in link)*
 - NetImgui Server keyboard Input fixes
 - Added ***Imgui Unreal Commands*** support (browse and execute Unreal Commands)
 - Upgraded to **Dear Imgui 1.83** *(docking branch)*
 - Upgraded to [**NetImgui 1.5**](https://github.com/sammyfreg/netImgui/releases/tag/v1.5.0) *(more details in link)*
 - Tested on **Unreal 4.26** *(other versions should be supported without issues)*
 - **NetImgui Server** now requires less CPU/GPU

# Credits
Sincere thanks to [Omar Cornut](https://github.com/ocornut/imgui/commits?author=ocornut) for the incredible work on [**Dear ImGui**](https://github.com/ocornut/imgui).

Code inspired by existing [**UnrealImGui**](https://github.com/segross/UnrealImGui/tree/net_imgui) plugin for Unreal 4.

**Icons**
Various icons have been integrated to **UnrealNetImgui** existing fonts and accessible as normal unicode entries. The following credits made it possible:

- [IconFontCppHeaders](https://github.com/juliettef/IconFontCppHeaders "IconFontCppHeaders") by Doug Binks, giving easy access to icons symbols, using simple C++ defines.

- [Kenney's Gaming Icons](https://kenney.nl/assets/game-icons "gaming icons") for his set of useful game related symbols.

- [Font Awesome](https://fontawesome.com "Font Awesome") (the free subset) for the nice selection of every day use icons.