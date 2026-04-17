// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimData/CurveIdentifier.h"
#include "Animation/AttributesRuntime.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "AnimationUtils.h"
#include "Animation/AnimNotifyQueue.h"
#include "AnimationRuntime.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Animation/AnimNotifyEndDataContext.h"
#include "Animation/Skeleton.h"
#include "Logging/MessageLog.h"
#include "UObject/AssetRegistryTagsContext.h"
#include "UObject/FrameworkObjectVersion.h"
#include "UObject/FortniteMainBranchObjectVersion.h"
#include "Animation/AnimationPoseData.h"
#include "Animation/AnimSequenceHelpers.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimData/AnimDataModel.h"
#include "Modules/ModuleManager.h"
#include "MathUtil.h"
#include "UObject/LinkerLoad.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"

#if WITH_EDITOR
#include "IAnimationDataControllerModule.h"
#include "Modules/ModuleManager.h"
#endif // WITH_EDITOR

#include "Animation/AnimationSettings.h"
#include "UObject/UE5MainStreamObjectVersion.h"
#include "UObject/UObjectThreadContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimSequenceBase)

DEFINE_LOG_CATEGORY(LogAnimMarkerSync);
CSV_DECLARE_CATEGORY_MODULE_EXTERN(ENGINE_API, Animation);

#define LOCTEXT_NAMESPACE "AnimSequenceBase"
/////////////////////////////////////////////////////

UAnimSequenceBase::UAnimSequenceBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, RateScale(1.0f)
	, bLoop(false)
#if WITH_EDITORONLY_DATA
	, DataModel(nullptr)
	, bPopulatingDataModel(false)
	, Controller(nullptr)
#endif // WITH_EDITORONLY_DATA
{
#if WITH_EDITOR
	if (!HasAnyFlags(EObjectFlags::RF_ClassDefaultObject| EObjectFlags::RF_NeedLoad))
	{
		CreateModel();
		GetController();
	}
#endif // WITH_EDITOR
}

bool UAnimSequenceBase::IsPostLoadThreadSafe() const
{
	return WITH_EDITORONLY_DATA == 0;	// Not thread safe in editor because new objects can be constructed on upgrade and the skeleton can be modified
}

#if WITH_EDITORONLY_DATA
void UAnimSequenceBase::DeclareConstructClasses(TArray<FTopLevelAssetPath>& OutConstructClasses, const UClass* SpecificSubclass)
{
	Super::DeclareConstructClasses(OutConstructClasses, SpecificSubclass);

	OutConstructClasses.Add(FTopLevelAssetPath(TEXT("/Script/Engine.AnimDataModel")));

	// We need to declare all types that can be returned from UE::Anim::DataModel::IAnimationDataModels::FindClassForAnimationAsset(UAnimSequenceBase*);
	// 我们需要声明可以从 UE::Anim::DataModel::IAnimationDataModels::FindClassForAnimationAsset(UAnimSequenceBase*); 返回的所有类型。
	OutConstructClasses.Add(FTopLevelAssetPath(TEXT("/Script/AnimationData.AnimationSequencerDataModel")));

	// We can call Controller->CreateModel, which can add objects, so add every ControllerClass as something we can construct.
	// 我们可以调用Controller->CreateModel，它可以添加对象，因此将每个ControllerClass添加为我们可以构造的东西。
	// THe caller will add on all recursively constructable classes
	// 调用者将添加所有可递归构造的类
	UClass* AnimationControllerClass = UAnimationDataController::StaticClass();
	for (TObjectIterator<UClass> Iter; Iter; ++Iter)
	{
		if ((*Iter)->ImplementsInterface(AnimationControllerClass))
		{
			OutConstructClasses.Add(FTopLevelAssetPath(*Iter));
		}
	}
}
#endif

void UAnimSequenceBase::GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize)
{
	Super::GetResourceSizeEx(CumulativeResourceSize);

	const FRawCurveTracks& CurveData = GetCurveData();
	const UStruct* Struct = CurveData.StaticStruct();
	if (ensure(Struct))
	{
		CumulativeResourceSize.AddDedicatedSystemMemoryBytes(Struct->GetStructureSize());
		CumulativeResourceSize.AddDedicatedSystemMemoryBytes(CurveData.FloatCurves.GetAllocatedSize());
	}
}

void UAnimSequenceBase::PostLoad()
{
#if WITH_EDITORONLY_DATA
	if (!HasAnyFlags(EObjectFlags::RF_ClassDefaultObject))
	{
		LLM_SCOPE(ELLMTag::Animation);

		auto PreloadSkeleton = [this]()
		{
			if (USkeleton* MySkeleton = GetSkeleton())
			{
				MySkeleton->ConditionalPreload();
				MySkeleton->ConditionalPostLoad();
			}
		};

		if(ShouldDataModelBeValid())
		{
		    const UClass* TargetDataModelClass = UE::Anim::DataModel::IAnimationDataModels::FindClassForAnimationAsset(this);
		    const bool bRequiresModelCreation = DataModelInterface == nullptr || DataModelInterface.GetObject()->GetClass() != TargetDataModelClass ||  GetLinkerCustomVersion(FUE5MainStreamObjectVersion::GUID) < FUE5MainStreamObjectVersion::ReintroduceAnimationDataModelInterface;

		    TScriptInterface<IAnimationDataModel> CachedDataModelInterface = DataModelInterface;
		    
		    const bool bRequiresModelPopulation = GetLinkerCustomVersion(FUE5MainStreamObjectVersion::GUID) < FUE5MainStreamObjectVersion::IntroducingAnimationDataModel;
		    PRAGMA_DISABLE_DEPRECATION_WARNINGS
		    checkf(bRequiresModelPopulation || DataModel != nullptr || DataModelInterface != nullptr, TEXT("Invalid Animation Sequence base state, no data model found past upgrade object version. AnimSequenceBase:%s"), *GetPathName());
		    PRAGMA_ENABLE_DEPRECATION_WARNINGS
    
		    // Construct a new IAnimationDataModel instance
		    // [翻译失败: Construct a new IAnimationDataModel instance]
		    if(bRequiresModelCreation)
		    {
			    CreateModel();
		    }

			PreloadSkeleton();
			
		    ValidateModel();
			if (UObject* DataModelObject = DataModelInterface.GetObject())
			{
				DataModelObject->ConditionalPostLoad();
				DataModelObject->ConditionalPostLoadSubobjects();
			}
		    GetController();
		    BindToModelModificationEvent();		    

		    if (bRequiresModelPopulation || bRequiresModelCreation)
		    {
			    const bool bDoNotTransactAction = false;
    
		    	Controller->OpenBracket(LOCTEXT("UAnimSequenceBase::PostLoad_PopulatingModelInterface","Populating Animation Data Model Interface"), bDoNotTransactAction);
			    
		    	bPopulatingDataModel = true;
		    	
		    	Controller->InitializeModel();
    
		    	PRAGMA_DISABLE_DEPRECATION_WARNINGS
				// In case a data model has already been created populate the new one with its data
				// [翻译失败: In case a data model has already been created populate the new one with its data]
				if (DataModel != nullptr)
				{
					DataModel->ConditionalPreload();
					DataModel->PostLoad();
					DataModel->ConditionalPostLoadSubobjects();
					PopulateWithExistingModel(DataModel.Get());

					TScriptInterface<IAnimationDataController> DataModelController = DataModel->GetController();
					DataModelController->ResetModel(false);
					
					DataModel = nullptr;
					Controller->NotifyPopulated();
				}
		    	PRAGMA_ENABLE_DEPRECATION_WARNINGS
				// If switching to a different DataModelInterface implementation, copy the data from the existing one
				// [翻译失败: If switching to a different DataModelInterface implementation, copy the data from the existing one]
				else if (CachedDataModelInterface != nullptr)
				{
					PopulateWithExistingModel(CachedDataModelInterface);
					Controller->NotifyPopulated();
				}
		    	// Otherwise upgrade this animation asset to be model-based
		    	// 否则将此动画资源升级为基于模型的
		    	else
		    	{
					PopulateModel();
		    		Controller->NotifyPopulated();
		    	}
		    	bPopulatingDataModel = false;
		    	Controller->CloseBracket();
		    }
		    else
		    {
		    	// Fix-up to ensure correct curves are used for compression
		    	// 修复以确保使用正确的曲线进行压缩
		    	PRAGMA_DISABLE_DEPRECATION_WARNINGS
				RawCurveData.FloatCurves = DataModelInterface->GetCurveData().FloatCurves;
		    	RawCurveData.TransformCurves = DataModelInterface->GetCurveData().TransformCurves;
		    	RawCurveData.RemoveRedundantKeys(0.f);
		    	PRAGMA_ENABLE_DEPRECATION_WARNINGS
			}
		}
		else
		{
			PreloadSkeleton();
		}
	}
#endif // WITH_EDITORONLY_DATA

	Super::PostLoad();

#if WITH_EDITORONLY_DATA
	// Convert Notifies to new data
	// 将通知转换为新数据
	if( GIsEditor && Notifies.Num() > 0 )
	{
		if(GetLinkerUEVersion() < VER_UE4_CLEAR_NOTIFY_TRIGGERS)
		{
			for(FAnimNotifyEvent Notify : Notifies)
			{
				if(Notify.Notify)
				{
					// Clear end triggers for notifies that are not notify states
					// 清除非通知状态通知的结束触发器
					Notify.EndTriggerTimeOffset = 0.0f;
				}
			}
		}
	}

	InitializeNotifyTrack();
#endif	// WITH_EDITORONLY_DATA
	RefreshCacheData();

#if WITH_EDITORONLY_DATA
	if (!GetPackage()->GetHasBeenEndLoaded())
	{
		FCoreUObjectDelegates::OnEndLoadPackage.AddUObject(this, &UAnimSequenceBase::OnEndLoadPackage);
	}
	else
	{
		OnAnimModelLoaded();
	}

	if(USkeleton* MySkeleton = GetSkeleton())
	{
		if (IsDataModelValid())
		{
			const bool bDoNotTransactAction = false;
			if (GetLinkerCustomVersion(FFortniteMainBranchObjectVersion::GUID) < FFortniteMainBranchObjectVersion::FixUpNoneNameAnimationCurves)
			{
				Controller->OpenBracket(LOCTEXT("FFortniteMainBranchObjectVersion::FixUpNoneNameAnimationCurves_Bracket","FFortniteMainBranchObjectVersion::FixUpNoneNameAnimationCurves"), bDoNotTransactAction);
				{
					const TArray<FFloatCurve>& FloatCurves = DataModelInterface->GetFloatCurves();
					for (int32 Index = 0; Index < FloatCurves.Num(); ++Index)
					{
						const FFloatCurve& Curve = FloatCurves[Index];
						if (Curve.GetName() == NAME_None)
						{
							// give unique name
							// [翻译失败: give unique name]
							const FName UniqueName = FName(*FString(GetName() + TEXT("_CurveNameFix_") + FString::FromInt(Index)));
							UE_LOG(LogAnimation, Warning, TEXT("[AnimSequence %s] contains invalid curve name \'None\'. Renaming this to %s. Please fix this curve in the editor. "), *GetFullName(), *Curve.GetName().ToString());

							Controller->RenameCurve(FAnimationCurveIdentifier(Curve.GetName(), ERawCurveTrackTypes::RCT_Float), FAnimationCurveIdentifier(UniqueName, ERawCurveTrackTypes::RCT_Float), bDoNotTransactAction);
						}
					}
				}
				Controller->CloseBracket(bDoNotTransactAction);
			}
		}

		// this should continue to add if skeleton hasn't been saved either 
		// [翻译失败: this should continue to add if skeleton hasn't been saved either]
		// we don't wipe out data, so make sure you add back in if required
		// [翻译失败: we don't wipe out data, so make sure you add back in if required]
		if (GetLinkerCustomVersion(FFrameworkObjectVersion::GUID) < FFrameworkObjectVersion::MoveCurveTypesToSkeleton
			|| MySkeleton->GetLinkerCustomVersion(FFrameworkObjectVersion::GUID) < FFrameworkObjectVersion::MoveCurveTypesToSkeleton)
		{
			// This is safe as the data model will have been created during this PostLoad call
			// [翻译失败: This is safe as the data model will have been created during this PostLoad call]
			const TArray<FFloatCurve>& FloatCurves = DataModelInterface->GetFloatCurves();

			// fix up curve flags to skeleton
			// 将曲线标志修复到骨架
			for (const FFloatCurve& Curve : FloatCurves)
			{
				bool bMorphtargetSet = Curve.GetCurveTypeFlag(AACF_DriveMorphTarget_DEPRECATED);
				bool bMaterialSet = Curve.GetCurveTypeFlag(AACF_DriveMaterial_DEPRECATED);

				// only add this if that has to 
				// 仅在必须时添加此内容
				if (bMorphtargetSet || bMaterialSet)
				{
					MySkeleton->AccumulateCurveMetaData(Curve.GetName(), bMaterialSet, bMorphtargetSet);
				}
			}
		}
	}
#endif // WITH_EDITORONLY_DATA
}

