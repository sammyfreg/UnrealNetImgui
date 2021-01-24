Example of using 'NetImgui' with 'Dear ImGui' inside an Actor class. 

The sources files are not compiled nor needed by the 'UnrealNetImgui' Plugin, it is only here to help integrating it to your own codebase. 

The 'Dear ImGui' draws can be done from anywhere in the engine, on the GameThread, not limited to 'AActor::Tick()' or an Actor class.