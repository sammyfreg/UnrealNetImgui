//=================================================================================================
// NetImgui Demo Actor
//-------------------------------------------------------------------------------------------------
// Example of using 'NetImgui' with 'Dear ImGui' inside an Actor class. 
//
// The sources files are not compiled nor needed by the 'UnrealNetImgui' Plugin, it is only here 
// to help integrating it to your own codebase. 
//
// The 'Dear ImGui' draws can be done from anywhere in the engine, on the GameThread, not limited 
// to 'AActor::Tick()' or an Actor class.
//=================================================================================================

#include "NetImguiActor.h"
#include <NetImguiModule.h>

#if NETIMGUI_ENABLED
const ImVec4 kHighlightColor(0.1f, 1.f, 0.1f, 1.f);

//=================================================================================================
// Helper function to display a sample text with a particualr font
//=================================================================================================
void ShowScopedFont(FNetImguiModule::eFont font)
{
    // This will automatically change the current font, 
    // and restore to previous value once out of scope
    NetImguiScopedFont scopedFont(font); 
    
    // Display Font name and sample text filler
    ImGui::NewLine();
    ImGui::TextColored(kHighlightColor, "Font: %s", ImGui::GetFont()->GetDebugName());
    ImGui::TextWrapped("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.");
}

#endif


//=================================================================================================
// Sample ImGui Drawing code.
//-------------------------------------------------------------------------------------------------
// For more info on what can be done with 'Dear ImGui' please look at the content of
// 'ImGui::ShowDemoWindow()' in '"UnrealNetImgui\Source\ThirdParty\ImGuiLib\Source\imgui_demo.cpp'
// and in its repository 'https://github.com/ocornut/imgui'
//=================================================================================================
void ANetImguiActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
#if NETIMGUI_ENABLED
    //---------------------------------------------------------------------------------------------
    // Avoid drawing ImGui menus when not expecting a new frame, reducing CPU cost.
    // Mandatary when 'bSupportFrameSkip' is enabled in 'NetImgui.Build.cs', otherwise
    // 'Dear ImGui' will assert on a missing 'ImGui::NewFrame()'
    if( FNetImguiModule::IsDrawing() )
    //---------------------------------------------------------------------------------------------
    {
        //-----------------------------------------------------------------------------------------
        // A single 'ANetImguiActor' will display the following content
        // (could use a FCoreDelegates::OnBeginFrame delegate instead of checking frame number)
        //-----------------------------------------------------------------------------------------        
        static uint64 sLastFrame = 0;
        if( sLastFrame != GFrameCounter )
        {    
            sLastFrame = GFrameCounter;
            ImGui::SetNextWindowSize(ImVec2(800.f, 600.f), ImGuiCond_FirstUseEver);
            if( ImGui::Begin("NetImgui Client test") )
            {        
                // Show all loaded fonts directly from Dear ImGui
                ImGui::ShowFontSelector("Default Font");
                ImGui::TextWrapped("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.");

                // Demonstrate NetImguiScopedFont and look of each available fonts
                ShowScopedFont(FNetImguiModule::eFont::kProggyClean);         
                ShowScopedFont(FNetImguiModule::eFont::kCousineRegular);         
                ShowScopedFont(FNetImguiModule::eFont::kDroidSans);         
                ShowScopedFont(FNetImguiModule::eFont::kKarlaRegular);         
                ShowScopedFont(FNetImguiModule::eFont::kProggyTiny);
                ShowScopedFont(FNetImguiModule::eFont::kRobotoMedium);

                ImGui::NewLine();
                ImGui::TextColored(kHighlightColor, "List of NetImguiActors");
            }
            ImGui::End();
            ImGui::ShowDemoWindow();
        }

        //-----------------------------------------------------------------------------------------
        // Every 'ANetImguiActor' display the following content
        //-----------------------------------------------------------------------------------------
        if (ImGui::Begin("NetImgui Client test"))
        {
            ImGui::Text("Actor : %s", TCHAR_TO_ANSI(GetName().GetCharArray().GetData()));
        }
        ImGui::End();
    }
#endif
}

