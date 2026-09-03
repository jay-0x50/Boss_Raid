// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/BRPlayerAnimNotifies.h"

#include "Components/SkeletalMeshComponent.h"
#include "Player/Character/ExceptionCharacter.h"

namespace
{
	AExceptionCharacter* GetExceptionPlayer(USkeletalMeshComponent* MeshComp)
	{
		return MeshComp ? Cast<AExceptionCharacter>(MeshComp->GetOwner()) : nullptr;
	}
}

void UBRPlayerAnimNotifyState::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	if (AExceptionCharacter* Player = GetExceptionPlayer(MeshComp))
	{
		Player->BeginAnimationWindow(Window);
	}
}

void UBRPlayerAnimNotifyState::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	if (AExceptionCharacter* Player = GetExceptionPlayer(MeshComp))
	{
		Player->EndAnimationWindow(Window);
	}
}

FString UBRPlayerAnimNotifyState::GetNotifyName_Implementation() const
{
	if (const UEnum* WindowEnum = StaticEnum<EBRPlayerAnimWindow>())
	{
		return FString::Printf(TEXT("Player %s"), *WindowEnum->GetNameStringByValue(static_cast<int64>(Window)));
	}
	return TEXT("Player Window");
}

void UBRPlayerAnimNotify::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	if (AExceptionCharacter* Player = GetExceptionPlayer(MeshComp))
	{
		Player->HandleAnimationEvent(Event);
	}
}

FString UBRPlayerAnimNotify::GetNotifyName_Implementation() const
{
	if (const UEnum* EventEnum = StaticEnum<EBRPlayerAnimEvent>())
	{
		return FString::Printf(TEXT("Player %s"), *EventEnum->GetNameStringByValue(static_cast<int64>(Event)));
	}
	return TEXT("Player Event");
}
