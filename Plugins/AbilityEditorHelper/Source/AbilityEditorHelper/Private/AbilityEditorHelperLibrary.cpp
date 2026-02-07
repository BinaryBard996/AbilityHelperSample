// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilityEditorHelperLibrary.h"
#include "AbilityEditorHelperSettings.h"
#include "AbilityEditorHelperSubsystem.h"
#include "Editor.h"
#include "Misc/PackageName.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "Kismet/DataTableFunctionLibrary.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "ObjectTools.h"
#include "ObjectTools.h"
// GE Components (5.3+)
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"
#include "GameplayEffectComponents/BlockAbilityTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/AbilitiesGameplayEffectComponent.h"
#include "GameplayEffectComponents/ImmunityGameplayEffectComponent.h"
#include "GameplayEffectComponents/RemoveOtherGameplayEffectComponent.h"

// Calculation classes and abilities
#include "GameplayModMagnitudeCalculation.h"
#include "GameplayEffectExecutionCalculation.h"
#include "Abilities/GameplayAbility.h"

// Added for Schema export / reflection / file ops
#include "UObject/UnrealType.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"
#include "Interfaces/IPluginManager.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GameplayEffectComponents/CancelAbilityTagsGameplayEffectComponent.h"
#include "Misc/SecureHash.h"
#include "Serialization/ArchiveUObject.h"

#if WITH_EDITOR
#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#endif

namespace
{
	// 将可能的包路径或对象路径解析为标准的包名/资产名/对象路径
	static bool ParseAssetPath(const FString& InPath, FString& OutPackageName, FString& OutAssetName, FString& OutObjectPath)
	{
		OutPackageName.Reset();
		OutAssetName.Reset();
		OutObjectPath.Reset();

		if (InPath.IsEmpty())
		{
			return false;
		}

		FString PackageName;
		FString AssetName;

		if (InPath.Contains(TEXT(".")))
		{
			PackageName = FPackageName::ObjectPathToPackageName(InPath);
			AssetName   = FPackageName::ObjectPathToObjectName(InPath);
			OutObjectPath = InPath;
		}
		else
		{
			PackageName = InPath;
			AssetName   = FPackageName::GetLongPackageAssetName(InPath);

			if (!FPackageName::IsValidLongPackageName(PackageName))
			{
				if (!PackageName.StartsWith(TEXT("/")))
				{
					PackageName = TEXT("/Game/") + PackageName;
				}
			}

			OutObjectPath = PackageName + TEXT(".") + AssetName;
		}

		OutPackageName = PackageName;
		OutAssetName   = AssetName;
		return true;
	}

	/**
	 * 从路径加载类
	 */
	template<typename T>
	static UClass* LoadClassFromPath(const FString& InPath)
	{
		FString PackageName, AssetName, ObjectPath;
		if (ParseAssetPath(InPath, PackageName, AssetName, ObjectPath))
		{
			return LoadClass<T>(nullptr, *ObjectPath);
		}
		return nullptr;
	}

	/**
	 * 从路径加载对象
	 */
	template<typename T>
	static T* LoadObjectFromPath(const FString& InPath)
	{
		FString PackageName, AssetName, ObjectPath;
		if (ParseAssetPath(InPath, PackageName, AssetName, ObjectPath))
		{
			return Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, *ObjectPath));
		}
		return nullptr;
	}

	/**
	 * 获取基础配置与 DataTable
	 */
	static bool GetSettingsAndDataTable(const UAbilityEditorHelperSettings*& OutSettings, UDataTable*& OutDataTable)
	{
		OutSettings = GetDefault<UAbilityEditorHelperSettings>();
		if (!OutSettings)
		{
			return false;
		}

		OutDataTable = OutSettings->GameplayEffectDataTable.IsValid() ? OutSettings->GameplayEffectDataTable.Get() : nullptr;
		if (!OutDataTable)
		{
			OutDataTable = OutSettings->GameplayEffectDataTable.LoadSynchronous();
		}

		return OutDataTable != nullptr;
	}

	/**
	 * 获取 GE 资产的基础存放路径
	 */
	static FString GetGameplayEffectBasePath(const UAbilityEditorHelperSettings* Settings)
	{
		if (!Settings) return TEXT("/Game/GameplayEffects");
		
		FString BasePath = Settings->GameplayEffectPath;
		if (BasePath.IsEmpty())
		{
			BasePath = TEXT("/Game/GameplayEffects");
		}
		if (!BasePath.StartsWith(TEXT("/")))
		{
			BasePath = TEXT("/Game/") + BasePath;
		}
		// 去掉末尾斜杠
		if (BasePath.EndsWith(TEXT("/")))
		{
			BasePath.LeftChopInline(1);
		}
		return BasePath;
	}

#if WITH_EDITOR
	/**
	 * 清理指定目录下不在 DataTable 中的 GE 资产
	 */
	static void CleanupGameplayEffectFolder(const FString& BasePath, const UDataTable* DataTable)
	{
		if (BasePath.IsEmpty() || !DataTable) return;

		// 预构建 DataTable 中期望存在的 GE 资产名集合（与创建时的命名规则一致）
		TSet<FName> DesiredAssetNames;
		for (const TPair<FName, uint8*>& RowPair : DataTable->GetRowMap())
		{
			FString RowAssetName = RowPair.Key.ToString();
			if (!RowAssetName.Contains(TEXT("GE_")))
			{
				RowAssetName = TEXT("GE_") + RowAssetName;
			}
			DesiredAssetNames.Add(FName(*RowAssetName));
		}

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

		FARFilter Filter;
		Filter.bRecursivePaths = true;
		Filter.PackagePaths.Add(FName(*BasePath));
		Filter.ClassPaths.Add(UGameplayEffect::StaticClass()->GetClassPathName());

		TArray<FAssetData> Assets;
		AssetRegistry.GetAssets(Filter, Assets);

		TArray<UObject*> ObjectsToDelete;
		ObjectsToDelete.Reserve(Assets.Num());
		for (const FAssetData& AssetData : Assets)
		{
			// 若该 GE 名称不在 DataTable 目标集合中，则标记删除
			if (!DesiredAssetNames.Contains(AssetData.AssetName))
			{
				if (UObject* Obj = AssetData.GetAsset())
				{
					ObjectsToDelete.Add(Obj);
				}
			}
		}

		if (ObjectsToDelete.Num() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("[AbilityEditorHelper] 清理 %d 个未在 DataTable 中的 GE 资产。"), ObjectsToDelete.Num());
			ObjectTools::DeleteObjectsUnchecked(ObjectsToDelete);
		}
	}
#endif

	/**
	 * 将配置转换为 GameplayEffectQuery
	 */
	static FGameplayEffectQuery ToGameplayEffectQuery(const FEffectQueryConfig& Config)
	{
		FGameplayEffectQuery Query;
		Query.OwningTagQuery = FGameplayTagQuery::MakeQuery_MatchAnyTags(Config.MatchAnyOwningTags);

		if (!Config.MatchAllOwningTags.IsEmpty())
		{
			Query.OwningTagQuery = FGameplayTagQuery::MakeQuery_MatchAllTags(Config.MatchAllOwningTags);
		}

		Query.EffectTagQuery = FGameplayTagQuery::MakeQuery_MatchAnyTags(Config.MatchAnySourceTags);

		if (!Config.MatchAllSourceTags.IsEmpty())
		{
			Query.EffectTagQuery = FGameplayTagQuery::MakeQuery_MatchAllTags(Config.MatchAllSourceTags);
		}
		
		return Query;
	}

	/**
	 * 支持 FObjectPtr 序列化的内存写入 Archive（FMemoryWriter 不支持 TObjectPtr）
	 */
	class FGEStateWriter : public FArchiveUObject
	{
	public:
		TArray<uint8>& Bytes;

		FGEStateWriter(TArray<uint8>& InBytes) : Bytes(InBytes)
		{
			SetIsSaving(true);
		}

		virtual void Serialize(void* Data, int64 Num) override
		{
			if (Num > 0)
			{
				const int64 StartIndex = Bytes.AddUninitialized(Num);
				FMemory::Memcpy(Bytes.GetData() + StartIndex, Data, Num);
			}
		}

		virtual FArchive& operator<<(UObject*& Obj) override
		{
			// 将对象引用序列化为路径字符串，用于比较
			FString PathName = Obj ? Obj->GetPathName() : FString();
			*this << PathName;
			return *this;
		}

		virtual FString GetArchiveName() const override
		{
			return TEXT("FGEStateWriter");
		}
	};

	/**
	 * 将 UObject 及其所有子对象序列化为字节数组，用于变更检测
	 * 适用于 GE、GA CDO 等需要比较前后状态的场景
	 */
	static TArray<uint8> SerializeObjectState(UObject* Obj)
	{
		TArray<uint8> Bytes;
		FGEStateWriter Ar(Bytes);
		Obj->Serialize(Ar);

		// 同时序列化所有子对象（GE Components 等），以捕获组件属性变更
		TArray<UObject*> SubObjects;
		GetObjectsWithOuter(Obj, SubObjects, false);
		SubObjects.Sort([](const UObject& A, const UObject& B)
		{
			return A.GetName() < B.GetName();
		});
		for (UObject* SubObj : SubObjects)
		{
			SubObj->Serialize(Ar);
		}

		return Bytes;
	}

}

const UAbilityEditorHelperSettings* UAbilityEditorHelperLibrary::GetAbilityEditorHelperSettings()
{
	return GetDefault<UAbilityEditorHelperSettings>();
}

UBlueprint* UAbilityEditorHelperLibrary::CreateBlueprintAsset(const FString& BlueprintPath, TSubclassOf<UObject> ParentClass, bool& bOutSuccess)
{
	bOutSuccess = false;

	if (BlueprintPath.IsEmpty() || !ParentClass)
	{
		return nullptr;
	}

#if WITH_EDITOR
	// 规范化路径与名称（通用解析）
	FString PackageName, AssetName, ObjectPath;
	if (!ParseAssetPath(BlueprintPath, PackageName, AssetName, ObjectPath))
	{
		return nullptr;
	}

	// 再次校验合法长包名
	if (!FPackageName::IsValidLongPackageName(PackageName))
	{
		return nullptr;
	}

	// 生成唯一资产名，避免重名冲突
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	FString UniquePackageName, UniqueAssetName;
	AssetToolsModule.Get().CreateUniqueAssetName(PackageName, TEXT(""), UniquePackageName, UniqueAssetName);

	// 创建包
	UPackage* Package = CreatePackage(*UniquePackageName);
	if (!Package)
	{
		return nullptr;
	}

	// 创建Blueprint资产
	UBlueprint* NewBlueprint = FKismetEditorUtilities::CreateBlueprint(
		ParentClass,
		Package,
		FName(*UniqueAssetName),
		EBlueprintType::BPTYPE_Normal,
		UBlueprint::StaticClass(),
		UBlueprintGeneratedClass::StaticClass()
	);

	if (NewBlueprint)
	{
		// 注册到资产注册表，并标记脏包
		FAssetRegistryModule::AssetCreated(NewBlueprint);
		const bool bPackageDirtyResult = NewBlueprint->MarkPackageDirty();

		bOutSuccess = true;
	}

	return NewBlueprint;
#else
	return nullptr;
#endif
}