void UAnimSequenceBase::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);

#if WITH_EDITOR
	if (!GetPackage()->HasAnyPackageFlags(PKG_Cooked))
	{
		checkf(DataModelInterface.GetObject()->GetOuter() == this, TEXT("Animation Data Model interface has incorrect outer, expected %s - found %s"), *this->GetName(), *DataModelInterface.GetObject()->GetOuter()->GetName());
		BindToModelModificationEvent();
	}
#endif // WITH_EDITOR
}

float UAnimSequenceBase::GetPlayLength() const
{
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	return SequenceLength;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

void UAnimSequenceBase::SortNotifies()
{
	// Sorts using FAnimNotifyEvent::operator<()
	// 使用 FAnimNotifyEvent::operator<() 进行排序
	Notifies.Sort();
}

bool UAnimSequenceBase::RemoveNotifies(const TArray<FName>& NotifiesToRemove)
{
	bool bSequenceModified = false;
	for (int32 NotifyIndex = Notifies.Num() - 1; NotifyIndex >= 0; --NotifyIndex)
	{
		FAnimNotifyEvent& AnimNotify = Notifies[NotifyIndex];
		if (NotifiesToRemove.Contains(AnimNotify.NotifyName))
		{
			if (!bSequenceModified)
			{
				Modify();
				bSequenceModified = true;
			}
			Notifies.RemoveAtSwap(NotifyIndex);
		}
	}

	if (bSequenceModified)
	{
		MarkPackageDirty();
		RefreshCacheData();
	}
	return bSequenceModified;
}

void UAnimSequenceBase::RemoveNotifies()
{
	if (Notifies.Num() == 0)
	{
		return;
	}
	Modify();
	Notifies.Reset();
	MarkPackageDirty();
	RefreshCacheData();
}

#if WITH_EDITOR
void UAnimSequenceBase::RenameNotifies(FName InOldName, FName InNewName)
{
	Modify();

	for(FAnimNotifyEvent& Notify : Notifies)
	{
		// Only handle named notifies
		// 只处理命名通知
		if(!Notify.IsBlueprintNotify())
		{
			if(Notify.NotifyName == InOldName)
			{
				Notify.NotifyName = InNewName;
			}
		}
	}

	// notification broadcast
	// [翻译失败: notification broadcast]
	OnNotifyChanged.Broadcast();
}
#endif

bool UAnimSequenceBase::IsNotifyAvailable() const
{
	return (Notifies.Num() != 0) && (GetPlayLength() > 0.f);
}

void UAnimSequenceBase::GetAnimNotifies(const float& StartTime, const float& DeltaTime, FAnimNotifyContext& NotifyContext) const
{
	// Early out if we have no notifies
	// [翻译失败: Early out if we have no notifies]
	if (!IsNotifyAvailable())
	{
		return;
	}
	
	bool const bPlayingBackwards = (DeltaTime < 0.f);
	float PreviousPosition = StartTime;
	float CurrentPosition = StartTime;
	float DesiredDeltaMove = DeltaTime;
	const float PlayLength = GetPlayLength();

	// previous behaviour could get the same notify multiple times  - support this within reasonable limits
	// 以前的行为可能会多次收到相同的通知 - 在合理的范围内支持这一点
	uint32_t MaxLoopCount = 2;
	if (PlayLength > 0.0f && FMath::Abs(DeltaTime) > PlayLength)
	{
		MaxLoopCount = FMath::Clamp(uint32_t(DesiredDeltaMove / PlayLength), 2, 1000);
	}

	for (uint32_t i = 0; i < MaxLoopCount; i++)
	{
		// Disable looping here. Advance to desired position, or beginning / end of animation
		// 此处禁用循环。前进到所需位置，或动画的开始/结束
		const ETypeAdvanceAnim AdvanceType = FAnimationRuntime::AdvanceTime(false, DesiredDeltaMove, CurrentPosition, PlayLength);

		// Verify position assumptions
		// 验证位置假设
		ensureMsgf(bPlayingBackwards ? (CurrentPosition <= PreviousPosition) : (CurrentPosition >= PreviousPosition), TEXT("in Animation %s(Skeleton %s) : bPlayingBackwards(%d), PreviousPosition(%0.2f), Current Position(%0.2f)"),
			*GetName(), *GetNameSafe(GetSkeleton()), bPlayingBackwards, PreviousPosition, CurrentPosition);

		GetAnimNotifiesFromDeltaPositions(PreviousPosition, CurrentPosition, NotifyContext);

		// If we've hit the end of the animation, and we're allowed to loop, keep going.
		// 如果我们已经到达动画的结尾，并且允许循环，请继续。
		if ((AdvanceType == ETAA_Finished) && NotifyContext.TickRecord && NotifyContext.TickRecord->bLooping)
		{
			const float ActualDeltaMove = (CurrentPosition - PreviousPosition);
			DesiredDeltaMove -= ActualDeltaMove;
			PreviousPosition = bPlayingBackwards ? GetPlayLength() : 0.f;
			CurrentPosition = PreviousPosition;
		}
		else
		{
			break;
		}
	}
}

void UAnimSequenceBase::GetAnimNotifiesFromDeltaPositions(const float& PreviousPosition, const float& CurrentPosition,  FAnimNotifyContext& NotifyContext) const
{
	// Early out if we have no notifies
	// 如果我们没有收到通知，请提早出发
	if (Notifies.Num() == 0)
	{
		return;
	}

	bool const bPlayingBackwards = (CurrentPosition < PreviousPosition);

	// If playing backwards, flip Min and Max.
	// 如果向后播放，请翻转“最小值”和“最大值”。
	if( bPlayingBackwards )
	{
		for (int32 NotifyIndex=0; NotifyIndex<Notifies.Num(); NotifyIndex++)
		{
			const FAnimNotifyEvent& AnimNotifyEvent = Notifies[NotifyIndex];
			const float NotifyStartTime = AnimNotifyEvent.GetTriggerTime();
			const float NotifyEndTime = AnimNotifyEvent.GetEndTriggerTime();

			if( (NotifyStartTime < PreviousPosition) && (NotifyEndTime >= CurrentPosition) )
			{
				if (NotifyContext.TickRecord)
				{
					NotifyContext.ActiveNotifies.Emplace(&AnimNotifyEvent, this, NotifyContext.TickRecord->MirrorDataTable);
					NotifyContext.ActiveNotifies.Top().GatherTickRecordData(*NotifyContext.TickRecord);
				}
				else
				{
					NotifyContext.ActiveNotifies.Emplace(&AnimNotifyEvent, this, nullptr);
				}

				const bool bHasFinished = CurrentPosition <= FMathf::Max(NotifyStartTime, 0.f);
				if (bHasFinished)
				{
					NotifyContext.ActiveNotifies.Top().AddContextData<UE::Anim::FAnimNotifyEndDataContext>(true);
				}
			}
		}
	}
	else
	{
		for (int32 NotifyIndex=0; NotifyIndex<Notifies.Num(); NotifyIndex++)
		{
			const FAnimNotifyEvent& AnimNotifyEvent = Notifies[NotifyIndex];
			const float NotifyStartTime = AnimNotifyEvent.GetTriggerTime();
			const float NotifyEndTime = AnimNotifyEvent.GetEndTriggerTime();

			// Note that if you arrive with zero delta time (CurrentPosition == PreviousPosition), only Notify States will be extracted
			// 请注意，如果您以零增量时间到达（CurrentPosition == PreviousPosition），则仅提取通知状态
			if( (NotifyStartTime <= CurrentPosition) && (NotifyEndTime > PreviousPosition) )
			{
				if (NotifyContext.TickRecord)
				{
					NotifyContext.ActiveNotifies.Emplace(&AnimNotifyEvent, this, NotifyContext.TickRecord->MirrorDataTable);
					NotifyContext.ActiveNotifies.Top().GatherTickRecordData(*NotifyContext.TickRecord); 
				}
				else
				{
					NotifyContext.ActiveNotifies.Emplace(&AnimNotifyEvent, this, nullptr);
				}

				const bool bHasFinished = CurrentPosition >= NotifyEndTime;
				if (bHasFinished)
				{
					NotifyContext.ActiveNotifies.Top().AddContextData<UE::Anim::FAnimNotifyEndDataContext>(true);
				}
			}
		}
	}
}

void UAnimSequenceBase::TickAssetPlayer(FAnimTickRecord& Instance, struct FAnimNotifyQueue& NotifyQueue, FAnimAssetTickContext& Context) const
{
	// Local copy of the asset player's time that is about to be updated.
	// 即将更新的资产播放器时间的本地副本。
	float CurrentTime = *(Instance.TimeAccumulator);
	
	if (Context.ShouldResyncToSyncGroup() && !Instance.bIsEvaluator)
	{
		// Synchronize the asset player time to the other sync group members when (re)joining the group
		// 当（重新）加入群组时，将资产播放器时间同步到其他同步群组成员
		CurrentTime = Context.GetAnimationPositionRatio() * GetPlayLength();
	}

	float PreviousTime = CurrentTime;
	float DeltaTime = 0.f;
	const float PlayRate = Instance.PlayRateMultiplier * RateScale;

	if (Context.IsLeader())
	{
		DeltaTime = PlayRate * Context.GetDeltaTime();

		// Prepare context before ticking.
		// 在勾选之前准备好上下文。
		Context.SetLeaderDelta(DeltaTime);
		Context.SetPreviousAnimationPositionRatio(PreviousTime / GetPlayLength());

		// Tick as leader using marked based syncing if possible, other fallback to normal ticking.
		// 如果可能的话，使用基于标记的同步来标记为领导者，其他回退到正常标记。
		if (DeltaTime != 0.f)
		{
			if (Instance.bCanUseMarkerSync && Context.CanUseMarkerPosition())
			{
				TickByMarkerAsLeader(*Instance.MarkerTickRecord, Context.MarkerTickContext, CurrentTime, PreviousTime, DeltaTime, Instance.bLooping, Instance.MirrorDataTable);
			}
			else
			{
				// Advance time
				// 提前时间
				FAnimationRuntime::AdvanceTime(Instance.bLooping, DeltaTime, CurrentTime, GetPlayLength());
				UE_LOG(LogAnimMarkerSync, Log, TEXT("Leader (%s) (normal advance)  - PreviousTime (%0.2f), CurrentTime (%0.2f), MoveDelta (%0.2f), Looping (%d) "), *GetName(), PreviousTime, CurrentTime, DeltaTime, Instance.bLooping ? 1 : 0);
			}
		}
		else if (Instance.bCanUseMarkerSync && Context.CanUseMarkerPosition() && !Instance.MarkerTickRecord->IsValid(Instance.bLooping))
		{
			// Re-compute marker indices since the asset's tick record is invalid. Get previous and next markers.
			// 由于资产的报价记录无效，因此重新计算标记指数。获取上一个和下一个标记。
			GetMarkerIndicesForTime(CurrentTime, Instance.bLooping, Context.MarkerTickContext.GetValidMarkerNames(), Instance.MarkerTickRecord->PreviousMarker, Instance.MarkerTickRecord->NextMarker);
		}

		// Update context's data after ticking.
		// 勾选后更新上下文的数据。
		Context.SetAnimationPositionRatio(CurrentTime / GetPlayLength());
	}
	else
	{
		// Follow the leader using marker based syncing if possible, otherwise fallback to length based syncing.
		// [翻译失败: Follow the leader using marker based syncing if possible, otherwise fallback to length based syncing.]
		if (Instance.bCanUseMarkerSync)
		{
			if (Context.CanUseMarkerPosition() && Context.MarkerTickContext.IsMarkerSyncStartValid())
			{
				TickByMarkerAsFollower(*Instance.MarkerTickRecord, Context.MarkerTickContext, CurrentTime, PreviousTime, Context.GetLeaderDelta(), Instance.bLooping, Instance.MirrorDataTable);
			}
			else
			{
				DeltaTime = PlayRate * Context.GetDeltaTime();
				
				// If leader is not valid, advance time as normal, do not jump position and pop.
				// 如果leader无效，则正常提前时间，不要跳转位置和pop。
				FAnimationRuntime::AdvanceTime(Instance.bLooping, DeltaTime, CurrentTime, GetPlayLength());
				UE_LOG(LogAnimMarkerSync, Log, TEXT("Follower (%s) (normal advance) - PreviousTime (%0.2f), CurrentTime (%0.2f), MoveDelta (%0.2f), Looping (%d) "), *GetName(), PreviousTime, CurrentTime, DeltaTime, Instance.bLooping ? 1 : 0);
			}
		}
		else
		{
			// Match group leader's last calculated anim position since we're not using marker based sync.
			// 由于我们不使用基于标记的同步，因此匹配组领导者最后计算的动画位置。
			PreviousTime = Context.GetPreviousAnimationPositionRatio() * GetPlayLength();
			CurrentTime = Context.GetAnimationPositionRatio() * GetPlayLength();
			
			UE_LOG(LogAnimMarkerSync, Log, TEXT("Follower (%s) (normalized position advance) - PreviousTime (%0.2f), CurrentTime (%0.2f), MoveDelta (%0.2f), Looping (%d) "), *GetName(), PreviousTime, CurrentTime, DeltaTime, Instance.bLooping ? 1 : 0);
		}
		
		if (CurrentTime != PreviousTime)
		{
			// Figure out delta time 
			// 计算出增量时间
			DeltaTime = CurrentTime - PreviousTime;
			
			// If we went against play rate, then loop around.
			// 如果我们反对播放率，则循环播放。
			if ((DeltaTime * PlayRate) < 0.f)
			{
				DeltaTime += FMath::Sign<float>(PlayRate) * GetPlayLength();
			}
		}
	}

	// Update the instance's TimeAccumulator after all side effects on the local copy of the asset player's time have been applied.
	// 在应用资产播放器时间的本地副本上的所有副作用后，更新实例的 TimeAccumulator。
	*(Instance.TimeAccumulator) = CurrentTime;
	
	// Capture the final adjusted delta time and previous frame time as an asset player record
	// 捕获最终调整的增量时间和前一帧时间作为资产播放器记录
	check(Instance.DeltaTimeRecord);
	Instance.DeltaTimeRecord->Set(PreviousTime, DeltaTime);

	// Allow asset to react right after its asset player has being ticked.
	// 允许资产在其资产播放器被勾选后立即做出反应。
	HandleAssetPlayerTickedInternal(Context, PreviousTime, DeltaTime, Instance, NotifyQueue);
}

void UAnimSequenceBase::TickByMarkerAsFollower(FMarkerTickRecord &Instance, FMarkerTickContext &MarkerContext, float& CurrentTime, float& OutPreviousTime, const float MoveDelta, const bool bLooping, const UMirrorDataTable* MirrorTable) const
{
	// Re-compute marker indices since the asset's tick record is invalid. Get previous and next markers.
	// 由于资产的报价记录无效，因此重新计算标记指数。获取上一个和下一个标记。
	if (!Instance.IsValid(bLooping))
	{
		GetMarkerIndicesForPosition(MarkerContext.GetMarkerSyncStartPosition(), bLooping, Instance.PreviousMarker, Instance.NextMarker, CurrentTime, MirrorTable);
	}

	// Keep track the asset's previous time, since its current time is about to be modified.
	// 跟踪资产的先前时间，因为其当前时间即将被修改。
	OutPreviousTime = CurrentTime;

	// Tick and update as follower.
	// 勾选并更新为关注者。
	AdvanceMarkerPhaseAsFollower(MarkerContext, MoveDelta, bLooping, CurrentTime, Instance.PreviousMarker, Instance.NextMarker, MirrorTable);

	UE_LOG(LogAnimMarkerSync, Log, TEXT("Follower (%s) (TickByMarker) PreviousTime(%0.2f) CurrentTime(%0.2f) MoveDelta(%0.2f) Looping(%d) %s"), *GetName(), OutPreviousTime, CurrentTime, MoveDelta, bLooping ? 1 : 0, *MarkerContext.ToString());
}

void UAnimSequenceBase::TickByMarkerAsLeader(FMarkerTickRecord& Instance, FMarkerTickContext& MarkerContext, float& CurrentTime, float& OutPreviousTime, const float MoveDelta, const bool bLooping, const UMirrorDataTable* MirrorTable) const
{
	// Re-compute marker indices since the asset's tick record is invalid. Get previous and next markers.
	// 由于资产的报价记录无效，因此重新计算标记指数。获取上一个和下一个标记。
	if (!Instance.IsValid(bLooping))
	{
		if (MarkerContext.IsMarkerSyncStartValid())
		{
			GetMarkerIndicesForPosition(MarkerContext.GetMarkerSyncStartPosition(), bLooping, Instance.PreviousMarker, Instance.NextMarker, CurrentTime, MirrorTable);
		}
		else
		{
			GetMarkerIndicesForTime(CurrentTime, bLooping, MarkerContext.GetValidMarkerNames(), Instance.PreviousMarker, Instance.NextMarker);
		}
	}

	// Store the sync anim position BEFORE the asset has being ticked.
	// 在勾选资源之前存储同步动画位置。
	MarkerContext.SetMarkerSyncStartPosition(GetMarkerSyncPositionFromMarkerIndicies(Instance.PreviousMarker.MarkerIndex, Instance.NextMarker.MarkerIndex, CurrentTime, MirrorTable));

	// Keep track the asset's previous time, since its current time is about to be modified.
	// 跟踪资产的先前时间，因为其当前时间即将被修改。
	OutPreviousTime = CurrentTime;

	// Advance as leader.
	// 作为领导者前进。
	AdvanceMarkerPhaseAsLeader(bLooping, MoveDelta, MarkerContext.GetValidMarkerNames(), CurrentTime, Instance.PreviousMarker, Instance.NextMarker, MarkerContext.MarkersPassedThisTick, MirrorTable);

	// Store the sync anim position AFTER the asset has being ticked.
	// 勾选资产后存储同步动画位置。
	MarkerContext.SetMarkerSyncEndPosition(GetMarkerSyncPositionFromMarkerIndicies(Instance.PreviousMarker.MarkerIndex, Instance.NextMarker.MarkerIndex, CurrentTime, MirrorTable));

	UE_LOG(LogAnimMarkerSync, Log, TEXT("Leader (%s) (TickByMarker) PreviousTime(%0.2f) CurrentTime(%0.2f) MoveDelta(%0.2f) Looping(%d) %s"), *GetName(), OutPreviousTime, CurrentTime, MoveDelta, bLooping ? 1 : 0, *MarkerContext.ToString());
}

bool CanNotifyUseTrack(const FAnimNotifyTrack& Track, const FAnimNotifyEvent& Notify)
{
	for (const FAnimNotifyEvent* Event : Track.Notifies)
	{
		if (FMath::IsNearlyEqual(Event->GetTime(), Notify.GetTime()))
		{
			return false;
		}
	}
	return true;
}

FAnimNotifyTrack& AddNewTrack(TArray<FAnimNotifyTrack>& Tracks)
{
	const int32 Index = Tracks.Add(FAnimNotifyTrack(*FString::FromInt(Tracks.Num() + 1), FLinearColor::White));
	return Tracks[Index];
}

void UAnimSequenceBase::RefreshCacheData()
{
	SortNotifies();

#if WITH_EDITOR
	for (int32 TrackIndex = 0; TrackIndex < AnimNotifyTracks.Num(); ++TrackIndex)
	{
		AnimNotifyTracks[TrackIndex].Notifies.Empty();
	}

	for (FAnimNotifyEvent& Notify : Notifies)
	{
		// Handle busted track indices
		// 处理损坏的轨道索引
		if (!AnimNotifyTracks.IsValidIndex(Notify.TrackIndex))
		{
			// This really shouldn't happen (unless we are a cooked asset), but try to handle it
			// 这确实不应该发生（除非我们是熟资产），但尝试处理它
			ensureMsgf(GetOutermost()->bIsCookedForEditor, TEXT("AnimNotifyTrack: Anim (%s) has notify (%s) with track index (%i) that does not exist"), *GetFullName(), *Notify.NotifyName.ToString(), Notify.TrackIndex);

			// Don't create lots of extra tracks if we are way off supporting this track
			// 如果我们距离支持该曲目还很远，请不要创建大量额外的曲目
			if (Notify.TrackIndex < 0 || Notify.TrackIndex > 20)
			{
				Notify.TrackIndex = 0;
			}

			while (!AnimNotifyTracks.IsValidIndex(Notify.TrackIndex))
			{
				AddNewTrack(AnimNotifyTracks);
			}
		}

		// Handle overlapping notifies
		// 处理重叠通知
		FAnimNotifyTrack* TrackToUse = nullptr;
		int32 TrackIndexToUse = INDEX_NONE;
		for (int32 TrackOffset = 0; TrackOffset < AnimNotifyTracks.Num(); ++TrackOffset)
		{
			const int32 TrackIndex = (Notify.TrackIndex + TrackOffset) % AnimNotifyTracks.Num();
			if (CanNotifyUseTrack(AnimNotifyTracks[TrackIndex], Notify))
			{
				TrackToUse = &AnimNotifyTracks[TrackIndex];
				TrackIndexToUse = TrackIndex;
				break;
			}
		}

		if (TrackToUse == nullptr)
		{
			TrackToUse = &AddNewTrack(AnimNotifyTracks);
			TrackIndexToUse = AnimNotifyTracks.Num() - 1;
		}

		check(TrackToUse);
		check(TrackIndexToUse != INDEX_NONE);

		Notify.TrackIndex = TrackIndexToUse;
		TrackToUse->Notifies.Add(&Notify);
	}

	// this is a separate loop of checkin if they contains valid notifies
	// 如果它们包含有效通知，则这是一个单独的签入循环
	for (int32 NotifyIndex = 0; NotifyIndex < Notifies.Num(); ++NotifyIndex)
	{
		const FAnimNotifyEvent& Notify = Notifies[NotifyIndex];
		// make sure if they can be placed in editor
		// 确保它们是否可以放入编辑器中
		if (Notify.Notify)
		{
			if (Notify.Notify->CanBePlaced(this) == false)
			{
				static FName NAME_LoadErrors("LoadErrors");
				FMessageLog LoadErrors(NAME_LoadErrors);

				TSharedRef<FTokenizedMessage> Message = LoadErrors.Error();
				Message->AddToken(FTextToken::Create(LOCTEXT("InvalidAnimNotify1", "The Animation ")));
				Message->AddToken(FAssetNameToken::Create(GetPathName(), FText::FromString(GetNameSafe(this))));
				Message->AddToken(FTextToken::Create(LOCTEXT("InvalidAnimNotify2", " contains invalid notify ")));
				Message->AddToken(FAssetNameToken::Create(Notify.Notify->GetPathName(), FText::FromString(GetNameSafe(Notify.Notify))));
				LoadErrors.Open();
			}
		}

		if (Notify.NotifyStateClass)
		{
			if (Notify.NotifyStateClass->CanBePlaced(this) == false)
			{
				static FName NAME_LoadErrors("LoadErrors");
				FMessageLog LoadErrors(NAME_LoadErrors);

				TSharedRef<FTokenizedMessage> Message = LoadErrors.Error();
				Message->AddToken(FTextToken::Create(LOCTEXT("InvalidAnimNotify1", "The Animation ")));
				Message->AddToken(FAssetNameToken::Create(GetPathName(), FText::FromString(GetNameSafe(this))));
				Message->AddToken(FTextToken::Create(LOCTEXT("InvalidAnimNotify2", " contains invalid notify ")));
				Message->AddToken(FAssetNameToken::Create(Notify.NotifyStateClass->GetPathName(), FText::FromString(GetNameSafe(Notify.NotifyStateClass))));
				LoadErrors.Open();
			}
		}
	}
	// notification broadcast
	// 通知广播
	OnNotifyChanged.Broadcast();
#endif //WITH_EDITOR
}

int32 UAnimSequenceBase::GetNumberOfSampledKeys() const
{
	return GetSamplingFrameRate().AsFrameTime(GetPlayLength()).RoundToFrame().Value;
}

FFrameRate UAnimSequenceBase::GetSamplingFrameRate() const
{
	static const FFrameRate DefaultFrameRate = UAnimationSettings::Get()->GetDefaultFrameRate();
	return DefaultFrameRate;
}

float UAnimSequenceBase::GetTimeAtFrame(const int32 Frame) const
{
	return FMath::Clamp((float)GetSamplingFrameRate().AsSeconds(Frame), 0.f, GetPlayLength());
}

#if WITH_EDITOR
void UAnimSequenceBase::InitializeNotifyTrack()
{
	if ( AnimNotifyTracks.Num() == 0 ) 
	{
		AnimNotifyTracks.Add(FAnimNotifyTrack(TEXT("1"), FLinearColor::White ));
	}
}

int32 UAnimSequenceBase::GetFrameAtTime(const float Time) const
{
	return FMath::Clamp(GetSamplingFrameRate().AsFrameTime(Time).RoundToFrame().Value, 0, GetNumberOfSampledKeys() - 1);
}

void UAnimSequenceBase::RegisterOnNotifyChanged(const FOnNotifyChanged& Delegate)
{
	OnNotifyChanged.Add(Delegate);
}
void UAnimSequenceBase::UnregisterOnNotifyChanged(FDelegateUserObject Unregister)
{
	OnNotifyChanged.RemoveAll(Unregister);
}

void UAnimSequenceBase::ClampNotifiesAtEndOfSequence()
{
	const float NotifyClampTime = GetPlayLength();
	for(int i = 0; i < Notifies.Num(); ++ i)
	{
		if(Notifies[i].GetTime() >= GetPlayLength())
		{
			Notifies[i].SetTime(NotifyClampTime);
			Notifies[i].TriggerTimeOffset = GetTriggerTimeOffsetForType(EAnimEventTriggerOffsets::OffsetBefore);
		}
	}
}

EAnimEventTriggerOffsets::Type UAnimSequenceBase::CalculateOffsetForNotify(float NotifyDisplayTime) const
{
	if(NotifyDisplayTime == 0.f)
	{
		return EAnimEventTriggerOffsets::OffsetAfter;
	}
	else if(NotifyDisplayTime == GetPlayLength())
	{
		return EAnimEventTriggerOffsets::OffsetBefore;
	}
	return EAnimEventTriggerOffsets::NoOffset;
}

void UAnimSequenceBase::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
	PRAGMA_DISABLE_DEPRECATION_WARNINGS;
	Super::GetAssetRegistryTags(OutTags);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS;
}

