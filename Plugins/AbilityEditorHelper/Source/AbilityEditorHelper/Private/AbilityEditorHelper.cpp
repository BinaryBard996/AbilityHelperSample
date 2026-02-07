// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilityEditorHelper.h"

#if WITH_EDITOR
#include "ToolMenus.h"
#include "AbilityEditorHelperLibrary.h"
#include "AbilityEditorHelperSettings.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/CoreDelegates.h"
#endif

#define LOCTEXT_NAMESPACE "FAbilityEditorHelperModule"

void FAbilityEditorHelperModule::StartupModule()
{
#if WITH_EDITOR
	// 延迟注册菜单，确保 UToolMenus 已初始化
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FAbilityEditorHelperModule::RegisterMenuExtensions));

	// 在引擎循环初始化完成后注册 Python（此时 PythonScriptPlugin 已完成初始化）
	EngineLoopInitHandle = FCoreDelegates::OnFEngineLoopInitComplete.AddRaw(
		this, &FAbilityEditorHelperModule::RegisterPythonScripts);
#endif
}

void FAbilityEditorHelperModule::ShutdownModule()
{
#if WITH_EDITOR
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	FCoreDelegates::OnFEngineLoopInitComplete.Remove(EngineLoopInitHandle);
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

void FAbilityEditorHelperModule::RegisterPythonScripts()
{
	if (!GEngine)
	{
		return;
	}

	TSharedPtr<IPlugin> ThisPlugin = IPluginManager::Get().FindPlugin(TEXT("AbilityEditorHelper"));
	if (!ThisPlugin.IsValid())
	{
		return;
	}

	FString PythonDir = FPaths::ConvertRelativePathToFull(ThisPlugin->GetContentDir() / TEXT("Python"));
	// Python 需要正斜杠路径
	PythonDir.ReplaceInline(TEXT("\\"), TEXT("/"));

	// 将插件 Content/Python 加入 sys.path
	GEngine->Exec(nullptr, *FString::Printf(
		TEXT("py import sys; p='%s'; (p not in sys.path) and sys.path.insert(0,p)"),
		*PythonDir));

	// 从 Settings 读取启动脚本列表并逐个执行
	const UAbilityEditorHelperSettings* Settings = GetDefault<UAbilityEditorHelperSettings>();
	for (const FString& Script : Settings->StartupPythonScripts)
	{
		if (Script.IsEmpty())
		{
			continue;
		}

		FString FullPath = PythonDir / Script;
		FullPath.ReplaceInline(TEXT("\\"), TEXT("/"));

		GEngine->Exec(nullptr, *FString::Printf(
			TEXT("py exec(open('%s',encoding='utf-8').read())"),
			*FullPath));
	}
}
#endif

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAbilityEditorHelperModule, AbilityEditorHelper)