UBlueprint* UAbilityEditorHelperLibrary::GetOrCreateBlueprintAsset(const FString& BlueprintPath, TSubclassOf<UObject> ParentClass, bool& bOutSuccess)
{
	bOutSuccess = false;

	if (BlueprintPath.IsEmpty() || !ParentClass)
	{
		return nullptr;
	}

	// 组装标准对象路径 /Long/Package/Path.AssetName（通用解析）
	FString PackageName, AssetName, ObjectPath;
	if (!ParseAssetPath(BlueprintPath, PackageName, AssetName, ObjectPath))
	{
		return nullptr;
	}

	// 先尝试加载已存在的蓝图资产
	if (!ObjectPath.IsEmpty())
	{
		if (UObject* ExistingObj = StaticLoadObject(UBlueprint::StaticClass(), nullptr, *ObjectPath))
		{
			if (UBlueprint* ExistingBP = Cast<UBlueprint>(ExistingObj))
			{
				bOutSuccess = true;
				return ExistingBP;
			}
		}
	}

	// 若不存在，则尝试创建（非编辑器环境下CreateBlueprintAsset将返回nullptr）
	return CreateBlueprintAsset(BlueprintPath, ParentClass, bOutSuccess);
}

UGameplayEffect* UAbilityEditorHelperLibrary::CreateOrImportGameplayEffect(const FString& GameplayEffectPath, const FGameplayEffectConfig& Config, bool& bOutSuccess)
{
	bOutSuccess = false;

	if (GameplayEffectPath.IsEmpty())
	{
		return nullptr;
	}

	// 解析路径（通用）
	FString PackageName, AssetName, ObjectPath;
	if (!ParseAssetPath(GameplayEffectPath, PackageName, AssetName, ObjectPath))
	{
		return nullptr;
	}

	// 先尝试加载已存在的GE
	UGameplayEffect* GE = nullptr;
	if (!ObjectPath.IsEmpty())
	{
		if (UObject* ExistingObj = StaticLoadObject(UGameplayEffect::StaticClass(), nullptr, *ObjectPath))
		{
			GE = Cast<UGameplayEffect>(ExistingObj);
		}
	}

#if WITH_EDITOR
	bool bIsNewlyCreated = false;

	// 若已存在且提供了 ParentClass，比较现有 GE 的父类与配置的父类，不一致则删除资产以触发重建
	if (GE && !Config.ParentClass.IsEmpty())
	{
		UClass* DesiredParentClass = LoadClassFromPath<UGameplayEffect>(Config.ParentClass);

		// 回退：若不是类路径，则当作 GE 资产路径加载，再取其 Class
		if (!DesiredParentClass)
		{
			if (UGameplayEffect* ParentGEObj = LoadObjectFromPath<UGameplayEffect>(Config.ParentClass))
			{
				DesiredParentClass = ParentGEObj->GetClass();
			}
		}

		if (DesiredParentClass)
		{
			// GE 通过 NewObject(Package, ParentGEClass, ...) 创建，
			// GE->GetClass() 即为创建时指定的类，直接比较即可
			UClass* ExistingClass = GE->GetClass();

			if (ExistingClass != DesiredParentClass)
			{
				UE_LOG(LogTemp, Log, TEXT("[AbilityEditorHelper] 现有 GE 父类与配置不一致，删除并重建：%s"), *GameplayEffectPath);

				TArray<UObject*> ToDelete;
				ToDelete.Add(GE);
				ObjectTools::DeleteObjectsUnchecked(ToDelete);
				GE = nullptr; // 置空以走创建流程
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[AbilityEditorHelper] 无法加载 ParentClass 指定的类型：%s"), *Config.ParentClass);
		}
	}

	// 不存在则创建包与GE资产
	if (!GE)
	{
		if (!FPackageName::IsValidLongPackageName(PackageName))
		{
			return nullptr;
		}

		UPackage* Package = CreatePackage(*PackageName);
		if (!Package)
		{
			return nullptr;
		}

		// 优先：根据 Config.ParentClass 复制父 GE 资产作为新资产
		if (!Config.ParentClass.IsEmpty())
		{
			if (UClass* ParentGEClass = LoadClassFromPath<UGameplayEffect>(Config.ParentClass))
			{
				if (UGameplayEffect* Duplicated = NewObject<UGameplayEffect>(Package, ParentGEClass, FName(*AssetName), RF_Public | RF_Standalone | RF_Transactional))
				{
					GE = Duplicated;
					// 确保为资产标志
					GE->SetFlags(RF_Public | RF_Standalone | RF_Transactional);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("[AbilityEditorHelper] 从 ParentClass 复制 GE 失败：%s，将回退到默认创建"), *Config.ParentClass);
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[AbilityEditorHelper] 无法加载 ParentClass GE：%s，将回退到默认创建"), *Config.ParentClass);
			}
		}

		// 回退：未能从父资产复制时，按设置或默认类创建
		if (!GE)
		{
			UClass* GEClass = UGameplayEffect::StaticClass();
			if (const UAbilityEditorHelperSettings* Settings = GetDefault<UAbilityEditorHelperSettings>())
			{
				if (Settings->GameplayEffectClass)
				{
					GEClass = Settings->GameplayEffectClass;
				}
			}

			GE = NewObject<UGameplayEffect>(Package, GEClass, FName(*AssetName), RF_Public | RF_Standalone | RF_Transactional);
			if (!GE)
			{
				return nullptr;
			}
		}

		FAssetRegistryModule::AssetCreated(GE);
		Package->MarkPackageDirty();
		bIsNewlyCreated = true;
	}

	// 将配置写入GE（基础版）
	if (GE)
	{
		// 变更检测：记录配置应用前的序列化状态
		TArray<uint8> BeforeBytes;
		if (!bIsNewlyCreated)
		{
			BeforeBytes = SerializeObjectState(GE);
		}
		// 基础
		GE->DurationPolicy = Config.DurationType;
		GE->DurationMagnitude = FScalableFloat(Config.DurationMagnitude);
		GE->Period = Config.Period;

		// 堆叠
		GE->StackingType = Config.StackingType;
		GE->StackLimitCount = Config.StackLimitCount;
		GE->StackDurationRefreshPolicy = Config.StackDurationRefreshPolicy;
		GE->StackPeriodResetPolicy = Config.StackPeriodResetPolicy;

		// === Tag 需求配置（UE 5.7+ 使用 Component API）===
		{
			// 检查是否有任何 Tag Requirements 配置
			bool bHasAnyTagRequirements =
				!Config.ApplicationTagRequirements.RequireTags.IsEmpty() ||
				!Config.ApplicationTagRequirements.IgnoreTags.IsEmpty() ||
				!Config.OngoingTagRequirements.RequireTags.IsEmpty() ||
				!Config.OngoingTagRequirements.IgnoreTags.IsEmpty() ||
				!Config.RemovalTagRequirements.RequireTags.IsEmpty() ||
				!Config.RemovalTagRequirements.IgnoreTags.IsEmpty();

			if (bHasAnyTagRequirements)
			{
				UTargetTagRequirementsGameplayEffectComponent& TagReqComp = GE->FindOrAddComponent<UTargetTagRequirementsGameplayEffectComponent>();

				// Application Tag Requirements
				TagReqComp.ApplicationTagRequirements.RequireTags = Config.ApplicationTagRequirements.RequireTags;
				TagReqComp.ApplicationTagRequirements.IgnoreTags = Config.ApplicationTagRequirements.IgnoreTags;

				// Ongoing Tag Requirements
				TagReqComp.OngoingTagRequirements.RequireTags = Config.OngoingTagRequirements.RequireTags;
				TagReqComp.OngoingTagRequirements.IgnoreTags = Config.OngoingTagRequirements.IgnoreTags;

				// Removal Tag Requirements
				TagReqComp.RemovalTagRequirements.RequireTags = Config.RemovalTagRequirements.RequireTags;
				TagReqComp.RemovalTagRequirements.IgnoreTags = Config.RemovalTagRequirements.IgnoreTags;
			}
			else
			{
				RemoveGEComponent<UTargetTagRequirementsGameplayEffectComponent>(GE);
			}
		}

		// === Block Abilities（UE 5.7+ 使用 BlockAbilityTagsGameplayEffectComponent）===
		if (!Config.BlockAbilitiesWithTags.IsEmpty())
		{
			UBlockAbilityTagsGameplayEffectComponent& BlockComp = GE->FindOrAddComponent<UBlockAbilityTagsGameplayEffectComponent>();
			FInheritedTagContainer& BlockTagContainer = const_cast<FInheritedTagContainer&>(BlockComp.GetConfiguredBlockedAbilityTagChanges());
			BlockTagContainer.Added = Config.BlockAbilitiesWithTags;
		}
		else
		{
			RemoveGEComponent<UBlockAbilityTagsGameplayEffectComponent>(GE);
		}

		// === Cancel Abilities（UE 5.7+ 使用 CancelAbilityTagsGameplayEffectComponent）===
		if (!Config.CancelAbilitiesWithTags.IsEmpty())
		{
			UCancelAbilityTagsGameplayEffectComponent& CancelComp = GE->FindOrAddComponent<UCancelAbilityTagsGameplayEffectComponent>();

			FInheritedTagContainer CancelAbilityTags;
			CancelAbilityTags.Added = Config.CancelAbilitiesWithTags;
			CancelComp.SetAndApplyCanceledAbilityTagChanges(CancelAbilityTags, FInheritedTagContainer());
		}
		else
		{
			RemoveGEComponent<UCancelAbilityTagsGameplayEffectComponent>(GE);
		}

		// === Granted Abilities（UE 5.7+ 使用 AbilitiesGameplayEffectComponent）===
		if (Config.GrantedAbilityClasses.Num() > 0)
		{
			UAbilitiesGameplayEffectComponent& AbilitiesComp = GE->FindOrAddComponent<UAbilitiesGameplayEffectComponent>();
			TArray<FGameplayAbilitySpecConfig> AbilityConfigs;

			for (const FString& AbilityClassPath : Config.GrantedAbilityClasses)
			{
				if (UClass* AbilityClass = LoadClassFromPath<UGameplayAbility>(AbilityClassPath))
				{
					FGameplayAbilitySpecConfig SpecConfig;
					SpecConfig.Ability = AbilityClass;
					AbilityConfigs.Add(SpecConfig);
				}
				else if (!AbilityClassPath.IsEmpty())
				{
					UE_LOG(LogTemp, Warning, TEXT("[AbilityEditorHelper] 无法加载 Ability 类：%s"), *AbilityClassPath);
				}
			}

			// 通过 UE 反射访问 protected 成员 GrantAbilityConfigs
			if (AbilityConfigs.Num() > 0)
			{
				FArrayProperty* GrantAbilityConfigsProp = CastField<FArrayProperty>(
					UAbilitiesGameplayEffectComponent::StaticClass()->FindPropertyByName(TEXT("GrantAbilityConfigs"))
				);

				if (GrantAbilityConfigsProp)
				{
					TArray<FGameplayAbilitySpecConfig>* GrantAbilityConfigsPtr =
						GrantAbilityConfigsProp->ContainerPtrToValuePtr<TArray<FGameplayAbilitySpecConfig>>(&AbilitiesComp);

					if (GrantAbilityConfigsPtr)
					{
						*GrantAbilityConfigsPtr = AbilityConfigs;
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("[AbilityEditorHelper] 无法通过反射找到 GrantAbilityConfigs 属性"));
				}
			}
		}
		else
		{
			RemoveGEComponent<UAbilitiesGameplayEffectComponent>(GE);
		}

		// === Tags 组件配置 ===
		// 目标（Granted）Tags 组件
		if (!Config.GrantedTags.IsEmpty())
		{
			UTargetTagsGameplayEffectComponent& TargetTagsComp = GE->FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
			FInheritedTagContainer& TargetTags = const_cast<FInheritedTagContainer&>(TargetTagsComp.GetConfiguredTargetTagChanges());
			TargetTags.Added = Config.GrantedTags;
		}
		else
		{
			RemoveGEComponent<UTargetTagsGameplayEffectComponent>(GE);
		}

		// 资产（Owned/Asset）Tags 组件
		if (!Config.AssetTags.IsEmpty())
		{
			UAssetTagsGameplayEffectComponent& AssetTagsComp = GE->FindOrAddComponent<UAssetTagsGameplayEffectComponent>();
			FInheritedTagContainer& AssetTags =	const_cast<FInheritedTagContainer&>(AssetTagsComp.GetConfiguredAssetTagChanges());
			AssetTags.Added = Config.AssetTags;
		}
		else
		{
			RemoveGEComponent<UAssetTagsGameplayEffectComponent>(GE);
		}

		// === 增强的 Modifiers ===
		GE->Modifiers.Reset();
		for (const FGEModifierConfig& Mod : Config.Modifiers)
		{
			// 通过简化字符串解析 Attribute
			FGameplayAttribute ParsedAttribute;
			if (!ParseAttributeString(Mod.Attribute, ParsedAttribute))
			{
				UE_LOG(LogTemp, Warning, TEXT("[AbilityEditorHelper] 跳过无效的 Modifier Attribute：%s"), *Mod.Attribute);
				continue;
			}

			FGameplayModifierInfo Info;
			Info.Attribute = ParsedAttribute;
			Info.ModifierOp = Mod.ModifierOp;

			// Magnitude 设置（根据类型）
			switch (Mod.MagnitudeCalculationType)
			{
			case EGameplayEffectMagnitudeCalculation::ScalableFloat:
				Info.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Mod.Magnitude));
				break;

			case EGameplayEffectMagnitudeCalculation::AttributeBased:
			{
				FAttributeBasedFloat AttrBased;
				AttrBased.Coefficient = FScalableFloat(Mod.AttributeBasedConfig.Coefficient);
				AttrBased.PreMultiplyAdditiveValue = FScalableFloat(Mod.AttributeBasedConfig.PreMultiplyAdditiveValue);
				AttrBased.PostMultiplyAdditiveValue = FScalableFloat(Mod.AttributeBasedConfig.PostMultiplyAdditiveValue);

				// 解析 BackingAttribute 字符串
				FGameplayAttribute BackingAttribute;
				if (ParseAttributeString(Mod.AttributeBasedConfig.BackingAttribute, BackingAttribute))
				{
					// 创建 Attribute Capture Definition
					FGameplayEffectAttributeCaptureDefinition CaptureDef;
					CaptureDef.AttributeToCapture = BackingAttribute;
					CaptureDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Source;
					CaptureDef.bSnapshot = false;

					AttrBased.BackingAttribute = CaptureDef;
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("[AbilityEditorHelper] 无法解析 BackingAttribute：%s"), *Mod.AttributeBasedConfig.BackingAttribute);
				}

				AttrBased.AttributeCalculationType = Mod.AttributeBasedConfig.AttributeCalculationType;

				Info.ModifierMagnitude = FGameplayEffectModifierMagnitude(AttrBased);
				break;
			}

			case EGameplayEffectMagnitudeCalculation::SetByCaller:
			{
				FSetByCallerFloat SetByCaller;
				SetByCaller.DataTag = Mod.SetByCallerConfig.DataTag;
				SetByCaller.DataName = Mod.SetByCallerConfig.DataName;

				Info.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
				break;
			}

			case EGameplayEffectMagnitudeCalculation::CustomCalculationClass:
			{
				if (UClass* CalcClass = LoadClassFromPath<UGameplayModMagnitudeCalculation>(Mod.CustomCalculationClass))
				{
					FCustomCalculationBasedFloat CustomCalc;
					CustomCalc.CalculationClassMagnitude = CalcClass;
					Info.ModifierMagnitude = FGameplayEffectModifierMagnitude(CustomCalc);
				}
				break;
			}
			default:
				// 默认回退到 ScalableFloat
				Info.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Mod.Magnitude));
				break;
			}

			// Source/Target Tag Requirements
			Info.SourceTags.RequireTags = Mod.SourceTagRequirements.RequireTags;
			Info.SourceTags.IgnoreTags = Mod.SourceTagRequirements.IgnoreTags;
			Info.TargetTags.RequireTags = Mod.TargetTagRequirements.RequireTags;
			Info.TargetTags.IgnoreTags = Mod.TargetTagRequirements.IgnoreTags;

			GE->Modifiers.Add(Info);
		}

		// === GameplayCues ===
		if (Config.GameplayCues.Num() > 0)
		{
			GE->GameplayCues.Reset();
			for (const FGameplayCueConfig& CueConfig : Config.GameplayCues)
			{
				FGameplayEffectCue Cue;
				Cue.GameplayCueTags.AddTag(CueConfig.GameplayCueTag);
				Cue.MinLevel = CueConfig.MinLevel;
				Cue.MaxLevel = CueConfig.MaxLevel;
				GE->GameplayCues.Add(Cue);
			}
		}

		// === Immunity Queries（UE 5.7+ 使用 ImmunityGameplayEffectComponent）===
		if (Config.ImmunityQueries.Num() > 0)
		{
			UImmunityGameplayEffectComponent& ImmunityComp = GE->FindOrAddComponent<UImmunityGameplayEffectComponent>();
			TArray<FGameplayEffectQuery> Queries;

			for (const FEffectQueryConfig& QueryConfig : Config.ImmunityQueries)
			{
				Queries.Add(ToGameplayEffectQuery(QueryConfig));
			}

			ImmunityComp.ImmunityQueries = Queries;
		}
		else
		{
			RemoveGEComponent<UImmunityGameplayEffectComponent>(GE);
		}

		// === Remove Effects Queries（UE 5.7+ 使用 RemoveOtherGameplayEffectComponent）===
		if (Config.RemovalQueries.Num() > 0)
		{
			URemoveOtherGameplayEffectComponent& RemoveComp = GE->FindOrAddComponent<URemoveOtherGameplayEffectComponent>();
			TArray<FGameplayEffectQuery> Queries;

			for (const FEffectQueryConfig& QueryConfig : Config.RemovalQueries)
			{
				Queries.Add(ToGameplayEffectQuery(QueryConfig));
			}

			RemoveComp.RemoveGameplayEffectQueries = Queries;
		}
		else
		{
			RemoveGEComponent<URemoveOtherGameplayEffectComponent>(GE);
		}

		// === Executions ===
		if (Config.Executions.Num() > 0)
		{
			GE->Executions.Reset();
			for (const FExecutionConfig& ExecConfig : Config.Executions)
			{
				if (!ExecConfig.CalculationClass.IsEmpty())
				{
					FString ExecPackageName, ExecAssetName, ExecObjectPath;
					if (ParseAssetPath(ExecConfig.CalculationClass, ExecPackageName, ExecAssetName, ExecObjectPath))
					{
						if (UClass* CalcClass = LoadClass<UGameplayEffectExecutionCalculation>(nullptr, *ExecObjectPath))
						{
							FGameplayEffectExecutionDefinition ExecDef;
							ExecDef.CalculationClass = CalcClass;
							ExecDef.PassedInTags = ExecConfig.PassedInTags;
							GE->Executions.Add(ExecDef);
						}
					}
				}
			}
		}
		
		// 广播后处理委托，让项目 Source 可以处理派生类的扩展字段
		if (GEditor)
		{
			if (UAbilityEditorHelperSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAbilityEditorHelperSubsystem>())
			{
				Subsystem->BroadcastPostProcessGameplayEffect(&Config, GE);
			}
		}

		// 变更检测：仅在属性实际发生变化时才标记脏包
		if (bIsNewlyCreated)
		{
			GE->MarkPackageDirty();
		}
		else
		{
			TArray<uint8> AfterBytes = SerializeObjectState(GE);
			if (BeforeBytes != AfterBytes)
			{
				GE->MarkPackageDirty();
				UE_LOG(LogTemp, Log, TEXT("[AbilityEditorHelper] GE 已变更，标记脏包：%s"), *GE->GetName());
			}
			else
			{
				UE_LOG(LogTemp, Verbose, TEXT("[AbilityEditorHelper] GE 未变更，跳过标记脏包：%s"), *GE->GetName());
			}
		}
	}

	bOutSuccess = GE != nullptr;
	return GE;
