// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "AbilityEditorHelperWidget.generated.h"

/**
 * Ability Editor Helper 的 EditorUtilityWidget 基类。
 * 使用方式：
 *   1. 在编辑器中创建一个 EditorUtilityWidgetBlueprint，将此类设置为父类
 *   2. 在 Project Settings → Ability Editor Helper → EditorWidget 中指定该蓝图资产
 *   3. 通过 Window → Ability Editor Helper Widget 菜单打开
 */
UCLASS(Blueprintable)
class ABILITYEDITORHELPER_API UAbilityEditorHelperWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:

	/** 读取 Settings 中的 GameplayEffectExcelName，自动补齐 .xlsx 后缀 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="AbilityEditorHelper|EditorWidget")
	FString GetGameplayEffectExcelName() const;

	/** 读取 Settings 中的 GameplayAbilityExcelName，自动补齐 .xlsx 后缀 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="AbilityEditorHelper|EditorWidget")
	FString GetGameplayAbilityExcelName() const;

	/** 读取 Settings 中的 GameplayEffectDataType */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="AbilityEditorHelper|EditorWidget")
	FString GetGameplayEffectDataType() const;

	/** 读取 Settings 中的 GameplayAbilityDataType */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="AbilityEditorHelper|EditorWidget")
	FString GetGameplayAbilityDataType() const;

	/** 将 GameplayEffectExcelName 写入 Settings 并保存 */
	UFUNCTION(BlueprintCallable, Category="AbilityEditorHelper|EditorWidget")
	void SaveGameplayEffectExcelName(const FString& InName);

	/** 将 GameplayAbilityExcelName 写入 Settings 并保存 */
	UFUNCTION(BlueprintCallable, Category="AbilityEditorHelper|EditorWidget")
	void SaveGameplayAbilityExcelName(const FString& InName);

	/** 在编辑器右下角显示 Toast 通知（Duration <= 0 时需手动点击关闭） */
	UFUNCTION(BlueprintCallable, Category="AbilityEditorHelper|EditorWidget", meta=(AdvancedDisplay="Duration"))
	void ShowEditorNotification(const FText& Message, float Duration = 3.f);
};
