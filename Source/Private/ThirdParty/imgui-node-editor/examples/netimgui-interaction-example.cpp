//=================================================================================================
// Modified version of the library original 'Interaction' example. 
// Improved the 'Pin' handling to show connection feedback
// 
// Added tracking of which pin(s) have a valid connection and matching status icon. 
// Fairly simple change to the original code.
//=================================================================================================

#if NETIMGUI_NODE_EDITOR_ENABLED && NETIMGUI_DEMO_IMGUI_ENABLED // Avoid compiling this library when not requested8
#include "Sample\NetImguiDemoNodeEditor.h"

namespace NodeEditorSampleNetImgui
{

using Application = NodeEditorDemo::Application;

namespace ed = ax::NodeEditor;

struct Example:
    public Application
{
    // Struct to hold basic information about connection between
    // pins. Note that connection (aka. link) has its own ID.
    // This is useful later with dealing with selections, deletion
    // or other operations.
    struct LinkInfo
    {
        ed::LinkId Id;
        ed::PinId  InputId;
        ed::PinId  OutputId;
    };

    using Application::Application;

    void OnStart() override
    {
        ed::Config config;
        config.SettingsFile = "NodeEditorSampleNetImgui.json";
        m_Context = ed::CreateEditor(&config);
        ed::SetCurrentEditor(m_Context);
        ed::GetStyle().FlowSpeed    *= 0.5f;
        ed::GetStyle().FlowDuration = 1.f;
        m_ActivesPin.Init(false, 64);
        ed::SetCurrentEditor(nullptr);
    }

    void OnStop() override
    {
        ed::DestroyEditor(m_Context);
    }

    void ImGuiEx_BeginColumn()
    {
        ImGui::BeginGroup();
    }

    void ImGuiEx_NextColumn()
    {
        ImGui::EndGroup();
        ImGui::SameLine();
        ImGui::BeginGroup();
    }

    void ImGuiEx_EndColumn()
    {
        ImGui::EndGroup();
    }
    
    //---------------------------------------------------------------------------------------------
    // Pick the most appropriate Icon from the list of available one
    //---------------------------------------------------------------------------------------------
    void AddPin(const ed::PinId& pin, bool isInput, const char* zLabel)
    {
        int index = pin.Get();
    
#if NETIMGUI_FONT_ICON_GAMEKENNEY
        const char* zIcon = m_ActivesPin.IsValidIndex(index) && m_ActivesPin[index] ? ICON_KI_RADIO_CHECKED ICON_KI_CARET_RIGHT : ICON_KI_RADIO ICON_KI_CARET_RIGHT;
    #elif NETIMGUI_FONT_ICON_MATERIALDESIGN
        const char* zIcon =  m_ActivesPin.IsValidIndex(index) && m_ActivesPin[index] ? ICON_MD_RADIO_BUTTON_CHECKED ICON_MD_CHEVRON_RIGHT : ICON_MD_RADIO_BUTTON_UNCHECKED ICON_MD_CHEVRON_RIGHT;
    #elif NETIMGUI_FONT_ICON_AWESOME
        const char* zIcon =  m_ActivesPin.IsValidIndex(index) && m_ActivesPin[index] ? ICON_FA_CIRCLE_DOT ICON_FA_CARET_RIGHT : ICON_FA_CIRCLE ICON_FA_CARET_RIGHT;
    #else
        const char* zIcon =  m_ActivesPin.IsValidIndex(index) && m_ActivesPin[index] ? "[X]>" : "[ ]>";
    #endif

        ed::BeginPin(pin, isInput ? ed::PinKind::Input : ed::PinKind::Output);
            ImGui::Text("%s %s", (isInput ? zIcon : zLabel), (!isInput ? zIcon : zLabel));
        ed::EndPin();
    }

    void OnFrame(float deltaTime) override
    {
        auto& io = ImGui::GetIO();

        ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);

        ImGui::Separator();

        ed::SetCurrentEditor(m_Context);

        // Start interaction with editor.
        ed::Begin("My Editor", ImVec2(0.0, 0.0f));

        int uniqueId = 1;

        //
        // 1) Commit known data to editor
        //

        // Submit Node A
        ed::NodeId nodeA_Id = uniqueId++;
        ed::PinId  nodeA_InputPinId = uniqueId++;
        ed::PinId  nodeA_OutputPinId = uniqueId++;

        if (m_FirstFrame)
            ed::SetNodePosition(nodeA_Id, ImVec2(10, 10));
        ed::BeginNode(nodeA_Id);
            ImGui::Text("Node A");
            AddPin(nodeA_InputPinId, true, "Input A");
            ImGui::SameLine();
            AddPin(nodeA_OutputPinId, false, "Output A");
        ed::EndNode();

        // Submit Node B
        ed::NodeId nodeB_Id = uniqueId++;
        ed::PinId  nodeB_InputPinId1 = uniqueId++;
        ed::PinId  nodeB_InputPinId2 = uniqueId++;
        ed::PinId  nodeB_OutputPinId = uniqueId++;

        if (m_FirstFrame){
            ed::SetNodePosition(nodeB_Id, ImVec2(300, 10));
             //ADDED
            m_Links.push_back({ ed::LinkId(m_NextLinkId++), nodeA_OutputPinId, nodeB_InputPinId1}); // 1 default connection
            m_ActivesPin[nodeB_InputPinId1.Get()] = true;
            m_ActivesPin[nodeA_OutputPinId.Get()] = true;
        }

        ed::BeginNode(nodeB_Id);
            ImGui::Text("Node B");
            ImGuiEx_BeginColumn();
                AddPin(nodeB_InputPinId1, true,"Color");
                AddPin(nodeB_InputPinId2, true, "Opacity");

            ImGuiEx_NextColumn();
                AddPin(nodeB_OutputPinId, false, "Final");
            ImGuiEx_EndColumn();
        ed::EndNode();

        // Submit Links
        for (auto& linkInfo : m_Links){
            ed::Link(linkInfo.Id, linkInfo.InputId, linkInfo.OutputId);
             //ADDED
            if( ed::IsLinkSelected(linkInfo.Id) || linkInfo.Id == ed::GetHoveredLink()){
                ed::Flow(linkInfo.Id, ed::FlowDirection::Forward);
            }
        }
        //
        // 2) Handle interactions
        //

        // Handle creation action, returns true if editor want to create new object (node or link)
        if (ed::BeginCreate())
        {
            ed::PinId inputPinId, outputPinId;
            if (ed::QueryNewLink(&inputPinId, &outputPinId))
            {
                // QueryNewLink returns true if editor want to create new link between pins.
                //
                // Link can be created only for two valid pins, it is up to you to
                // validate if connection make sense. Editor is happy to make any.
                //
                // Link always goes from input to output. User may choose to drag
                // link from output pin or input pin. This determine which pin ids
                // are valid and which are not:
                //   * input valid, output invalid - user started to drag new ling from input pin
                //   * input invalid, output valid - user started to drag new ling from output pin
                //   * input valid, output valid   - user dragged link over other pin, can be validated

                if (inputPinId && outputPinId) // both are valid, let's accept link
                {
                    // ed::AcceptNewItem() return true when user release mouse button.
                    if (ed::AcceptNewItem())
                    {
                        // Since we accepted new link, lets add one to our list of links.
                        m_Links.push_back({ ed::LinkId(m_NextLinkId++), inputPinId, outputPinId });

                        // Draw new link.
                        ed::Link(m_Links.back().Id, m_Links.back().InputId, m_Links.back().OutputId);

                        //ADDED
                        m_ActivesPin[inputPinId.Get()] = true;
                        m_ActivesPin[outputPinId.Get()] = true;
                    }

                    // You may choose to reject connection between these nodes
                    // by calling ed::RejectNewItem(). This will allow editor to give
                    // visual feedback by changing link thickness and color.
                }
            }
        }
        ed::EndCreate(); // Wraps up object creation action handling.


        // Handle deletion action
        if (ed::BeginDelete())
        {
            // There may be many links marked for deletion, let's loop over them.
            ed::LinkId deletedLinkId;
            while (ed::QueryDeletedLink(&deletedLinkId))
            {
                // If you agree that link can be deleted, accept deletion.
                if (ed::AcceptDeletedItem())
                {
                    // Then remove link from your data.
                    for (auto& link : m_Links)
                    {
                        if (link.Id == deletedLinkId)
                        {
                            m_ActivesPin[link.InputId.Get()] = false;
                            m_ActivesPin[link.OutputId.Get()] = false;
                            m_Links.erase(&link);
                            break;
                        }
                    }
                }

                // You may reject link deletion by calling:
                // ed::RejectDeletedItem();
            }
        }
        ed::EndDelete(); // Wrap up deletion action



        // End of interaction with editor.
        ed::End();

        if (m_FirstFrame)
            ed::NavigateToContent(0.0f);

        ed::SetCurrentEditor(nullptr);

        m_FirstFrame = false;

        // ImGui::ShowMetricsWindow();
    }

    ed::EditorContext*  m_Context = nullptr;    // Editor context, required to trace a editor state.
    bool                m_FirstFrame = true;    // Flag set for first frame only, some action need to be executed once.
    ImVector<LinkInfo>  m_Links;                // List of live links. It is dynamic unless you want to create read-only view over nodes.
    int                 m_NextLinkId = 100;     // Counter to help generate link ids. In real application this will probably based on pointer to user data structure.
    TBitArray<>         m_ActivesPin;           // ADDED: Keep track of active pins
};

} // namespace NodeEditorSampleNetImgui

namespace NodeEditorDemo
{

Application* CreateSampleNetImgui()
{
   Application* pNewDemoApplication = new NodeEditorSampleNetImgui::Example();
   if( pNewDemoApplication ){
       pNewDemoApplication->OnStart();
   }
   return pNewDemoApplication;
}

}


#endif // NETIMGUI_NODE_EDITOR_ENABLED && NETIMGUI_DEMO_IMGUI_ENABLED
//=================================================================================================
