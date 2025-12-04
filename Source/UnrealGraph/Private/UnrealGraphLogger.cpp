// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnrealGraphLogger.h"
#include "EdGraph/EdGraphNode.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "UObject/PropertyAccessUtil.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "Math/Vector2D.h"
#include "Misc/VarArgs.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "Math/Vector2D.h"

bool FUnrealGraphLogger::bIsInitialized = false;
FString FUnrealGraphLogger::LogFilePath;
TArray<FString> FUnrealGraphLogger::LogBuffer;

void FUnrealGraphLogger::Initialize(const FString& LogFileName)
{
	if (bIsInitialized)
	{
		Shutdown();
	}

	// Create log file path in project logs directory
	FString LogDir = FPaths::ProjectLogDir();
	FString Timestamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
	LogFilePath = LogDir / FString::Printf(TEXT("%s_%s.log"), *LogFileName, *Timestamp);

	bIsInitialized = true;

	// Write header
	FString Header = TEXT("================================================================================\n");
	Header += TEXT("UnrealGraph Plugin - Debug Log\n");
	Header += TEXT("================================================================================\n");
	Header += FString::Printf(TEXT("Timestamp: %s\n"), *FDateTime::Now().ToString());
	Header += FString::Printf(TEXT("Log File: %s\n"), *LogFilePath);
	Header += TEXT("================================================================================\n\n");

	LogBuffer.Add(Header);
	Log(TEXT("Logger initialized"));
}

void FUnrealGraphLogger::Shutdown()
{
	if (!bIsInitialized)
	{
		return;
	}

	FlushLogBuffer();
	
	LogBuffer.Empty();
	bIsInitialized = false;
}

void FUnrealGraphLogger::Log(const FString& Message)
{
	if (!bIsInitialized)
	{
		// Auto-initialize if not initialized
		Initialize();
	}

	FString Timestamp = FDateTime::Now().ToString(TEXT("[%H:%M:%S]"));
	FString LogLine = FString::Printf(TEXT("%s %s\n"), *Timestamp, *Message);
	
	LogBuffer.Add(LogLine);

	// Flush periodically or immediately for important messages
	if (LogBuffer.Num() > 50)
	{
		FlushLogBuffer();
	}
}

void FUnrealGraphLogger::LogFormatted(const TCHAR* Format, ...)
{
	if (!bIsInitialized)
	{
		Initialize();
	}

	va_list Args;
	va_start(Args, Format);
	
	TCHAR Buffer[2048];
	const int32 BufferSize = sizeof(Buffer) / sizeof(Buffer[0]);
	FCString::GetVarArgs(Buffer, BufferSize, Format, Args);
	
	va_end(Args);

	Log(FString(Buffer));
}

void FUnrealGraphLogger::LogSection(const FString& SectionName)
{
	FString SectionHeader = TEXT("\n");
	SectionHeader += TEXT("────────────────────────────────────────────────────────────────────────\n");
	SectionHeader += FString::Printf(TEXT("  %s\n"), *SectionName);
	SectionHeader += TEXT("────────────────────────────────────────────────────────────────────────\n");
	
	LogBuffer.Add(SectionHeader);
	
	if (LogBuffer.Num() > 0)
	{
		FlushLogBuffer();
	}
}

void FUnrealGraphLogger::LogNodeDetails(UEdGraphNode* Node)
{
	if (!Node)
	{
		Log(TEXT("  Node: NULL"));
		return;
	}

	LogSection(FString::Printf(TEXT("Node Details: %s"), *Node->GetName()));
	
	LogFormatted(TEXT("  Node Class: %s"), *Node->GetClass()->GetName());
	LogFormatted(TEXT("  Node Type: %s"), *Node->GetClass()->GetPathName());
	LogFormatted(TEXT("  Node Title: %s"), *Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
	LogFormatted(TEXT("  Number of Pins: %d"), Node->Pins.Num());
	
	// Log all properties
	Log(TEXT("  Properties:"));
	for (TFieldIterator<FProperty> PropIt(Node->GetClass()); PropIt; ++PropIt)
	{
		FProperty* Prop = *PropIt;
		if (Prop)
		{
			FString PropName = Prop->GetName();
			FString PropType = Prop->GetClass()->GetName();
			
			// Try to get property value as string
			FString PropValue = TEXT("<unable to read>");
			
			if (FStructProperty* StructProp = CastField<FStructProperty>(Prop))
			{
				if (StructProp->Struct)
				{
					PropType = FString::Printf(TEXT("%s (%s)"), *PropType, *StructProp->Struct->GetName());
					
					// Check if it's a Vector2D
					if (StructProp->Struct->GetFName() == NAME_Vector2D)
					{
						const FVector2D* VecPtr = StructProp->ContainerPtrToValuePtr<FVector2D>(Node);
						if (VecPtr)
						{
							PropValue = FString::Printf(TEXT("(%.2f, %.2f)"), VecPtr->X, VecPtr->Y);
						}
					}
				}
			}
			else if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
			{
				int32 Value = IntProp->GetPropertyValue_InContainer(Node);
				PropValue = FString::Printf(TEXT("%d"), Value);
			}
			else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
			{
				float Value = FloatProp->GetPropertyValue_InContainer(Node);
				PropValue = FString::Printf(TEXT("%.2f"), Value);
			}
			else if (FNameProperty* NameProp = CastField<FNameProperty>(Prop))
			{
				FName Value = NameProp->GetPropertyValue_InContainer(Node);
				PropValue = Value.ToString();
			}
			else if (FStrProperty* StrProp = CastField<FStrProperty>(Prop))
			{
				FString Value = StrProp->GetPropertyValue_InContainer(Node);
				PropValue = Value;
			}
			
			LogFormatted(TEXT("    - %s (%s): %s"), *PropName, *PropType, *PropValue);
		}
	}
	
	Log(TEXT(""));
}

void FUnrealGraphLogger::LogProperty(const FString& PropertyName, const FString& PropertyValue)
{
	LogFormatted(TEXT("  %s: %s"), *PropertyName, *PropertyValue);
}

void FUnrealGraphLogger::FlushLogBuffer()
{
	if (LogBuffer.Num() == 0 || LogFilePath.IsEmpty())
	{
		return;
	}

	FString LogContent;
	for (const FString& Line : LogBuffer)
	{
		LogContent += Line;
	}

	// Append to file (creates if doesn't exist)
	FFileHelper::SaveStringToFile(LogContent, *LogFilePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), EFileWrite::FILEWRITE_Append);
	
	LogBuffer.Empty();
}