void UAnimSequenceBase::GetAssetRegistryTags(FAssetRegistryTagsContext Context) const
{
	Super::GetAssetRegistryTags(Context);

	// Add notify IDs to a tag list, or a delimiter if we have no notifies.
	// 将通知 ID 添加到标签列表，如果没有通知，则添加分隔符。
	// The delimiter is necessary so we can distinguish between data with no curves and old data, as the asset registry
	// 分隔符是必要的，这样我们就可以区分没有曲线的数据和旧数据，就像资产注册表一样
	// strips tags that have empty values 
	// 删除具有空值的标签
	FString NotifyList = USkeleton::AnimNotifyTagDelimiter;
	for(auto Iter=Notifies.CreateConstIterator(); Iter; ++Iter)
	{
		// only add if not BP anim notify since they're handled separately
		// 仅添加 BP 动画通知，因为它们是单独处理的
		if(Iter->IsBlueprintNotify() == false)
		{
			NotifyList += FString::Printf(TEXT("%s%s"), *Iter->NotifyName.ToString(), *USkeleton::AnimNotifyTagDelimiter);
		}
	}
	
	Context.AddTag(FAssetRegistryTag(USkeleton::AnimNotifyTag, NotifyList, FAssetRegistryTag::TT_Hidden));

	// Add curve IDs to a tag list, or a delimiter if we have no curves.
	// 将曲线 ID 添加到标签列表，如果没有曲线，则添加分隔符。
	// The delimiter is necessary so we can distinguish between data with no curves and old data, as the asset registry
	// 分隔符是必要的，这样我们就可以区分没有曲线的数据和旧数据，就像资产注册表一样
	// strips tags that have empty values 
	// 删除具有空值的标签
	FString CurveNameList = USkeleton::CurveTagDelimiter;
	if (DataModelInterface.GetObject())
	{
		for(const FFloatCurve& Curve : DataModelInterface->GetFloatCurves())
		{
			CurveNameList += FString::Printf(TEXT("%s%s"), *Curve.GetName().ToString(), *USkeleton::CurveTagDelimiter);
		}
	}
	Context.AddTag(FAssetRegistryTag(USkeleton::CurveNameTag, CurveNameList, FAssetRegistryTag::TT_Hidden));

	if (DataModelInterface.GetObject())
	{
		FAnimationAttributeIdentifierArray AttributeIdentifierArray;
		AttributeIdentifierArray.AttributeIdentifiers.Reserve(DataModelInterface->GetNumberOfAttributes());
		
		for (const FAnimatedBoneAttribute& BoneAttribute : DataModelInterface->GetAttributes())
		{
			AttributeIdentifierArray.AttributeIdentifiers.Add(BoneAttribute.Identifier);
		}

		FString AttributeList;
		FAnimationAttributeIdentifierArray::StaticStruct()->ExportText(AttributeList, &AttributeIdentifierArray.AttributeIdentifiers, nullptr, nullptr, PPF_None, nullptr);
		
		Context.AddTag(FAssetRegistryTag(USkeleton::AttributeTag, AttributeList, FAssetRegistryTag::TT_Hidden));
	}
}

