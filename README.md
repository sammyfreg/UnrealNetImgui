
<p align="center" style="font-size:30px"><b>Unreal NetImgui</b></p>
<p align="center" style="vertical-align:middle">
<img src="https://avatars3.githubusercontent.com/u/6615685?s=200&v=4" width=128 height=128>
<img src="https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/DearImGui.png" width=128 height=128>
<img src="https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/netImguiLogo.png" width=128 height=128>
</p>

# Summary
### Support of [NetImgui](https://github.com/sammyfreg/netImgui "NetImgui") in [Unreal Engine 4 & 5](https://github.com/EpicGames) 

**UnrealNetImgui** is a plugin adding remote debug GUI interface to **Unreal Engine** using the [**Dear ImGui**](https://github.com/ocornut/imgui "Dear ImGui") paired with [**NetImgui**](https://github.com/sammyfreg/netImgui). Allows **Unreal Engine** users to remotely display and control some custom GUI on the dedicated **NetImgui Server** application. This proves convenient with games running on limited inputs/display hardware, such as gaming consoles and smartphones. Also reduces the game screen clutter of debug informations contents.

![NetImgui](https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/UnrealNetImgui.gif)

> **Note 1:** Allows use of **Dear ImGui** in **Unreal Engine 4 & 5** in a separate window/PC. To have Dear ImGui GUI content displayed locally (game screen), please take a look at the excellent [**UnrealImGui**](https://github.com/segross/UnrealImGui/tree/net_imgui) plugin (also has NetImgui support in the **net_imgui branch**).

> **Note 2:** Useful library when **Dear ImGui** is not already supported in your UE engine codebase. Otherwise, ignore this plugin and add [**NetImgui's**](https://github.com/sammyfreg/netImgui "NetImgui") client code alongside your **Dear ImGui's** code. It requires minimal integration time and you can refer to this plugin for implementation details.

# Fonts and Icons
The plugin comes packaged with various Latin fonts, a Japanese Mincho font, [Kenney's Gaming Icons](https://kenney.nl/assets/game-icons "gaming icons"), [Font Awesome](https://fontawesome.com "Font Awesome") (the free subset) and [Google Material Designs icons](https://github.com/google/material-design-icons "Google Material Designs icons"), for a nice selection of useful icons. The screenshot above shows a small subset of available icons. Mixing latin text, kanjis and icons is kept straightforward using utf8 strings.

# Dear ImGui extensions
Additional to the inclusion of extra fonts and icons, some Dear ImGui extension are already integrated and ready to use.
### ImPlot
From the author :
> ImPlot is an immediate mode, GPU accelerated plotting library for [Dear ImGui](https://github.com/ocornut/imgui). It aims to provide a first-class API that ImGui fans will love. ImPlot is well suited for visualizing program data in real-time or creating interactive plots, and requires minimal code to integrate. Just like ImGui, it does not burden the end user with GUI state management, avoids STL containers and C++ headers, and has no external dependencies except for ImGui itself.
>
><img src="https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/ImPlot-pie.gif" width="32%">
><img src="https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/ImPlot-tables.gif" width="32%">
><img src="https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/ImPlot-controls.gif" width="32%">

https://github.com/epezent/implot

### Node-Editor
From the author :
> An implementation of node editor with ImGui-like API. Project purpose is to serve as a basis for more complex solutions like blueprint editors.
> <img src="https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/Node-Editor.gif">

https://github.com/thedmd/imgui-node-editor

# Connecting to the NetImgui Server
There are mutliple ways of connecting your game to the **NetImguiServer**.

### Default
- Launch your Editor/Game on your local PC, a connection should be automatically be established when NetImguiServer and game are on the same PC.
- When launching on remote hardware, add connection information in the NetImgui Server's clients list. Then, the connection will also automatically be established.
- **Note :** The default behaviour is to start waiting on a connection from the NetImguiServer on the default port of the executable type 8889(Game) / 8890(Editor) / 8891(Dedicated Server). If you do not wish this plugin to automatically open a port, you can disable it in `NetImgui.Build.cs` and rely on the NetImgui commands below.

![NetImgui](https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/NetImguiServer_AddClient.gif)

 ### Optional
When launching your game or using the Unreal Console, you can also manually control the connection to the NetImgui Server using these commands:

Command Name | Parameter | Description
--- | --- | ---
**NetImguiConnect** | Hostname/IP:[Port]  | Try reaching the NetImgui Server Application directly.
**NetImguiListen** | [Port] | Start waiting for a connection from the NetImgui Server application (if not already connected).
**NetImguiDisconnect** | None | Disconnect from the NetImgui Server and stop waiting for a connection.

*Note :* The Port parameter is optional, it will use default values unless specified.

*Example :* `UEEditor.exe -NetImguiListen` Launch Unreal Editor and wait for a connection on default port.

*Example :* `UEEditor.exe -NetImguiListen 8000` Launch Unreal Editor and wait for a connection on port 8000.

*Example :* `UEEditor.exe -NetImguiConnect MyPCName` Launch Unreal Editor and try connecting to NetImguiServer running on Windows PC with network name 'MyPCName' on default port.

*Example :* (In Unreal Console) `NetImguiConnect 192.168.1.10:7000` Launch Unreal Editor and try connecting to NetImguiServer running on PC with IP 192.168.1.10 and Port 7000.

# Unreal Commands
This plugins comes with ***Imgui Unreal Commands***, adding Unreal Commands browsing and execution functionalities.

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

2. Regenerate your project solution to have the new plugin included *(right-click [ProjectName].uproject-> Generate Visual Studio Project Files)*

3. In your game project `(ProjectName).Build.cs` file, add the `NetImgui` dependency to `PublicDependencyModuleNames` entries.

4. In editor, enable the plugin `2D\NetImgui`.

5. Start the `UnrealNetImgui\NetImguiServer\NetImguiServer.exe` application.
   - **Dear ImGui's** menu content created in your code, will be displayed and controlled in it (after a connection is established).
   - The client list comes pre-configured with 3 clients configuration (game, editor, server) that will be automatically connected to when detected. For remote PCs, game consoles or others, create a new client configuration with proper address settings.

6. You can now use **Dear ImGui's** drawing functions to generate your GUI every frame.
   - Any code running on the Game Thread can make drawing calls (as long as  `NetImguiHelper::IsDrawing()` is true)
   - You can also add a callback to `FNetImguiModule::OnDrawImgui` to be invoked by **UnrealNetImgui** when some drawing is expected.
   - The define `#if NETIMGUI_ENABLED` allows to selectively disable code if planning to remove **NetImgui** on certain game configurations (shipping, ...)

7. The Unreal build file `NetImgui.Build.cs` contains many option to toggle features/fonts.

8. When using this plugin in the Editor, unselect the option `Edit->Editor Preferences->General->Performances->Use Less CPU when in Background`, otherwise framerate will be low when focus is on the NetImguiServer window instead of the Unreal Editor.

9. It is possible to have some compilation linking errors concerning `Freetype`. The **Freetype** library comes with UnrealEngine and might have some issues on your current engine version, or target platform. Fortunatly, **UnrealNetImgui** use of it is optional (relies on it to improve the font quality) and can be safely disabled. **In UnrealNetImgui\Source\NetImgui.Build.cs : 85**
```
    // bool bFreeType_Enabled = true;
    bool bFreeType_Enabled = false;
```
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
    // Mandatary when 'bSupportFrameSkip' is enabled in 'NetImgui.Build.cs', otherwise
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
# Release notes 1.10
 - Updated to [Dear ImGui 1.89.5](https://github.com/ocornut/imgui/releases/tag/v1.89.5) (docking branch)
 - Updated to [NetImgui 1.19](https://github.com/sammyfreg/netImgui/releases/tag/v1.9.0)
- Tested on **Unreal Engine 4.27, 5.0, 5.2** *(other versions should be supported without issues)*
 - Added support for [ImPlot](https://github.com/epezent/implot)
 - Added support for [Node-Editor](https://github.com/thedmd/imgui-node-editor)

# Release notes 1.9
 - Tested with **Unreal Engine 5**
 - Updated **Font Awesome** icons (v5 -> v6)
 
# Release notes 1.8
 - Added Japanese Font
 - Added Kenney's gaming icons
 - Added Font Awesome's icons
 - Added Font Material Design icons
 - Added FreeType font rendering support (for sharper text)
 - Added the delegate `FNetImguiModule::OnDrawImgui` to listen to for drawing
 - Cache module lookup(every frame) when calling `FNetImguiModule::Get()` instead of more expensive search
 - Upgraded to **Dear Imgui 1.86.5** *(docking branch)*
 - Upgraded to [**NetImgui 1.7.5**](https://github.com/sammyfreg/netImgui/releases/tag/v1.7.5) *(more details in link)*
 
# Release notes (older)
 - Upgraded to [**NetImgui 1.6**](https://github.com/sammyfreg/netImgui/releases/tag/v1.6.0) *(more details in link)*
 - NetImgui Server keyboard Input fixes
 - Added ***Imgui Unreal Commands*** support (browse and execute Unreal Commands)
 - Upgraded to **Dear Imgui 1.83** *(docking branch)*
 - Upgraded to [**NetImgui 1.5**](https://github.com/sammyfreg/netImgui/releases/tag/v1.5.0) *(more details in link)*
 - Tested on **Unreal Engine 4.26, ** *(other versions should be supported without issues)*
 - **NetImgui Server** now requires less CPU/GPU

# Credits
Sincere thanks to [Omar Cornut](https://github.com/ocornut/imgui/commits?author=ocornut) for the incredible work on [**Dear ImGui**](https://github.com/ocornut/imgui).

Code inspired by existing [**UnrealImGui**](https://github.com/segross/UnrealImGui/tree/net_imgui) plugin for Unreal 4.

**Icons**
Various icons have been integrated to **UnrealNetImgui** existing fonts and accessible as normal unicode entries. The following credits made it possible:

- [IconFontCppHeaders](https://github.com/juliettef/IconFontCppHeaders "IconFontCppHeaders") by Doug Binks, giving easy access to icons symbols, using simple C++ defines.

- [Kenney's Gaming Icons](https://kenney.nl/assets/game-icons "gaming icons") for his set of useful game related symbols.

- [Font Awesome](https://fontawesome.com "Font Awesome") (the free subset) for the nice selection of every day use icons.

**Dear ImGui Extensions**
 - [ImPlot](https://github.com/epezent/implot) by [Evan Pezent](https://github.com/epezent).
 - [Node-Editor](https://github.com/thedmd/imgui-node-editor) by [Michał Cichoń](https://github.com/thedmd).