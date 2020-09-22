using UnrealBuildTool;

//=================================================================================================
// netImgui Library
// (Library can be found here : https://github.com/sammyfreg/netImgui)
//
// Code is actually compiled under ThirdPartyBuildNetImgui.cpp, avoiding issues with PCH and DLL
// functions definitions. This mostly informational
//=================================================================================================
public class NetImguiLib : ModuleRules
{
#if WITH_FORWARDED_MODULE_RULES_CTOR
	public NetImguiLib(ReadOnlyTargetRules Target) : base(Target)
#else
	public NetImguiLib(TargetInfo Target)
#endif
	{
		Type = ModuleType.External;
	}
}