uint8* UAnimSequenceBase::FindNotifyPropertyData(int32 NotifyIndex, FArrayProperty*& ArrayProperty)
{
	// initialize to nullptr
	// 初始化为 nullptr
	ArrayProperty = nullptr;

	if(Notifies.IsValidIndex(NotifyIndex))
	{
		return FindArrayProperty(TEXT("Notifies"), ArrayProperty, NotifyIndex);
	}
	return nullptr;
}

uint8* UAnimSequenceBase::FindArrayProperty(const TCHAR* PropName, FArrayProperty*& ArrayProperty, int32 ArrayIndex)
{
	// find Notifies property start point
	// find 通知属性起点
	FProperty* Property = FindFProperty<FProperty>(GetClass(), PropName);

	// found it and if it is array
	// 找到它，如果它是数组
	if (Property && Property->IsA(FArrayProperty::StaticClass()))
	{
		// find Property Value from UObject we got
		// 从我们得到的UObject中找到属性值
		uint8* PropertyValue = Property->ContainerPtrToValuePtr<uint8>(this);

		// it is array, so now get ArrayHelper and find the raw ptr of the data
		// 它是数组，所以现在获取 ArrayHelper 并找到数据的原始 ptr
		ArrayProperty = CastFieldChecked<FArrayProperty>(Property);
		FScriptArrayHelper ArrayHelper(ArrayProperty, PropertyValue);

		if (ArrayProperty->Inner && ArrayIndex < ArrayHelper.Num())
		{
			//Get property data based on selected index
			//根据选定的索引获取属性数据
			return ArrayHelper.GetRawPtr(ArrayIndex);
		}
	}
	return nullptr;
}

