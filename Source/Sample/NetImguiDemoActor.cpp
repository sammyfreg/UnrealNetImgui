//=================================================================================================
// NetImguiDemo Actor
//-------------------------------------------------------------------------------------------------
// Example of using 'NetImgui' with 'Dear ImGui' inside an Actor class. Just drop actors of this 
// class, in your scene, to see the demo 'Dear ImGui' content appear on the server.
//
// The 'Dear ImGui' draws can be done from anywhere in the engine, on the GameThread, 
// and not limited to 'AActor::Tick()' or an Actor class.
// 
// For more info on what can be done with 'Dear ImGui' please look at the content of
// 'ImGui::ShowDemoWindow()' in 'UnrealNetImgui\Source\Private\ThirdParty\DearImgui\imgui_demo.cpp'
// and in its repository 'https://github.com/ocornut/imgui'
// 
// 'UnrealNetImgui\Source\Private\ThirdParty\DearImgui\imgui.h' has all the UI methods than can 
// be used to draw menus.
// 
// !!! This class is not needed to use NetImgui, it is only here as an example !!!
//=================================================================================================

#include "NetImguiDemoActor.h"

#if NETIMGUI_DEMO_ACTOR_ENABLED

#include <NetImguiModule.h>
#include <Misc/CoreDelegates.h>
#include <Runtime/Engine/Public/EngineUtils.h>

static bool sbShowDemoNetImgui	= false;

static const ImVec4 kColorHighlight = ImVec4(0.1f, 0.85f, 0.1f, 1.0f);

enum eStringIcons : uint8_t { Name, Visibility, Position, _Count };

const char* zStringIcons[eStringIcons::_Count] = 
{

#if NETIMGUI_FONT_ICON_AWESOME
	ICON_FA_INFO_CIRCLE " Name",
	ICON_FA_EYE,
	ICON_FA_MAP_MARKER_ALT " Position",

#elif NETIMGUI_FONT_ICON_MATERIALDESIGN
	ICON_MD_INFO  " Name", 
	ICON_MD_VISIBILITY, 
	ICON_MD_PLACE  " Position", 

#elif NETIMGUI_FONT_ICON_GAMEKENNEY

#else
	"Name", 
	"Position", 
	"Visible"
#endif
};

//=================================================================================================
// DrawImgui_OncePerFrame
//-------------------------------------------------------------------------------------------------