#else
	// 运行时环境不创建新资产，仅在存在时返回
	bOutSuccess = (GE != nullptr);
	return GE;
#endif
}

void UAbilityEditorHelperLibrary::CreateOrUpdateGameplayEffectsFromSettings(bool bClearGameplayEffectFolderFirst)
{
	const UAbilityEditorHelperSettings* Settings = nullptr;
	UDataTable* DataTable = nullptr;
	if (!GetSettingsAndDataTable(Settings, DataTable))
	{
		UE_LOG(LogTemp, Warning, TEXT("[AbilityEditorHelper] Settings 未找到或 DataTable 未设置。"));
		return;
	}

	// 校验行结构（支持 FGameplayEffectConfig 及其派生类）
	if (!DataTable->GetRowStruct() || !DataTable->GetRowStruct()->IsChildOf(FGameplayEffectConfig::StaticStruct()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[AbilityEditorHelper] DataTable 行结构不是 FGameplayEffectConfig 或其派生类，无法导入。"));
		return;
	}

	// 规范化基础路径
	FString BasePath = GetGameplayEffectBasePath(Settings);

#if WITH_EDITOR
	// 可选：在导入前清理 BasePath 下（含子目录）中不在 DataTable 的 GE 资产
	if (bClearGameplayEffectFolderFirst)
	{
		CleanupGameplayEffectFolder(BasePath, DataTable);
	}
#endif

	// 遍历每一行并创建/更新 GE，打印结果
	for (const TPair<FName, uint8*>& RowPair : DataTable->GetRowMap())
	{
		const FName RowName = RowPair.Key;
		const uint8* RowData = RowPair.Value;
		if (!RowData)
		{
			UE_LOG(LogTemp, Warning, TEXT("[AbilityEditorHelper] 行 %s 数据为空，已跳过。"), *RowName.ToString());
			continue;
		}

		const FGameplayEffectConfig* Config = reinterpret_cast<const FGameplayEffectConfig*>(RowData);
		FString RowAssetName = RowName.ToString();
		if (!RowAssetName.Contains(TEXT("GE_")))
		{
			RowAssetName = TEXT("GE_") + RowAssetName;
		}
		const FString GEPath = FString::Printf(TEXT("%s/%s"), *BasePath, *RowAssetName);

		bool bOK = false;
		UGameplayEffect* GE = CreateOrImportGameplayEffect(GEPath, *Config, bOK);
		if (bOK && GE)
		{
			UE_LOG(LogTemp, Log, TEXT("[AbilityEditorHelper] 成功创建/更新 GameplayEffect：%s"), *GEPath);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[AbilityEditorHelper] 创建/更新失败：%s"), *GEPath);
		}
	}
}

// ===================== Schema 导出实现 =====================

namespace
{
	static void FillEnumInfo(UEnum* Enum, FString& OutEnumPath, TArray<FString>& OutEnumValues)
	{
		OutEnumPath.Reset();
		OutEnumValues.Reset();
		if (!Enum) return;

		OutEnumPath = Enum->GetPathName();
		const int32 Num = Enum->NumEnums();
		for (int32 i = 0; i < Num; ++i)
		{
			const FString NameString = Enum->GetNameStringByIndex(i);
			if (NameString.EndsWith(TEXT("_MAX")))
			{
				continue;
			}
			OutEnumValues.Add(NameString);
		}
	}

	static FString GetPropertyKind(const FProperty* Prop)
	{
		if (CastField<FBoolProperty>(Prop)) return TEXT("bool");
		if (CastField<FIntProperty>(Prop) || CastField<FInt64Property>(Prop) || CastField<FUInt32Property>(Prop) || CastField<FUInt64Property>(Prop)) return TEXT("int");
		if (CastField<FFloatProperty>(Prop)) return TEXT("float");
		if (CastField<FDoubleProperty>(Prop)) return TEXT("double");
		if (CastField<FStrProperty>(Prop)) return TEXT("string");
		if (CastField<FNameProperty>(Prop)) return TEXT("name");
		if (CastField<FTextProperty>(Prop)) return TEXT("text");
		if (CastField<FEnumProperty>(Prop)) return TEXT("enum");

		if (const FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
		{
			if (ByteProp->Enum) return TEXT("enum");
			return TEXT("int");
		}
		if (CastField<FStructProperty>(Prop)) return TEXT("struct");
		if (CastField<FArrayProperty>(Prop)) return TEXT("array");
		return TEXT("unknown");
	}

	static FString MakeStructSignatureHash(UScriptStruct* Struct)
	{
		if (!Struct) return TEXT("");

		FString Sig;
		for (TFieldIterator<FProperty> It(Struct, EFieldIteratorFlags::IncludeSuper); It; ++It)
		{
			const FProperty* Prop = *It;
			Sig += Prop->GetName();
			Sig += TEXT(":");
			Sig += Prop->GetClass()->GetName();
			Sig += TEXT(";");
		}
		return FMD5::HashAnsiString(*Sig);
	}

	static FString GetPluginPythonSchemaDir()
	{
		FString BaseDir;

#if WITH_EDITOR
		if (IPluginManager::Get().FindPlugin(TEXT("AbilityEditorHelper")).IsValid())
		{
			BaseDir = IPluginManager::Get().FindPlugin(TEXT("AbilityEditorHelper"))->GetBaseDir();
		}
#endif
		if (BaseDir.IsEmpty())
		{
			// 兜底：项目本地插件路径
			BaseDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("Plugins/AbilityEditorHelper"));
		}
		return FPaths::Combine(BaseDir, TEXT("Content/Python/Schema"));
	}

	static bool EnsureDirectoryTree(const FString& Dir)
	{
		IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
		return PF.CreateDirectoryTree(*Dir);
	}
}

bool UAbilityEditorHelperLibrary::WriteStructSchemaToJson(UScriptStruct* StructType, const FString& OutJsonFilePath, FString& OutError)
{
	OutError.Reset();
	if (OutJsonFilePath.IsEmpty())
	{
		OutError = TEXT("StructPath 或 OutJsonFilePath 为空");
		return false;
	}

	if (!StructType)
	{
		OutError = FString::Printf(TEXT("UScriptStruct为空"));
		return false;
	}

	FExcelSchema Schema;
	Schema.StructPath = StructType->GetPathName();
	Schema.Hash = MakeStructSignatureHash(StructType);
	// 特殊类型规则提示（按需扩展）
	Schema.SpecialRules.Add(TEXT("GameplayTagContainer"), TEXT("tag_container_rule"));
	Schema.SpecialRules.Add(TEXT("GameplayAttribute"), TEXT("attribute_rule"));
	Schema.SpecialRules.Add(TEXT("TagRequirementsConfig"), TEXT("tag_requirements_rule"));
	Schema.SpecialRules.Add(TEXT("SoftClassPath"), TEXT("asset_path_rule"));
	Schema.SpecialRules.Add(TEXT("SoftObjectPath"), TEXT("asset_path_rule"));

	for (TFieldIterator<FProperty> It(StructType, EFieldIteratorFlags::IncludeSuper); It; ++It)
	{
		FProperty* Prop = *It;
		if (!Prop) continue;

		FExcelSchemaField Field;
		Field.Name = Prop->GetFName();
		Field.Category = Prop->GetMetaData(TEXT("Category"));
		Field.Kind = GetPropertyKind(Prop);

		// Excel 自定义元数据
		Field.bExcelIgnore = Prop->GetBoolMetaData(TEXT("ExcelIgnore"));
		Field.ExcelName = Prop->GetMetaData(TEXT("ExcelName"));
		Field.ExcelHint = Prop->GetMetaData(TEXT("ExcelHint"));
		Field.ExcelSheet = Prop->GetMetaData(TEXT("ExcelSheet"));
		Field.ExcelSeparator = Prop->GetMetaData(TEXT("ExcelSeparator"));

		if (Field.Kind == TEXT("enum"))
		{
			if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(Prop))
			{
				FillEnumInfo(EnumProp->GetEnum(), Field.EnumPath, Field.EnumValues);
			}
			else if (const FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
			{
				FillEnumInfo(ByteProp->Enum, Field.EnumPath, Field.EnumValues);
			}
		}
		else if (Field.Kind == TEXT("struct"))
		{
			if (const FStructProperty* StructProp = CastField<FStructProperty>(Prop))
			{
				if (StructProp->Struct)
				{
					Field.StructPath = StructProp->Struct->GetPathName();
				}
			}
		}
		else if (Field.Kind == TEXT("array"))
		{
			if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop))
			{
				FProperty* Inner = ArrayProp->Inner;
				Field.InnerKind = Inner ? GetPropertyKind(Inner) : TEXT("unknown");

				if (Inner)
				{
					if (const FStructProperty* InnerStructProp = CastField<FStructProperty>(Inner))
					{
						if (InnerStructProp->Struct)
						{
							Field.InnerStructPath = InnerStructProp->Struct->GetPathName();
						}
					}
					else if (const FEnumProperty* InnerEnumProp = CastField<FEnumProperty>(Inner))
					{
						FillEnumInfo(InnerEnumProp->GetEnum(), Field.InnerEnumPath, Field.InnerEnumValues);
					}
					else if (const FByteProperty* InnerByteProp = CastField<FByteProperty>(Inner))
					{
						if (InnerByteProp->Enum)
						{
							FillEnumInfo(InnerByteProp->Enum, Field.InnerEnumPath, Field.InnerEnumValues);
						}
					}
				}
			}
		}

		Schema.Fields.Add(MoveTemp(Field));
	}

	FString JsonString;
	if (!FJsonObjectConverter::UStructToJsonObjectString(Schema, JsonString))
	{
		OutError = TEXT("UStructToJsonObjectString 失败");
		return false;
	}

	const FString OutDir = FPaths::GetPath(OutJsonFilePath);
	if (!EnsureDirectoryTree(OutDir))
	{
		OutError = FString::Printf(TEXT("创建目录失败：%s"), *OutDir);
		return false;
	}

	if (!FFileHelper::SaveStringToFile(JsonString, *OutJsonFilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		OutError = FString::Printf(TEXT("写文件失败：%s"), *OutJsonFilePath);
		return false;
	}

	return true;
}