void UAnimSequenceBase::RefreshParentAssetData()
{
	Super::RefreshParentAssetData();

	check(ParentAsset);

	UAnimSequenceBase* ParentSeqBase = CastChecked<UAnimSequenceBase>(ParentAsset);

	RateScale = ParentSeqBase->RateScale;

	ValidateModel();
	Controller->OpenBracket(LOCTEXT("RefreshParentAssetData_Bracket", "Refreshing Parent Asset Data"));
	{	
		const IAnimationDataModel* ParentDataModel = ParentSeqBase->GetDataModel();
		
		if (!FMath::IsNearlyEqual(GetPlayLength(), ParentSeqBase->GetPlayLength()))
		{
			const FFrameRate& ParentFrameRate = ParentDataModel->GetFrameRate();
			const FFrameRate& ChildFrameRate = GetDataModelInterface()->GetFrameRate();

			if (ChildFrameRate.IsMultipleOf(ParentFrameRate) || ChildFrameRate.IsFactorOf(ParentFrameRate))
			{
				// Ensure that play lengths are the same by converting the parent number of frames into the child's frame rate
				// 通过将父帧数转换为子帧速率来确保播放长度相同
				FFrameTime ChildNumFrames = FFrameRate::TransformTime(ParentFrameRate.AsFrameTime(ParentSeqBase->GetPlayLength()), ParentFrameRate, ChildFrameRate);
				Controller->SetNumberOfFrames(ChildNumFrames.RoundToFrame());
			}
			else
			{
				UE_LOG(LogTemp, Display,
						TEXT("Child %s %s has incompatible frame rate with Parent %s. Child: %s, Parent: %s. Reimport the child animation with a compatible frame rate."),
					*ParentSeqBase->GetClass()->GetName(), 
					*GetName(), 
					*ParentSeqBase->GetName(), 
					*ChildFrameRate.ToPrettyText().ToString(), 
					*ParentFrameRate.ToPrettyText().ToString());
			}
		}
		
		Controller->RemoveAllCurvesOfType(ERawCurveTrackTypes::RCT_Float);
		for (const FFloatCurve& FloatCurve : ParentDataModel->GetFloatCurves())
		{
			const FAnimationCurveIdentifier CurveId(FloatCurve.GetName(), ERawCurveTrackTypes::RCT_Float);
			Controller->AddCurve(CurveId, FloatCurve.GetCurveTypeFlags());
			Controller->SetCurveKeys(CurveId, FloatCurve.FloatCurve.GetConstRefOfKeys());
		}

		Controller->RemoveAllCurvesOfType(ERawCurveTrackTypes::RCT_Transform);
		for (const FTransformCurve& TransformCurve : ParentDataModel->GetTransformCurves())
		{
			const FAnimationCurveIdentifier CurveId(TransformCurve.GetName(), ERawCurveTrackTypes::RCT_Transform);
			Controller->AddCurve(CurveId, TransformCurve.GetCurveTypeFlags());

			// Set each individual channel rich curve keys, to account for any custom tangents etc.
			// 设置每个单独通道的丰富曲线键，以考虑任何自定义切线等。
			for (int32 SubCurveIndex = 0; SubCurveIndex < 3; ++SubCurveIndex)
			{
				const ETransformCurveChannel Channel = static_cast<ETransformCurveChannel>(SubCurveIndex);
				const FVectorCurve* VectorCurve = TransformCurve.GetVectorCurveByIndex(SubCurveIndex);
				for (int32 ChannelIndex = 0; ChannelIndex < 3; ++ChannelIndex)
				{
					const EVectorCurveChannel Axis = static_cast<EVectorCurveChannel>(ChannelIndex);
					FAnimationCurveIdentifier TargetCurveIdentifier = CurveId;
					UAnimationCurveIdentifierExtensions::GetTransformChildCurveIdentifier(TargetCurveIdentifier, Channel, Axis);
					Controller->SetCurveKeys(TargetCurveIdentifier, VectorCurve->FloatCurves[ChannelIndex].GetConstRefOfKeys());
				}
			}
		}
	}
	Controller->CloseBracket();

	// should do deep copy because notify contains outer
	// 应该进行深复制，因为通知包含外部
	Notifies = ParentSeqBase->Notifies;
	// update notify
	// [翻译失败: update notify]
	for (int32 NotifyIdx = 0; NotifyIdx < Notifies.Num(); ++NotifyIdx)
	{
		FAnimNotifyEvent& NotifyEvent = Notifies[NotifyIdx];
		if (NotifyEvent.Notify)
		{
			class UAnimNotify* NewNotifyClass = DuplicateObject(NotifyEvent.Notify, this);
			NotifyEvent.Notify = NewNotifyClass;
		}
		if (NotifyEvent.NotifyStateClass)
		{
			class UAnimNotifyState* NewNotifyStateClass = DuplicateObject(NotifyEvent.NotifyStateClass, this);
			NotifyEvent.NotifyStateClass = (NewNotifyStateClass);
		}

		NotifyEvent.Link(this, NotifyEvent.GetTime(), NotifyEvent.GetSlotIndex());
		NotifyEvent.EndLink.Link(this, NotifyEvent.GetTime() + NotifyEvent.Duration, NotifyEvent.GetSlotIndex());
	}
#if WITH_EDITORONLY_DATA
	// if you change Notifies array, this will need to be rebuilt
	// [翻译失败: if you change Notifies array, this will need to be rebuilt]
	AnimNotifyTracks = ParentSeqBase->AnimNotifyTracks;

	// fix up notify links, brute force and ugly code
	// 修复通知链接、暴力破解和丑陋的代码
	for (FAnimNotifyTrack& Track : AnimNotifyTracks)
	{
		for (FAnimNotifyEvent*& Notify : Track.Notifies)
		{
			for (int32 ParentNotifyIdx = 0; ParentNotifyIdx < ParentSeqBase->Notifies.Num(); ++ParentNotifyIdx)
			{
				if (Notify == &ParentSeqBase->Notifies[ParentNotifyIdx])
				{
					Notify = &Notifies[ParentNotifyIdx];
					break;
				}
			}
		}
	}
#endif // WITH_EDITORONLY_DATA

}
#endif	//WITH_EDITOR


