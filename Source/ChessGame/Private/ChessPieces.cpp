#include "ChessPieces.h"

AChessPieces::AChessPieces()
{
	PrimaryActorTick.bCanEverTick = true;

	PieceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PieceMesh"));
	RootComponent = PieceMesh;

	PieceMesh->SetMobility(EComponentMobility::Movable);


}
void AChessPieces::BeginPlay()
{
	Super::BeginPlay();
}

void AChessPieces::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AChessPieces::InitalizePiece(EPieceType InType, ETeam InTeam, UStaticMesh* InMesh)
{
	PieceType = InType;
	Team = InTeam;
	if (InMesh)
		PieceMesh->SetStaticMesh(InMesh);
}
