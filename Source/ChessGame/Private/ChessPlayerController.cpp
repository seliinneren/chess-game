#include "ChessPlayerController.h"
#include "EnhancedInputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ChessPieces.h"

AChessPlayerController::AChessPlayerController()
{
}

void AChessPlayerController::BeginPlay()
{
    Super::BeginPlay();

    TArray<AActor*> ChessBoards;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AChessBoardActor::StaticClass(), ChessBoards);
    if (ChessBoards.Num() > 0)
        ChessBoardRef = Cast<AChessBoardActor>(ChessBoards[0]);

    if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            if (DefaultMappingContext)
                Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }

    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;

    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(InputMode);

    CurrentTurn = ETeam::White;
    LastMoveStart = FVector2D(-1, -1);
    LastMoveEnd = FVector2D(-1, -1);
}

void AChessPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
    {
        if (LeftClickAction)
            EIC->BindAction(LeftClickAction, ETriggerEvent::Started, this, &AChessPlayerController::Input_LeftClickAction);
    }
}

void AChessPlayerController::Input_LeftClickAction(const FInputActionValue& Value)
{
    FHitResult HitResult;
    GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

    if (HitResult.bBlockingHit)
        TrySelectOrMovePiece(HitResult.GetActor(), HitResult.Location);
}

void AChessPlayerController::TrySelectOrMovePiece(AActor* ClickedActor, const FVector& ClickLocation)
{
    if (!ChessBoardRef) return;

    FVector2D BoardPos = ChessBoardRef->ConvertWorldToBoardPosition(ClickLocation);
    BoardPos.X = FMath::Clamp(FMath::RoundToInt(BoardPos.X), 0, 7);
    BoardPos.Y = FMath::Clamp(FMath::RoundToInt(BoardPos.Y), 0, 7);

    AChessPieces* ClickedPiece = Cast<AChessPieces>(ClickedActor);

    if (SelectedPiece)
    {
        if (IsValidMove(BoardPos))
        {
            MoveSelectedPiece(BoardPos);
            ChessBoardRef->ClearHighlights();
            CurrentTurn = (CurrentTurn == ETeam::White) ? ETeam::Black : ETeam::White;
        }
        else if (!ClickedPiece)
        {
            SelectedPiece = nullptr;
            PossibleMoves.Empty();
            ChessBoardRef->ClearHighlights();
        }
        else if (ClickedPiece->Team == CurrentTurn)
        {
            SelectedPiece = ClickedPiece;
            CalculatePossibleMoves();
            ChessBoardRef->ShowHighlights(PossibleMoves);
        }
    }
    else if (ClickedPiece && ClickedPiece->Team == CurrentTurn)
    {
        SelectedPiece = ClickedPiece;
        CalculatePossibleMoves();
        ChessBoardRef->ShowHighlights(PossibleMoves);
    }
}

