// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyAIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "TagGameGameMode.h"
#include "Ball.h"
#include "NavigationSystem.h"



AEnemyAIController::AEnemyAIController()
{
	static ConstructorHelpers::FObjectFinder<UBlackboardData> BBObject(TEXT("/Script/AIModule.BlackboardData'/Game/Blueprint/AI/BBP_AIBlackBoard.BBP_AIBlackBoard'"));
	if (BBObject.Succeeded())
	{
		BlackBoardData = BBObject.Object;
	}
}


void AEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

	GoToPlayer = MakeShared<FAivState>(
		[](AAIController* AIController) //Enter
		{
			AIController->MoveToActor(AIController->GetWorld()->GetFirstPlayerController()->GetPawn(), 100.0f);
			
		},
		nullptr, //Exit
		[this](AAIController* AIController, const float DeltaTime) -> TSharedPtr<FAivState> //Tick
		{
			ABall* BestBall = Cast<ABall>(BlackBoardComponent->GetValueAsObject(TEXT("Bestball")));

			EPathFollowingStatus::Type State = AIController->GetMoveStatus();

			if (State == EPathFollowingStatus::Moving)
			{
				return nullptr;
			}			
			if (BestBall)
			{
				BestBall->AttachToActor(AIController->GetWorld()->GetFirstPlayerController()->GetPawn(), FAttachmentTransformRules::KeepRelativeTransform);
				BestBall->SetActorRelativeLocation(FVector(0, 0, 0));
				BestBall = nullptr;
			}
			return SearchForBall;
			


		}
	);

	SearchForBall = MakeShared<FAivState>(
		[this](AAIController* AIController)
		{
			AGameModeBase* GameMode = AIController->GetWorld()->GetAuthGameMode();
			ATagGameGameMode* AIGameMode = Cast<ATagGameGameMode>(GameMode);
			const TArray<ABall*>& BallList = AIGameMode->GetBalls();

			ABall* NearestBall = nullptr;

			for (int32 i = 0; i < BallList.Num(); i++)
			{
				if (!BallList[i]->GetAttachParentActor() &&
					(!NearestBall || FVector::Distance(AIController->GetPawn()->GetActorLocation(), BallList[i]->GetActorLocation()) <
					FVector::Distance(AIController->GetPawn()->GetActorLocation(), NearestBall->GetActorLocation())))
				{
					NearestBall = BallList[i];
				}
			}

			if (UseBlackboard(BlackBoardData, BlackBoardComponent))
			{
				BlackBoardComponent->SetValueAsObject(TEXT("Bestball"), NearestBall);
			}

			ABall* BestBall = Cast<ABall>(BlackBoardComponent->GetValueAsObject(TEXT("Bestball")));
		},
		nullptr,
		[this](AAIController* AIController, const float DeltaTime) -> TSharedPtr<FAivState> //Tick
		{
			ABall* BestBall = Cast<ABall>(BlackBoardComponent->GetValueAsObject(TEXT("Bestball")));

			if (BestBall)
			{
				return GoToBall;	
			}
			else
			{
				return Patrol;
			}

		}
	);

	GoToBall = MakeShared<FAivState>(
		[this](AAIController* AIController) //Enter
		{
			ABall* BestBall = Cast<ABall>(BlackBoardComponent->GetValueAsObject(TEXT("Bestball")));

			AIController->MoveToActor(BestBall, 100.0f);
		}, 
		nullptr, //Exit
		[this](AAIController* AIController, const float DeltaTime) -> TSharedPtr<FAivState> //Tick
		{
			EPathFollowingStatus::Type State = AIController->GetMoveStatus();

			if (State == EPathFollowingStatus::Moving)
			{
				return nullptr;
			}

			return GrabBall;

		}
		);

	GrabBall = MakeShared<FAivState>(
		[this](AAIController* AIController)
		{
			ABall* BestBall = Cast<ABall>(BlackBoardComponent->GetValueAsObject(TEXT("Bestball")));

			if (BestBall->GetAttachParentActor())
			{
				BestBall = nullptr;
			}
		},//Enter
		nullptr, //Exit
		[this](AAIController* AIController, const float DeltaTime) -> TSharedPtr<FAivState> //Tick
		{
			ABall* BestBall = Cast<ABall>(BlackBoardComponent->GetValueAsObject(TEXT("Bestball")));

			if (!BestBall)
			{
				return SearchForBall;
			}

			BestBall->AttachToActor(AIController->GetPawn(), FAttachmentTransformRules::KeepRelativeTransform);
			BestBall->SetActorRelativeLocation(FVector(0, 0, 0));
			return GoToPlayer;

		}
	);

	Patrol = MakeShared<FAivState>(
		[this](AAIController* AIController)
		{
			UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(AIController->GetWorld());

			FNavLocation RandomLocation;

			bool CanReachPoint = NavSystem->GetRandomReachablePointInRadius(AIController->GetPawn()->GetActorLocation(), 1000.f, RandomLocation);

			if (CanReachPoint)
			{
				if (UseBlackboard(BlackBoardData, BlackBoardComponent))
				{
					BlackBoardComponent->SetValueAsVector(TEXT("RandomPoint"), RandomLocation.Location);
				}
			}

			AIController->MoveToLocation(BlackBoardComponent->GetValueAsVector(TEXT("RandomPoint")));
		},
		nullptr,
		[this](AAIController* AIController, const float DeltaTime) -> TSharedPtr<FAivState> //Tick
		{
			
			EPathFollowingStatus::Type State = AIController->GetMoveStatus();

			if (State == EPathFollowingStatus::Moving)
			{
				return nullptr;
			}
			else
			{
				UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(AIController->GetWorld());

				FNavLocation RandomLocation;

				bool CanReachPoint = NavSystem->GetRandomReachablePointInRadius(AIController->GetPawn()->GetActorLocation(), 1000.f, RandomLocation);

				if (CanReachPoint)
				{
					if (UseBlackboard(BlackBoardData, BlackBoardComponent))
					{
						BlackBoardComponent->SetValueAsVector(TEXT("RandomPoint"), RandomLocation.Location);
					}
				}

				AIController->MoveToLocation(BlackBoardComponent->GetValueAsVector(TEXT("RandomPoint")));
				return Patrol;
			}

		}
	);

	CurrentState = SearchForBall;
	CurrentState->CallEnter(this);
}

void AEnemyAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentState)
	{
		CurrentState = CurrentState->CallTick(this, DeltaTime);
	}
}