bool UAbilityEditorHelperLibrary::GenerateStructSchemaToPythonFolder(UScriptStruct* StructType, FString& OutError)
{
	OutError.Reset();

	if (!StructType)
	{
		OutError = FString::Printf(TEXT("UScriptStruct为空"));
		return false;
	}

	const FString SchemaDir = GetPluginPythonSchemaDir();
	if (!EnsureDirectoryTree(SchemaDir))
	{
		OutError = FString::Printf(TEXT("创建 Schema 目录失败：%s"), *SchemaDir);
		return false;
	}

	const FString FileName = FString::Printf(TEXT("%s.schema.json"), *StructType->GetName());
	const FString SchemaJsonFilePath = FPaths::Combine(SchemaDir, FileName);

	return WriteStructSchemaToJson(StructType, SchemaJsonFilePath, OutError);
}

UScriptStruct* UAbilityEditorHelperLibrary::LoadStructFromPath(const FString& StructPath)
{
	if (StructPath.IsEmpty())
	{
		return nullptr;
	}

	// 尝试从路径加载结构体
	UScriptStruct* LoadedStruct = FindObject<UScriptStruct>(nullptr, *StructPath);

	if (!LoadedStruct)
	{
		// 如果 FindObject 失败，尝试使用 LoadObject
		LoadedStruct = LoadObject<UScriptStruct>(nullptr, *StructPath);
	}

	if (!LoadedStruct)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AbilityEditorHelper] 无法从路径加载结构体：%s"), *StructPath);
	}

	return LoadedStruct;
}

