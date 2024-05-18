// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnemyAIController.generated.h"

struct FAivState : TSharedFromThis<FAivState>
{
private:
	TFunction<void(AAIController*, UBlackboardComponent*)> Enter;
	TFunction<void(AAIController*, UBlackboardComponent*)> Exit;
	TFunction<TSharedPtr<FAivState>(AAIController*, const float, UBlackboardComponent*)> Tick;

public:
	FAivState()
	{
		Enter = nullptr;
		Exit = nullptr;
		Tick = nullptr;
	}

	FAivState(TFunction<void(AAIController*, UBlackboardComponent*)> InEnter = nullptr, TFunction<void(AAIController*, UBlackboardComponent*)> InExit = nullptr, TFunction<TSharedPtr<FAivState>(AAIController*, const float, UBlackboardComponent*)> InTick = nullptr)
	{
		Enter = InEnter;
		Exit = InExit;
		Tick = InTick;
	}

	FAivState(const FAivState& Other) = delete;
	FAivState(FAivState&& Other) = delete;
	FAivState& operator=(const FAivState& Other) = delete;
	FAivState& operator=(FAivState&& Other) = delete;


	void CallEnter(AAIController* AIController, UBlackboardComponent* BlackboardComponent)
	{
		if (Enter)
		{
			Enter(AIController, BlackboardComponent);
		}
	}
	void CallExit(AAIController* AIController, UBlackboardComponent* BlackboardComponent)
	{
		if (Exit)
		{
			Exit(AIController, BlackboardComponent);
		}

	}
	TSharedPtr<FAivState> CallTick(AAIController* AIController, const float DeltaTime, UBlackboardComponent* BlackboardComponent)
	{
		if (Tick)
		{
			TSharedPtr<FAivState> NewState = Tick(AIController, DeltaTime, BlackboardComponent);
			if (NewState != nullptr && NewState != AsShared())
			{
				CallExit(AIController, BlackboardComponent);
				NewState->CallEnter(AIController, BlackboardComponent);
				return NewState;
			}
		}

		return AsShared();
	}
};

/**
 *
 */
UCLASS()
class TAGGAME_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

protected:
	TSharedPtr<FAivState> CurrentState;
	TSharedPtr<FAivState> GoToPlayer;
	TSharedPtr<FAivState> SearchForBall;
	TSharedPtr<FAivState> GoToBall;
	TSharedPtr<FAivState> GrabBall;
	TSharedPtr<FAivState> Dead;

	//ABall* BestBall;

	void BeginPlay() override;
	void Tick(float DeltaTime) override;

	UPROPERTY(EditDefaultsOnly)
		UBlackboardData* BlackBoardData;

	UBlackboardComponent* BlackBoardComponent;
public:
	AEnemyAIController();
};