void UAnimSequenceBase::EvaluateCurveData(FBlendedCurve& OutCurve, const FAnimExtractContext& AnimExtractContext, bool bForceUseRawData) const
{
	CSV_SCOPED_TIMING_STAT(Animation, EvaluateCurveData);

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	RawCurveData.EvaluateCurveData(OutCurve, AnimExtractContext.CurrentTime);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

float UAnimSequenceBase::EvaluateCurveData(FName CurveName, const FAnimExtractContext& AnimExtractContext, bool bForceUseRawData) const
{
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	const FFloatCurve* Curve = (const FFloatCurve*)RawCurveData.GetCurveData(CurveName, ERawCurveTrackTypes::RCT_Float);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	return Curve != nullptr ? Curve->Evaluate(AnimExtractContext.CurrentTime) : 0.0f;
}

const FRawCurveTracks& UAnimSequenceBase::GetCurveData() const
{
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	return RawCurveData;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

bool UAnimSequenceBase::HasCurveData(FName CurveName, bool bForceUseRawData) const
{
#if WITH_EDITOR
	if (IsDataModelValid())
	{
		return DataModelInterface->FindFloatCurve(FAnimationCurveIdentifier(CurveName, ERawCurveTrackTypes::RCT_Float)) != nullptr;
	}	
#endif
	
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	return RawCurveData.GetCurveData(CurveName) != nullptr;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

void UAnimSequenceBase::Serialize(FArchive& Ar)
{
	Ar.UsingCustomVersion(FFrameworkObjectVersion::GUID);
	Ar.UsingCustomVersion(FFortniteMainBranchObjectVersion::GUID);
	Ar.UsingCustomVersion(FUE5MainStreamObjectVersion::GUID);

	Super::Serialize(Ar);

	if (GetLinkerCustomVersion(FUE5MainStreamObjectVersion::GUID) >= FUE5MainStreamObjectVersion::AnimationDataModelInterface_BackedOut &&
		GetLinkerCustomVersion(FUE5MainStreamObjectVersion::GUID) < FUE5MainStreamObjectVersion::BackoutAnimationDataModelInterface)
	{
		UE_LOG(LogAnimation, Fatal, TEXT("This package (%s) was saved with a version that had to be backed out and is no longer able to be loaded."), *GetPathNameSafe(this));
	}

	// fix up version issue and so on
	// 修复版本问题等
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	RawCurveData.PostSerializeFixup(Ar);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

void UAnimSequenceBase::HandleAssetPlayerTickedInternal(FAnimAssetTickContext &Context, const float PreviousTime, const float MoveDelta, const FAnimTickRecord &Instance, struct FAnimNotifyQueue& NotifyQueue) const
{
	// Harvest and record notifies
	// 收获和记录通知
	FAnimNotifyContext NotifyContext(Instance);
	GetAnimNotifies(PreviousTime, MoveDelta, NotifyContext);
	NotifyQueue.AddAnimNotifies(Context.ShouldGenerateNotifies(), NotifyContext.ActiveNotifies, Instance.EffectiveBlendWeight);
}

#if WITH_EDITOR
void UAnimSequenceBase::OnModelModified(const EAnimDataModelNotifyType& NotifyType, IAnimationDataModel* Model, const FAnimDataModelNotifPayload& Payload)
{
	NotifyCollector.Handle(NotifyType);

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	InitializeNotifyTrack();
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	auto HandleNumberOfFramesChange = [&](int32 NewNumFrames, int32 OldNumFrames, int32 Frame0, int32 Frame1)
	{
		// Do not handle changes during model population, or undo-redo (notifies are transacted so will be restored/set, so only handle in case of, initial, user interaction)
		// 不要在模型填充或撤消重做期间处理更改（通知已处理，因此将被恢复/设置，因此仅在初始用户交互的情况下处理）
		if (bPopulatingDataModel || GIsTransacting)
		{
			return;
		}

		const FFrameRate& ModelFrameRate = DataModelInterface->GetFrameRate();
		const float T0 = ModelFrameRate.AsSeconds(Frame0);
		const float T1 = ModelFrameRate.AsSeconds(Frame1);
		const float NewLength = ModelFrameRate.AsSeconds(NewNumFrames);
		
		const float StartTime = ModelFrameRate.AsSeconds(Frame0);
		const float EndTime = ModelFrameRate.AsSeconds(Frame1);

		// Total time value for frames that were removed
		// 已删除帧的总时间值
		const float Duration = T1 - T0;

		if (NewNumFrames > OldNumFrames)
		{
			const float InsertTime = T0;
			for (FAnimNotifyEvent& Notify : Notifies)
			{
				// Proportional notifies don't need to be adjusted
				// 比例通知无需调整
				if (Notify.GetLinkMethod() == EAnimLinkMethod::Proportional)
				{
					continue;
				}

				float CurrentTime = Notify.GetTime();
				float NewDuration = Notify.GetDuration();

				// if state, make sure to adjust timings
				// 如果有的话，请务必调整时间
				if (Notify.NotifyStateClass)
				{
					const float NotifyDuration = Notify.GetDuration();
					const float NotifyEnd = CurrentTime + NotifyDuration;

					TRange<float> NotifyRange(CurrentTime, TRange<float>::BoundsType(CurrentTime+NotifyDuration));
					TRange<float> InsertionRange(StartTime, EndTime);

					// If insertion range or start time falls inside of notify range, add length to notify duration
					// 如果插入范围或开始时间落在通知范围内，则添加长度以通知持续时间
					if (NotifyRange.Contains(InsertionRange) || NotifyRange.Contains(StartTime))
					{
						// Removal range falls entirely inside of notify state duration
						// 移除范围完全在通知状态持续时间内
						const float InsertDuration = InsertionRange.GetUpperBound().GetValue() - InsertionRange.GetLowerBound().GetValue();
						NewDuration += InsertDuration;
					}					
				}

				// Shift out notify by the time allotted for the inserted keys
				// 按分配给插入键的时间移出通知
				if (CurrentTime >= InsertTime)
				{
					CurrentTime += Duration;
				}

				// Clamps against the sequence length, ensuring that the notify does not end up outside of the playback bounds
				// 限制序列长度，确保通知不会超出播放范围
				const float ClampedCurrentTime = FMath::Clamp(CurrentTime, 0.f, NewLength);

				Notify.Link(this, ClampedCurrentTime);
				Notify.SetDuration(NewDuration);

				if (ClampedCurrentTime == 0.f)
				{
					Notify.TriggerTimeOffset = GetTriggerTimeOffsetForType(EAnimEventTriggerOffsets::OffsetAfter);
				}
				else if (ClampedCurrentTime == NewLength)
				{
					Notify.TriggerTimeOffset = GetTriggerTimeOffsetForType(EAnimEventTriggerOffsets::OffsetBefore);
				}
			}
		}
		else if (NewNumFrames < OldNumFrames)
		{
			// re-locate notifies
			// 重新定位通知
			for (FAnimNotifyEvent& Notify : Notifies)
			{
				// Proportional notifies don't need to be adjusted
				// 比例通知无需调整
				if (Notify.GetLinkMethod() == EAnimLinkMethod::Proportional)
				{
					continue;
				}

				float CurrentTime = Notify.GetTime();
				float NewDuration = 0.f;
				
				// if state, make sure to adjust end time
				// 如果有状态，请确保调整结束时间
				if (Notify.NotifyStateClass)
				{
					const float NotifyDuration = Notify.GetDuration();
					const float NotifyEnd = CurrentTime + NotifyDuration;
					NewDuration = NotifyDuration;

					TRange<float> NotifyRange(CurrentTime, TRange<float>::BoundsType(CurrentTime+NotifyDuration));					
					TRange<float> RemovalRange(StartTime, EndTime);

					// If Notify is inside of the trimmed time frame, zero out the duration
					// 如果“通知”在修剪的时间范围内，则将持续时间清零
					if (RemovalRange.Contains(NotifyRange))
					{
						// small number @todo see if there is define for this
						// 少量@todo 看看是否有对此的定义
						NewDuration = DataModelInterface->GetFrameRate().AsInterval();
					}
					else if (NotifyRange.Contains(RemovalRange))
					{
						// Removal range falls entirely inside of notify state duration
						// 移除范围完全在通知状态持续时间内
						const float RemovalDuration = RemovalRange.GetUpperBound().GetValue() - RemovalRange.GetLowerBound().GetValue();
						NewDuration -= RemovalDuration;
					}
					// If Notify overlaps trimmed time frame, remove trimmed duration
					// 如果通知与修剪的时间范围重叠，请删除修剪的持续时间
					else if (RemovalRange.Overlaps(NotifyRange))
					{
						TRange<float> Overlap = TRange<float>::Intersection(NotifyRange, RemovalRange);
						if (Overlap.GetLowerBoundValue() > CurrentTime)
						{
							//Removing from back-end of notify
							//从通知后端删除
							NewDuration = Overlap.GetLowerBoundValue() - CurrentTime;
						}
						else if (Overlap.GetUpperBoundValue() < NotifyEnd)
						{
							// Removing from front-end of notify
							// 从通知前端删除
							NewDuration = NotifyEnd - Overlap.GetUpperBoundValue();
						}
					}

					NewDuration = FMath::Max(NewDuration, (float)DataModelInterface->GetFrameRate().AsInterval());
				}

				if (CurrentTime >= StartTime && CurrentTime <= EndTime)
				{
					CurrentTime = StartTime;
				}
				else if (CurrentTime > EndTime)
				{
					CurrentTime -= Duration;
				}

				const float ClampedCurrentTime = FMath::Clamp(CurrentTime, 0.f, NewLength);

				Notify.Link(this, ClampedCurrentTime);
				Notify.SetDuration(NewDuration);

				if (ClampedCurrentTime == 0.f)
				{
					Notify.TriggerTimeOffset = GetTriggerTimeOffsetForType(EAnimEventTriggerOffsets::OffsetAfter);
				}
				else if (ClampedCurrentTime == NewLength)
				{
					Notify.TriggerTimeOffset = GetTriggerTimeOffsetForType(EAnimEventTriggerOffsets::OffsetBefore);
				}
			}
		}
	};

	// Copy any float/transform curves from the model into RawCurveData, as it is used at runtime for AnimMontage/Composite(s)
	// 将模型中的任何浮动/变换曲线复制到 RawCurveData，因为它在运行时用于 AnimMontage/Composite(s)
	auto CopyCurvesFromModel = [this]()
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		RawCurveData.FloatCurves = DataModelInterface->GetCurveData().FloatCurves;
		RawCurveData.TransformCurves = DataModelInterface->GetCurveData().TransformCurves;
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	};

	bool bShouldMarkPackageDirty = !FUObjectThreadContext::Get().IsRoutingPostLoad && NotifyType != EAnimDataModelNotifyType::BracketOpened;

	switch (NotifyType)
	{
		case EAnimDataModelNotifyType::SequenceLengthChanged:
		{
			const FSequenceLengthChangedPayload& TypedPayload = Payload.GetPayload<FSequenceLengthChangedPayload>();
			const FFrameNumber PreviousNumberOfFrames = TypedPayload.PreviousNumberOfFrames;
			const int32 CurrentNumberOfFrames = Model->GetNumberOfFrames();

			PRAGMA_DISABLE_DEPRECATION_WARNINGS
			SequenceLength = Model->GetFrameRate().AsSeconds(CurrentNumberOfFrames);
			PRAGMA_ENABLE_DEPRECATION_WARNINGS

			HandleNumberOfFramesChange(CurrentNumberOfFrames, PreviousNumberOfFrames.Value, TypedPayload.Frame0.Value, TypedPayload.Frame1.Value);

			// Ensure that we only clamp the notifies at the end of a bracket
			// 确保我们只将通知夹在括号末尾
			if (NotifyCollector.IsNotWithinBracket())
			{
				ClampNotifiesAtEndOfSequence();
				CopyCurvesFromModel();
			}
			
			break;
		}

		case EAnimDataModelNotifyType::CurveAdded:
		case EAnimDataModelNotifyType::CurveChanged:
		case EAnimDataModelNotifyType::CurveRemoved:
		case EAnimDataModelNotifyType::CurveFlagsChanged:
		case EAnimDataModelNotifyType::CurveScaled:
		{
			if (NotifyCollector.IsNotWithinBracket())
			{
				CopyCurvesFromModel();
			}
			break;
		}
		
		case EAnimDataModelNotifyType::CurveRenamed:
		{
			const FCurveRenamedPayload& TypedPayload = Payload.GetPayload<FCurveRenamedPayload>();
			FAnimCurveBase* CurvePtr = [this, Identifier=TypedPayload.Identifier]() -> FAnimCurveBase*
			{
				if (Identifier.CurveType == ERawCurveTrackTypes::RCT_Float)
				{
					PRAGMA_DISABLE_DEPRECATION_WARNINGS
					return RawCurveData.FloatCurves.FindByPredicate([CurveName=Identifier.CurveName](FFloatCurve& Curve)
	                {
	                    return Curve.GetName() == CurveName;
	                });
					PRAGMA_ENABLE_DEPRECATION_WARNINGS
				}
				else if (Identifier.CurveType == ERawCurveTrackTypes::RCT_Transform)
				{
					PRAGMA_DISABLE_DEPRECATION_WARNINGS
					return RawCurveData.TransformCurves.FindByPredicate([CurveName=Identifier.CurveName](FTransformCurve& Curve)
                    {
                        return Curve.GetName() == CurveName;
                    });
					PRAGMA_ENABLE_DEPRECATION_WARNINGS
				}

				return nullptr;
			}();
				
			if (CurvePtr)
			{
				CurvePtr->SetName(TypedPayload.NewIdentifier.CurveName);
			}
			break;
		}

		case EAnimDataModelNotifyType::Populated:
		{
			const float CurrentSequenceLength = Model->GetPlayLength();
			PRAGMA_DISABLE_DEPRECATION_WARNINGS
			SequenceLength = CurrentSequenceLength;
			PRAGMA_ENABLE_DEPRECATION_WARNINGS

			if (NotifyCollector.IsNotWithinBracket())
			{
				ClampNotifiesAtEndOfSequence();
				CopyCurvesFromModel();
			}

			break;
		}

		case EAnimDataModelNotifyType::BracketClosed:
		{
			if (NotifyCollector.IsNotWithinBracket())
			{
				const auto CurveCopyNotifies = { EAnimDataModelNotifyType::CurveAdded, EAnimDataModelNotifyType::CurveChanged, EAnimDataModelNotifyType::CurveRemoved, EAnimDataModelNotifyType::CurveFlagsChanged, EAnimDataModelNotifyType::CurveScaled, EAnimDataModelNotifyType::Populated, EAnimDataModelNotifyType::Reset };

				const auto LengthChangingNotifies = { EAnimDataModelNotifyType::SequenceLengthChanged, EAnimDataModelNotifyType::FrameRateChanged, EAnimDataModelNotifyType::Reset, EAnimDataModelNotifyType::Populated };

				bShouldMarkPackageDirty = NotifyCollector.WasDataModified();

				if (NotifyCollector.Contains(CurveCopyNotifies) || NotifyCollector.Contains(LengthChangingNotifies))
				{
					CopyCurvesFromModel();
				}
				
				if (NotifyCollector.Contains(LengthChangingNotifies))
				{
					CopyCurvesFromModel();
					ClampNotifiesAtEndOfSequence();
				}
			}
			break;
		}
	}

	if (NotifyCollector.IsNotWithinBracket())
	{
		if (bShouldMarkPackageDirty)
		{
			MarkPackageDirty();
		}
	}
	else if (bShouldMarkPackageDirty)
	{
		NotifyCollector.MarkDataModified();
	}
}

IAnimationDataModel* UAnimSequenceBase::GetDataModel() const
{
	return DataModelInterface.GetInterface();
}

TScriptInterface<IAnimationDataModel> UAnimSequenceBase::GetDataModelInterface() const
{
	return DataModelInterface;
}

IAnimationDataController& UAnimSequenceBase::GetController()
{
	ValidateModel();

	if(Controller == nullptr)
	{
		Controller = DataModelInterface->GetController();
		checkf(Controller, TEXT("Failed to create AnimationDataController"));
		Controller->SetModel(DataModelInterface);
	}

	ensureAlways(Controller->GetModelInterface() == DataModelInterface);

	return *Controller;
}

void UAnimSequenceBase::PopulateModel()
{
	check(!HasAnyFlags(EObjectFlags::RF_ClassDefaultObject));	
	
	// Make a copy of the current data, to mitigate any changes due to notify callbacks
	// 复制当前数据，以减轻由于通知回调而导致的任何更改
	const FFrameRate FrameRate = GetSamplingFrameRate();
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	const float PlayLength = SequenceLength;
	const FRawCurveTracks CurveData = RawCurveData;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
		
	IAnimationDataController::FScopedBracket ScopedBracket(Controller, LOCTEXT("UAnimSequenceBase::PopulateModel_Bracket", "Generating Animation Model Data for Animation Sequence Base"));
	Controller->SetFrameRate(FrameRate);
	Controller->SetNumberOfFrames(Controller->ConvertSecondsToFrameNumber(PlayLength));

	USkeleton* TargetSkeleton = GetSkeleton();	
	UE::Anim::CopyCurveDataToModel(CurveData, TargetSkeleton,  *Controller);	
}

void UAnimSequenceBase::PopulateWithExistingModel(TScriptInterface<IAnimationDataModel> ExistingDataModel)
{
	Controller->PopulateWithExistingModel(ExistingDataModel);
}

void UAnimSequenceBase::BindToModelModificationEvent()
{
	ValidateModel();
	DataModelInterface->GetModifiedEvent().RemoveAll(this);
	DataModelInterface->GetModifiedEvent().AddUObject(this, &UAnimSequenceBase::OnModelModified);
}

void UAnimSequenceBase::CopyDataModel(const TScriptInterface<IAnimationDataModel>& ModelToDuplicate)
{
	checkf(ModelToDuplicate != nullptr, TEXT("Invalidate data model %s"), *GetFullName());
	if (ModelToDuplicate)
	{
		ModelToDuplicate->GetModifiedEvent().RemoveAll(this);
	}

	DataModelInterface = DuplicateObject(ModelToDuplicate.GetObject(), this);

	if(Controller)
	{
		Controller->SetModel(DataModelInterface);
	}
	else
	{
		GetController();
	}

	BindToModelModificationEvent();
}

void UAnimSequenceBase::CreateModel()
{
	const UClass* TargetClass = UE::Anim::DataModel::IAnimationDataModels::FindClassForAnimationAsset(this);
	checkf(TargetClass != nullptr, TEXT("Unable to find valid AnimationDataModel class"));

	checkf(!DataModelInterface || DataModelInterface.GetObject()->GetClass() != TargetClass, TEXT("Invalid attempt to override the existing data model %s"), *GetFullName());
	UObject* ClassDataModel = NewObject<UObject>(this, TargetClass, TargetClass->GetFName());
	DataModelInterface = ClassDataModel;

	BindToModelModificationEvent();
}

void UAnimSequenceBase::ValidateModel() const
{
	checkf(DataModelInterface != nullptr, TEXT("Invalid AnimSequenceBase state (%s), no data model found"), *GetPathName());
}

bool UAnimSequenceBase::ShouldDataModelBeValid() const
{
	return
#if WITH_EDITOR
		!GetOutermost()->HasAnyPackageFlags(PKG_Cooked);
#else
		false;
#endif
}
#endif // WITH_EDITOR

#undef LOCTEXT_NAMESPACE 

#if WITH_EDITOR
void UAnimSequenceBase::OnEndLoadPackage(const FEndLoadPackageContext& Context)
{
	if (!GetPackage()->GetHasBeenEndLoaded())
	{
		return;
	}

	FCoreUObjectDelegates::OnEndLoadPackage.RemoveAll(this);

	OnAnimModelLoaded();
}

void UAnimSequenceBase::OnAnimModelLoaded()
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_OnAnimModelLoaded);
	if(USkeleton* MySkeleton = GetSkeleton())
	{		
		if (IsDataModelValid())
		{
			const TArray<FFloatCurve>& FloatCurves = DataModelInterface->GetFloatCurves();
			for (int32 Index = 0; Index < FloatCurves.Num(); ++Index)
			{
				const FFloatCurve& Curve = FloatCurves[Index];
				ensureMsgf(Curve.GetName() != NAME_None, TEXT("[AnimSequencer %s] has invalid curve name."), *GetFullName());
			}
		}
	}
}
#endif // WITH_EDITOR