bool UAbilityEditorHelperLibrary::GenerateAllSchemasFromSettings(bool bClearSchemaFolderFirst, int32& OutSuccessCount, int32& OutFailureCount, FString& OutErrors)
{
	OutSuccessCount = 0;
	OutFailureCount = 0;
	OutErrors.Reset();

	const UAbilityEditorHelperSettings* Settings = GetDefault<UAbilityEditorHelperSettings>();
	if (!Settings)
	{
		OutErrors = TEXT("无法获取 UAbilityEditorHelperSettings");
		OutFailureCount = 1;
		return false;
	}

	const TArray<FString>& StructPaths = Settings->StructTypePathsToExportSchema;
	if (StructPaths.Num() == 0)
	{
		OutErrors = TEXT("StructTypePathsToExportSchema 列表为空，没有需要导出的结构体");
		return true; // 不算失败，只是没有工作要做
	}

	// 如果需要，先清空 Schema 文件夹
	if (bClearSchemaFolderFirst)
	{
		const FString SchemaDir = GetPluginPythonSchemaDir();
		IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();

		if (PF.DirectoryExists(*SchemaDir))
		{
			TArray<FString> FilesToDelete;
			PF.FindFiles(FilesToDelete, *SchemaDir, TEXT(".schema.json"));

			for (const FString& FilePath : FilesToDelete)
			{
				PF.DeleteFile(*FilePath);
				UE_LOG(LogTemp, Log, TEXT("[AbilityEditorHelper] 删除旧 Schema 文件：%s"), *FilePath);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[AbilityEditorHelper] 开始批量生成 Schema，共 %d 个结构体"), StructPaths.Num());

	TArray<FString> ErrorMessages;

	for (const FString& StructPath : StructPaths)
	{
		if (StructPath.IsEmpty())
		{
			ErrorMessages.Add(TEXT("遇到空的结构体路径"));
			OutFailureCount++;
			continue;
		}

		// 从路径加载结构体
		UScriptStruct* StructType = LoadStructFromPath(StructPath);
		if (!StructType)
		{
			const FString Error = FString::Printf(TEXT("无法加载结构体：%s"), *StructPath);
			ErrorMessages.Add(Error);
			OutFailureCount++;
			UE_LOG(LogTemp, Error, TEXT("[AbilityEditorHelper] %s"), *Error);
			continue;
		}

		FString ErrorMsg;
		const bool bSuccess = GenerateStructSchemaToPythonFolder(StructType, ErrorMsg);

		if (bSuccess)
		{
			OutSuccessCount++;
			UE_LOG(LogTemp, Log, TEXT("[AbilityEditorHelper] 成功生成 Schema：%s"), *StructType->GetName());
		}
		else
		{
			OutFailureCount++;
			const FString FullError = FString::Printf(TEXT("[%s] %s"), *StructType->GetName(), *ErrorMsg);
			ErrorMessages.Add(FullError);
			UE_LOG(LogTemp, Error, TEXT("[AbilityEditorHelper] 生成 Schema 失败：%s"), *FullError);
		}
	}

	// 组合所有错误消息
	if (ErrorMessages.Num() > 0)
	{
		OutErrors = FString::Join(ErrorMessages, TEXT("\n"));
	}

	const bool bAllSuccess = (OutFailureCount == 0);
	UE_LOG(LogTemp, Log, TEXT("[AbilityEditorHelper] 批量生成 Schema 完成：成功 %d 个，失败 %d 个"),
	       OutSuccessCount, OutFailureCount);

	return bAllSuccess;
}

bool UAbilityEditorHelperLibrary::ImportDataTableFromJsonFile(UDataTable* TargetDataTable, const FString& JsonFileName, bool bClearBeforeImport, int32& OutImportedRowCount, FString& OutError)
{
	OutError.Reset();
	OutImportedRowCount = 0;

	if (!TargetDataTable)
	{
		OutError = TEXT("TargetDataTable 为空");
		return false;
	}

	if (JsonFileName.IsEmpty())
	{
		OutError = TEXT("JsonFileName 为空");
		return false;
	}

	// 从设置获取 JsonPath 并拼接完整路径
	const UAbilityEditorHelperSettings* Settings = GetDefault<UAbilityEditorHelperSettings>();
	if (!Settings || Settings->JsonPath.IsEmpty())
	{
		OutError = TEXT("UAbilityEditorHelperSettings 的 JsonPath 未配置");
		return false;
	}

	const FString JsonFilePath = FPaths::Combine(Settings->JsonPath, JsonFileName);

	if (!FPaths::FileExists(JsonFilePath))
	{
		OutError = FString::Printf(TEXT("JSON 文件不存在：%s"), *JsonFilePath);
		return false;
	}

	// 可选：导入前清空旧数据
	if (bClearBeforeImport)
	{
		TargetDataTable->EmptyTable();
	}

	// 调用引擎函数从 JSON 文件填充 DataTable
	OutImportedRowCount = UDataTableFunctionLibrary::FillDataTableFromJSONFile(TargetDataTable, JsonFilePath);

	const bool bSuccess = (OutImportedRowCount >= 0);

#if WITH_EDITOR
	if (bSuccess)
	{
		TargetDataTable->MarkPackageDirty();
	}
#endif

	return bSuccess;
}

bool UAbilityEditorHelperLibrary::ParseAttributeString(const FString& AttributeString, FGameplayAttribute& OutAttribute)
{
	OutAttribute = FGameplayAttribute();

	if (AttributeString.IsEmpty())
	{
		return false;
	}

	// 支持两种格式：
	// 1. 简化格式：ClassName.PropertyName（如 TestAttributeSet.TestPropertyOne）
	// 2. 完整格式：/Script/Module.ClassName:PropertyName

	FString ClassName;
	FString PropertyName;

	// 检查是否是完整路径格式
	if (AttributeString.Contains(TEXT(":")))
	{
		// 完整格式：/Script/Module.ClassName:PropertyName
		FString LeftPart;
		if (!AttributeString.Split(TEXT(":"), &LeftPart, &PropertyName))
		{
			UE_LOG(LogTemp, Warning, TEXT("[AbilityEditorHelper] 无法解析完整格式的属性字符串：%s"), *AttributeString);
			return false;
		}

		// 从路径中提取类名（最后一个.后的部分）
		int32 LastDotIndex;
		if (LeftPart.FindLastChar(TEXT('.'), LastDotIndex))
		{
			ClassName = LeftPart.Mid(LastDotIndex + 1);
		}
		else
		{
			ClassName = LeftPart;
		}
	}
	else if (AttributeString.Contains(TEXT(".")))
	{
		// 简化格式：ClassName.PropertyName
		if (!AttributeString.Split(TEXT("."), &ClassName, &PropertyName))
		{
			UE_LOG(LogTemp, Warning, TEXT("[AbilityEditorHelper] 无法解析简化格式的属性字符串：%s"), *AttributeString);
			return false;
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[AbilityEditorHelper] 属性字符串格式无效：%s"), *AttributeString);
		return false;
	}

	// 确保类名以 U 开头（UE 类命名规范）
	FString FullClassName = ClassName;
	if (!FullClassName.StartsWith(TEXT("U")))
	{
		FullClassName = TEXT("U") + ClassName;
	}

	// 只遍历 UAttributeSet 的派生类（性能优化）
	UClass* FoundClass = nullptr;
	TArray<UClass*> AttributeSetClasses;
	GetDerivedClasses(UAttributeSet::StaticClass(), AttributeSetClasses, true);

	for (UClass* Class : AttributeSetClasses)
	{
		if (!Class || Class->HasAnyClassFlags(CLASS_Abstract))
		{
			continue;
		}

		const FString CurrentClassName = Class->GetName();
		// 比较类名（带U前缀或不带都支持）
		if (CurrentClassName.Equals(FullClassName, ESearchCase::IgnoreCase) ||
			CurrentClassName.Equals(ClassName, ESearchCase::IgnoreCase))
		{
			FoundClass = Class;
			break;
		}
	}

	if (!FoundClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AbilityEditorHelper] 无法找到 AttributeSet 类：%s"), *ClassName);
		return false;
	}

	// 从类中查找属性
	FProperty* FoundProperty = FoundClass->FindPropertyByName(FName(*PropertyName));
	if (!FoundProperty)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AbilityEditorHelper] 在类 %s 中无法找到属性：%s"), *FoundClass->GetName(), *PropertyName);
		return false;
	}

	// 验证是 FGameplayAttributeData 类型的属性
	const FStructProperty* StructProperty = CastField<FStructProperty>(FoundProperty);
	if (!StructProperty || StructProperty->Struct != FGameplayAttributeData::StaticStruct())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AbilityEditorHelper] 属性 %s.%s 不是 FGameplayAttributeData 类型"),
			*FoundClass->GetName(), *PropertyName);
		return false;
	}

	// 构建 FGameplayAttribute
	OutAttribute = FGameplayAttribute(FoundProperty);

	UE_LOG(LogTemp, Log, TEXT("[AbilityEditorHelper] 成功解析属性：%s -> %s.%s"),
		*AttributeString, *FoundClass->GetName(), *PropertyName);

	return true;
}

// ===================== 增量更新实现 =====================

namespace
{
	/**
	 * 将结构体序列化为 JSON 字符串（用于比较）
	 * 支持 FGameplayEffectConfig 及其派生类
	 */
	static FString SerializeStructToJsonString(UScriptStruct* StructType, const void* StructData)
	{
		FString JsonString;
		if (FJsonObjectConverter::UStructToJsonObjectString(StructType, StructData, JsonString, 0, 0))
		{
			return JsonString;
		}
		return TEXT("");
	}

	/**
	 * 比较两个结构体是否相同
	 * 通过序列化为 JSON 字符串后比较来实现
	 */
	static bool AreStructsEqual(UScriptStruct* StructType, const void* A, const void* B)
	{
		const FString JsonA = SerializeStructToJsonString(StructType, A);
		const FString JsonB = SerializeStructToJsonString(StructType, B);
		return JsonA.Equals(JsonB);
	}
}

