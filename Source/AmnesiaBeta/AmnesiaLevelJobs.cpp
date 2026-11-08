#include "AmnesiaLevelJobs.h"

#if WITH_EDITOR

#include "AmnesiaBeta.h"
#include "AssetImportTask.h"
#include "AssetToolsModule.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Dom/JsonObject.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/StringOutputDevice.h"
#include "RHI.h"
#include "RenderTimer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/UnrealType.h"

FTSTicker::FDelegateHandle FAmnesiaLevelJobs::TickerHandle;
FTSTicker::FDelegateHandle FAmnesiaLevelJobs::CommandTickerHandle;

namespace
{
	FString JobFolder()
	{
		return FPaths::ProjectSavedDir() / TEXT("LevelJobs");
	}

	/** One running cmd.json job. The commands run at once, then frame times are sampled for a while */
	struct FCommandJob
	{
		bool bActive = false;
		double SampleStart = 0.0;
		double SampleEnd = 0.0;
		FString WorldName;
		TArray<FString> Outputs;
		TArray<float> Frame;
		TArray<float> Game;
		TArray<float> Render;
		TArray<float> Rhi;
		TArray<float> Gpu;
	};

	FCommandJob CommandJob;
	float CommandPollTime = 0.0f;

	TSharedPtr<FJsonObject> MakeStat(TArray<float> Values)
	{
		TSharedPtr<FJsonObject> Stat = MakeShared<FJsonObject>();

		if (Values.IsEmpty())
		{
			return Stat;
		}

		Values.Sort();

		double Sum = 0.0;

		for (const float Value : Values)
		{
			Sum += Value;
		}

		Stat->SetNumberField(TEXT("avg"), Sum / Values.Num());
		Stat->SetNumberField(TEXT("p95"), Values[FMath::Min(Values.Num() - 1, FMath::FloorToInt(Values.Num() * 0.95f))]);
		Stat->SetNumberField(TEXT("max"), Values.Last());

		return Stat;
	}

	/** Reads a three number array like loc, rot or scale */
	bool ReadVector(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, double& X, double& Y, double& Z)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values;

		if (Object->TryGetArrayField(Field, Values) && Values->Num() >= 3)
		{
			X = (*Values)[0]->AsNumber();
			Y = (*Values)[1]->AsNumber();
			Z = (*Values)[2]->AsNumber();

			return true;
		}

		return false;
	}

	UWorld* GetEditorWorld()
	{
		if (GEditor)
		{
			return GEditor->GetEditorWorldContext().World();
		}

		return nullptr;
	}

	/** True if an actor matches the delete filters. Empty filters are ignored */
	bool MatchesFilter(const AActor* Actor, const FString& Folder, const FString& LabelPrefix, const FString& Tag, bool bFolderPrefix)
	{
		if (!Folder.IsEmpty())
		{
			const FString ActorFolder = Actor->GetFolderPath().ToString();

			if (bFolderPrefix ? !(ActorFolder == Folder || ActorFolder.StartsWith(Folder + TEXT("/"))) : ActorFolder != Folder)
			{
				return false;
			}
		}

		if (!LabelPrefix.IsEmpty() && !Actor->GetActorLabel().StartsWith(LabelPrefix))
		{
			return false;
		}

		if (!Tag.IsEmpty() && !Actor->ActorHasTag(FName(*Tag)))
		{
			return false;
		}

		return true;
	}

	/** Applies text property values to an actor: {"EnemyLevel": "3"} */
	void ApplyProps(UObject* Object, const TSharedPtr<FJsonObject>& Props)
	{
		if (!Props.IsValid())
		{
			return;
		}

		for (const auto& Pair : Props->Values)
		{
			FProperty* Property = Object->GetClass()->FindPropertyByName(FName(*Pair.Key));

			if (!Property)
			{
				UE_LOG(LogAmnesiaBeta, Warning, TEXT("Level job: %s has no property %s"), *Object->GetName(), *Pair.Key);
				continue;
			}

			FString ValueText;

			if (Pair.Value->Type == EJson::String)
			{
				ValueText = Pair.Value->AsString();

			} else if (Pair.Value->Type == EJson::Boolean) {

				ValueText = Pair.Value->AsBool() ? TEXT("True") : TEXT("False");

			} else {

				ValueText = FString::SanitizeFloat(Pair.Value->AsNumber());
			}

			void* Address = Property->ContainerPtrToValuePtr<void>(Object);
			Property->ImportText_Direct(*ValueText, Address, Object, PPF_None);
		}
	}
}

void FAmnesiaLevelJobs::Start()
{
	IFileManager::Get().MakeDirectory(*JobFolder(), true);

	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&FAmnesiaLevelJobs::Tick), 0.5f);

	// every frame, so the frame time samples are not skipped
	CommandTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&FAmnesiaLevelJobs::TickCommands), 0.0f);
}

void FAmnesiaLevelJobs::Stop()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
	FTSTicker::GetCoreTicker().RemoveTicker(CommandTickerHandle);
}

