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


#if NETIMGUI_FONT_ICON_AWESOME
#define CLIENTSTRING_NAME	ICON_FA_INFO_CIRCLE " Name"
#define CLIENTSTRING_SHOW	ICON_FA_EYE
#define CLIENTSTRING_POS	ICON_FA_MAP_MARKER_ALT " Position"

#elif NETIMGUI_FONT_ICON_MATERIALDESIGN
#define CLIENTSTRING_NAME	ICON_MD_INFO  " Name"
#define CLIENTSTRING_SHOW	ICON_MD_VISIBILITY
#define CLIENTSTRING_POS	ICON_MD_PLACE  " Position"

#else
#define CLIENTSTRING_NAME	"Name"
#define CLIENTSTRING_SHOW	"Position"
#define CLIENTSTRING_POS	"Visible"
#endif

//=================================================================================================
// DrawImgui_OncePerFrame
//-------------------------------------------------------------------------------------------------
// Note:	We do not need to check NetImguiHelper::IsDrawing() since this callback method is
//			only invoked by NetImgui client code, when the server wants some new Dear Imgui drawing
//=================================================================================================
void ANetImguiDemoActor::DrawImgui_OncePerFrame()
{
    
	
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
			ImGui::TextWrapped(u8"This Windows is drawn once per frame (when NetImguiServer request it), using a NetImgui callback.");
			ImGui::NewLine();

			//-------------------------------------------------------------------------------------
			// Demonstrations of using icons and japanese font
			// Note:	To create a utf8 string that can represent unicode character, 
			//			prefix it with 'u8'
			//-------------------------------------------------------------------------------------
			if( ImGui::CollapsingHeader("Icons & Font", ImGuiTreeNodeFlags_DefaultOpen))
			{
				uint32 iconAnimFrame = (GFrameNumber/60);

			#if NETIMGUI_FONT_JAPANESE
				ImGui::TextColored(kColorHighlight, "Japanese font");
				{
					//--- Showcase using a utf8 string mixing japanese and latin content ---
					NetImguiScopedFont iconFont(FNetImguiModule::eFont::kJapanese32);
					ImGui::TextWrapped(u8"日本語とカタカナとひらがなとlatinを使用することができます。やった！");
				}
			#endif
			
			#if NETIMGUI_FONT_ICON_GAMEKENNEY
				//--- Showcase using a FString to mix icon and text together ---
				ImGui::NewLine();
				FString titleKenney = FString::Format(TEXT("{0} Game Kenney Icons"), {UTF8_TO_TCHAR(ICON_KI_INFO_CIRCLE)});
				ImGui::TextColored(kColorHighlight, TCHAR_TO_UTF8(*titleKenney));

				//--- Showcase using multiple strings that includes normal text and icons, merged together in 1 utf8 string constant ---
				ImGui::TextUnformatted("I " ICON_KI_HEART " icons in my text.");
				{
					NetImguiScopedFont iconFont(FNetImguiModule::eFont::kIcons64);
					const char* zAnimation[]={ICON_KI_DPAD, ICON_KI_DPAD_TOP, ICON_KI_DPAD_RIGHT, ICON_KI_DPAD_BOTTOM, ICON_KI_DPAD_LEFT };
					ImGui::Text( ICON_KI_GAMEPAD " %s " ICON_KI_BUTTON_SELECT " " ICON_KI_BUTTON_START " " ICON_KI_BUTTON_A " " ICON_KI_BUTTON_B , zAnimation[iconAnimFrame%UE_ARRAY_COUNT(zAnimation)]);
				}
			#endif
			
			#if NETIMGUI_FONT_ICON_AWESOME
				//--- Showcase using a FString to mix icon and text together ---
				FString titleAwesome = FString::Format(TEXT("{0} Font Awesome Icons"), {UTF8_TO_TCHAR(ICON_FA_INFO_CIRCLE)});
				ImGui::TextColored(kColorHighlight, TCHAR_TO_UTF8(*titleAwesome));
				
				//--- Showcase using a utf8 string with icons inserted in it as a regular printf string constant ---
				ImGui::Text(u8"I %s icons in my text.", ICON_FA_HEART);
				{
					NetImguiScopedFont iconFont(FNetImguiModule::eFont::kIcons64);
					const char* zAnimation[]={ICON_FA_BATTERY_EMPTY, ICON_FA_BATTERY_QUARTER, ICON_FA_BATTERY_HALF, ICON_FA_BATTERY_THREE_QUARTERS, ICON_FA_BATTERY_FULL };
					ImGui::Text(ICON_FA_SMILE " " ICON_FA_EXPAND_ARROWS_ALT " " ICON_FA_COGS " " ICON_FA_ARROW_ALT_CIRCLE_LEFT " " ICON_FA_ARROW_ALT_CIRCLE_RIGHT " %s", zAnimation[iconAnimFrame%UE_ARRAY_COUNT(zAnimation)]);
				}
			#endif
			
			#if NETIMGUI_FONT_ICON_MATERIALDESIGN
				//--- Showcase using a FString to mix icon and text together ---
				FString titleMaterial = FString::Format(TEXT("{0} Material Design Icons"), {UTF8_TO_TCHAR(ICON_MD_INFO)});
				ImGui::TextColored(kColorHighlight, TCHAR_TO_UTF8(*titleMaterial));
				//--- Showcase using a utf8 string with icons inserted in it as a regular printf string constant ---
				ImGui::TextUnformatted("I " ICON_MD_FAVORITE " icons in my text.");
				{
					NetImguiHelper::ScopedFont iconFont(FNetImguiModule::eFont::kIcons64);
					const char* zAnimation[]={ ICON_MD_BRIGHTNESS_1, ICON_MD_BRIGHTNESS_2, ICON_MD_BRIGHTNESS_3, ICON_MD_BRIGHTNESS_4, ICON_MD_BRIGHTNESS_5, ICON_MD_BRIGHTNESS_6, ICON_MD_BRIGHTNESS_7 };
					ImGui::Text(ICON_MD_SENTIMENT_SATISFIED " " ICON_MD_OPEN_WITH " " ICON_MD_SETTINGS " " ICON_MD_KEYBOARD_ARROW_LEFT " " ICON_MD_KEYBOARD_ARROW_RIGHT " %s", zAnimation[iconAnimFrame%UE_ARRAY_COUNT(zAnimation)]);
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
				if (ImGui::BeginTable("Actors List", 3, ImGuiTableFlags_RowBg|ImGuiTableFlags_Resizable|ImGuiTableFlags_Reorderable))
				{
					// Table header, with a specific font assigned
					{
						NetImguiScopedFont headerFont(FNetImguiModule::eFont::kCousineFixed20);
						ImVec4 tableBgColor = ImGui::GetStyleColorVec4(ImGuiCol_TableHeaderBg);
						tableBgColor.w = 1.f;
						ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, tableBgColor);
						ImGui::TableSetupColumn(CLIENTSTRING_NAME);
						ImGui::TableSetupColumn(CLIENTSTRING_POS);
						ImGui::TableSetupColumn(CLIENTSTRING_SHOW, ImGuiTableColumnFlags_WidthFixed, 40.0f);
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
							ImGui::TextUnformatted(TCHAR_TO_UTF8(*pActor->GetName()));
						
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
	if( sbShowDemoNetImgui )
	{
		//-----------------------------------------------------------------------------------------
		// Every 'ANetImguiDemoActor' display the following content
		//-----------------------------------------------------------------------------------------
		FString windowName = FString::Format(TEXT("DemoActor: {0}"), {GetName()});
		ImGui::SetNextWindowSize(ImVec2(400.f, 200.f), ImGuiCond_Once);
		if (ImGui::Begin(TCHAR_TO_UTF8(*windowName)))
		{
			ImGui::TextWrapped(u8"One window per 'ANetImguiDemoActor' instance will be displayed. The Dear ImGui content is being drawn inside the actor's tick method, without any callback needed.");
			ImGui::NewLine();
				
			ImGui::TextColored(kColorHighlight, "Name: ");
			ImGui::SameLine(64.f); ImGui::TextUnformatted(TCHAR_TO_UTF8(*GetName()));
			
			FVector pos = GetTransform().GetLocation();
			ImGui::TextColored(kColorHighlight, "Pos: ");
			ImGui::SameLine(64.f); ImGui::Text("(%.02f, %.02f, %.02f)", pos.X, pos.Y, pos.Z);
		}
		ImGui::End();
	}
}

//=================================================================================================
// Tick
//-------------------------------------------------------------------------------------------------
// Main update method of this actor. Called every frame on each 'ANetImguiDemoActor' instance
//=================================================================================================
void ANetImguiDemoActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// This test is mandatory when 'bFrameSkip_Enabled' is enabled in 'NetImgui.Build.cs'
	// since this Tick method is called every frame and we want to avoid drawing content unless
	// expecting it this frame.
	// 
	// Note:	This Dear Imgui drawing doesn't have to occurs on a Tick method, the only 
	//			requirement is that it is on the gamethread and 'NetImguiHelper::Get().IsDrawing()'
	//			is tested before drawing
    if( FNetImguiModule::Get().IsDrawing() )
    {
		DrawImgui_OncePerActor();
	}
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
	}
#endif
}