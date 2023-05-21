//=================================================================================================
// NetImguiDemo Actor
//-------------------------------------------------------------------------------------------------
// Example of using 'NetImgui' with 'Dear ImGui' inside an Actor class. Just drop actors of this 
// class, in your scene, to see the demo 'Dear ImGui' content appear on the server.
//
// The 'Dear ImGui' draws can be done from anywhere in the engine (on the GameThread),
// and not limited to 'AActor::Tick()' or delegates. This file present some ways of 
// knowing when to draw content, but user are free to create their own. For example, 
// a single ImGui Manager where you call your own drawing callbacks after testing 
// 'FNetImguiModule::Get().IsDrawing()".
// 
// For more info on what can be done with 'Dear ImGui' please look at the content of
// 'ImGui::ShowDemoWindow()' in 'UnrealNetImgui\Source\Private\ThirdParty\DearImgui\imgui_demo.cpp'
// and in its repository 'https://github.com/ocornut/imgui'
// 
// 'UnrealNetImgui\Source\Private\ThirdParty\DearImgui\imgui.h' has all the UI methods than can 
// be used to draw menus.
// 
// !!! This class is not needed to use NetImgui, it is only here as an example !!! 
// You can remove this source file or disable the actor in 'NetImgui.Build.cs'
//=================================================================================================

#include "NetImguiDemoActor.h"

#if NETIMGUI_DEMO_ACTOR_ENABLED

#include <NetImguiModule.h>
#include <Misc/CoreDelegates.h>
#include <Runtime/Engine/Public/EngineUtils.h>

static bool sbShowDemoNetImgui		= false;
static const ImVec4 kColorHighlight = ImVec4(0.1f, 0.85f, 0.1f, 1.0f);

#if NETIMGUI_FONT_ICON_AWESOME
#define CLIENTSTRING_NAME	ICON_FA_CIRCLE_INFO " Name"
#define CLIENTSTRING_SHOW	ICON_FA_EYE
#define CLIENTSTRING_POS	ICON_FA_LOCATION_DOT " Position"

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
// [Method A] DrawImgui FrameCallback
//-------------------------------------------------------------------------------------------------
// Add a listener to 'FNetImguiModule::Get().OnDrawImgui'
// 
// Since we cannot guarantee that the NetImgui module is loaded when this global variable is 
// initialized, we use a delegate to check NetImgui's availability and then add our listener.
//=================================================================================================
void MethodA_DrawImgui_FrameCallback();

static FDelegateHandle sDelegateNetImguiFrameCallback = FCoreDelegates::OnBeginFrame.AddLambda([]()
{
	if( FNetImguiModule::IsAvailable() )
	{
		FCoreDelegates::OnBeginFrame.Remove(sDelegateNetImguiFrameCallback);
		sDelegateNetImguiFrameCallback = FNetImguiModule::Get().OnDrawImgui.AddStatic(&MethodA_DrawImgui_FrameCallback);
		
		//-----------------------------------------------------------------------------------------
		// [Optional] Remove NetImgui Delegate when this module is unloaded
		// When a module is HotReloaded, the old module remains in memory alongside the new one.
		// This insure that only the reloaded module tries to draws.
		//-----------------------------------------------------------------------------------------
		static FDelegateHandle sDelegateModuleUnload = FModuleManager::Get().OnModulesChanged().AddLambda([](FName ModuleName, EModuleChangeReason ReasonForChange)
		{
			static const FName ModuleFName("YourModuleNameHere"); // SET YOUR MODULE NAME HERE
			if( ReasonForChange == EModuleChangeReason::ModuleUnloaded && ModuleName == ModuleFName )
			{
				FNetImguiModule::Get().OnDrawImgui.Remove(sDelegateNetImguiFrameCallback);
				FModuleManager::Get().OnModulesChanged().Remove(sDelegateModuleUnload);
			}
		});
	}
});