bool FAmnesiaLevelJobs::TickCommands(float DeltaTime)
{
	if (CommandJob.bActive)
	{
		const double Now = FPlatformTime::Seconds();

		if (Now >= CommandJob.SampleStart)
		{
			CommandJob.Frame.Add(DeltaTime * 1000.0f);
			CommandJob.Game.Add(FPlatformTime::ToMilliseconds(GGameThreadTime));
			CommandJob.Render.Add(FPlatformTime::ToMilliseconds(GRenderThreadTime));
			CommandJob.Rhi.Add(FPlatformTime::ToMilliseconds(GRHIThreadTime));
			CommandJob.Gpu.Add(FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()));
		}

		if (Now >= CommandJob.SampleEnd)
		{
			FinishCommandJob();
		}

		return true;
	}

	CommandPollTime += DeltaTime;

	if (CommandPollTime < 0.2f)
	{
		return true;
	}

	CommandPollTime = 0.0f;

	const FString CommandPath = JobFolder() / TEXT("cmd.json");

	if (!IFileManager::Get().FileExists(*CommandPath))
	{
		return true;
	}

	FString Text;

	if (!FFileHelper::LoadFileToString(Text, *CommandPath))
	{
		return true;
	}

	IFileManager::Get().Delete(*CommandPath);

	TSharedPtr<FJsonObject> Root;

	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
	{
		FFileHelper::SaveStringToFile(TEXT("{\"error\":\"bad json\"}"), *(JobFolder() / TEXT("cmd_result.json")));
		return true;
	}

	// commands run in the game world while it is playing, unless the job asks for the editor world
	FString WorldName = TEXT("auto");
	Root->TryGetStringField(TEXT("world"), WorldName);

	UWorld* World = GetEditorWorld();

	if (GEditor && GEditor->PlayWorld && WorldName != TEXT("editor"))
	{
		World = GEditor->PlayWorld;
	}

	CommandJob = FCommandJob();
	CommandJob.WorldName = (World && World->IsPlayInEditor()) ? TEXT("pie") : TEXT("editor");

	const TArray<TSharedPtr<FJsonValue>>* Commands = nullptr;

	if (Root->TryGetArrayField(TEXT("cmds"), Commands))
	{
		for (const TSharedPtr<FJsonValue>& Value : *Commands)
		{
			const FString Command = Value->AsString();

			FStringOutputDevice Output;
			bool bHandled = IConsoleManager::Get().ProcessUserConsoleInput(*Command, Output, World);

			if (!bHandled && GEngine)
			{
				bHandled = GEngine->Exec(World, *Command, Output);
			}

			CommandJob.Outputs.Add(FString::Printf(TEXT("%s | %s | %s"), *Command, bHandled ? TEXT("ok") : TEXT("unhandled"), *Output.TrimStartAndEnd()));
		}
	}

	double Wait = 0.0;
	double Sample = 0.0;
	Root->TryGetNumberField(TEXT("wait"), Wait);
	Root->TryGetNumberField(TEXT("sample"), Sample);

	CommandJob.bActive = true;
	CommandJob.SampleStart = FPlatformTime::Seconds() + Wait;
	CommandJob.SampleEnd = CommandJob.SampleStart + Sample;

	if (Sample <= 0.0)
	{
		FinishCommandJob();
	}

	return true;
}

void FAmnesiaLevelJobs::FinishCommandJob()
{
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("world"), CommandJob.WorldName);

	TArray<TSharedPtr<FJsonValue>> OutputValues;

	for (const FString& Line : CommandJob.Outputs)
	{
		OutputValues.Add(MakeShared<FJsonValueString>(Line));
	}

	Result->SetArrayField(TEXT("outputs"), OutputValues);
	Result->SetNumberField(TEXT("samples"), CommandJob.Frame.Num());

	if (!CommandJob.Frame.IsEmpty())
	{
		TSharedPtr<FJsonObject> FrameStat = MakeStat(CommandJob.Frame);
		Result->SetObjectField(TEXT("frame_ms"), FrameStat);
		Result->SetNumberField(TEXT("fps"), 1000.0 / FMath::Max(0.001, FrameStat->GetNumberField(TEXT("avg"))));
		Result->SetObjectField(TEXT("game_ms"), MakeStat(CommandJob.Game));
		Result->SetObjectField(TEXT("render_ms"), MakeStat(CommandJob.Render));
		Result->SetObjectField(TEXT("rhi_ms"), MakeStat(CommandJob.Rhi));
		Result->SetObjectField(TEXT("gpu_ms"), MakeStat(CommandJob.Gpu));
	}

	FString Text;
	FJsonSerializer::Serialize(Result.ToSharedRef(), TJsonWriterFactory<>::Create(&Text));
	FFileHelper::SaveStringToFile(Text, *(JobFolder() / TEXT("cmd_result.json")));

	CommandJob = FCommandJob();
}

