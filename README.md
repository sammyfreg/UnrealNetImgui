<p align="center" style="font-size:30px"><b>Unreal NetImgui</b></p>
<br>
<p align="center">
<img src="https://avatars3.githubusercontent.com/u/6615685?s=200&v=4" width=128 height=128>
+
<img src="https://github.com/sammyfreg/netImgui/blob/master/Web/img/netImguiLogo.png" width=128 height=128>
</p>


# Summary
### [Unreal Engine 4](https://github.com/EpicGames)'s support of [NetImgui](https://github.com/sammyfreg/netImgui "NetImgui").

**NetImgui** is a library to remotely display and control **Dear ImGui** menus with an associated netImgui server application. 

This plugin allows **UE4** users to remotely connect to their game and display [**Dear ImGui**](https://github.com/ocornut/imgui "Dear ImGui")'s generated menus in a separate window. The game can be running on a different computer or even a different platform such as console, cellpone, etc...

> **Note:** Allows a very simple use of **netImgui** in **Unreal Engine 4**. To support more complex usage, with **Dear ImGui** content displayed locally on the game screen, please take a look at the excellent [**UnrealImGui**](https://github.com/segross/UnrealImGui/tree/net_imgui) plugin. It also has netImgui support integrated.

# Integration
 - Download and copy the **UnrealNetImgui** folder to **Unreal Engine**'s Plugin directory (.\Engine\Plugins)
 - In your game project `(ProjectName).Build.cs` file, add the `NetImgui` dependency to `PublicDependencyModuleNames` entry.
 - In editor, enable the plugin `2D\NetImgui`.
 - Start the `Engine\Plugins\UnrealNetImgui\NetImguiServer\NetImguiServer.exe` application. 
  - **Dear ImGui**'s menu content created in the game, will be displayed and controlled in it, after establishing a connection.
  - The config file comes pre-configured with 2 clients configuration (game and editor running on the same PC). For remote PCs, game consoles or others, create a new client configuration with proper address settings.
 - Anywhere where code is running on the Game Thread, you can now make standard **Dear ImGui** drawing commands to generate your menus. 
  - The define `NETIMGUI_ENABLED` allows to selectively disable code if planning to disable **NetImgui** on certain game configuration (shipping, ...)
 - If wanting to use in editor, make sure to unselect the option `Edit->Editor Preferences->General->Performances->Use Less CPU when in Background`, otherwise framerate will be low when using the netImguiServer window.
 
 # Example

Code example of **Dear ImGui** to display a very basic menu from the `Tick()` method of an actor. For more information on how to generate [**Dear ImGui**](https://github.com/ocornut/imgui "Dear ImGui") menus, please look on their webpage.

```cpp
// ...
#include <NetImguiModule.h>

// ...
void AMyImGuiActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

#if NETIMGUI_ENABLED
	// Prevents displaying the same content if there's multiple AMyImguiActor present in the scene.
	// Could just use a FCoreDelegates::OnBeginFrame delegate instead
	static uint32 sLastFrame = 0;
	if( sLastFrame != GFrameNumber )
	{	
		sLastFrame = GFrameNumber;
		ImGui::Begin("Test Imgui");
		ImGui::TextWrapped("Test of debug drawing");
		ImGui::End();

		ImGui::ShowDemoWindow();
	}
#endif
}
```

# Release notes

# Credits
Sincere thanks to [Omar Cornut](https://github.com/ocornut/imgui/commits?author=ocornut) for the incredible work on [**Dear ImGui**](https://github.com/ocornut/imgui).

Code inspired by existing [**UnrealImGui**](https://github.com/segross/UnrealImGui/tree/net_imgui) plugin for Unreal 4.