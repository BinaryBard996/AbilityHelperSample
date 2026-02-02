// EditorSampleTypes.h
// DevTest: FGameplayEffectConfig 和 FGameplayAbilityConfig 的派生结构体示例

#pragma once

#include "CoreMinimal.h"
#include "AbilityEditorTypes.h"
#include "EditorSampleTypes.generated.h"

/**
 * FGameplayEffectConfig 的派生结构体示例
 * 用于测试方案三的扩展机制
 * 新增字段将通过 OnPostProcessGameplayEffect 委托应用到 GE
 *
 * 使用 ExcelSheet 元数据将扩展字段放入单独的子表，不影响基类的主表结构
 */
USTRUCT(BlueprintType)
struct ABILITYHELPERSAMPLE_API FGameplayEffectSampleConfig : public FGameplayEffectConfig
{
	GENERATED_BODY()

	/** 测试用整数属性，将写入 TestGameplayEffectComponent */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SampleExtension",
		meta = (ExcelHint = "测试整数值", ExcelSheet = "SampleExtension"))
	int32 TestIntValue = 0;

	/** 测试用布尔属性，将写入 TestGameplayEffectComponent */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SampleExtension",
		meta = (ExcelHint = "测试布尔值", ExcelSheet = "SampleExtension"))
	bool bTestBoolValue = false;
};

/**
 * FGameplayAbilityConfig 的派生结构体示例
 * 用于测试 GA 的扩展机制
 * 新增字段将通过 OnPostProcessGameplayAbility 委托应用到 GA
 *
 * 使用 ExcelSheet 元数据将扩展字段放入单独的子表，不影响基类的主表结构
 */
USTRUCT(BlueprintType)
struct ABILITYHELPERSAMPLE_API FGameplayAbilitySampleConfig : public FGameplayAbilityConfig
{
	GENERATED_BODY()

	/** 测试用浮点属性 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SampleExtension",
		meta = (ExcelHint = "测试浮点值", ExcelSheet = "SampleExtension"))
	float TestFloatValue = 0.0f;

	/** 测试用整数属性 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SampleExtension",
		meta = (ExcelHint = "测试整数值", ExcelSheet = "SampleExtension"))
	int32 TestIntValue = 0;

	/** 测试用布尔属性 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SampleExtension",
		meta = (ExcelHint = "测试布尔值", ExcelSheet = "SampleExtension"))
	bool bTestBoolValue = false;
};
