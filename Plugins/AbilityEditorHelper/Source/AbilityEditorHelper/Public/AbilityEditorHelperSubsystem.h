// AbilityEditorHelperSubsystem.h

#pragma once

#include "CoreMinimal.h"
#include "AbilityEditorHelperSettings.h"
#include "Engine/DataTable.h"
#include "EditorSubsystem.h"
#include "AbilityEditorHelperSubsystem.generated.h"

class UGameplayEffect;

/**
 * 后处理委托：在 CreateOrImportGameplayEffect 完成基础配置后广播
 * 项目 Source 可注册此委托来处理派生类的扩展字段
 * @param Config - 配置结构体指针（可转换为派生类型）
 * @param GE - 已创建/更新的 GameplayEffect 对象
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPostProcessGameplayEffect, const FTableRowBase* /*Config*/, UGameplayEffect* /*GE*/);

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

	/**
	 * 后处理委托：项目 Source 可注册此委托来处理派生类的扩展字段
	 * 使用示例（在项目模块的 StartupModule 中）：
	 *
	 * FCoreDelegates::OnPostEngineInit.AddLambda([]() {
	 *     if (GEditor) {
	 *         auto* Subsystem = GEditor->GetEditorSubsystem<UAbilityEditorHelperSubsystem>();
	 *         if (Subsystem) {
	 *             Subsystem->OnPostProcessGameplayEffect.AddLambda([](const FTableRowBase* Config, UGameplayEffect* GE) {
	 *                 // 处理扩展字段...
	 *             });
	 *         }
	 *     }
	 * });
	 */
	FOnPostProcessGameplayEffect OnPostProcessGameplayEffect;

	/** 广播后处理委托（供 AbilityEditorHelperLibrary 内部调用） */
	void BroadcastPostProcessGameplayEffect(const FTableRowBase* Config, UGameplayEffect* GE);

private:
	/** 缓存的 GE 配置 DataTable（编辑器运行期内存缓存） */
	UPROPERTY(Transient)
	TObjectPtr<UDataTable> CachedGameplayEffectDataTable = nullptr;
};

