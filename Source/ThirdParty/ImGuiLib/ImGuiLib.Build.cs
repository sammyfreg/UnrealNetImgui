using UnrealBuildTool;

//=================================================================================================
// Dear ImGui Library
// (Library can be found here : https://github.com/ocornut/imgui)
//
// Code is actually compiled under ThirdPartyBuildImGui.cpp, avoiding issues with PCH and DLL
// functions definitions. This mostly informational
//=================================================================================================
public class ImGuiLib : ModuleRules
{
#if WITH_FORWARDED_MODULE_RULES_CTOR
	public ImGuiLib(ReadOnlyTargetRules Target) : base(Target)
#else
	public ImGuiLib(TargetInfo Target)
#endif
	{
		Type = ModuleType.External;
	}
}