//=================================================================================================
void ANetImguiDemoActor::DrawImgui_OncePerFrame()
{
    // Note: We do not need to check NetImguiHelper::IsDrawing() since this callback method is
	// only invoked when NetImgui wants some Dear Imgui drawing
	
	//---------------------------------------------------------------------------------------------
	// Insert Demo entry in MainMenu bar
	//---------------------------------------------------------------------------------------------
	if (ImGui::BeginMainMenuBar()) {
			if( ImGui::BeginMenu("NetImgui") ){
				ImGui::MenuItem("Demo: DemoActor", nullptr, &sbShowDemoNetImgui);
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}
	
	
	//---------------------------------------------------------------------------------------------
	// Draw NetImgui Demo content if requested
	//---------------------------------------------------------------------------------------------
	if( sbShowDemoNetImgui )
	{
		if( ImGui::Begin("DemoActor: PerFrame") )
		{
			ImGui::Text("Imgui callback called once per frame");
		
			//-------------------------------------------------------------------------------------
			// Demonstrations of using icons and japanese font
			// Note:	To create a utf8 string that can represent unicode character, 
			//			prefix it with 'u8'
			//-------------------------------------------------------------------------------------
			if( ImGui::CollapsingHeader("Icons & Font", ImGuiTreeNodeFlags_DefaultOpen))
			{
			#if NETIMGUI_FONT_JAPANESE
				ImGui::TextColored(kColorHighlight, "Japanese font");
				{
					NetImguiHelper::ScopedFont iconFont(FNetImguiModule::eFont::kJapanese32);
					//
					// Showcase using a utf8 string mixing japanese and latin content
					//
					ImGui::TextWrapped(u8"日本語とカタカナとひらがなとlatinを使用することができます。やった！");
				}
			#endif
			#if NETIMGUI_FONT_ICON_GAMEKENNEY
				ImGui::NewLine();
				//
				// Showcase using a FString to mix icon and text together
				//
				FString titleKenney = FString::Format(TEXT("{0} Game Kenney Icons"), {UTF8_TO_TCHAR(ICON_KI_INFO_CIRCLE)});
				ImGui::TextColored(kColorHighlight, TCHAR_TO_UTF8(*titleKenney));
				//
				// Showcase using multiple strings that includes normal text and icons, that are merged together in 1 utf8 string constant
				//
				ImGui::TextUnformatted("I " ICON_KI_HEART " icons in my text.");
				{
					NetImguiHelper::ScopedFont iconFont(FNetImguiModule::eFont::kIcons64);
					ImGui::TextUnformatted(ICON_KI_GAMEPAD " " ICON_KI_DPAD_TOP " " ICON_KI_BUTTON_A " " ICON_KI_BUTTON_B " " ICON_KI_BUTTON_START " " ICON_KI_BUTTON_SELECT);
				}
			#endif
			#if NETIMGUI_FONT_ICON_AWESOME
				ImGui::NewLine();
				//
				// Showcase using a FString to mix icon and text together
				//
				FString titleAwesome = FString::Format(TEXT("{0} Font Awesome Icons"), {UTF8_TO_TCHAR(ICON_FA_INFO_CIRCLE)});
				ImGui::TextColored(kColorHighlight, TCHAR_TO_UTF8(*titleAwesome));
				//
				// Showcase using a utf8 string with icons inserted in it as a regular printf string constant
				//
				ImGui::Text(u8"I %s icons in my text.", ICON_FA_HEART);
				{
					NetImguiHelper::ScopedFont iconFont(FNetImguiModule::eFont::kIcons64);
					const char* zBattery[]={ICON_FA_BATTERY_EMPTY, ICON_FA_BATTERY_QUARTER, ICON_FA_BATTERY_HALF, ICON_FA_BATTERY_THREE_QUARTERS, ICON_FA_BATTERY_FULL };
					ImGui::Text(ICON_FA_SMILE " " ICON_FA_EXPAND_ARROWS_ALT " " ICON_FA_COGS " " ICON_FA_ARROW_ALT_CIRCLE_LEFT " " ICON_FA_ARROW_ALT_CIRCLE_RIGHT " %s", zBattery[(GFrameNumber/60)%UE_ARRAY_COUNT(zBattery)]);
				}
			#endif
			#if NETIMGUI_FONT_ICON_MATERIALDESIGN
				ImGui::NewLine();
				FString titleMaterial = FString::Format(TEXT("{0} Material Design Icons"), {UTF8_TO_TCHAR(ICON_MD_INFO)});
				ImGui::TextColored(kColorHighlight, TCHAR_TO_UTF8(*titleMaterial));
				ImGui::TextUnformatted("I " ICON_MD_FAVORITE " icons in my text.");
				{
					NetImguiHelper::ScopedFont iconFont(FNetImguiModule::eFont::kIcons64);
				}
			#endif
			}


			//-------------------------------------------------------------------------------------
			// Display a list of actor in the Scene
			//-------------------------------------------------------------------------------------
			ImGui::NewLine();
			if( ImGui::CollapsingHeader("Actors", ImGuiTreeNodeFlags_DefaultOpen))
			{
				static uint32 sCount = 0;
				static char sFilterName[256];
				ImGui::InputText("Name Filter", sFilterName, sizeof(sFilterName) );
				ImGui::Text("%d actors in world '%ls'.", sCount, *GetNameSafe(GWorld));

				if (ImGui::BeginTable("Actors List", 3, ImGuiTableFlags_RowBg|ImGuiTableFlags_Resizable|ImGuiTableFlags_Reorderable))
				{
					// Table header, with a specific font assigned
					{
						NetImguiHelper::ScopedFont headerFont(FNetImguiModule::eFont::kCousineFixed20);
						ImVec4 tableBgColor = ImGui::GetStyleColorVec4(ImGuiCol_TableHeaderBg);
						tableBgColor.w = 1.f;
						ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, tableBgColor);
						ImGui::TableSetupColumn(zStringIcons[eStringIcons::Name]);
						ImGui::TableSetupColumn(zStringIcons[eStringIcons::Position]);
						ImGui::TableSetupColumn(zStringIcons[eStringIcons::Visibility], ImGuiTableColumnFlags_WidthFixed, 40.0f);
						if( ImGui::TableGetColumnFlags(2) & ImGuiTableColumnFlags_IsHovered ){
							ImGui::SetTooltip("Toggle visibility (Editor only)");
						}
						ImGui::TableHeadersRow();
						ImGui::PopStyleColor();
					}
				
					// Table content, with list of actors in the world
					sCount = 0;
					for (TActorIterator<AActor> It(GWorld); It; ++It)
					{
						AActor* pActor = *It;
						if( pActor != nullptr && pActor->GetOwner() == nullptr ){
							int uniqueId = GetTypeHash(pActor->GetFName());
							ImGui::PushID(uniqueId);
						
							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(TCHAR_TO_UTF8(*(pActor->GetName())));
						
							ImGui::TableNextColumn();
							FVector pos = pActor->GetTransform().GetLocation();
							ImGui::Text("(%.02f, %.02f, %.02f)", pos.X, pos.Y, pos.Z);

							ImGui::TableNextColumn();
							bool bVisible = !pActor->IsHiddenEd();
							if (ImGui::Checkbox("", &bVisible)) {
								pActor->SetIsTemporarilyHiddenInEditor(!bVisible);
							}

							ImGui::PopID();
							sCount++;
						}
					}
					ImGui::EndTable();
				}
			}
		}
		ImGui::End();
	}
}