void AChessPlayerController::CalculatePossibleMoves()
{
    if (!SelectedPiece || !ChessBoardRef) return;

    PossibleMoves.Empty();
    EnPassantTarget = nullptr;

    int32 Row = SelectedPiece->BoardRow;
    int32 Col = SelectedPiece->BoardCol;

    auto AddMoveLine = [&](int32 R, int32 C) -> bool
        {
            if (R < 0 || R >= 8 || C < 0 || C >= 8) return false;
            AChessPieces* Target = ChessBoardRef->GetPieceAt(R, C);
            if (!Target) { PossibleMoves.Add(FVector2D(R, C)); return true; }
            if (Target->Team != SelectedPiece->Team) { PossibleMoves.Add(FVector2D(R, C)); return false; }
            return false;
        };

    auto AddPawnCapture = [&](int32 R, int32 C)
        {
            if (R < 0 || R >= 8 || C < 0 || C >= 8) return;
            AChessPieces* Target = ChessBoardRef->GetPieceAt(R, C);
            if (Target && Target->Team != SelectedPiece->Team)
                PossibleMoves.Add(FVector2D(R, C));
        };

    switch (SelectedPiece->PieceType)
    {
    case EPieceType::Pawn:
    {
        int32 Dir = (SelectedPiece->Team == ETeam::White) ? 1 : -1;
        int32 StartRow = (SelectedPiece->Team == ETeam::White) ? 1 : 6;

        if (!ChessBoardRef->GetPieceAt(Row + Dir, Col))
        {
            PossibleMoves.Add(FVector2D(Row + Dir, Col));
            if (Row == StartRow && !ChessBoardRef->GetPieceAt(Row + 2 * Dir, Col))
                PossibleMoves.Add(FVector2D(Row + 2 * Dir, Col));
        }

        AddPawnCapture(Row + Dir, Col - 1);
        AddPawnCapture(Row + Dir, Col + 1);

        if (LastMoveStart != FVector2D(-1, -1))
        {
            if (FMath::Abs(LastMoveStart.X - LastMoveEnd.X) == 2)
            {
                int32 LastRow = LastMoveEnd.X;
                int32 LastCol = LastMoveEnd.Y;
                for (int dc = -1; dc <= 1; dc += 2)
                {
                    int32 AdjCol = Col + dc;
                    if (AdjCol < 0 || AdjCol >= 8) continue;
                    AChessPieces* AdjPiece = ChessBoardRef->GetPieceAt(Row, AdjCol);
                    if (AdjPiece && AdjPiece->BoardRow == LastRow && AdjPiece->BoardCol == LastCol)
                    {
                        PossibleMoves.Add(FVector2D(Row + Dir, AdjCol));
                        EnPassantTarget = AdjPiece;
                    }
                }
            }
        }
        break;
    }
    case EPieceType::Rook:
    case EPieceType::Bishop:
    case EPieceType::Queen:
    {
        TArray<FVector2D> Directions;
        if (SelectedPiece->PieceType == EPieceType::Rook)
            Directions = { {1,0}, {-1,0}, {0,1}, {0,-1} };
        else if (SelectedPiece->PieceType == EPieceType::Bishop)
            Directions = { {1,1}, {1,-1}, {-1,1}, {-1,-1} };
        else
            Directions = { {1,0}, {-1,0}, {0,1}, {0,-1}, {1,1}, {1,-1}, {-1,1}, {-1,-1} };

        for (auto& D : Directions)
            for (int32 i = 1; i < 8; ++i)
                if (!AddMoveLine(Row + D.X * i, Col + D.Y * i)) break;
        break;
    }
    case EPieceType::Knight:
    {
        const int32 Moves[8][2] = { {2,1},{1,2},{-1,2},{-2,1},{-2,-1},{-1,-2},{1,-2},{2,-1} };
        for (auto& M : Moves)
        {
            int32 R = Row + M[0], C = Col + M[1];
            if (R >= 0 && R < 8 && C >= 0 && C < 8)
            {
                AChessPieces* Target = ChessBoardRef->GetPieceAt(R, C);
                if (!Target || Target->Team != SelectedPiece->Team)
                    PossibleMoves.Add(FVector2D(R, C));
            }
        }
        break;
    }
    case EPieceType::King:
    {
        for (int dr = -1; dr <= 1; ++dr)
            for (int dc = -1; dc <= 1; ++dc)
            {
                if (dr == 0 && dc == 0) continue;
                int32 R = Row + dr, C = Col + dc;
                if (R >= 0 && R < 8 && C >= 0 && C < 8)
                {
                    AChessPieces* Target = ChessBoardRef->GetPieceAt(R, C);
                    if (!Target || Target->Team != SelectedPiece->Team)
                        PossibleMoves.Add(FVector2D(R, C));
                }
            }

        if (!SelectedPiece->bHasMoved)
        {
            if (AChessPieces* Rook = ChessBoardRef->GetPieceAt(Row, 7))
            {
                if (Rook->PieceType == EPieceType::Rook && !Rook->bHasMoved)
                {
                    bool bClear = true;
                    for (int c = Col + 1; c < 7; c++)
                        if (ChessBoardRef->GetPieceAt(Row, c)) bClear = false;
                    if (bClear) PossibleMoves.Add(FVector2D(Row, Col + 2));
                }
            }
            if (AChessPieces* Rook = ChessBoardRef->GetPieceAt(Row, 0))
            {
                if (Rook->PieceType == EPieceType::Rook && !Rook->bHasMoved)
                {
                    bool bClear = true;
                    for (int c = Col - 1; c > 0; c--)
                        if (ChessBoardRef->GetPieceAt(Row, c)) bClear = false;
                    if (bClear) PossibleMoves.Add(FVector2D(Row, Col - 2));
                }
            }
        }
        break;
    }
    }
}

bool AChessPlayerController::IsValidMove(const FVector2D& TargetPosition)
{
    int32 TR = FMath::RoundToInt(TargetPosition.X);
    int32 TC = FMath::RoundToInt(TargetPosition.Y);
    for (auto& M : PossibleMoves)
        if (TR == FMath::RoundToInt(M.X) && TC == FMath::RoundToInt(M.Y))
            return true;
    return false;
}

