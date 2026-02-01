// AbilityHelperSampleSubsystem.h
// DevTest: 项目级 EditorSubsystem，用于绑定 OnPostProcessGameplayEffect 委托

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "AbilityHelperSampleSubsystem.generated.h"

class UGameplayEffect;
struct FTableRowBase;

/**
 * 项目级 EditorSubsystem
 * 在 Initialize 时绑定 UAbilityEditorHelperSubsystem 的 OnPostProcessGameplayEffect 委托
 * 处理 FGameplayEffectSampleConfig 派生类型的扩展字段
 */
UCLASS()
class ABILITYHELPERSAMPLE_API UAbilityHelperSampleSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	/** 处理派生类型的扩展字段 */
	void HandlePostProcessGameplayEffect(const FTableRowBase* Config, UGameplayEffect* GE);

	/** 委托句柄，用于解绑 */
	FDelegateHandle PostProcessDelegateHandle;
};
