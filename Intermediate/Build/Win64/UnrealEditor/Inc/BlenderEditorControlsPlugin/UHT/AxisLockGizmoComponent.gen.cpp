// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "UI/AxisLockGizmoComponent.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

void EmptyLinkFunctionForGeneratedCodeAxisLockGizmoComponent() {}

// ********** Begin Cross Module References ********************************************************
BLENDEREDITORCONTROLSPLUGIN_API UClass* Z_Construct_UClass_UAxisLockGizmoComponent();
BLENDEREDITORCONTROLSPLUGIN_API UClass* Z_Construct_UClass_UAxisLockGizmoComponent_NoRegister();
COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FLinearColor();
COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FVector();
ENGINE_API UClass* Z_Construct_UClass_UMaterialInstanceDynamic_NoRegister();
ENGINE_API UClass* Z_Construct_UClass_UMaterialInterface_NoRegister();
ENGINE_API UClass* Z_Construct_UClass_UPrimitiveComponent();
UPackage* Z_Construct_UPackage__Script_BlenderEditorControlsPlugin();
// ********** End Cross Module References **********************************************************

// ********** Begin Class UAxisLockGizmoComponent Function SetAxisColor ****************************
struct Z_Construct_UFunction_UAxisLockGizmoComponent_SetAxisColor_Statics
{
	struct AxisLockGizmoComponent_eventSetAxisColor_Parms
	{
		FLinearColor InColor;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "// default\n" },
#endif
		{ "ModuleRelativePath", "Public/UI/AxisLockGizmoComponent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "default" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_InColor_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FStructPropertyParams NewProp_InColor;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FStructPropertyParams Z_Construct_UFunction_UAxisLockGizmoComponent_SetAxisColor_Statics::NewProp_InColor = { "InColor", nullptr, (EPropertyFlags)0x0010000008000182, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(AxisLockGizmoComponent_eventSetAxisColor_Parms, InColor), Z_Construct_UScriptStruct_FLinearColor, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_InColor_MetaData), NewProp_InColor_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UAxisLockGizmoComponent_SetAxisColor_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UAxisLockGizmoComponent_SetAxisColor_Statics::NewProp_InColor,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UAxisLockGizmoComponent_SetAxisColor_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UAxisLockGizmoComponent_SetAxisColor_Statics::FuncParams = { { (UObject*(*)())Z_Construct_UClass_UAxisLockGizmoComponent, nullptr, "SetAxisColor", Z_Construct_UFunction_UAxisLockGizmoComponent_SetAxisColor_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UAxisLockGizmoComponent_SetAxisColor_Statics::PropPointers), sizeof(Z_Construct_UFunction_UAxisLockGizmoComponent_SetAxisColor_Statics::AxisLockGizmoComponent_eventSetAxisColor_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00C20401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UAxisLockGizmoComponent_SetAxisColor_Statics::Function_MetaDataParams), Z_Construct_UFunction_UAxisLockGizmoComponent_SetAxisColor_Statics::Function_MetaDataParams)},  };
static_assert(sizeof(Z_Construct_UFunction_UAxisLockGizmoComponent_SetAxisColor_Statics::AxisLockGizmoComponent_eventSetAxisColor_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UAxisLockGizmoComponent_SetAxisColor()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UAxisLockGizmoComponent_SetAxisColor_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UAxisLockGizmoComponent::execSetAxisColor)
{
	P_GET_STRUCT_REF(FLinearColor,Z_Param_Out_InColor);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->SetAxisColor(Z_Param_Out_InColor);
	P_NATIVE_END;
}
// ********** End Class UAxisLockGizmoComponent Function SetAxisColor ******************************

// ********** Begin Class UAxisLockGizmoComponent **************************************************
void UAxisLockGizmoComponent::StaticRegisterNativesUAxisLockGizmoComponent()
{
	UClass* Class = UAxisLockGizmoComponent::StaticClass();
	static const FNameNativePtrPair Funcs[] = {
		{ "SetAxisColor", &UAxisLockGizmoComponent::execSetAxisColor },
	};
	FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
}
FClassRegistrationInfo Z_Registration_Info_UClass_UAxisLockGizmoComponent;
UClass* UAxisLockGizmoComponent::GetPrivateStaticClass()
{
	using TClass = UAxisLockGizmoComponent;
	if (!Z_Registration_Info_UClass_UAxisLockGizmoComponent.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			StaticPackage(),
			TEXT("AxisLockGizmoComponent"),
			Z_Registration_Info_UClass_UAxisLockGizmoComponent.InnerSingleton,
			StaticRegisterNativesUAxisLockGizmoComponent,
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
	return Z_Registration_Info_UClass_UAxisLockGizmoComponent.InnerSingleton;
}
UClass* Z_Construct_UClass_UAxisLockGizmoComponent_NoRegister()
{
	return UAxisLockGizmoComponent::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UAxisLockGizmoComponent_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintSpawnableComponent", "" },
		{ "ClassGroupNames", "Editor" },
		{ "HideCategories", "Mobility VirtualTexture Trigger" },
		{ "IncludePath", "UI/AxisLockGizmoComponent.h" },
		{ "ModuleRelativePath", "Public/UI/AxisLockGizmoComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Origin_MetaData[] = {
		{ "Category", "Gizmo" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// editable gizmo inputs:\n" },
#endif
		{ "ModuleRelativePath", "Public/UI/AxisLockGizmoComponent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "editable gizmo inputs:" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_AxisDir_MetaData[] = {
		{ "Category", "Gizmo" },
		{ "ModuleRelativePath", "Public/UI/AxisLockGizmoComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_LineLength_MetaData[] = {
		{ "Category", "Gizmo" },
		{ "ModuleRelativePath", "Public/UI/AxisLockGizmoComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Color_MetaData[] = {
		{ "Category", "Gizmo" },
		{ "ModuleRelativePath", "Public/UI/AxisLockGizmoComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ThicknessPx_MetaData[] = {
		{ "Category", "Gizmo" },
		{ "ModuleRelativePath", "Public/UI/AxisLockGizmoComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_bDashed_MetaData[] = {
		{ "Category", "Gizmo" },
		{ "ModuleRelativePath", "Public/UI/AxisLockGizmoComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_AxisMaterial_MetaData[] = {
		{ "ModuleRelativePath", "Public/UI/AxisLockGizmoComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_AxisMID_MetaData[] = {
		{ "ModuleRelativePath", "Public/UI/AxisLockGizmoComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_AxisColor_MetaData[] = {
		{ "Category", "Gizmo" },
		{ "ModuleRelativePath", "Public/UI/AxisLockGizmoComponent.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FStructPropertyParams NewProp_Origin;
	static const UECodeGen_Private::FStructPropertyParams NewProp_AxisDir;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_LineLength;
	static const UECodeGen_Private::FStructPropertyParams NewProp_Color;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_ThicknessPx;
	static void NewProp_bDashed_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bDashed;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_AxisMaterial;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_AxisMID;
	static const UECodeGen_Private::FStructPropertyParams NewProp_AxisColor;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_UAxisLockGizmoComponent_SetAxisColor, "SetAxisColor" }, // 1343920145
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UAxisLockGizmoComponent>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UAxisLockGizmoComponent_Statics::NewProp_Origin = { "Origin", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UAxisLockGizmoComponent, Origin), Z_Construct_UScriptStruct_FVector, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Origin_MetaData), NewProp_Origin_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UAxisLockGizmoComponent_Statics::NewProp_AxisDir = { "AxisDir", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UAxisLockGizmoComponent, AxisDir), Z_Construct_UScriptStruct_FVector, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_AxisDir_MetaData), NewProp_AxisDir_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UAxisLockGizmoComponent_Statics::NewProp_LineLength = { "LineLength", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UAxisLockGizmoComponent, LineLength), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_LineLength_MetaData), NewProp_LineLength_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UAxisLockGizmoComponent_Statics::NewProp_Color = { "Color", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UAxisLockGizmoComponent, Color), Z_Construct_UScriptStruct_FLinearColor, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Color_MetaData), NewProp_Color_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UAxisLockGizmoComponent_Statics::NewProp_ThicknessPx = { "ThicknessPx", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UAxisLockGizmoComponent, ThicknessPx), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ThicknessPx_MetaData), NewProp_ThicknessPx_MetaData) };
void Z_Construct_UClass_UAxisLockGizmoComponent_Statics::NewProp_bDashed_SetBit(void* Obj)
{
	((UAxisLockGizmoComponent*)Obj)->bDashed = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_UAxisLockGizmoComponent_Statics::NewProp_bDashed = { "bDashed", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(UAxisLockGizmoComponent), &Z_Construct_UClass_UAxisLockGizmoComponent_Statics::NewProp_bDashed_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_bDashed_MetaData), NewProp_bDashed_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UAxisLockGizmoComponent_Statics::NewProp_AxisMaterial = { "AxisMaterial", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UAxisLockGizmoComponent, AxisMaterial), Z_Construct_UClass_UMaterialInterface_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_AxisMaterial_MetaData), NewProp_AxisMaterial_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UAxisLockGizmoComponent_Statics::NewProp_AxisMID = { "AxisMID", nullptr, (EPropertyFlags)0x0010000000002000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UAxisLockGizmoComponent, AxisMID), Z_Construct_UClass_UMaterialInstanceDynamic_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_AxisMID_MetaData), NewProp_AxisMID_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UAxisLockGizmoComponent_Statics::NewProp_AxisColor = { "AxisColor", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UAxisLockGizmoComponent, AxisColor), Z_Construct_UScriptStruct_FLinearColor, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_AxisColor_MetaData), NewProp_AxisColor_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UAxisLockGizmoComponent_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UAxisLockGizmoComponent_Statics::NewProp_Origin,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UAxisLockGizmoComponent_Statics::NewProp_AxisDir,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UAxisLockGizmoComponent_Statics::NewProp_LineLength,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UAxisLockGizmoComponent_Statics::NewProp_Color,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UAxisLockGizmoComponent_Statics::NewProp_ThicknessPx,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UAxisLockGizmoComponent_Statics::NewProp_bDashed,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UAxisLockGizmoComponent_Statics::NewProp_AxisMaterial,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UAxisLockGizmoComponent_Statics::NewProp_AxisMID,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UAxisLockGizmoComponent_Statics::NewProp_AxisColor,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UAxisLockGizmoComponent_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UAxisLockGizmoComponent_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UPrimitiveComponent,
	(UObject* (*)())Z_Construct_UPackage__Script_BlenderEditorControlsPlugin,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UAxisLockGizmoComponent_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UAxisLockGizmoComponent_Statics::ClassParams = {
	&UAxisLockGizmoComponent::StaticClass,
	"Engine",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	Z_Construct_UClass_UAxisLockGizmoComponent_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	UE_ARRAY_COUNT(Z_Construct_UClass_UAxisLockGizmoComponent_Statics::PropPointers),
	0,
	0x00A000A4u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UAxisLockGizmoComponent_Statics::Class_MetaDataParams), Z_Construct_UClass_UAxisLockGizmoComponent_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UAxisLockGizmoComponent()
{
	if (!Z_Registration_Info_UClass_UAxisLockGizmoComponent.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UAxisLockGizmoComponent.OuterSingleton, Z_Construct_UClass_UAxisLockGizmoComponent_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UAxisLockGizmoComponent.OuterSingleton;
}
UAxisLockGizmoComponent::UAxisLockGizmoComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR(UAxisLockGizmoComponent);
UAxisLockGizmoComponent::~UAxisLockGizmoComponent() {}
// ********** End Class UAxisLockGizmoComponent ****************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_BlenderControlsPlug_Plugins_BlenderEditorControlsPlugin_Source_BlenderEditorControlsPlugin_Public_UI_AxisLockGizmoComponent_h__Script_BlenderEditorControlsPlugin_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UAxisLockGizmoComponent, UAxisLockGizmoComponent::StaticClass, TEXT("UAxisLockGizmoComponent"), &Z_Registration_Info_UClass_UAxisLockGizmoComponent, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UAxisLockGizmoComponent), 376323676U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_BlenderControlsPlug_Plugins_BlenderEditorControlsPlugin_Source_BlenderEditorControlsPlugin_Public_UI_AxisLockGizmoComponent_h__Script_BlenderEditorControlsPlugin_2932141046(TEXT("/Script/BlenderEditorControlsPlugin"),
	Z_CompiledInDeferFile_FID_BlenderControlsPlug_Plugins_BlenderEditorControlsPlugin_Source_BlenderEditorControlsPlugin_Public_UI_AxisLockGizmoComponent_h__Script_BlenderEditorControlsPlugin_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_BlenderControlsPlug_Plugins_BlenderEditorControlsPlugin_Source_BlenderEditorControlsPlugin_Public_UI_AxisLockGizmoComponent_h__Script_BlenderEditorControlsPlugin_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
