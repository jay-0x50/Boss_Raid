// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "BRPlayerAnimNotifies.generated.h"

UENUM(BlueprintType)
enum class EBRPlayerAnimWindow : uint8
{
	AttackTrace,
	Invincibility,
	Parry,
	ComboInput,
	RootMotionLock
};

UENUM(BlueprintType)
enum class EBRPlayerAnimEvent : uint8
{
	Footstep,
	LightWeaponSwing,
	HeavyWeaponSwing,
	Heal,
	HitVFX,
	HitSFX,
	Dodge,
	ParryAttempt,
	ParrySuccess,
	PlayerHit,
	PlayerDeath,
	ExecutionDamage,
	FinishAction
};

/** Animation-authored gameplay window. Timers remain a fallback when this notify is absent. */
UCLASS(meta=(DisplayName="Exception Player Window"))
class EXCEPTION_API UBRPlayerAnimNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exception|Animation")
	EBRPlayerAnimWindow Window = EBRPlayerAnimWindow::AttackTrace;

	virtual void NotifyBegin(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};

/** One-shot animation event used for feedback and exact gameplay moments. */
UCLASS(meta=(DisplayName="Exception Player Event"))
class EXCEPTION_API UBRPlayerAnimNotify : public UAnimNotify
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exception|Animation")
	EBRPlayerAnimEvent Event = EBRPlayerAnimEvent::Footstep;

	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
