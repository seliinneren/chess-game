// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChessPieces.generated.h"

UENUM(BlueprintType)
enum class EPieceType : uint8
{
	Pawn,
	Rook,
	Knight,
	Bishop,
	Queen,
	King
};

UENUM(BlueprintType)
enum class ETeam : uint8
{
	White,
	Black
};

UCLASS()
class CHESSGAME_API AChessPieces : public AActor
{
	GENERATED_BODY()
	
public:	
	AChessPieces();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChessPiece")
	UStaticMeshComponent* PieceMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChessPiece")
	EPieceType PieceType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChessPiece")
	ETeam Team;

	UPROPERTY(BlueprintReadWrite, Category = "ChessPiece")
	int32 BoardRow;

	UPROPERTY(BlueprintReadWrite, Category = "ChessPiece")
	int32 BoardCol;

	bool bHasMoved; 

protected:
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void InitalizePiece(EPieceType InType, ETeam InTeam, UStaticMesh* InMesh);
public:
	FORCEINLINE EPieceType GetPieceType() const { return PieceType; }

};
