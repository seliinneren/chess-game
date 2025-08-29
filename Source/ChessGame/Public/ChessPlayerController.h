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

	FVector2D LastMoveStart;
	FVector2D LastMoveEnd;

	AChessPieces* EnPassantTarget = nullptr;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	void Input_LeftClickAction(const FInputActionValue& Value);
	void TrySelectOrMovePiece(AActor* ClickedActor, const FVector& ClickLocation);
	void CalculatePossibleMoves();
	bool IsValidMove(const FVector2D& TargetPosition);
	void MoveSelectedPiece(const FVector2D& TargetPosition);

	void MoveRookForCastle(int32 Row, int32 RookCol, int32 TargetCol);

	void PromotePawn(AChessPieces* Pawn);

	bool IsInCheck(ETeam Team);

	bool IsCheckmate(ETeam Team);

	UPROPERTY()
	AChessPieces* SelectedPiece = nullptr;

	UPROPERTY()
	AChessBoardActor* ChessBoardRef = nullptr;

	TArray<FVector2D> PossibleMoves;
};
