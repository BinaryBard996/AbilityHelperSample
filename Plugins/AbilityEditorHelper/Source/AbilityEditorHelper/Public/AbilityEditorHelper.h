// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FAbilityEditorHelperModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/** 注册编辑器菜单扩展（Window 菜单） */
	void RegisterMenuExtensions();

	/** 在引擎初始化完成后注册 Python 路径并执行启动脚本 */
	void RegisterPythonScripts();

	FDelegateHandle EngineLoopInitHandle;
};