bool UAbilityEditorHelperLibrary::ImportAndUpdateGameplayEffectsFromJson(
	const FString& JsonFileName,
	bool bClearGameplayEffectFolderFirst,
	TArray<FName>& OutUpdatedRowNames)
{
	OutUpdatedRowNames.Reset();

	const UAbilityEditorHelperSettings* Settings = nullptr;
	UDataTable* DataTable = nullptr;
	if (!GetSettingsAndDataTable(Settings, DataTable))
	{
		UE_LOG(LogAbilityEditor, Error, TEXT("无法获取 Settings 或 DataTable"));
		return false;
	}

	// 校验行结构（支持 FGameplayEffectConfig 及其派生类）
	UScriptStruct* RowStruct = const_cast<UScriptStruct*>(DataTable->GetRowStruct());
	if (!RowStruct || !RowStruct->IsChildOf(FGameplayEffectConfig::StaticStruct()))
	{
		UE_LOG(LogAbilityEditor, Error, TEXT("DataTable 行结构不是 FGameplayEffectConfig 或其派生类"));
		return false;
	}

	const int32 StructSize = RowStruct->GetStructureSize();

	// 构建 JSON 文件完整路径
	if (Settings->JsonPath.IsEmpty())
	{
		UE_LOG(LogAbilityEditor, Error, TEXT("UAbilityEditorHelperSettings 的 JsonPath 未配置"));
		return false;
	}
	const FString JsonFilePath = FPaths::Combine(Settings->JsonPath, JsonFileName);

	if (!FPaths::FileExists(JsonFilePath))
	{
		UE_LOG(LogAbilityEditor, Error, TEXT("JSON 文件不存在：%s"), *JsonFilePath);
		return false;
	}

	// 读取 JSON 文件内容
	FString JsonContent;
	if (!FFileHelper::LoadFileToString(JsonContent, *JsonFilePath))
	{
		UE_LOG(LogAbilityEditor, Error, TEXT("无法读取 JSON 文件：%s"), *JsonFilePath);
		return false;
	}

	// 解析 JSON 为数组
	TArray<TSharedPtr<FJsonValue>> JsonArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(Reader, JsonArray))
	{
		UE_LOG(LogAbilityEditor, Error, TEXT("JSON 解析失败，格式不正确"));
		return false;
	}

	// 缓存现有 DataTable 数据的 JSON 表示（用于比较）
	TMap<FName, FString> ExistingJsonMap;
	for (const TPair<FName, uint8*>& RowPair : DataTable->GetRowMap())
	{
		if (RowPair.Value)
		{
			FString JsonStr = SerializeStructToJsonString(RowStruct, RowPair.Value);
			ExistingJsonMap.Add(RowPair.Key, JsonStr);
		}
	}

	// 使用动态内存分配存储新配置数据
	TMap<FName, TSharedPtr<uint8, ESPMode::ThreadSafe>> NewConfigsMemory;
	TMap<FName, FString> NewJsonMap;

	for (const TSharedPtr<FJsonValue>& JsonValue : JsonArray)
	{
		if (!JsonValue.IsValid() || JsonValue->Type != EJson::Object)
		{
			continue;
		}

		const TSharedPtr<FJsonObject>& JsonObject = JsonValue->AsObject();
		if (!JsonObject.IsValid())
		{
			continue;
		}

		// 获取行名（Name 字段）
		FString RowNameStr;
		if (!JsonObject->TryGetStringField(TEXT("Name"), RowNameStr) || RowNameStr.IsEmpty())
		{
			UE_LOG(LogAbilityEditor, Warning, TEXT("JSON 条目缺少 Name 字段，已跳过"));
			continue;
		}

		// 分配内存并初始化结构体
		TSharedPtr<uint8, ESPMode::ThreadSafe> ConfigMemory(
			static_cast<uint8*>(FMemory::Malloc(StructSize)),
			[](uint8* Ptr) { FMemory::Free(Ptr); }
		);
		RowStruct->InitializeStruct(ConfigMemory.Get());

		// 反序列化为实际的结构体类型
		if (!FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), RowStruct, ConfigMemory.Get()))
		{
			UE_LOG(LogAbilityEditor, Warning, TEXT("无法反序列化行 %s，已跳过"), *RowNameStr);
			RowStruct->DestroyStruct(ConfigMemory.Get());
			continue;
		}

		FName RowName(*RowNameStr);
		NewConfigsMemory.Add(RowName, ConfigMemory);

		// 序列化为 JSON 字符串用于比较
		FString NewJsonStr = SerializeStructToJsonString(RowStruct, ConfigMemory.Get());
		NewJsonMap.Add(RowName, NewJsonStr);

		// 比较是否有变化
		const FString* ExistingJson = ExistingJsonMap.Find(RowName);
		if (!ExistingJson || !ExistingJson->Equals(NewJsonStr))
		{
			// 新增或变化的行
			OutUpdatedRowNames.Add(RowName);
			UE_LOG(LogAbilityEditor, Log, TEXT("检测到变化的行：%s"), *RowNameStr);
		}
	}

	// 如果没有变化，直接返回
	if (OutUpdatedRowNames.Num() == 0)
	{
		UE_LOG(LogAbilityEditor, Log, TEXT("未检测到任何数据变化，无需更新"));
		return true;
	}

	UE_LOG(LogAbilityEditor, Log, TEXT("共检测到 %d 行数据变化"), OutUpdatedRowNames.Num());

