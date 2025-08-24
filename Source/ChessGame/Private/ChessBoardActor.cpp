#include "ChessBoardActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

AChessBoardActor::AChessBoardActor()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	ChessBoardComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChessBoardComponent"));
	ChessBoardComponent->SetupAttachment(RootComponent);
}

float AChessBoardActor::GetSafeZOffset(UStaticMesh* M, float Factor) const
{
	return M ? M->GetBounds().BoxExtent.Z * Factor : 2.f;
}

void AChessBoardActor::BeginPlay()
{
	Super::BeginPlay();

	SpawnBoard();
	BoardGrid.SetNum(64);
	SpawnPieces();
}

void AChessBoardActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AChessBoardActor::SpawnBoard()
{
	if (!BlackTileMesh || !WhiteTileMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("Tile Meshes are not set!"));
		return;
	}

	const int32 BoardSize = 8;
	const FVector TileExtent = BlackTileMesh->GetBounds().BoxExtent;
	const FVector ParentScale = ChessBoardComponent->GetComponentScale();
	TileSizeX = TileExtent.X * 2.f * ParentScale.X;
	TileSizeY = TileExtent.Y * 2.f * ParentScale.Y;

	FVector BoardOrigin = ChessBoardComponent->GetComponentLocation();
	FVector BoardBottomLeft = BoardOrigin - FVector((BoardSize * TileSizeX) / 2.f, (BoardSize * TileSizeY) / 2.f, 0.f);

	for (int32 Row = 0; Row < BoardSize; Row++)
	{
		for (int32 Col = 0; Col < BoardSize; Col++)
		{
			bool bIsWhite = (Row + Col) % 2 == 0;
			UStaticMesh* TileMesh = bIsWhite ? WhiteTileMesh : BlackTileMesh;

			UStaticMeshComponent* Tile = NewObject<UStaticMeshComponent>(this);
			Tile->SetStaticMesh(TileMesh);
			Tile->SetMobility(EComponentMobility::Movable);
			Tile->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Tile->AttachToComponent(ChessBoardComponent, FAttachmentTransformRules::KeepRelativeTransform);
			Tile->RegisterComponent();

			FVector TileLocation = BoardBottomLeft + FVector(
				Col * TileSizeX + TileSizeX / 2,
				Row * TileSizeY + TileSizeY / 2,
				0.f); 

			float tileZ = GetSafeZOffset(TileMesh, 0.05f);
			Tile->SetWorldLocation(TileLocation + FVector(0, 0, tileZ));

			SpawnedTiles.Add(Tile);
		}
	}
}

void AChessBoardActor::SpawnPieces()
{
	auto SpawnPiece = [&](UStaticMesh* Mesh, int Row, int Col, EPieceType Type, ETeam Team)
		{
			if (!Mesh || !ChessPieceClass) return;

			float pieceZ = GetSafeZOffset(Mesh, -0.1f);

			FVector SpawnLocation = GetTileWorldPosition(Row, Col) + FVector(0, 0, pieceZ);
			FRotator SpawnRotation = FRotator::ZeroRotator;
			FActorSpawnParameters SpawnParams;

			AChessPieces* Piece = GetWorld()->SpawnActor<AChessPieces>(ChessPieceClass, SpawnLocation, SpawnRotation, SpawnParams);
			if (Piece)
			{
				Piece->PieceType = Type;
				Piece->Team = Team;
				Piece->BoardRow = Row;
				Piece->BoardCol = Col;

				if (Piece->PieceMesh)
					Piece->PieceMesh->SetStaticMesh(Mesh);

				SetPieceAt(Row, Col, Piece);
			}
		};

	for (int Col = 0; Col < 8; ++Col)
	{
		SpawnPiece(WhitePawn, 1, Col, EPieceType::Pawn, ETeam::White);
		SpawnPiece(BlackPawn, 6, Col, EPieceType::Pawn, ETeam::Black);
	}

	UStaticMesh* WhitePieces[8] = { WhiteRook, WhiteKnight, WhiteBishop, WhiteQueen,
									WhiteKing, WhiteBishop, WhiteKnight, WhiteRook };
	UStaticMesh* BlackPieces[8] = { BlackRook, BlackKnight, BlackBishop, BlackQueen,
									BlackKing, BlackBishop, BlackKnight, BlackRook };

	EPieceType WhitePieceTypes[8] = { EPieceType::Rook, EPieceType::Knight, EPieceType::Bishop, EPieceType::Queen,
									  EPieceType::King, EPieceType::Bishop, EPieceType::Knight, EPieceType::Rook };
	EPieceType BlackPieceTypes[8] = { EPieceType::Rook, EPieceType::Knight, EPieceType::Bishop, EPieceType::Queen,
									  EPieceType::King, EPieceType::Bishop, EPieceType::Knight, EPieceType::Rook };

	for (int Col = 0; Col < 8; ++Col)
	{
		SpawnPiece(WhitePieces[Col], 0, Col, WhitePieceTypes[Col], ETeam::White);
		SpawnPiece(BlackPieces[Col], 7, Col, BlackPieceTypes[Col], ETeam::Black);
	}
}