//=================================================================================================
// DrawImgui_OncePerActor
//-------------------------------------------------------------------------------------------------
// For more info on what can be done with 'Dear ImGui' please look at the content of
// 'ImGui::ShowDemoWindow()' in 'UnrealNetImgui\Source\Private\ThirdParty\DearImgui\imgui_demo.cpp'
// and in its repository 'https://github.com/ocornut/imgui'
// 
// 'UnrealNetImgui\Source\Private\ThirdParty\DearImgui\imgui.h' has all the UI methods than can 
// be used to draw menus.
//=================================================================================================
void ANetImguiDemoActor::DrawImgui_OncePerActor()
{
	//---------------------------------------------------------------------------------------------
    // Avoid drawing ImGui menus when not expecting a new frame, reducing CPU cost.
    if( NetImguiHelper::IsDrawing() )
    //---------------------------------------------------------------------------------------------
    {
		if( sbShowDemoNetImgui )
		{
			//-----------------------------------------------------------------------------------------
			// A single 'ANetImguiActor' will display the following content
			// (could use a FCoreDelegates::OnBeginFrame delegate instead of checking frame number)
			//-----------------------------------------------------------------------------------------
			static uint64 sLastFrame	= 0;
			static float sFontScale		= 1.f;
		
			if( sLastFrame != GFrameCounter )
			{    
				sLastFrame = GFrameCounter;
			}

			//-----------------------------------------------------------------------------------------
			// Every 'ANetImguiDemoActor' display the following content
			//-----------------------------------------------------------------------------------------
			FString windowName = FString::Format(TEXT("DemoActor: {0}"), {GetName()});
			if (ImGui::Begin(TCHAR_TO_UTF8(*windowName)))
			{
				ImGui::Text(u8"Test");
			}
			ImGui::End();
		}
    }
}


void ANetImguiDemoActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	DrawImgui_OncePerActor();
}

#endif // #if NETIMGUI_DEMO_ACTOR_ENABLED

//=================================================================================================
// ANetImguiDemoActor Constructor
//-------------------------------------------------------------------------------------------------
// Makes sure the actors are always ticked so each actor can cell 'DrawImgui_OncePerActor()'
// every frame.
// Also add a single callback on 'OnBeginFrame' so 'DrawImgui_OncePerFrame()' can be called 
// once per frame (not per actor).
//=================================================================================================
ANetImguiDemoActor::ANetImguiDemoActor()
{ 
#if NETIMGUI_DEMO_ACTOR_ENABLED
	PrimaryActorTick.bCanEverTick = true;

	//---------------------------------------------------------------------------------------------
	// Add BeginFrame callback only on they Default object.
	//---------------------------------------------------------------------------------------------
	// This means it will only be called once per frame (not per instance), even if there are
	// no ANetImguiDemoActor instance in the scene.
	if (HasAnyFlags(RF_ClassDefaultObject)) {
		FNetImguiModule::OnDrawImgui.AddUObject(this, &ANetImguiDemoActor::DrawImgui_OncePerFrame);
		//FCoreDelegates::OnBeginFrame.AddUObject(this, &ANetImguiDemoActor::DrawImgui_OncePerFrame);
	}
#endif
}