#if WITH_EDITOR
	// 更新 DataTable 中变化的行
	for (const FName& RowName : OutUpdatedRowNames)
	{
		TSharedPtr<uint8, ESPMode::ThreadSafe>* NewConfigMemory = NewConfigsMemory.Find(RowName);
		if (!NewConfigMemory || !NewConfigMemory->IsValid())
		{
			continue;
		}

		// 检查行是否存在
		uint8* ExistingRowData = DataTable->FindRowUnchecked(RowName);
		if (ExistingRowData)
		{
			// 更新现有行（复制结构体数据）
			RowStruct->CopyScriptStruct(ExistingRowData, NewConfigMemory->Get());
		}
		else
		{
			// 添加新行
			DataTable->AddRow(RowName, *reinterpret_cast<FTableRowBase*>(NewConfigMemory->Get()));
		}
	}

	DataTable->MarkPackageDirty();

	// 规范化基础路径
	FString BasePath = GetGameplayEffectBasePath(Settings);

	// 可选：清理不在 DataTable 中的 GE 资产
	if (bClearGameplayEffectFolderFirst)
	{
		CleanupGameplayEffectFolder(BasePath, DataTable);
	}

	// 只对变化的行创建/更新 GameplayEffect
	int32 SuccessCount = 0;
	int32 FailCount = 0;
	for (const FName& RowName : OutUpdatedRowNames)
	{
		// 从 DataTable 获取更新后的配置
		uint8* ConfigData = DataTable->FindRowUnchecked(RowName);
		if (!ConfigData)
		{
			continue;
		}
		const FGameplayEffectConfig* Config = reinterpret_cast<const FGameplayEffectConfig*>(ConfigData);

		FString RowAssetName = RowName.ToString();
		if (!RowAssetName.Contains(TEXT("GE_")))
		{
			RowAssetName = TEXT("GE_") + RowAssetName;
		}
		const FString GEPath = FString::Printf(TEXT("%s/%s"), *BasePath, *RowAssetName);

		bool bOK = false;
		UGameplayEffect* GE = CreateOrImportGameplayEffect(GEPath, *Config, bOK);
		if (bOK && GE)
		{
			UE_LOG(LogTemp, Log, TEXT("[AbilityEditorHelper] 成功创建/更新 GameplayEffect：%s"), *GEPath);
			++SuccessCount;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[AbilityEditorHelper] 创建/更新失败：%s"), *GEPath);
			++FailCount;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[AbilityEditorHelper] 增量更新完成：成功 %d 个，失败 %d 个"), SuccessCount, FailCount);

	return FailCount == 0;
#else
	OutError = TEXT("此功能仅在编辑器环境下可用");
	return false;
#endif
}

// ===========================================
// GameplayAbility 相关函数实现
// ===========================================

bool UAbilityEditorHelperLibrary::GetGASettingsAndDataTable(const UAbilityEditorHelperSettings*& OutSettings, UDataTable*& OutDataTable)
{
	OutSettings = GetDefault<UAbilityEditorHelperSettings>();
	if (!OutSettings)
	{
		UE_LOG(LogAbilityEditor, Warning, TEXT("无法获取 UAbilityEditorHelperSettings"));
		return false;
	}

	OutDataTable = OutSettings->GameplayAbilityDataTable.IsValid()
		? OutSettings->GameplayAbilityDataTable.Get()
		: OutSettings->GameplayAbilityDataTable.LoadSynchronous();

	if (!OutDataTable)
	{
		UE_LOG(LogAbilityEditor, Warning, TEXT("GameplayAbilityDataTable 未配置或加载失败"));
		return false;
	}

	return true;
}

FString UAbilityEditorHelperLibrary::GetGameplayAbilityBasePath(const UAbilityEditorHelperSettings* Settings)
{
	if (!Settings || Settings->GameplayAbilityPath.IsEmpty())
	{
		return TEXT("/Game/Abilities/Abilities");
	}

	FString BasePath = Settings->GameplayAbilityPath;
	if (!BasePath.StartsWith(TEXT("/")))
	{
		BasePath = TEXT("/Game/") + BasePath;
	}
	BasePath.RemoveFromEnd(TEXT("/"));
	return BasePath;
}

void UAbilityEditorHelperLibrary::CleanupGameplayAbilityFolder(const FString& BasePath, const UDataTable* DataTable)
{
#if WITH_EDITOR
	if (!DataTable)
	{
		return;
	}

	// 收集 DataTable 中的所有行名（带 GA_ 前缀）
	TSet<FString> ValidAssetNames;
	for (const TPair<FName, uint8*>& RowPair : DataTable->GetRowMap())
	{
		FString RowAssetName = RowPair.Key.ToString();
		if (!RowAssetName.Contains(TEXT("GA_")))
		{
			RowAssetName = TEXT("GA_") + RowAssetName;
		}
		ValidAssetNames.Add(RowAssetName);
	}

	// 扫描目录下的蓝图资产
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> AssetDataList;
	AssetRegistry.GetAssetsByPath(FName(*BasePath), AssetDataList, true);

	TArray<UObject*> ToDelete;
	for (const FAssetData& AssetData : AssetDataList)
	{
		if (AssetData.AssetClassPath == UBlueprint::StaticClass()->GetClassPathName())
		{
			FString AssetName = AssetData.AssetName.ToString();
			if (!ValidAssetNames.Contains(AssetName))
			{
				if (UObject* Asset = AssetData.GetAsset())
				{
					ToDelete.Add(Asset);
					UE_LOG(LogAbilityEditor, Log, TEXT("[AbilityEditorHelper] 清理不在 DataTable 中的 GA 资产：%s"), *AssetName);
				}
			}
		}
	}

	if (ToDelete.Num() > 0)
	{
		ObjectTools::DeleteObjectsUnchecked(ToDelete);
	}
#endif
}

UGameplayAbility* UAbilityEditorHelperLibrary::CreateOrImportGameplayAbility(
	const FString& GameplayAbilityPath,
	const FGameplayAbilityConfig& Config,
	bool& bOutSuccess)
{
	bOutSuccess = false;

	FString PackageName, AssetName, ObjectPath;
	if (!ParseAssetPath(GameplayAbilityPath, PackageName, AssetName, ObjectPath))
	{
		UE_LOG(LogAbilityEditor, Warning, TEXT("[AbilityEditorHelper] 无法解析路径：%s"), *GameplayAbilityPath);
		return nullptr;
	}

	// 尝试加载现有蓝图
	UBlueprint* ExistingBlueprint = LoadObject<UBlueprint>(nullptr, *ObjectPath);
	UGameplayAbility* GA = nullptr;

	if (ExistingBlueprint)
	{
		GA = Cast<UGameplayAbility>(ExistingBlueprint->GeneratedClass->GetDefaultObject());
	}

#if WITH_EDITOR
	bool bIsNewlyCreated = false;

	// 确定目标父类
	UClass* DesiredParentClass = UGameplayAbility::StaticClass();
	if (!Config.ParentClass.IsEmpty())
	{
		if (UClass* LoadedClass = LoadClassFromPath<UGameplayAbility>(Config.ParentClass))
		{
			DesiredParentClass = LoadedClass;
		}
		else
		{
			UE_LOG(LogAbilityEditor, Warning, TEXT("[AbilityEditorHelper] 无法加载 ParentClass：%s，使用默认类"), *Config.ParentClass);
		}
	}
	else
	{
		// 从 Settings 获取默认类
		if (const UAbilityEditorHelperSettings* Settings = GetDefault<UAbilityEditorHelperSettings>())
		{
			if (Settings->GameplayAbilityClass)
			{
				DesiredParentClass = Settings->GameplayAbilityClass;
			}
		}
	}

	// 检查父类是否匹配，不匹配则删除重建
	if (ExistingBlueprint && GA)
	{
		UClass* ExistingParentClass = ExistingBlueprint->ParentClass;
		if (ExistingParentClass != DesiredParentClass)
		{
			UE_LOG(LogAbilityEditor, Log, TEXT("[AbilityEditorHelper] 现有 GA 父类与配置不一致，删除并重建：%s"), *GameplayAbilityPath);
			TArray<UObject*> ToDelete;
			ToDelete.Add(ExistingBlueprint);
			ObjectTools::DeleteObjectsUnchecked(ToDelete);
			ExistingBlueprint = nullptr;
			GA = nullptr;
		}
	}

	// 不存在则创建蓝图
	if (!ExistingBlueprint)
	{
		if (!FPackageName::IsValidLongPackageName(PackageName))
		{
			return nullptr;
		}

		UPackage* Package = CreatePackage(*PackageName);
		if (!Package)
		{
			return nullptr;
		}

		// 创建蓝图
		ExistingBlueprint = FKismetEditorUtilities::CreateBlueprint(
			DesiredParentClass,
			Package,
			FName(*AssetName),
			BPTYPE_Normal,
			UBlueprint::StaticClass(),
			UBlueprintGeneratedClass::StaticClass()
		);

		if (!ExistingBlueprint)
		{
			UE_LOG(LogAbilityEditor, Error, TEXT("[AbilityEditorHelper] 无法创建蓝图：%s"), *GameplayAbilityPath);
			return nullptr;
		}

		FAssetRegistryModule::AssetCreated(ExistingBlueprint);
		Package->MarkPackageDirty();
		bIsNewlyCreated = true;
	}

	// 获取 CDO
	GA = Cast<UGameplayAbility>(ExistingBlueprint->GeneratedClass->GetDefaultObject());
	if (!GA)
	{
		UE_LOG(LogAbilityEditor, Error, TEXT("[AbilityEditorHelper] 无法获取 GA CDO：%s"), *GameplayAbilityPath);
		return nullptr;
	}

	// 变更检测：记录配置应用前的序列化状态
	TArray<uint8> BeforeBytes;
	if (!bIsNewlyCreated)
	{
		BeforeBytes = SerializeObjectState(GA);
	}

	// 写入配置数据到 GA（通过 UE 反射访问 protected 成员）
	{
		UClass* GAClass = UGameplayAbility::StaticClass();

		// === 辅助 Lambda：通过反射设置 TSubclassOf<UGameplayEffect> 属性 ===
		auto SetClassProperty = [&](const TCHAR* PropertyName, UClass* ClassValue)
		{
			if (!ClassValue) return;
			if (FClassProperty* Prop = CastField<FClassProperty>(GAClass->FindPropertyByName(PropertyName)))
			{
				void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(GA);
				Prop->SetObjectPropertyValue(ValuePtr, ClassValue);
			}
		};

		// === 辅助 Lambda：通过反射设置 FGameplayTagContainer 属性 ===
		auto SetTagContainerProperty = [&](const TCHAR* PropertyName, const FGameplayTagContainer& Tags)
		{
			if (FStructProperty* Prop = CastField<FStructProperty>(GAClass->FindPropertyByName(PropertyName)))
			{
				FGameplayTagContainer* ValuePtr = Prop->ContainerPtrToValuePtr<FGameplayTagContainer>(GA);
				if (ValuePtr)
				{
					*ValuePtr = Tags;
				}
			}
		};

		// === 辅助 Lambda：通过反射设置 bool 属性 ===
		auto SetBoolProperty = [&](const TCHAR* PropertyName, bool Value)
		{
			if (FBoolProperty* Prop = CastField<FBoolProperty>(GAClass->FindPropertyByName(PropertyName)))
			{
				void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(GA);
				Prop->SetPropertyValue(ValuePtr, Value);
			}
		};

		// === 辅助 Lambda：通过反射设置 TEnumAsByte 属性 ===
		auto SetByteProperty = [&](const TCHAR* PropertyName, uint8 Value)
		{
			if (FByteProperty* Prop = CastField<FByteProperty>(GAClass->FindPropertyByName(PropertyName)))
			{
				void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(GA);
				Prop->SetPropertyValue(ValuePtr, Value);
			}
		};

		// Cost GE
		if (!Config.CostGameplayEffectClass.IsEmpty())
		{
			if (UClass* CostClass = LoadClassFromPath<UGameplayEffect>(Config.CostGameplayEffectClass))
			{
				SetClassProperty(TEXT("CostGameplayEffectClass"), CostClass);
			}
		}

		// Cooldown GE
		if (!Config.CooldownGameplayEffectClass.IsEmpty())
		{
			if (UClass* CooldownClass = LoadClassFromPath<UGameplayEffect>(Config.CooldownGameplayEffectClass))
			{
				SetClassProperty(TEXT("CooldownGameplayEffectClass"), CooldownClass);
			}
		}

		// Tags（UE 5.7 中这些是 FGameplayTagContainer 类型）
		SetTagContainerProperty(TEXT("AbilityTags"), Config.AbilityTags);
		SetTagContainerProperty(TEXT("CancelAbilitiesWithTag"), Config.CancelAbilitiesWithTag);
		SetTagContainerProperty(TEXT("BlockAbilitiesWithTag"), Config.BlockAbilitiesWithTag);
		SetTagContainerProperty(TEXT("ActivationOwnedTags"), Config.ActivationOwnedTags);
		SetTagContainerProperty(TEXT("ActivationRequiredTags"), Config.ActivationRequiredTags);
		SetTagContainerProperty(TEXT("ActivationBlockedTags"), Config.ActivationBlockedTags);
		SetTagContainerProperty(TEXT("SourceRequiredTags"), Config.SourceRequiredTags);
		SetTagContainerProperty(TEXT("SourceBlockedTags"), Config.SourceBlockedTags);
		SetTagContainerProperty(TEXT("TargetRequiredTags"), Config.TargetRequiredTags);
		SetTagContainerProperty(TEXT("TargetBlockedTags"), Config.TargetBlockedTags);

		// Triggers（通过反射访问 TArray<FAbilityTriggerData>）
		if (FArrayProperty* TriggersProp = CastField<FArrayProperty>(GAClass->FindPropertyByName(TEXT("AbilityTriggers"))))
		{
			TArray<FAbilityTriggerData>* TriggersPtr = TriggersProp->ContainerPtrToValuePtr<TArray<FAbilityTriggerData>>(GA);
			if (TriggersPtr)
			{
				TriggersPtr->Empty();
				for (const FAbilityTriggerConfig& TriggerConfig : Config.AbilityTriggers)
				{
					FAbilityTriggerData TriggerData;
					TriggerData.TriggerTag = TriggerConfig.TriggerTag;
					TriggerData.TriggerSource = TriggerConfig.TriggerSource;
					TriggersPtr->Add(TriggerData);
				}
			}
		}

		// Advanced bool 属性
		SetBoolProperty(TEXT("bServerRespectsRemoteAbilityCancellation"), Config.bServerRespectsRemoteAbilityCancellation);
		SetBoolProperty(TEXT("bReplicateInputDirectly"), Config.bReplicateInputDirectly);
		SetBoolProperty(TEXT("bRetriggerInstancedAbility"), Config.bRetriggerInstancedAbility);

		// 枚举策略属性（TEnumAsByte）
		SetByteProperty(TEXT("NetExecutionPolicy"), static_cast<uint8>(Config.NetExecutionPolicy));
		SetByteProperty(TEXT("NetSecurityPolicy"), static_cast<uint8>(Config.NetSecurityPolicy));
		SetByteProperty(TEXT("InstancingPolicy"), static_cast<uint8>(Config.InstancingPolicy));
		SetByteProperty(TEXT("ReplicationPolicy"), static_cast<uint8>(Config.ReplicationPolicy));

		// 广播后处理委托
		if (GEditor)
		{
			if (UAbilityEditorHelperSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAbilityEditorHelperSubsystem>())
			{
				Subsystem->BroadcastPostProcessGameplayAbility(&Config, GA);
			}
		}

		// 变更检测：仅在属性实际发生变化时才标记脏包
		if (bIsNewlyCreated)
		{
			ExistingBlueprint->MarkPackageDirty();
		}
		else
		{
			TArray<uint8> AfterBytes = SerializeObjectState(GA);
			if (BeforeBytes != AfterBytes)
			{
				ExistingBlueprint->MarkPackageDirty();
				UE_LOG(LogAbilityEditor, Log, TEXT("[AbilityEditorHelper] GA 已变更，标记脏包：%s"), *GA->GetName());
			}
			else
			{
				UE_LOG(LogAbilityEditor, Verbose, TEXT("[AbilityEditorHelper] GA 未变更，跳过标记脏包：%s"), *GA->GetName());
			}
		}
	}

	bOutSuccess = GA != nullptr;
	return GA;
#else
	bOutSuccess = (GA != nullptr);
	return GA;
#endif
}

void UAbilityEditorHelperLibrary::CreateOrUpdateGameplayAbilitiesFromSettings(bool bClearGameplayAbilityFolderFirst)
{
	const UAbilityEditorHelperSettings* Settings = nullptr;
	UDataTable* DataTable = nullptr;
	if (!GetGASettingsAndDataTable(Settings, DataTable))
	{
		UE_LOG(LogAbilityEditor, Warning, TEXT("[AbilityEditorHelper] Settings 未找到或 DataTable 未设置。"));
		return;
	}

	// 校验行结构
	if (!DataTable->GetRowStruct() || !DataTable->GetRowStruct()->IsChildOf(FGameplayAbilityConfig::StaticStruct()))
	{
		UE_LOG(LogAbilityEditor, Warning, TEXT("[AbilityEditorHelper] DataTable 行结构不是 FGameplayAbilityConfig 或其派生类，无法导入。"));
		return;
	}

	FString BasePath = GetGameplayAbilityBasePath(Settings);

#if WITH_EDITOR
	if (bClearGameplayAbilityFolderFirst)
	{
		CleanupGameplayAbilityFolder(BasePath, DataTable);
	}
#endif

	// 遍历每一行并创建/更新 GA
	int32 SuccessCount = 0;
	int32 FailCount = 0;

	for (const TPair<FName, uint8*>& RowPair : DataTable->GetRowMap())
	{
		const FName RowName = RowPair.Key;
		const uint8* RowData = RowPair.Value;

		if (!RowData)
		{
			continue;
		}

		const FGameplayAbilityConfig* Config = reinterpret_cast<const FGameplayAbilityConfig*>(RowData);

		FString RowAssetName = RowName.ToString();
		if (!RowAssetName.Contains(TEXT("GA_")))
		{
			RowAssetName = TEXT("GA_") + RowAssetName;
		}

		const FString GAPath = FString::Printf(TEXT("%s/%s"), *BasePath, *RowAssetName);

		bool bOK = false;
		UGameplayAbility* GA = CreateOrImportGameplayAbility(GAPath, *Config, bOK);

		if (bOK && GA)
		{
			UE_LOG(LogAbilityEditor, Log, TEXT("[AbilityEditorHelper] 成功创建/更新 GameplayAbility：%s"), *GAPath);
			++SuccessCount;
		}
		else
		{
			UE_LOG(LogAbilityEditor, Error, TEXT("[AbilityEditorHelper] 创建/更新失败：%s"), *GAPath);
			++FailCount;
		}
	}

	UE_LOG(LogAbilityEditor, Log, TEXT("[AbilityEditorHelper] GA 导入完成：成功 %d 个，失败 %d 个"), SuccessCount, FailCount);
}

bool UAbilityEditorHelperLibrary::ImportAndUpdateGameplayAbilitiesFromJson(
	const FString& JsonFileName,
	bool bClearGameplayAbilityFolderFirst,
	TArray<FName>& OutUpdatedRowNames)
{
	OutUpdatedRowNames.Reset();

	const UAbilityEditorHelperSettings* Settings = nullptr;
	UDataTable* DataTable = nullptr;
	if (!GetGASettingsAndDataTable(Settings, DataTable))
	{
		UE_LOG(LogAbilityEditor, Error, TEXT("无法获取 Settings 或 DataTable"));
		return false;
	}

	// 校验行结构
	UScriptStruct* RowStruct = const_cast<UScriptStruct*>(DataTable->GetRowStruct());
	if (!RowStruct || !RowStruct->IsChildOf(FGameplayAbilityConfig::StaticStruct()))
	{
		UE_LOG(LogAbilityEditor, Error, TEXT("DataTable 行结构不是 FGameplayAbilityConfig 或其派生类"));
		return false;
	}

	const int32 StructSize = RowStruct->GetStructureSize();

	// 构建 JSON 文件完整路径
	if (Settings->JsonPath.IsEmpty())
	{
		UE_LOG(LogAbilityEditor, Error, TEXT("UAbilityEditorHelperSettings 的 JsonPath 未配置"));
		return false;
	}
	const FString JsonFilePath = FPaths::Combine(Settings->JsonPath, JsonFileName);

	if (!FPaths::FileExists(JsonFilePath))
	{
		UE_LOG(LogAbilityEditor, Error, TEXT("JSON 文件不存在：%s"), *JsonFilePath);
		return false;
	}

	// 读取 JSON 文件内容
	FString JsonContent;
	if (!FFileHelper::LoadFileToString(JsonContent, *JsonFilePath))
	{
		UE_LOG(LogAbilityEditor, Error, TEXT("无法读取 JSON 文件：%s"), *JsonFilePath);
		return false;
	}

	// 解析 JSON 为数组
	TArray<TSharedPtr<FJsonValue>> JsonArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(Reader, JsonArray))
	{
		UE_LOG(LogAbilityEditor, Error, TEXT("JSON 解析失败，格式不正确"));
		return false;
	}

	// 缓存现有 DataTable 数据的 JSON 表示
	TMap<FName, FString> ExistingJsonMap;
	for (const TPair<FName, uint8*>& RowPair : DataTable->GetRowMap())
	{
		if (RowPair.Value)
		{
			FString JsonStr = SerializeStructToJsonString(RowStruct, RowPair.Value);
			ExistingJsonMap.Add(RowPair.Key, JsonStr);
		}
	}

	// 解析新配置
	TMap<FName, TSharedPtr<uint8, ESPMode::ThreadSafe>> NewConfigsMemory;
	TMap<FName, FString> NewJsonMap;

	for (const TSharedPtr<FJsonValue>& JsonValue : JsonArray)
	{
		if (!JsonValue.IsValid() || JsonValue->Type != EJson::Object)
		{
			continue;
		}

		const TSharedPtr<FJsonObject>& JsonObject = JsonValue->AsObject();
		if (!JsonObject.IsValid())
		{
			continue;
		}

		FString RowNameStr;
		if (!JsonObject->TryGetStringField(TEXT("Name"), RowNameStr) || RowNameStr.IsEmpty())
		{
			UE_LOG(LogAbilityEditor, Warning, TEXT("JSON 条目缺少 Name 字段，已跳过"));
			continue;
		}

		TSharedPtr<uint8, ESPMode::ThreadSafe> ConfigMemory(
			static_cast<uint8*>(FMemory::Malloc(StructSize)),
			[](uint8* Ptr) { FMemory::Free(Ptr); }
		);
		RowStruct->InitializeStruct(ConfigMemory.Get());

		if (!FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), RowStruct, ConfigMemory.Get()))
		{
			UE_LOG(LogAbilityEditor, Warning, TEXT("无法反序列化行 %s，已跳过"), *RowNameStr);
			RowStruct->DestroyStruct(ConfigMemory.Get());
			continue;
		}

		FName RowName(*RowNameStr);
		NewConfigsMemory.Add(RowName, ConfigMemory);

		FString NewJsonStr = SerializeStructToJsonString(RowStruct, ConfigMemory.Get());
		NewJsonMap.Add(RowName, NewJsonStr);

		const FString* ExistingJson = ExistingJsonMap.Find(RowName);
		if (!ExistingJson || !ExistingJson->Equals(NewJsonStr))
		{
			OutUpdatedRowNames.Add(RowName);
			UE_LOG(LogAbilityEditor, Log, TEXT("检测到变化的行：%s"), *RowNameStr);
		}
	}

	if (OutUpdatedRowNames.Num() == 0)
	{
		UE_LOG(LogAbilityEditor, Log, TEXT("未检测到任何数据变化，无需更新"));
		return true;
	}

	UE_LOG(LogAbilityEditor, Log, TEXT("共检测到 %d 行数据变化"), OutUpdatedRowNames.Num());

