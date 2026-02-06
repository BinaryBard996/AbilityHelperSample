// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilityEditorHelper.h"

#if WITH_EDITOR
#include "ToolMenus.h"
#include "AbilityEditorHelperLibrary.h"
#endif

#define LOCTEXT_NAMESPACE "FAbilityEditorHelperModule"

void FAbilityEditorHelperModule::StartupModule()
{
#if WITH_EDITOR
	// 延迟注册菜单，确保 UToolMenus 已初始化
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FAbilityEditorHelperModule::RegisterMenuExtensions));
#endif
}

void FAbilityEditorHelperModule::ShutdownModule()
{
#if WITH_EDITOR
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
#endif
}

#if WITH_EDITOR
void FAbilityEditorHelperModule::RegisterMenuExtensions()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	// 扩展 Window 主菜单
	UToolMenu* WindowMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
	FToolMenuSection& Section = WindowMenu->FindOrAddSection("AbilityEditorHelper");

	Section.Label = LOCTEXT("AbilityEditorHelperSectionLabel", "Ability Editor Helper");

	Section.AddMenuEntry(
		"OpenAbilityEditorHelperWidget",
		LOCTEXT("OpenAbilityEditorHelperWidget", "Ability Editor Helper Widget"),
		LOCTEXT("OpenAbilityEditorHelperWidgetTooltip", "打开在 Settings 中配置的 Ability Editor Helper 工具窗口"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([]()
		{
			UAbilityEditorHelperLibrary::OpenEditorUtilityWidget();
		}))
	);
}
#endif

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAbilityEditorHelperModule, AbilityEditorHelper)