void AChessPlayerController::MoveSelectedPiece(const FVector2D& TargetPosition)
{
    if (!SelectedPiece || !ChessBoardRef) return;

    FVector2D OldPos(SelectedPiece->BoardRow, SelectedPiece->BoardCol);
    LastMoveStart = OldPos;
    LastMoveEnd = TargetPosition;
    int32 Row = OldPos.X;
    int32 Col = OldPos.Y;

    if (AChessPieces* Target = ChessBoardRef->GetPieceAt(TargetPosition.X, TargetPosition.Y))
    {
        if (Target->Team != SelectedPiece->Team)
        {
            ChessBoardRef->SetPieceAt(Target->BoardRow, Target->BoardCol, nullptr);
            Target->Destroy();
        }
    }

    if (SelectedPiece->PieceType == EPieceType::Pawn && EnPassantTarget)
    {
        if (TargetPosition.Y != OldPos.Y && !ChessBoardRef->GetPieceAt(TargetPosition.X, TargetPosition.Y))
        {
            if (EnPassantTarget->BoardRow == OldPos.X && EnPassantTarget->BoardCol == TargetPosition.Y)
            {
                ChessBoardRef->SetPieceAt(EnPassantTarget->BoardRow, EnPassantTarget->BoardCol, nullptr);
                EnPassantTarget->Destroy();
            }
        }
        EnPassantTarget = nullptr;
    }

    if (SelectedPiece->PieceType == EPieceType::King)
    {
        if (TargetPosition.Y == Col + 2)
        {
            AChessPieces* Rook = ChessBoardRef->GetPieceAt(Row, 7);
            if (Rook)
            {
                ChessBoardRef->SetPieceAt(Row, 7, nullptr);
                ChessBoardRef->SetPieceAt(Row, Col + 1, Rook);
                Rook->BoardCol = Col + 1;
                Rook->bHasMoved = true;
                Rook->SetActorLocation(ChessBoardRef->GetTileWorldPosition(Row, Col + 1));
            }
        }
        else if (TargetPosition.Y == Col - 2)
        {
            AChessPieces* Rook = ChessBoardRef->GetPieceAt(Row, 0);
            if (Rook)
            {
                ChessBoardRef->SetPieceAt(Row, 0, nullptr);
                ChessBoardRef->SetPieceAt(Row, Col - 1, Rook);
                Rook->BoardCol = Col - 1;
                Rook->bHasMoved = true;
                Rook->SetActorLocation(ChessBoardRef->GetTileWorldPosition(Row, Col - 1));
            }
        }
    }

    ChessBoardRef->SetPieceAt(OldPos.X, OldPos.Y, nullptr);
    SelectedPiece->BoardRow = FMath::RoundToInt(TargetPosition.X);
    SelectedPiece->BoardCol = FMath::RoundToInt(TargetPosition.Y);
    ChessBoardRef->SetPieceAt(SelectedPiece->BoardRow, SelectedPiece->BoardCol, SelectedPiece);

    FVector TargetLoc = ChessBoardRef->GetTileWorldPosition(SelectedPiece->BoardRow, SelectedPiece->BoardCol);
    float PieceZ = ChessBoardRef->GetSafeZOffset(SelectedPiece->PieceMesh ? SelectedPiece->PieceMesh->GetStaticMesh() : nullptr, 0.f);
    SelectedPiece->SetActorLocation(TargetLoc + FVector(0, 0, PieceZ));

    SelectedPiece->bHasMoved = true;

    SelectedPiece = nullptr;
    PossibleMoves.Empty();
}

void AChessPlayerController::MoveRookForCastle(int32 Row, int32 RookCol, int32 TargetCol)
{
    if (AChessPieces* Rook = ChessBoardRef->GetPieceAt(Row, RookCol))
    {
        ChessBoardRef->SetPieceAt(Row, RookCol, nullptr);
        ChessBoardRef->SetPieceAt(Row, TargetCol, Rook);
        Rook->BoardCol = TargetCol;
        Rook->bHasMoved = true;
        FVector TargetLoc = ChessBoardRef->GetTileWorldPosition(Row, TargetCol);
        if (Rook->PieceMesh) Rook->PieceMesh->SetWorldLocation(TargetLoc + FVector(0, 0, 0));
        else Rook->SetActorLocation(TargetLoc + FVector(0, 0, 0));
    }
}
