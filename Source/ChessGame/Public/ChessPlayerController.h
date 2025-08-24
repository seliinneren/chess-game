#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "ChessPieces.h"
#include "ChessBoardActor.h"
#include "EnhancedInputSubsystems.h"
#include "ChessPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;

UCLASS()
class CHESSGAME_API AChessPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AChessPlayerController();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* LeftClickAction;

	ETeam CurrentTurn = ETeam::White; 

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	void Input_LeftClickAction(const FInputActionValue& Value);
	void TrySelectOrMovePiece(AActor* ClickedActor, const FVector& ClickLocation);
	void CalculatePossibleMoves();
	bool IsValidMove(const FVector2D& TargetPosition);
	void MoveSelectedPiece(const FVector2D& TargetPosition);

	UPROPERTY()
	AChessPieces* SelectedPiece = nullptr;

	UPROPERTY()
	AChessBoardActor* ChessBoardRef = nullptr;

	TArray<FVector2D> PossibleMoves;
};
