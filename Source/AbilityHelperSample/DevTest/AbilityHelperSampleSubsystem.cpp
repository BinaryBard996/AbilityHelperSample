// AbilityHelperSampleSubsystem.cpp

#include "AbilityHelperSampleSubsystem.h"
#include "EditorSampleTypes.h"
#include "SampleGameplayAbility.h"
#include "TestGameplayEffectComponent.h"
#include "AbilityEditorHelperSubsystem.h"
#include "AbilityEditorHelperLibrary.h"
#include "AbilityEditorHelperSettings.h"
#include "GameplayEffect.h"
#include "Abilities/GameplayAbility.h"
#include "Editor.h"

void UAbilityHelperSampleSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 确保 AbilityEditorHelperSubsystem 先初始化
	Collection.InitializeDependency<UAbilityEditorHelperSubsystem>();

	// 向 AbilityEditorHelperSettings 注册项目特定的结构体类型
	if (UAbilityEditorHelperSettings* Settings = GetMutableDefault<UAbilityEditorHelperSettings>())
	{
		Settings->StructTypePathsToExportSchema.AddUnique(TEXT("/Script/AbilityHelperSample.GameplayEffectSampleConfig"));
		Settings->StructTypePathsToExportSchema.AddUnique(TEXT("/Script/AbilityHelperSample.GameplayAbilitySampleConfig"));
		UE_LOG(LogTemp, Log, TEXT("[AbilityHelperSample] 已注册 FGameplayEffectSampleConfig 和 FGameplayAbilitySampleConfig 到 Schema 导出列表"));
	}

	// 获取 AbilityEditorHelperSubsystem 并绑定委托
	if (GEditor)
	{
		if (UAbilityEditorHelperSubsystem* HelperSubsystem = GEditor->GetEditorSubsystem<UAbilityEditorHelperSubsystem>())
		{
			// 绑定 GE 委托
			PostProcessGEDelegateHandle = HelperSubsystem->OnPostProcessGameplayEffect.AddUObject(
				this, &UAbilityHelperSampleSubsystem::HandlePostProcessGameplayEffect);
			UE_LOG(LogTemp, Log, TEXT("[AbilityHelperSample] 已绑定 OnPostProcessGameplayEffect 委托"));

			// 绑定 GA 委托
			PostProcessGADelegateHandle = HelperSubsystem->OnPostProcessGameplayAbility.AddUObject(
				this, &UAbilityHelperSampleSubsystem::HandlePostProcessGameplayAbility);
			UE_LOG(LogTemp, Log, TEXT("[AbilityHelperSample] 已绑定 OnPostProcessGameplayAbility 委托"));
		}
	}
}

void UAbilityHelperSampleSubsystem::Deinitialize()
{
	// 解绑委托
	if (GEditor)
	{
		if (UAbilityEditorHelperSubsystem* HelperSubsystem = GEditor->GetEditorSubsystem<UAbilityEditorHelperSubsystem>())
		{
			HelperSubsystem->OnPostProcessGameplayEffect.Remove(PostProcessGEDelegateHandle);
			HelperSubsystem->OnPostProcessGameplayAbility.Remove(PostProcessGADelegateHandle);
		}
	}
	PostProcessGEDelegateHandle.Reset();
	PostProcessGADelegateHandle.Reset();

	Super::Deinitialize();
}

void UAbilityHelperSampleSubsystem::HandlePostProcessGameplayEffect(const FTableRowBase* Config, UGameplayEffect* GE)
{
	if (!Config || !GE)
	{
		return;
	}

	// 尝试转换为派生类型
	// 注意：这里使用 static_cast，因为 FTableRowBase 没有虚函数表
	// 实际项目中可以通过检查 DataTable 的 RowStruct 来确认类型
	const FGameplayEffectSampleConfig* SampleConfig = static_cast<const FGameplayEffectSampleConfig*>(Config);

	// 检查是否有扩展数据需要处理
	// 这里通过检查默认值来判断是否需要创建 Component
	bool bHasExtensionData = (SampleConfig->TestIntValue != 0) || SampleConfig->bTestBoolValue;

	if (bHasExtensionData)
	{
		// 创建或获取 TestGameplayEffectComponent
		UTestGameplayEffectComponent& TestComp = GE->FindOrAddComponent<UTestGameplayEffectComponent>();

		// 应用扩展字段
		TestComp.TestIntProperty = SampleConfig->TestIntValue;
		TestComp.bTestBoolProperty = SampleConfig->bTestBoolValue;

		UE_LOG(LogTemp, Log, TEXT("[AbilityHelperSample] 已应用扩展字段到 GE: %s (TestInt=%d, TestBool=%s)"),
			*GE->GetName(),
			SampleConfig->TestIntValue,
			SampleConfig->bTestBoolValue ? TEXT("true") : TEXT("false"));
	}
	else
	{
		// 如果没有扩展数据，移除 Component（如果存在）
		RemoveGEComponent<UTestGameplayEffectComponent>(GE);
	}
}

void UAbilityHelperSampleSubsystem::HandlePostProcessGameplayAbility(const FTableRowBase* Config, UGameplayAbility* GA)
{
	if (!Config || !GA)
	{
		return;
	}

	// 尝试转换为派生类型
	const FGameplayAbilitySampleConfig* SampleConfig = static_cast<const FGameplayAbilitySampleConfig*>(Config);

	// 检查 GA 是否是 USampleGameplayAbility 类型
	USampleGameplayAbility* SampleGA = Cast<USampleGameplayAbility>(GA);
	if (!SampleGA)
	{
		// 如果不是 SampleGameplayAbility，则不处理扩展字段
		return;
	}

	// 检查是否有扩展数据需要处理
	bool bHasExtensionData = (SampleConfig->TestFloatValue != 0.0f) ||
	                         (SampleConfig->TestIntValue != 0) ||
	                         SampleConfig->bTestBoolValue;

	if (bHasExtensionData)
	{
		// 应用扩展字段到 SampleGameplayAbility
		SampleGA->TestFloatValue = SampleConfig->TestFloatValue;
		SampleGA->TestIntValue = SampleConfig->TestIntValue;
		SampleGA->bTestBoolValue = SampleConfig->bTestBoolValue;

		UE_LOG(LogTemp, Log, TEXT("[AbilityHelperSample] 已应用扩展字段到 GA: %s (TestFloat=%.2f, TestInt=%d, TestBool=%s)"),
			*GA->GetName(),
			SampleConfig->TestFloatValue,
			SampleConfig->TestIntValue,
			SampleConfig->bTestBoolValue ? TEXT("true") : TEXT("false"));
	}
}