FVector AChessBoardActor::GetTileWorldPosition(int32 Row, int32 Col) const
{
	FVector BoardOrigin = ChessBoardComponent->GetComponentLocation();
	FVector BoardBottomLeft = BoardOrigin - FVector(8 * TileSizeX / 2, 8 * TileSizeY / 2, 0);

	return BoardBottomLeft + FVector(
		Col * TileSizeX + TileSizeX / 2,
		Row * TileSizeY + TileSizeY / 2,
		0);
}

FVector2D AChessBoardActor::ConvertWorldToBoardPosition(const FVector& WorldPosition) const
{
	const int32 BoardSize = 8;
	const FVector ParentScale = ChessBoardComponent->GetComponentScale();

	FVector BoardOrigin = ChessBoardComponent->GetComponentLocation();
	FVector BoardBottomLeft = BoardOrigin - FVector(BoardSize * TileSizeX / 2, BoardSize * TileSizeY / 2, 0);

	FVector Relative = WorldPosition - BoardBottomLeft;

	int32 Col = FMath::FloorToInt(Relative.X / (TileSizeX)); 
	int32 Row = FMath::FloorToInt(Relative.Y / (TileSizeY));

	Col = FMath::Clamp(Col, 0, 7);
	Row = FMath::Clamp(Row, 0, 7);

	return FVector2D(Row, Col);
}

void AChessBoardActor::ShowHighlights(const TArray<FVector2D>& Moves)
{
	for (UStaticMeshComponent* Tile : HighlightTiles)
		if (Tile) Tile->DestroyComponent();
	HighlightTiles.Empty();

	if (!HighlightMesh) return;

	for (const FVector2D& Move : Moves)
	{
		int32 Row = FMath::RoundToInt(Move.X);
		int32 Col = FMath::RoundToInt(Move.Y);

		UStaticMeshComponent* Highlight = NewObject<UStaticMeshComponent>(this);
		Highlight->SetStaticMesh(HighlightMesh);
		Highlight->SetMobility(EComponentMobility::Movable);
		Highlight->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Highlight->AttachToComponent(ChessBoardComponent, FAttachmentTransformRules::KeepRelativeTransform);
		Highlight->RegisterComponent();

		float hlZ = GetSafeZOffset(HighlightMesh, 0.1f);
		Highlight->SetWorldLocation(GetTileWorldPosition(Row, Col) + FVector(0, 0, hlZ));

		HighlightTiles.Add(Highlight);
	}
}

void AChessBoardActor::ClearHighlights()
{
	for (UStaticMeshComponent* Tile : HighlightTiles)
		if (Tile) Tile->DestroyComponent();
	HighlightTiles.Empty();
}


