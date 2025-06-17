// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Settings/BlenderEditorControlsPluginSettings.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

void EmptyLinkFunctionForGeneratedCodeBlenderEditorControlsPluginSettings() {}

// ********** Begin Cross Module References ********************************************************
BLENDEREDITORCONTROLSPLUGIN_API UClass* Z_Construct_UClass_UBlenderEditorControlsPluginSettings();
BLENDEREDITORCONTROLSPLUGIN_API UClass* Z_Construct_UClass_UBlenderEditorControlsPluginSettings_NoRegister();
DEVELOPERSETTINGS_API UClass* Z_Construct_UClass_UDeveloperSettings();
UPackage* Z_Construct_UPackage__Script_BlenderEditorControlsPlugin();
// ********** End Cross Module References **********************************************************

// ********** Begin Class UBlenderEditorControlsPluginSettings *************************************
void UBlenderEditorControlsPluginSettings::StaticRegisterNativesUBlenderEditorControlsPluginSettings()
{
}
FClassRegistrationInfo Z_Registration_Info_UClass_UBlenderEditorControlsPluginSettings;
UClass* UBlenderEditorControlsPluginSettings::GetPrivateStaticClass()
{
	using TClass = UBlenderEditorControlsPluginSettings;
	if (!Z_Registration_Info_UClass_UBlenderEditorControlsPluginSettings.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			StaticPackage(),
			TEXT("BlenderEditorControlsPluginSettings"),
			Z_Registration_Info_UClass_UBlenderEditorControlsPluginSettings.InnerSingleton,
			StaticRegisterNativesUBlenderEditorControlsPluginSettings,
			sizeof(TClass),
			alignof(TClass),
			TClass::StaticClassFlags,
			TClass::StaticClassCastFlags(),
			TClass::StaticConfigName(),
			(UClass::ClassConstructorType)InternalConstructor<TClass>,
			(UClass::ClassVTableHelperCtorCallerType)InternalVTableHelperCtorCaller<TClass>,
			UOBJECT_CPPCLASS_STATICFUNCTIONS_FORCLASS(TClass),
			&TClass::Super::StaticClass,
			&TClass::WithinClass::StaticClass
		);
	}
	return Z_Registration_Info_UClass_UBlenderEditorControlsPluginSettings.InnerSingleton;
}
UClass* Z_Construct_UClass_UBlenderEditorControlsPluginSettings_NoRegister()
{
	return UBlenderEditorControlsPluginSettings::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UBlenderEditorControlsPluginSettings_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * User-editable settings (shows up under\n * Editor Preferences \xe2\x86\x92 Plugins \xe2\x86\x92 Blender Controls)\n */" },
#endif
		{ "IncludePath", "Settings/BlenderEditorControlsPluginSettings.h" },
		{ "ModuleRelativePath", "Public/Settings/BlenderEditorControlsPluginSettings.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "User-editable settings (shows up under\nEditor Preferences \xe2\x86\x92 Plugins \xe2\x86\x92 Blender Controls)" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_bEnableOnStartup_MetaData[] = {
		{ "Category", "General" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Plugin starts enabled each editor launch */" },
#endif
		{ "ModuleRelativePath", "Public/Settings/BlenderEditorControlsPluginSettings.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Plugin starts enabled each editor launch" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_PrecisionScalar_MetaData[] = {
		{ "Category", "Controls" },
		{ "ClampMax", "1.0" },
		{ "ClampMin", "0.01" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Slow-drag multiplier when Shift is held */" },
#endif
		{ "ModuleRelativePath", "Public/Settings/BlenderEditorControlsPluginSettings.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Slow-drag multiplier when Shift is held" },
#endif
		{ "UIMax", "0.5" },
		{ "UIMin", "0.01" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_SurfaceSnapOffset_MetaData[] = {
		{ "Category", "Snapping" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Default offset applied in surface-snap mode (Ctrl-drag) */" },
#endif
		{ "ModuleRelativePath", "Public/Settings/BlenderEditorControlsPluginSettings.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Default offset applied in surface-snap mode (Ctrl-drag)" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_SlatePriority_MetaData[] = {
		{ "Category", "Advanced" },
		{ "ClampMax", "100" },
		{ "ClampMin", "-100" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Priority passed to Slate when registering the input processor */" },
#endif
		{ "ModuleRelativePath", "Public/Settings/BlenderEditorControlsPluginSettings.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Priority passed to Slate when registering the input processor" },
#endif
	};
#endif // WITH_METADATA
	static void NewProp_bEnableOnStartup_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bEnableOnStartup;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_PrecisionScalar;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_SurfaceSnapOffset;
	static const UECodeGen_Private::FIntPropertyParams NewProp_SlatePriority;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UBlenderEditorControlsPluginSettings>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
void Z_Construct_UClass_UBlenderEditorControlsPluginSettings_Statics::NewProp_bEnableOnStartup_SetBit(void* Obj)
{
	((UBlenderEditorControlsPluginSettings*)Obj)->bEnableOnStartup = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_UBlenderEditorControlsPluginSettings_Statics::NewProp_bEnableOnStartup = { "bEnableOnStartup", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(UBlenderEditorControlsPluginSettings), &Z_Construct_UClass_UBlenderEditorControlsPluginSettings_Statics::NewProp_bEnableOnStartup_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_bEnableOnStartup_MetaData), NewProp_bEnableOnStartup_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UBlenderEditorControlsPluginSettings_Statics::NewProp_PrecisionScalar = { "PrecisionScalar", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UBlenderEditorControlsPluginSettings, PrecisionScalar), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_PrecisionScalar_MetaData), NewProp_PrecisionScalar_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UBlenderEditorControlsPluginSettings_Statics::NewProp_SurfaceSnapOffset = { "SurfaceSnapOffset", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UBlenderEditorControlsPluginSettings, SurfaceSnapOffset), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_SurfaceSnapOffset_MetaData), NewProp_SurfaceSnapOffset_MetaData) };
const UECodeGen_Private::FIntPropertyParams Z_Construct_UClass_UBlenderEditorControlsPluginSettings_Statics::NewProp_SlatePriority = { "SlatePriority", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UBlenderEditorControlsPluginSettings, SlatePriority), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_SlatePriority_MetaData), NewProp_SlatePriority_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UBlenderEditorControlsPluginSettings_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UBlenderEditorControlsPluginSettings_Statics::NewProp_bEnableOnStartup,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UBlenderEditorControlsPluginSettings_Statics::NewProp_PrecisionScalar,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UBlenderEditorControlsPluginSettings_Statics::NewProp_SurfaceSnapOffset,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UBlenderEditorControlsPluginSettings_Statics::NewProp_SlatePriority,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UBlenderEditorControlsPluginSettings_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UBlenderEditorControlsPluginSettings_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UDeveloperSettings,
	(UObject* (*)())Z_Construct_UPackage__Script_BlenderEditorControlsPlugin,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UBlenderEditorControlsPluginSettings_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UBlenderEditorControlsPluginSettings_Statics::ClassParams = {
	&UBlenderEditorControlsPluginSettings::StaticClass,
	"EditorPerProjectUserSettings",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UBlenderEditorControlsPluginSettings_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UBlenderEditorControlsPluginSettings_Statics::PropPointers),
	0,
	0x001000A6u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UBlenderEditorControlsPluginSettings_Statics::Class_MetaDataParams), Z_Construct_UClass_UBlenderEditorControlsPluginSettings_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UBlenderEditorControlsPluginSettings()
{
	if (!Z_Registration_Info_UClass_UBlenderEditorControlsPluginSettings.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UBlenderEditorControlsPluginSettings.OuterSingleton, Z_Construct_UClass_UBlenderEditorControlsPluginSettings_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UBlenderEditorControlsPluginSettings.OuterSingleton;
}
UBlenderEditorControlsPluginSettings::UBlenderEditorControlsPluginSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR(UBlenderEditorControlsPluginSettings);
UBlenderEditorControlsPluginSettings::~UBlenderEditorControlsPluginSettings() {}
// ********** End Class UBlenderEditorControlsPluginSettings ***************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_BlenderControlsPlug_Plugins_BlenderEditorControlsPlugin_Source_BlenderEditorControlsPlugin_Public_Settings_BlenderEditorControlsPluginSettings_h__Script_BlenderEditorControlsPlugin_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UBlenderEditorControlsPluginSettings, UBlenderEditorControlsPluginSettings::StaticClass, TEXT("UBlenderEditorControlsPluginSettings"), &Z_Registration_Info_UClass_UBlenderEditorControlsPluginSettings, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UBlenderEditorControlsPluginSettings), 2493140617U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_BlenderControlsPlug_Plugins_BlenderEditorControlsPlugin_Source_BlenderEditorControlsPlugin_Public_Settings_BlenderEditorControlsPluginSettings_h__Script_BlenderEditorControlsPlugin_2235767134(TEXT("/Script/BlenderEditorControlsPlugin"),
	Z_CompiledInDeferFile_FID_BlenderControlsPlug_Plugins_BlenderEditorControlsPlugin_Source_BlenderEditorControlsPlugin_Public_Settings_BlenderEditorControlsPluginSettings_h__Script_BlenderEditorControlsPlugin_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_BlenderControlsPlug_Plugins_BlenderEditorControlsPlugin_Source_BlenderEditorControlsPlugin_Public_Settings_BlenderEditorControlsPluginSettings_h__Script_BlenderEditorControlsPlugin_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
