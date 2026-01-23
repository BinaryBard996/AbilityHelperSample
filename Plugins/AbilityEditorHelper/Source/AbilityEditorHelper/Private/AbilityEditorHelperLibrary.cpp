// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilityEditorHelperLibrary.h"
#include "AbilityEditorHelperSettings.h"
#include "Misc/PackageName.h"
#include "Runtime/Launch/Resources/Version.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "Kismet/DataTableFunctionLibrary.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
// GE Components (5.3+)
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"

// Added for Schema export / reflection / file ops
#include "UObject/UnrealType.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"
#include "Interfaces/IPluginManager.h"
#include "JsonObjectConverter.h"
#include "Misc/SecureHash.h"

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

		FAssetRegistryModule::AssetCreated(GE);
		Package->MarkPackageDirty();
	}

	// 将配置写入GE（基础版）
	if (GE)
	{
		// 基础
		GE->DurationPolicy = Config.DurationType;
		GE->DurationMagnitude = FScalableFloat(Config.DurationMagnitude);
		GE->Period = Config.Period;

		// 堆叠
		GE->StackingType = Config.StackingType;
		GE->StackLimitCount = Config.StackLimitCount;
		GE->StackDurationRefreshPolicy = Config.StackDurationRefreshPolicy;
		GE->StackPeriodResetPolicy = Config.StackPeriodResetPolicy;

		// Tags：5.3+ 使用 GE 组件写入；更早版本回退到旧容器
#if (ENGINE_MAJOR_VERSION > 5) || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3)
		// 目标（Granted）Tags 组件
		if (!Config.GrantedTags.IsEmpty())
		{
			UTargetTagsGameplayEffectComponent& TargetTagsComp = GE->FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
			FInheritedTagContainer& TargetTags = const_cast<FInheritedTagContainer&>(TargetTagsComp.GetConfiguredTargetTagChanges());
			TargetTags.Added = Config.GrantedTags;
		}

		// 资产（Owned/Asset）Tags 组件
		if (!Config.AssetTags.IsEmpty())
		{
			UAssetTagsGameplayEffectComponent& AssetTagsComp = GE->FindOrAddComponent<UAssetTagsGameplayEffectComponent>();
			FInheritedTagContainer& AssetTags =	const_cast<FInheritedTagContainer&>(AssetTagsComp.GetConfiguredAssetTagChanges());
			AssetTags.Added = Config.AssetTags;
		}
#else
		// 旧版（无 GE 组件）：写入继承容器
		GE->InheritableOwnedTagsContainer.Added   = Config.AssetTags;
		GE->InheritableGrantedTagsContainer.Added = Config.GrantedTags;
#endif

		// Modifiers（最简单版：仅支持ScalableFloat）
		GE->Modifiers.Reset();
		for (const FGEModifierConfig& Mod : Config.Modifiers)
		{
			if (!Mod.Attribute.IsValid())
			{
				continue;
			}

			FGameplayModifierInfo Info;
			Info.Attribute = Mod.Attribute;
			Info.ModifierOp = Mod.ModifierOp;

			FScalableFloat ScalableFloatMagnitude(Mod.Magnitude);
			Info.ModifierMagnitude = FGameplayEffectModifierMagnitude(ScalableFloatMagnitude);

			GE->Modifiers.Add(Info);
		}

		// 标记脏包
		GE->MarkPackageDirty();
	}

	bOutSuccess = GE != nullptr;
	return GE;
#else
	// 运行时环境不创建新资产，仅在存在时返回
	bOutSuccess = (GE != nullptr);
	return GE;
#endif
}

void UAbilityEditorHelperLibrary::CreateOrUpdateGameplayEffectsFromSettings()
{
	const UAbilityEditorHelperSettings* Settings = GetDefault<UAbilityEditorHelperSettings>();
	if (!Settings)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AbilityEditorHelper] Settings 未找到。"));
		return;
	}

	// 加载 DataTable（优先使用已加载对象，否则同步加载）
	UDataTable* DataTable = nullptr;
	if (Settings->GameplayEffectDataTable.IsValid())
	{
		DataTable = Settings->GameplayEffectDataTable.Get();
	}
	if (!DataTable)
	{
		DataTable = Settings->GameplayEffectDataTable.LoadSynchronous();
	}
	if (!DataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AbilityEditorHelper] GameplayEffectDataTable 未设置或加载失败。"));
		return;
	}

	// 校验行结构
	if (!DataTable->GetRowStruct() || DataTable->GetRowStruct() != FGameplayEffectConfig::StaticStruct())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AbilityEditorHelper] DataTable 行结构不是 FGameplayEffectConfig，无法导入。"));
		return;
	}

	// 规范化基础路径
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
	FString SchemaJsonFilePath = GetDefault<UAbilityEditorHelperSettings>()->JsonPath;

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
	SchemaJsonFilePath = FPaths::Combine(SchemaDir, FileName);

	return WriteStructSchemaToJson(StructType, SchemaJsonFilePath, OutError);
}

bool UAbilityEditorHelperLibrary::ImportDataTableFromJsonFile(UDataTable* TargetDataTable, const FString& JsonFilePath, bool bClearBeforeImport, int32& OutImportedRowCount, FString& OutError)
{
	OutError.Reset();
	OutImportedRowCount = 0;

	if (!TargetDataTable)
	{
		OutError = TEXT("TargetDataTable 为空");
		return false;
	}

	if (JsonFilePath.IsEmpty())
	{
		OutError = TEXT("JsonFilePath 为空");
		return false;
	}

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