bool FAmnesiaLevelJobs::Tick(float DeltaTime)
{
	const FString JobPath = JobFolder() / TEXT("job.json");

	if (!IFileManager::Get().FileExists(*JobPath))
	{
		return true;
	}

	FString JobText;

	if (!FFileHelper::LoadFileToString(JobText, *JobPath))
	{
		return true;
	}

	// don't touch the level while the game is running. Jobs marked for the game world only look, they run during play
	const bool bGameWorldJob = JobText.Contains(TEXT("\"world\": \"pie\"")) || JobText.Contains(TEXT("\"world\":\"pie\""));

	if (GEditor && GEditor->PlayWorld && !bGameWorldJob)
	{
		return true;
	}

	if (bGameWorldJob && !(GEditor && GEditor->PlayWorld))
	{
		return true;
	}

	// take the job out of the queue before running it, so a crash can't loop
	IFileManager::Get().Move(*(JobFolder() / TEXT("job.running.json")), *JobPath, true);

	const FString Result = RunJob(JobText);

	FFileHelper::SaveStringToFile(Result, *(JobFolder() / TEXT("result.json")));
	IFileManager::Get().Delete(*(JobFolder() / TEXT("job.running.json")));

	return true;
}

FString FAmnesiaLevelJobs::RunJob(const FString& JobText)
{
	TSharedPtr<FJsonObject> Root;

	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(JobText), Root) || !Root.IsValid())
	{
		return TEXT("{\"error\":\"bad json\"}");
	}

	FString WorldName;
	Root->TryGetStringField(TEXT("world"), WorldName);
	const bool bGameWorld = WorldName == TEXT("pie");

	UWorld* World = bGameWorld ? (GEditor ? GEditor->PlayWorld.Get() : nullptr) : GetEditorWorld();

	if (!World)
	{
		return TEXT("{\"error\":\"no world\"}");
	}

	int32 Spawned = 0;
	int32 Deleted = 0;
	int32 Moved = 0;
	TArray<FString> Errors;
	TArray<FString> Imported;
	TArray<FString> Report;

	// meshes and materials get loaded once and reused
	TMap<FString, UStaticMesh*> MeshCache;
	TMap<FString, UMaterialInterface*> MaterialCache;
	TMap<FString, UClass*> ClassCache;

	const TArray<TSharedPtr<FJsonValue>>* Ops = nullptr;
	Root->TryGetArrayField(TEXT("ops"), Ops);

	if (!Ops)
	{
		return TEXT("{\"error\":\"no ops\"}");
	}

	for (const TSharedPtr<FJsonValue>& OpValue : *Ops)
	{
		const TSharedPtr<FJsonObject> Op = OpValue->AsObject();

		if (!Op.IsValid())
		{
			continue;
		}

		const FString Type = Op->GetStringField(TEXT("op"));

		if (Type == TEXT("spawn"))
		{
			const FString MeshPath = Op->GetStringField(TEXT("mesh"));
			UStaticMesh** CachedMesh = MeshCache.Find(MeshPath);
			UStaticMesh* Mesh = CachedMesh ? *CachedMesh : LoadObject<UStaticMesh>(nullptr, *MeshPath);
			MeshCache.Add(MeshPath, Mesh);

			if (!Mesh)
			{
				Errors.AddUnique(TEXT("missing mesh ") + MeshPath);
				continue;
			}

			double X = 0.0, Y = 0.0, Z = 0.0;
			double Pitch = 0.0, Yaw = 0.0, Roll = 0.0;
			double SX = 1.0, SY = 1.0, SZ = 1.0;

			ReadVector(Op, TEXT("loc"), X, Y, Z);
			ReadVector(Op, TEXT("rot"), Pitch, Yaw, Roll);

			if (!ReadVector(Op, TEXT("scale"), SX, SY, SZ))
			{
				double Uniform = 1.0;
				Op->TryGetNumberField(TEXT("s"), Uniform);
				SX = SY = SZ = Uniform;
			}

			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(FVector(X, Y, Z), FRotator(Pitch, Yaw, Roll), Params);

			if (!Actor)
			{
				Errors.AddUnique(TEXT("spawn failed"));
				continue;
			}

			Actor->SetActorScale3D(FVector(SX, SY, SZ));

			UStaticMeshComponent* Component = Actor->GetStaticMeshComponent();
			Component->SetStaticMesh(Mesh);

			// optional material override on every slot
			FString MaterialPath;

			if (Op->TryGetStringField(TEXT("material"), MaterialPath))
			{
				UMaterialInterface** CachedMaterial = MaterialCache.Find(MaterialPath);
				UMaterialInterface* Material = CachedMaterial ? *CachedMaterial : LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
				MaterialCache.Add(MaterialPath, Material);

				if (Material)
				{
					for (int32 Slot = 0; Slot < Component->GetNumMaterials(); ++Slot)
					{
						Component->SetMaterial(Slot, Material);
					}
				}
			}

			FString Collision;

			if (Op->TryGetStringField(TEXT("collision"), Collision))
			{
				Component->SetCollisionProfileName(FName(*Collision));
			}

			bool bHidden = false;

			if (Op->TryGetBoolField(TEXT("hidden"), bHidden) && bHidden)
			{
				Component->SetVisibility(false);
				Component->SetHiddenInGame(true);
			}

			bool bCastShadow = true;

			if (Op->TryGetBoolField(TEXT("shadow"), bCastShadow))
			{
				Component->SetCastShadow(bCastShadow);
			}

			int32 LightChannels = 0;

			FString Label;

			if (Op->TryGetStringField(TEXT("label"), Label))
			{
				Actor->SetActorLabel(Label);
			}

			FString Folder;

			if (Op->TryGetStringField(TEXT("folder"), Folder))
			{
				Actor->SetFolderPath(FName(*Folder));
			}

			const TArray<TSharedPtr<FJsonValue>>* Tags;

			if (Op->TryGetArrayField(TEXT("tags"), Tags))
			{
				for (const TSharedPtr<FJsonValue>& Tag : *Tags)
				{
					Actor->Tags.AddUnique(FName(*Tag->AsString()));
				}
			}

			++Spawned;

		} else if (Type == TEXT("ism")) {

			// many copies of one mesh in a single actor: {"mesh", "label", "folder", "tags", "shadow", "material", "cull": [start, end],
			// "instances": [[x, y, z, pitch, yaw, roll, sx, sy, sz], ...]}. Cheap to draw and to keep in the level, so long stretches of
			// forest, grass and ruins are built from these instead of one actor per piece
			const FString MeshPath = Op->GetStringField(TEXT("mesh"));
			UStaticMesh** CachedMesh = MeshCache.Find(MeshPath);
			UStaticMesh* Mesh = CachedMesh ? *CachedMesh : LoadObject<UStaticMesh>(nullptr, *MeshPath);
			MeshCache.Add(MeshPath, Mesh);

			const TArray<TSharedPtr<FJsonValue>>* InstanceValues = nullptr;

			if (!Mesh || !Op->TryGetArrayField(TEXT("instances"), InstanceValues) || InstanceValues->Num() == 0)
			{
				Errors.AddUnique(TEXT("ism needs a mesh and instances: ") + MeshPath);
				continue;
			}

			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AActor* Actor = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);

			if (!Actor)
			{
				Errors.AddUnique(TEXT("ism actor failed"));
				continue;
			}

			USceneComponent* SceneRoot = NewObject<USceneComponent>(Actor, TEXT("Root"));
			SceneRoot->SetMobility(EComponentMobility::Static);
			Actor->SetRootComponent(SceneRoot);
			SceneRoot->RegisterComponent();

			UHierarchicalInstancedStaticMeshComponent* Instances = NewObject<UHierarchicalInstancedStaticMeshComponent>(Actor, TEXT("Instances"));
			Instances->SetStaticMesh(Mesh);
			Instances->SetMobility(EComponentMobility::Static);
			Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Instances->SetGenerateOverlapEvents(false);
			Instances->bCastDynamicShadow = true;
			Instances->SetupAttachment(SceneRoot);
			Actor->AddInstanceComponent(Instances);
			Instances->RegisterComponent();

			FString MaterialPath;

			if (Op->TryGetStringField(TEXT("material"), MaterialPath))
			{
				UMaterialInterface** CachedMaterial = MaterialCache.Find(MaterialPath);
				UMaterialInterface* Material = CachedMaterial ? *CachedMaterial : LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
				MaterialCache.Add(MaterialPath, Material);

				if (Material)
				{
					for (int32 Slot = 0; Slot < Instances->GetNumMaterials(); ++Slot)
					{
						Instances->SetMaterial(Slot, Material);
					}
				}
			}

			bool bInstanceShadow = true;

			if (Op->TryGetBoolField(TEXT("shadow"), bInstanceShadow))
			{
				Instances->SetCastShadow(bInstanceShadow);
			}

			const TArray<TSharedPtr<FJsonValue>>* CullValues = nullptr;

			if (Op->TryGetArrayField(TEXT("cull"), CullValues) && CullValues->Num() == 2)
			{
				Instances->SetCullDistances((int32)(*CullValues)[0]->AsNumber(), (int32)(*CullValues)[1]->AsNumber());
			}

			TArray<FTransform> Transforms;
			Transforms.Reserve(InstanceValues->Num());

			for (const TSharedPtr<FJsonValue>& InstanceValue : *InstanceValues)
			{
				const TArray<TSharedPtr<FJsonValue>>& Numbers = InstanceValue->AsArray();

				if (Numbers.Num() < 9)
				{
					continue;
				}

				Transforms.Add(FTransform(FRotator(Numbers[3]->AsNumber(), Numbers[4]->AsNumber(), Numbers[5]->AsNumber()),
					FVector(Numbers[0]->AsNumber(), Numbers[1]->AsNumber(), Numbers[2]->AsNumber()),
					FVector(Numbers[6]->AsNumber(), Numbers[7]->AsNumber(), Numbers[8]->AsNumber())));
			}

			Instances->AddInstances(Transforms, false, true);

			FString IsmLabel;

			if (Op->TryGetStringField(TEXT("label"), IsmLabel))
			{
				Actor->SetActorLabel(IsmLabel);
			}

			FString IsmFolder;

			if (Op->TryGetStringField(TEXT("folder"), IsmFolder))
			{
				Actor->SetFolderPath(FName(*IsmFolder));
			}

			const TArray<TSharedPtr<FJsonValue>>* IsmTags;

			if (Op->TryGetArrayField(TEXT("tags"), IsmTags))
			{
				for (const TSharedPtr<FJsonValue>& Tag : *IsmTags)
				{
					Actor->Tags.AddUnique(FName(*Tag->AsString()));
				}
			}

			Spawned += Transforms.Num();

		} else if (Type == TEXT("spawn_actor")) {

			const FString ClassPath = Op->GetStringField(TEXT("class"));
			UClass** CachedClass = ClassCache.Find(ClassPath);
			UClass* Class = CachedClass ? *CachedClass : LoadObject<UClass>(nullptr, *ClassPath);
			ClassCache.Add(ClassPath, Class);

			if (!Class)
			{
				Errors.AddUnique(TEXT("missing class ") + ClassPath);
				continue;
			}

			double X = 0.0, Y = 0.0, Z = 0.0;
			double Pitch = 0.0, Yaw = 0.0, Roll = 0.0;
			double SX = 1.0, SY = 1.0, SZ = 1.0;

			ReadVector(Op, TEXT("loc"), X, Y, Z);
			ReadVector(Op, TEXT("rot"), Pitch, Yaw, Roll);
			ReadVector(Op, TEXT("scale"), SX, SY, SZ);

			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			AActor* Actor = World->SpawnActor<AActor>(Class, FVector(X, Y, Z), FRotator(Pitch, Yaw, Roll), Params);

			if (!Actor)
			{
				Errors.AddUnique(TEXT("spawn_actor failed ") + ClassPath);
				continue;
			}

			if (ReadVector(Op, TEXT("scale"), SX, SY, SZ))
			{
				Actor->SetActorScale3D(FVector(SX, SY, SZ));
			}

			ApplyProps(Actor, Op->HasField(TEXT("props")) ? Op->GetObjectField(TEXT("props")) : nullptr);

			// let the actor react to its new settings, like lights applying their color
			Actor->RerunConstructionScripts();

			FString Label;

			if (Op->TryGetStringField(TEXT("label"), Label))
			{
				Actor->SetActorLabel(Label);
			}

			FString Folder;

			if (Op->TryGetStringField(TEXT("folder"), Folder))
			{
				Actor->SetFolderPath(FName(*Folder));
			}

			++Spawned;

		} else if (Type == TEXT("delete")) {

			FString Folder, LabelPrefix, Tag;
			Op->TryGetStringField(TEXT("folder"), Folder);
			Op->TryGetStringField(TEXT("label_prefix"), LabelPrefix);
			Op->TryGetStringField(TEXT("tag"), Tag);

			bool bFolderPrefix = true;
			Op->TryGetBoolField(TEXT("recursive"), bFolderPrefix);

			if (Folder.IsEmpty() && LabelPrefix.IsEmpty() && Tag.IsEmpty())
			{
				Errors.AddUnique(TEXT("delete without a filter"));
				continue;
			}

			// optional stretch of the level: only actors whose location lies between x_min and x_max
			double XMin = -TNumericLimits<double>::Max();
			double XMax = TNumericLimits<double>::Max();
			Op->TryGetNumberField(TEXT("x_min"), XMin);
			Op->TryGetNumberField(TEXT("x_max"), XMax);

			TArray<AActor*> ToDelete;

			for (TActorIterator<AActor> It(World); It; ++It)
			{
				const double X = It->GetActorLocation().X;

				if (X >= XMin && X <= XMax && MatchesFilter(*It, Folder, LabelPrefix, Tag, bFolderPrefix))
				{
					ToDelete.Add(*It);
				}
			}

			for (AActor* Actor : ToDelete)
			{
				World->EditorDestroyActor(Actor, true);
				++Deleted;
			}

		} else if (Type == TEXT("set_transform")) {

			const FString Label = Op->GetStringField(TEXT("label"));

			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (It->GetActorLabel() == Label)
				{
					FVector Location = It->GetActorLocation();
					FRotator Rotation = It->GetActorRotation();
					FVector Scale = It->GetActorScale3D();

					double X, Y, Z;

					if (ReadVector(Op, TEXT("loc"), X, Y, Z))
					{
						Location = FVector(X, Y, Z);
					}

					if (ReadVector(Op, TEXT("rot"), X, Y, Z))
					{
						Rotation = FRotator(X, Y, Z);
					}

					if (ReadVector(Op, TEXT("scale"), X, Y, Z))
					{
						Scale = FVector(X, Y, Z);
					}

					It->Modify();
					It->SetActorLocationAndRotation(Location, Rotation);
					It->SetActorScale3D(Scale);

					++Moved;
				}
			}

		} else if (Type == TEXT("set_props")) {

			const FString Label = Op->GetStringField(TEXT("label"));

			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (It->GetActorLabel() == Label)
				{
					It->Modify();
					ApplyProps(*It, Op->GetObjectField(TEXT("props")));
					++Moved;
				}
			}

		} else if (Type == TEXT("push_back")) {

			// decorations that stand in the path but have no collision look like walls the hero walks through. Push them behind it
			TArray<FString> Prefixes;
			const TArray<TSharedPtr<FJsonValue>>* LabelValues;

			if (Op->TryGetArrayField(TEXT("labels"), LabelValues))
			{
				for (const TSharedPtr<FJsonValue>& Value : *LabelValues)
				{
					Prefixes.Add(Value->AsString());
				}
			}

			double LimitY = -110.0;
			double FrontY = 110.0;
			double MinHeight = 120.0;
			Op->TryGetNumberField(TEXT("limit_y"), LimitY);
			Op->TryGetNumberField(TEXT("front_y"), FrontY);
			Op->TryGetNumberField(TEXT("min_height"), MinHeight);

			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (!It->ActorHasTag(FName("Decoration")))
				{
					continue;
				}

				const FString ActorLabel = It->GetActorLabel();
				bool bListed = false;

				for (const FString& Prefix : Prefixes)
				{
					if (ActorLabel.StartsWith(Prefix))
					{
						bListed = true;
						break;
					}
				}

				if (!bListed)
				{
					continue;
				}

				const FBox Box = It->GetComponentsBoundingBox(true);

				// flat things like floors and grass are fine to walk over
				if (!Box.IsValid || Box.Max.Z - Box.Min.Z < MinHeight)
				{
					continue;
				}

				if (Box.Max.Y > LimitY && Box.Min.Y < FrontY)
				{
					It->Modify();
					It->AddActorWorldOffset(FVector(0.0, LimitY - Box.Max.Y, 0.0));
					++Moved;
				}
			}

		} else if (Type == TEXT("import")) {

			// files from disk into a content folder: {"dest": "/Game/Audio/Hero", "files": ["C:/.../SW_Swing_01.wav"]}
			const FString Destination = Op->GetStringField(TEXT("dest"));
			const TArray<TSharedPtr<FJsonValue>>* FileValues;
			TArray<UAssetImportTask*> Tasks;

			if (Op->TryGetArrayField(TEXT("files"), FileValues))
			{
				for (const TSharedPtr<FJsonValue>& Value : *FileValues)
				{
					UAssetImportTask* Task = NewObject<UAssetImportTask>();
					Task->Filename = Value->AsString();
					Task->DestinationPath = Destination;
					Task->bReplaceExisting = true;
					Task->bAutomated = true;
					Task->bSave = false;
					Tasks.Add(Task);
				}
			}

			FAssetToolsModule::GetModule().Get().ImportAssetTasks(Tasks);

			for (const UAssetImportTask* Task : Tasks)
			{
				if (Task->ImportedObjectPaths.Num() > 0)
				{
					Imported.Append(Task->ImportedObjectPaths);

				} else {

					Errors.Add(TEXT("import failed ") + Task->Filename);
				}
			}

		} else if (Type == TEXT("set_object_props")) {

			// properties of an asset, or of a class default object when the path is a class: {"object": "/Game/X.X_C", "props": {...}}
			const FString ObjectPath = Op->GetStringField(TEXT("object"));
			UObject* Object = LoadObject<UObject>(nullptr, *ObjectPath);

			if (UClass* Class = Cast<UClass>(Object))
			{
				Object = Class->GetDefaultObject();
			}

			if (!Object)
			{
				Errors.Add(TEXT("missing object ") + ObjectPath);
				continue;
			}

			Object->Modify();
			ApplyProps(Object, Op->GetObjectField(TEXT("props")));
			Object->PostEditChange();
			Object->MarkPackageDirty();
			++Moved;

		} else if (bGameWorld && Type != TEXT("path_scan")) {

			Errors.Add(TEXT("only path_scan runs in the game world, not ") + Type);

		} else if (Type == TEXT("path_scan")) {

			// walks the hero's path in the running game and reports anything that blocks a pawn at body height:
			// {"world": "pie", "ops": [{"op": "path_scan", "x0": -1500, "x1": 64000, "step": 20}]}
			double X0 = 0.0, X1 = 1000.0, Step = 20.0, PathY = 0.0;
			Op->TryGetNumberField(TEXT("x0"), X0);
			Op->TryGetNumberField(TEXT("x1"), X1);
			Op->TryGetNumberField(TEXT("step"), Step);
			Op->TryGetNumberField(TEXT("y"), PathY);
			Step = FMath::Max(Step, 5.0);

			FCollisionQueryParams Params(SCENE_QUERY_STAT(AmnesiaPathScan), false);

			// the floor under the path, followed from step to step so the cave ceiling is never taken for it
			double Floor = 0.0;
			FHitResult Down;

			if (World->LineTraceSingleByChannel(Down, FVector(X0, PathY, 3000.0), FVector(X0, PathY, -4000.0), ECC_Pawn, Params))
			{
				Floor = Down.ImpactPoint.Z;
			}

			TMap<FString, int32> Seen;

			for (double X = X0; X < X1; X += Step)
			{
				if (World->LineTraceSingleByChannel(Down, FVector(X, PathY, Floor + 260.0), FVector(X, PathY, Floor - 900.0), ECC_Pawn, Params) && !Down.GetActor()->IsA<APawn>())
				{
					Floor = Down.ImpactPoint.Z;
				}

				for (const double Height : { 45.0, 100.0, 160.0 })
				{
					FHitResult Hit;
					const FVector Start(X, PathY, Floor + Height);

					if (!World->LineTraceSingleByChannel(Hit, Start, Start + FVector(Step, 0.0, 0.0), ECC_Pawn, Params))
					{
						continue;
					}

					AActor* HitActor = Hit.GetActor();

					// creatures are not obstacles, and a surface that rises gently is just a slope
					if (!HitActor || HitActor->IsA<APawn>() || Hit.ImpactNormal.Z > 0.6)
					{
						continue;
					}

					UPrimitiveComponent* Component = Hit.GetComponent();
					const FString Key = HitActor->GetName() + TEXT(".") + (Component ? Component->GetName() : TEXT("?"));

					if (Seen.Contains(Key))
					{
						continue;
					}

					Seen.Add(Key, 1);

					const bool bVisible = Component && Component->IsVisible() && !HitActor->IsHidden();
					const UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(Component);
					const FString MeshName = MeshComponent && MeshComponent->GetStaticMesh() ? MeshComponent->GetStaticMesh()->GetName() : TEXT("-");

					Report.Add(FString::Printf(TEXT("%.0f|%.0f|%s|%s|%s|%s|%s|%s|%s"), Hit.ImpactPoint.X, Floor, *HitActor->GetActorNameOrLabel(), *HitActor->GetClass()->GetName(),
						Component ? *Component->GetName() : TEXT("?"), *MeshName, bVisible ? TEXT("visible") : TEXT("INVISIBLE"),
						Component ? *Component->GetCollisionProfileName().ToString() : TEXT("?"), HitActor->ActorHasTag(FName("Decoration")) ? TEXT("decoration") : TEXT("-")));
				}
			}

		} else if (Type == TEXT("mesh_bounds")) {

			// size and pivot of meshes, to place them without guessing: "path min.x min.y min.z max.x max.y max.z"
			const TArray<TSharedPtr<FJsonValue>>* MeshValues;

			if (Op->TryGetArrayField(TEXT("meshes"), MeshValues))
			{
				for (const TSharedPtr<FJsonValue>& Value : *MeshValues)
				{
					const FString MeshPath = Value->AsString();

					if (const UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *MeshPath))
					{
						const FBox Bounds = Mesh->GetBoundingBox();
						Report.Add(FString::Printf(TEXT("%s %.1f %.1f %.1f %.1f %.1f %.1f"), *MeshPath, Bounds.Min.X, Bounds.Min.Y, Bounds.Min.Z, Bounds.Max.X, Bounds.Max.Y, Bounds.Max.Z));

					} else {

						Errors.Add(TEXT("missing mesh ") + MeshPath);
					}
				}
			}

		} else if (Type == TEXT("snap")) {

			// sets floating decorations down on the ground: {"folder": "Gen/Zone4", "skip": ["Ivy"], "dry": true}
			// the ground is anything that is not a decoration: terrain slabs, floors, platform collision
			const FString Folder = Op->GetStringField(TEXT("folder"));
			bool bDry = false;
			double MinGap = 6.0;
			double MaxDrop = 3000.0;
			double SinkFraction = 0.05;
			double MaxSink = 30.0;
			Op->TryGetBoolField(TEXT("dry"), bDry);
			Op->TryGetNumberField(TEXT("min_gap"), MinGap);
			Op->TryGetNumberField(TEXT("max_drop"), MaxDrop);
			Op->TryGetNumberField(TEXT("sink"), SinkFraction);
			Op->TryGetNumberField(TEXT("max_sink"), MaxSink);

			TArray<FString> SkipLabels;
			TArray<FString> SkipFolders;
			const TArray<TSharedPtr<FJsonValue>>* SkipValues;

			if (Op->TryGetArrayField(TEXT("skip"), SkipValues))
			{
				for (const TSharedPtr<FJsonValue>& Value : *SkipValues)
				{
					SkipLabels.Add(Value->AsString());
				}
			}

			if (Op->TryGetArrayField(TEXT("skip_folders"), SkipValues))
			{
				for (const TSharedPtr<FJsonValue>& Value : *SkipValues)
				{
					SkipFolders.Add(Value->AsString());
				}
			}

			// decorations never count as ground
			FCollisionQueryParams Params(SCENE_QUERY_STAT(AmnesiaSnap), true);
			TArray<AActor*> Decorations;

			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (It->ActorHasTag(FName("Decoration")))
				{
					Decorations.Add(*It);
				}
			}

			Params.AddIgnoredActors(Decorations);

			int32 Checked = 0;

			for (AActor* Actor : Decorations)
			{
				const FString ActorFolder = Actor->GetFolderPath().ToString();
				const FString ActorLabel = Actor->GetActorLabel();

				if (!Folder.IsEmpty() && !(ActorFolder == Folder || ActorFolder.StartsWith(Folder + TEXT("/"))))
				{
					continue;
				}

				bool bSkip = false;

				for (const FString& Prefix : SkipLabels)
				{
					bSkip |= ActorLabel.StartsWith(Prefix);
				}

				for (const FString& Prefix : SkipFolders)
				{
					bSkip |= ActorFolder.StartsWith(Prefix);
				}

				if (bSkip || !Actor->IsA<AStaticMeshActor>())
				{
					continue;
				}

				const FBox Box = Actor->GetComponentsBoundingBox(true);

				if (!Box.IsValid || Box.Max.Z - Box.Min.Z < 2.0)
				{
					continue;
				}

				++Checked;

				// the lowest ground under the middle and four points close around it, traced from the top of the object.
				// The points stay near the middle: a tree's bounds are its crown, and its trunk stands in the middle
				const FVector Center = Box.GetCenter();
				const FVector Extent(FMath::Min(Box.GetExtent().X * 0.35, 60.0), FMath::Min(Box.GetExtent().Y * 0.35, 60.0), 0.0);
				const FVector2D Samples[] = { FVector2D(0, 0), FVector2D(-1, -1), FVector2D(1, -1), FVector2D(-1, 1), FVector2D(1, 1) };
				double CenterHit = 0.0;
				double Lowest = TNumericLimits<double>::Max();
				bool bCenter = false;

				for (const FVector2D& Sample : Samples)
				{
					const FVector Start(Center.X + Sample.X * Extent.X, Center.Y + Sample.Y * Extent.Y, Box.Max.Z + 10.0);
					const FVector End(Start.X, Start.Y, Box.Min.Z - MaxDrop);
					FHitResult Hit;

					if (!World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
					{
						continue;
					}

					if (!bCenter)
					{
						// the first sample is the middle: it decides which ground the object stands on
						if (!Sample.IsZero())
						{
							break;
						}

						bCenter = true;
						CenterHit = Hit.ImpactPoint.Z;
						Lowest = CenterHit;
						continue;
					}

					// a slope lowers the rest height a little; a drop over an edge is not the object's ground
					if (CenterHit - Hit.ImpactPoint.Z < 40.0)
					{
						Lowest = FMath::Min(Lowest, Hit.ImpactPoint.Z);
					}
				}

				if (!bCenter)
				{
					continue;
				}

				const double Gap = Box.Min.Z - Lowest;

				if (Gap <= MinGap)
				{
					continue;
				}

				Report.Add(FString::Printf(TEXT("%s|%s|%.0f|%.0f|%.0f"), *ActorFolder, *ActorLabel, Gap, Center.X, Box.Max.Z - Box.Min.Z));

				if (!bDry)
				{
					// rest it on the ground and sink it in a little, so no edge shows daylight
					const double Sink = FMath::Min((Box.Max.Z - Box.Min.Z) * SinkFraction, MaxSink);
					Actor->Modify();
					Actor->AddActorWorldOffset(FVector(0.0, 0.0, -(Gap + Sink)));
					++Moved;
				}
			}

			Report.Add(FString::Printf(TEXT("checked %d"), Checked));

		} else if (Type == TEXT("save")) {

			FEditorFileUtils::SaveDirtyPackages(false, true, true, false, false, false);

		} else if (Type == TEXT("exec")) {

			// an editor console command, like "py C:/path/script.py" (full editor Python); what it prints goes into the report
			const FString Command = Op->GetStringField(TEXT("command"));
			FStringOutputDevice CommandOutput;
			CommandOutput.SetAutoEmitLineTerminator(true);

			if (!GEditor || !GEditor->Exec(World, *Command, CommandOutput))
			{
				Errors.Add(TEXT("exec failed: ") + Command);
			}

			Report.Add(CommandOutput);

		} else {

			Errors.Add(TEXT("unknown op ") + Type);
		}
	}

	FString Output;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);

	Writer->WriteObjectStart();
	Writer->WriteValue(TEXT("spawned"), Spawned);
	Writer->WriteValue(TEXT("deleted"), Deleted);
	Writer->WriteValue(TEXT("modified"), Moved);
	Writer->WriteArrayStart(TEXT("errors"));

	for (const FString& Error : Errors)
	{
		Writer->WriteValue(Error);
	}

	Writer->WriteArrayEnd();
	Writer->WriteArrayStart(TEXT("imported"));

	for (const FString& Path : Imported)
	{
		Writer->WriteValue(Path);
	}

	Writer->WriteArrayEnd();
	Writer->WriteArrayStart(TEXT("report"));

	for (const FString& Line : Report)
	{
		Writer->WriteValue(Line);
	}

	Writer->WriteArrayEnd();
	Writer->WriteObjectEnd();
	Writer->Close();

	return Output;
}

#endif