//=================================================================================================
// [Method A] DrawImgui FrameCallback
//-------------------------------------------------------------------------------------------------
// Actual Dear Imgui drawing code.
// 
// Used to add an entry to the MainMenu bar, and render some demo windows
//=================================================================================================
void MethodA_DrawImgui_FrameCallback()
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
				ImGui::TextColored(kColorHighlight, "%s", TCHAR_TO_UTF8(*titleKenney));

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
				FString titleAwesome = FString::Format(TEXT("{0} Font Awesome Icons"), {UTF8_TO_TCHAR(ICON_FA_CIRCLE_INFO)});
				ImGui::TextColored(kColorHighlight, "%s",  TCHAR_TO_UTF8(*titleAwesome));
				
				//--- Showcase using a utf8 string with icons inserted in it as a regular printf string constant ---
				ImGui::Text(u8"I %s icons in my text.", ICON_FA_HEART);
				{
					NetImguiScopedFont iconFont(FNetImguiModule::eFont::kIcons64);
					const char* zAnimation[]={ICON_FA_BATTERY_EMPTY, ICON_FA_BATTERY_QUARTER, ICON_FA_BATTERY_HALF, ICON_FA_BATTERY_THREE_QUARTERS, ICON_FA_BATTERY_FULL };
					ImGui::Text(ICON_FA_FACE_SMILE " " ICON_FA_MINIMIZE " " ICON_FA_GEAR " " ICON_FA_ARROW_ROTATE_LEFT " " ICON_FA_ARROW_ROTATE_RIGHT " %s", zAnimation[iconAnimFrame%UE_ARRAY_COUNT(zAnimation)]); // Font awesome standard icons
					ImGui::Text(ICON_FA_CANADIAN_MAPLE_LEAF " " ICON_FA_JIRA " " ICON_FA_ATLASSIAN " " ICON_FA_ANDROID); // Font Awesome free icons examples
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
			// Display a ImPlot example
			// Note:	We rely on the 'NETIMGUI_IMPLOT_ENABLED' define to know 
			//			if this code path should be enabled or not 
			//-------------------------------------------------------------------------------------
		#if NETIMGUI_IMPLOT_ENABLED
			ImGui::NewLine();
			if( ImGui::CollapsingHeader("ImPlot", ImGuiTreeNodeFlags_DefaultOpen))
			{
				constexpr int bar_data[] = {1,2,3,4,5,6,7,8,9,10};
				static bool sbImPlotInitOnce = false;
				static float x_data[512];
				static float y_data[512];
				static_assert(UE_ARRAY_COUNT(x_data) == UE_ARRAY_COUNT(y_data));
				for (int i(0); !sbImPlotInitOnce && i < UE_ARRAY_COUNT(x_data); ++i){
					x_data[i] = static_cast<float>(i) * 0.02f;
					y_data[i] = sin(x_data[i])*5.f + 5.f;
				}
				sbImPlotInitOnce = true;

				if (ImPlot::BeginPlot("My Plot")) {
					ImPlot::PlotBars("My Bar Plot", bar_data, UE_ARRAY_COUNT(bar_data));
					ImPlot::PlotLine("My Line Plot", x_data, y_data, UE_ARRAY_COUNT(x_data));
					ImPlot::EndPlot();
				}
			}
		#endif

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
#if WITH_EDITOR
							bool bVisible = !pActor->IsHiddenEd();
							if (ImGui::Checkbox("", &bVisible)) {
								pActor->SetIsTemporarilyHiddenInEditor(!bVisible);
							}
#else
							bool bVisible = !pActor->IsHidden();
							ImGui::Checkbox("", &bVisible);
#endif
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
// [Method B] Callback added to Actor
//-------------------------------------------------------------------------------------------------
// This method is called by a listener added to 'FNetImguiModule::OnDrawImgui' just like
// [Method A], but on the actor directly. 
// 
// This Dear ImGui drawing demonstrate being able to append to the same window, from multiple
// actor instances, by using the same Window Name.
//=================================================================================================
void ANetImguiDemoActor::MethodB_DrawImgui_ActorCallback()
{
	if( sbShowDemoNetImgui )
	{
		//-----------------------------------------------------------------------------------------
		// Because the Window name is always the same, each actor will appends
		// its name in this  windows
		//-----------------------------------------------------------------------------------------
		ImGui::SetNextWindowSize(ImVec2(400.f, 200.f), ImGuiCond_Once);
		if (ImGui::Begin("NetImguiDemoActor Lists")) {
			// Only first actor will output the following text
			static uint64_t sLastFrame = 0;
			if (sLastFrame != GFrameCounter) {
				sLastFrame = GFrameCounter;
				ImGui::TextColored(kColorHighlight, "Name of 'ANetImguiDemoActor' actor instances:");
			}

			// Every instances output their name, appending to the same window
			ImGui::TextUnformatted(TCHAR_TO_UTF8(*GetName()));
		}
	}
}

//=================================================================================================
// [Method C] Direct Dear ImGui drawing
//-------------------------------------------------------------------------------------------------
// This method is called normally by the Tick method. Any method on the game thread can draw
// content, as long as 'FNetImguiModule::Get().IsDrawing()' is tested first, since NetImgui is not 
// always expecting some new draw frame, when FrameSkip is enabled in the build config
//=================================================================================================
void ANetImguiDemoActor::MethodC_DrawImgui_ActorTick()
{
	// This test is mandatory when 'bFrameSkip_Enabled' is enabled in 'NetImgui.Build.cs'
	// since this Tick method is called every frame and we want to avoid drawing content unless
	// expecting it this frame.
    if( FNetImguiModule::Get().IsDrawing() )
	{
		if( sbShowDemoNetImgui )
		{
			//-----------------------------------------------------------------------------------------
			// Every 'ANetImguiDemoActor' display the following content
			//-----------------------------------------------------------------------------------------
			FString windowName = FString::Format(TEXT("NetImguiDemoActor Tick###{0}"), {GetTypeHash(this)}); // '###+IntegerID' Generates a unique Window ID so each actor have their own window
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
}

//=================================================================================================
// Tick
//-------------------------------------------------------------------------------------------------
// Main update method of this actor. Called every frame on each 'ANetImguiDemoActor' instance
//=================================================================================================
void ANetImguiDemoActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	MethodC_DrawImgui_ActorTick();
}

//=================================================================================================
// PostLoad
//-------------------------------------------------------------------------------------------------
// Add a NetImgui callback that will be invoked every frame when needing to draw some new 
// Dear Imgui content.
//=================================================================================================
void ANetImguiDemoActor::Initialize()
{
	PrimaryActorTick.bCanEverTick = true;

	// No need to add callbacks to class 'default object'
	if ( !HasAnyFlags(RF_ArchetypeObject|RF_ClassDefaultObject) )
	{
		// Do not add callback to PIE actor, since they are a copy of the editor actor and would 
		// draw 2x the same UI. You can ignore this condition if you would like to have UI for them.
		if( GetWorld()->WorldType !=  EWorldType::PIE )
		{
			FNetImguiModule::Get().OnDrawImgui.AddUObject(this, &ANetImguiDemoActor::MethodB_DrawImgui_ActorCallback);
		}
	}
}

#endif // #if NETIMGUI_DEMO_ACTOR_ENABLED
