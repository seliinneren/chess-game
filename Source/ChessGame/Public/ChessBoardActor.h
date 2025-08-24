#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChessPieces.h"
#include "ChessBoardActor.generated.h"

UCLASS()
class CHESSGAME_API AChessBoardActor : public AActor
{
	GENERATED_BODY()

public:
	AChessBoardActor();

	UPROPERTY(VisibleAnywhere, Category = "Chess Board")
	UStaticMeshComponent* ChessBoardComponent;

	UPROPERTY(EditAnywhere, Category = "Chess Board")
	UStaticMesh* BlackTileMesh;

	UPROPERTY(EditAnywhere, Category = "Chess Board")
	UStaticMesh* WhiteTileMesh;

	UPROPERTY()
	TArray<UStaticMeshComponent*> SpawnedTiles;

	UPROPERTY(EditAnywhere, Category = "Chess Pieces")
	TSubclassOf<AChessPieces> ChessPieceClass;

	UPROPERTY(EditAnywhere, Category = "Chess Pieces") UStaticMesh* WhitePawn;
	UPROPERTY(EditAnywhere, Category = "Chess Pieces") UStaticMesh* WhiteRook;
	UPROPERTY(EditAnywhere, Category = "Chess Pieces") UStaticMesh* WhiteKnight;
	UPROPERTY(EditAnywhere, Category = "Chess Pieces") UStaticMesh* WhiteBishop;
	UPROPERTY(EditAnywhere, Category = "Chess Pieces") UStaticMesh* WhiteQueen;
	UPROPERTY(EditAnywhere, Category = "Chess Pieces") UStaticMesh* WhiteKing;

	UPROPERTY(EditAnywhere, Category = "Chess Pieces") UStaticMesh* BlackPawn;
	UPROPERTY(EditAnywhere, Category = "Chess Pieces") UStaticMesh* BlackRook;
	UPROPERTY(EditAnywhere, Category = "Chess Pieces") UStaticMesh* BlackKnight;
	UPROPERTY(EditAnywhere, Category = "Chess Pieces") UStaticMesh* BlackBishop;
	UPROPERTY(EditAnywhere, Category = "Chess Pieces") UStaticMesh* BlackQueen;
	UPROPERTY(EditAnywhere, Category = "Chess Pieces") UStaticMesh* BlackKing;

	UPROPERTY(EditAnywhere, Category = "Board")
	UStaticMesh* HighlightMesh;

	UPROPERTY(Transient)
	TArray<UStaticMeshComponent*> HighlightTiles;

	UPROPERTY()
	TArray<AChessPieces*> BoardGrid;

	float TileSizeX = 0.f;
	float TileSizeY = 0.f;

	float GetSafeZOffset(UStaticMesh* M, float Factor = 0.5f) const;

protected:
	virtual void BeginPlay() override;

public:

	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	FVector2D ConvertWorldToBoardPosition(const FVector& WorldPosition) const;

	void ShowHighlights(const TArray<FVector2D>& Moves);

	void ClearHighlights();

	UFUNCTION()
	void SpawnBoard();

	UFUNCTION()
	void SpawnPieces();

	UFUNCTION()
	int32 GetIndex(int32 Row, int32 Col) const { return Row * 8 + Col; }

	UFUNCTION()
	void SetPieceAt(int32 Row, int32 Col, AChessPieces* Piece)
	{
		int32 Index = GetIndex(Row, Col);
		if (BoardGrid.IsValidIndex(Index))
			BoardGrid[Index] = Piece;
	}

	UFUNCTION()
	AChessPieces* GetPieceAt(int32 Row, int32 Col) const
	{
		int32 Index = GetIndex(Row, Col);
		if (BoardGrid.IsValidIndex(Index))
			return BoardGrid[Index];
		return nullptr;
	}

	UFUNCTION()
	FVector GetTileWorldPosition(int32 Row, int32 Col) const;

	float GetTileSizeX() const { return TileSizeX; }
	float GetTileSizeY() const { return TileSizeY; }
};
