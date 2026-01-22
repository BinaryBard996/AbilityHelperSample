// AbilityEditorHelperSubsystem.h

#pragma once

#include "CoreMinimal.h"
#include "AbilityEditorHelperSettings.h"
#include "Engine/DataTable.h"
#include "EditorSubsystem.h"
#include "AbilityEditorHelperSubsystem.generated.h"

/**
 * 在编辑器环境中启动时加载并缓存 AbilityEditorHelperSettings 中的 GameplayEffectDataTable
 */
UCLASS()
class ABILITYEDITORHELPER_API UAbilityEditorHelperSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override { CachedGameplayEffectDataTable = nullptr; Super::Deinitialize(); }

	/** 获取缓存的 DataTable */
	UFUNCTION(BlueprintCallable, Category="AbilityEditorHelper|Subsystem")
	UDataTable* GetCachedGameplayEffectDataTable() const { return CachedGameplayEffectDataTable; }

private:
	/** 缓存的 GE 配置 DataTable（编辑器运行期内存缓存） */
	UPROPERTY(Transient)
	TObjectPtr<UDataTable> CachedGameplayEffectDataTable = nullptr;
};