#if WITH_EDITOR
	// 更新 DataTable
	for (const FName& RowName : OutUpdatedRowNames)
	{
		TSharedPtr<uint8, ESPMode::ThreadSafe>* NewConfigMemory = NewConfigsMemory.Find(RowName);
		if (!NewConfigMemory || !NewConfigMemory->IsValid())
		{
			continue;
		}

		uint8* ExistingRowData = DataTable->FindRowUnchecked(RowName);
		if (ExistingRowData)
		{
			RowStruct->CopyScriptStruct(ExistingRowData, NewConfigMemory->Get());
		}
		else
		{
			DataTable->AddRow(RowName, *reinterpret_cast<FTableRowBase*>(NewConfigMemory->Get()));
		}
	}

	DataTable->MarkPackageDirty();

	FString BasePath = GetGameplayAbilityBasePath(Settings);

	if (bClearGameplayAbilityFolderFirst)
	{
		CleanupGameplayAbilityFolder(BasePath, DataTable);
	}

	// 创建/更新 GA
	int32 SuccessCount = 0;
	int32 FailCount = 0;

	for (const FName& RowName : OutUpdatedRowNames)
	{
		uint8* ConfigData = DataTable->FindRowUnchecked(RowName);
		if (!ConfigData)
		{
			continue;
		}
		const FGameplayAbilityConfig* Config = reinterpret_cast<const FGameplayAbilityConfig*>(ConfigData);

		FString RowAssetName = RowName.ToString();
		if (!RowAssetName.Contains(TEXT("GA_")))
		{
			RowAssetName = TEXT("GA_") + RowAssetName;
		}
		const FString GAPath = FString::Printf(TEXT("%s/%s"), *BasePath, *RowAssetName);

		bool bOK = false;
		UGameplayAbility* GA = CreateOrImportGameplayAbility(GAPath, *Config, bOK);
		if (bOK && GA)
		{
			UE_LOG(LogAbilityEditor, Log, TEXT("[AbilityEditorHelper] 成功创建/更新 GameplayAbility：%s"), *GAPath);
			++SuccessCount;
		}
		else
		{
			UE_LOG(LogAbilityEditor, Error, TEXT("[AbilityEditorHelper] 创建/更新失败：%s"), *GAPath);
			++FailCount;
		}
	}

	UE_LOG(LogAbilityEditor, Log, TEXT("[AbilityEditorHelper] GA 增量更新完成：成功 %d 个，失败 %d 个"), SuccessCount, FailCount);

	return FailCount == 0;
#else
	return false;
#endif
}

// ===========================================
// EditorUtilityWidget 相关
// ===========================================

bool UAbilityEditorHelperLibrary::OpenEditorUtilityWidget()
{
#if WITH_EDITOR
	const UAbilityEditorHelperSettings* Settings = GetAbilityEditorHelperSettings();
	if (!Settings)
	{
		UE_LOG(LogAbilityEditor, Warning, TEXT("[AbilityEditorHelper] 无法获取 AbilityEditorHelperSettings"));
		return false;
	}

	UEditorUtilityWidgetBlueprint* WidgetBP = Settings->EditorUtilityWidgetBlueprint.LoadSynchronous();
	if (!WidgetBP)
	{
		UE_LOG(LogAbilityEditor, Warning, TEXT("[AbilityEditorHelper] EditorUtilityWidgetBlueprint 未配置或加载失败，请在 Project Settings → Ability Editor Helper → EditorWidget 中设置"));
		return false;
	}

	UEditorUtilitySubsystem* Subsystem = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
	if (!Subsystem)
	{
		UE_LOG(LogAbilityEditor, Warning, TEXT("[AbilityEditorHelper] 无法获取 EditorUtilitySubsystem"));
		return false;
	}

	Subsystem->SpawnAndRegisterTab(WidgetBP);
	return true;
#else
	return false;
#endif
}
