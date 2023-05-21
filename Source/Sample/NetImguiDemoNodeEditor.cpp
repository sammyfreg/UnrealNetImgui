// Distributed under the MIT License (MIT) (see accompanying LICENSE file)
#include "NetImguiModule.h"

#if NETIMGUI_NODE_EDITOR_ENABLED && NETIMGUI_DEMO_IMGUI_ENABLED

#include <array>
#include "NetImguiDemoNodeEditor.h"

#define LOCTEXT_NAMESPACE "NodeEditorDemo"

namespace NodeEditorDemo
{
//=================================================================================================
// Declaration of NodeEditor Samples
//=================================================================================================
struct SampleEntry { 
        typedef Application* (*CreateSampleFunc)();
        enum class eID : int { NetImguiNode, Widget, Interaction, _Count };
        static constexpr int kSampleCount = static_cast<int>(eID::_Count);
        eID                 m_ID;
        const char*         m_pDesc             = nullptr;
        CreateSampleFunc    m_pCreateSampleCB   = nullptr;
};

struct PersistentState
{
    ImGuiContext* 	                                    m_pImguiContext = nullptr;	// Dear Imgui Context when this was initialized
    std::array<Application*, SampleEntry::kSampleCount>	m_pSamples = {};
    int                                                 m_SampleCurrent = 0;
} gDemoState;

Application* CreateSampleNetImgui();
Application* CreateSampleInteraction();
Application* CreateSampleWidget();
SampleEntry gpSamples[]={
        {SampleEntry::eID::NetImguiNode,    "NetImgui Sample: Nodes",           CreateSampleNetImgui},  // Modified version of the library original 'Interaction' example. Improved the 'Pin' handling to show connection feedback
        {SampleEntry::eID::Widget,          "NodeEditor Sample: Widget",        CreateSampleWidget},
        //{SampleEntry::eID::Interaction,     "NodeEditor Sample: Interaction",   CreateSampleInteraction}, // disabled by default. NetImguiNode is an improved version of the original Interacti}n
};

bool SampleComboTextCB(void* data, int idx, const char** out_text)
{
    *out_text = gpSamples[idx].m_pDesc;
    return true;
}

//=================================================================================================
// Initialize
//-------------------------------------------------------------------------------------------------
// Create all Sample objects
//=================================================================================================
void Initialize()
{
    if( !gDemoState.m_pImguiContext ){
        gDemoState.m_pImguiContext = ImGui::GetCurrentContext();
        for(int i(0); i<UE_ARRAY_COUNT(gpSamples); ++i){
            if( gpSamples[i].m_ID < SampleEntry::eID::_Count && !gDemoState.m_pSamples[i]){
                gDemoState.m_pSamples[i] = (*gpSamples[i].m_pCreateSampleCB)();
            }
        }
    }
}

//=================================================================================================
// Release
//-------------------------------------------------------------------------------------------------
// Release all allocated sample objects
//=================================================================================================
void Release()
{
    for(auto& pSample : gDemoState.m_pSamples){
        if (pSample) {
            delete(pSample);
        }
    }
    gDemoState = PersistentState();
}

//=================================================================================================
// ShowDemo
//-------------------------------------------------------------------------------------------------
// Display the NodeEditor Demo Window with selected sample content. 
// Also initialize the needed sample object, when not already allocated.
//=================================================================================================
void ShowDemo(bool& bVisible)
{
    if( bVisible ){
        
        // Release the NodeEditor sample application when Dll is hot reloaded (Dear ImGui Context changed) 
        // This is to allow re-creating the NodeEditor own node context with the activee Dear ImGui Context)
        if (gDemoState.m_pImguiContext && gDemoState.m_pImguiContext != ImGui::GetCurrentContext()) {
            Release();
        }

        // Create Sample Application Objects
        if( !gDemoState.m_pImguiContext ){
            Initialize();
        }

        // Update the selected Sample
        if(ImGui::Begin("Demo: Node Editor", &bVisible))
        {
            ImGui::TextUnformatted("Hint: Press 'F' to refocus");
            ImGui::Combo("##Sample", &gDemoState.m_SampleCurrent, SampleComboTextCB, nullptr, UE_ARRAY_COUNT(gpSamples));
            SampleEntry::eID sampleID = gpSamples[gDemoState.m_SampleCurrent].m_ID;
            Application* pApplication = gDemoState.m_pSamples[static_cast<int>(sampleID)];
            if( pApplication ) {
                pApplication->OnFrame(ImGui::GetIO().DeltaTime);
            }
        }
        ImGui::End();
    }
}


} // NodeEditorDemo

#undef LOCTEXT_NAMESPACE

#endif // NETIMGUI_NODE_EDITOR_ENABLED && NETIMGUI_DEMO_IMGUI_ENABLED
