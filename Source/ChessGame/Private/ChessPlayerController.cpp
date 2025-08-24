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

    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
        if (DefaultMappingContext)
            Subsystem->AddMappingContext(DefaultMappingContext, 0);

    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;

    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(InputMode);

    CurrentTurn = ETeam::White;
}

void AChessPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
        if (LeftClickAction)
            EIC->BindAction(LeftClickAction, ETriggerEvent::Started, this, &AChessPlayerController::Input_LeftClickAction);
}

void AChessPlayerController::Input_LeftClickAction(const FInputActionValue& Value)
{
    FHitResult HitResult;
    GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

    if (HitResult.bBlockingHit)
    {
        TrySelectOrMovePiece(HitResult.GetActor(), HitResult.Location);
    }
}

void AChessPlayerController::TrySelectOrMovePiece(AActor* ClickedActor, const FVector& ClickLocation)
{
    if (!ChessBoardRef) return;

    AChessPieces* ClickedPiece = Cast<AChessPieces>(ClickedActor);
    FVector2D BoardPosition = ChessBoardRef->ConvertWorldToBoardPosition(ClickLocation);
    BoardPosition.X = FMath::RoundToInt(BoardPosition.X);
    BoardPosition.Y = FMath::RoundToInt(BoardPosition.Y);

    if (SelectedPiece && IsValidMove(BoardPosition))
    {
        MoveSelectedPiece(BoardPosition);
        ChessBoardRef->ClearHighlights();
        CurrentTurn = (CurrentTurn == ETeam::White) ? ETeam::Black : ETeam::White;
        
    }
    else if (SelectedPiece && !ClickedPiece)
    {
        SelectedPiece = nullptr;
        PossibleMoves.Empty();
        ChessBoardRef->ClearHighlights();
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

    int32 Row = SelectedPiece->BoardRow;
    int32 Col = SelectedPiece->BoardCol;

    auto AddMoveLine = [&](int32 NewRow, int32 NewCol) -> bool
        {
            if (NewRow < 0 || NewRow >= 8 || NewCol < 0 || NewCol >= 8)
                return false;

            AChessPieces* TargetPiece = ChessBoardRef->GetPieceAt(NewRow, NewCol);

            if (!TargetPiece)
            {
                PossibleMoves.Add(FVector2D(NewRow, NewCol));
                return true;
            }
            else if (TargetPiece->Team != SelectedPiece->Team)
            {
                PossibleMoves.Add(FVector2D(NewRow, NewCol));
                return false;
            }
            else
            {
                return false;
            }
        };

    auto AddPawnCapture = [&](int32 NewRow, int32 NewCol)
        {
            if (NewRow < 0 || NewRow >= 8 || NewCol < 0 || NewCol >= 8)
                return;

            AChessPieces* TargetPiece = ChessBoardRef->GetPieceAt(NewRow, NewCol);
            if (TargetPiece && TargetPiece->Team != SelectedPiece->Team)
            {
                PossibleMoves.Add(FVector2D(NewRow, NewCol));
            }
        };

    switch (SelectedPiece->PieceType)
    {
    case EPieceType::Pawn:
    {
        int32 Dir = (SelectedPiece->Team == ETeam::White) ? 1 : -1;
        int32 StartRow = (SelectedPiece->Team == ETeam::White) ? 1 : 6;

        if (Row + Dir >= 0 && Row + Dir < 8)
        {
            if (!ChessBoardRef->GetPieceAt(Row + Dir, Col))
            {
                PossibleMoves.Add(FVector2D(Row + Dir, Col));

                if (Row == StartRow)
                {
                    if (!ChessBoardRef->GetPieceAt(Row + 2 * Dir, Col))
                        PossibleMoves.Add(FVector2D(Row + 2 * Dir, Col));
                }
            }
        }

        AddPawnCapture(Row + Dir, Col - 1);
        AddPawnCapture(Row + Dir, Col + 1);
        break;
    }
    case EPieceType::Rook:
    {
        const int32 Dirs[4][2] = { {1,0}, {-1,0}, {0,1}, {0,-1} };
        for (auto& D : Dirs)
        {
            for (int32 i = 1; i < 8; ++i)
            {
                int32 r = Row + D[0] * i;
                int32 c = Col + D[1] * i;

                if (!AddMoveLine(r, c))
                    break; 
            }
        }
        break;
    }
    case EPieceType::Bishop:
    {
        const int32 Dirs[4][2] = { {1,1}, {1,-1}, {-1,1}, {-1,-1} };
        for (auto& D : Dirs)
        {
            for (int32 i = 1; i < 8; ++i)
            {
                int32 r = Row + D[0] * i;
                int32 c = Col + D[1] * i;

                if (!AddMoveLine(r, c))
                    break; 
            }
        }
        break;
    }
    case EPieceType::Knight:
    {
        const int32 Moves[8][2] = { {2,1}, {1,2}, {-1,2}, {-2,1}, {-2,-1}, {-1,-2}, {1,-2}, {2,-1} };
        for (auto& M : Moves)
        {
            int32 r = Row + M[0];
            int32 c = Col + M[1];
            if (r >= 0 && r < 8 && c >= 0 && c < 8)
            {
                AChessPieces* Target = ChessBoardRef->GetPieceAt(r, c);
                if (!Target || Target->Team != SelectedPiece->Team)
                    PossibleMoves.Add(FVector2D(r, c));
            }
        }
        break;
    }
    case EPieceType::Queen:
    {
        const int32 Dirs[8][2] = { {1,0}, {-1,0}, {0,1}, {0,-1}, {1,1}, {1,-1}, {-1,1}, {-1,-1} };
        for (auto& D : Dirs)
        {
            for (int32 i = 1; i < 8; ++i)
            {
                int32 r = Row + D[0] * i;
                int32 c = Col + D[1] * i;

                if (!AddMoveLine(r, c))
                    break; 
            }
        }
        break;
    }
    case EPieceType::King:
    {
        for (int32 dr = -1; dr <= 1; ++dr)
        {
            for (int32 dc = -1; dc <= 1; ++dc)
            {
                if (dr == 0 && dc == 0) continue;
                int32 r = Row + dr;
                int32 c = Col + dc;
                if (r >= 0 && r < 8 && c >= 0 && c < 8)
                {
                    AChessPieces* Target = ChessBoardRef->GetPieceAt(r, c);
                    if (!Target || Target->Team != SelectedPiece->Team)
                        PossibleMoves.Add(FVector2D(r, c));
                }
            }
        }
        break;
    }
    
    }
}

bool AChessPlayerController::IsValidMove(const FVector2D& TargetPosition)
{
    const int32 TR = FMath::RoundToInt(TargetPosition.X);
    const int32 TC = FMath::RoundToInt(TargetPosition.Y);

    for (const FVector2D& M : PossibleMoves)
    {
        if (TR == FMath::RoundToInt(M.X) && TC == FMath::RoundToInt(M.Y))
            return true;
    }
    return false;
}

void AChessPlayerController::MoveSelectedPiece(const FVector2D& TargetPosition)
{
    if (!SelectedPiece || !ChessBoardRef) return;

    if (AChessPieces* Target = ChessBoardRef->GetPieceAt(TargetPosition.X, TargetPosition.Y))
    {
        if (Target->Team != SelectedPiece->Team)
        {
            ChessBoardRef->SetPieceAt(Target->BoardRow, Target->BoardCol, nullptr);
            Target->Destroy();
        }
    }

    ChessBoardRef->SetPieceAt(SelectedPiece->BoardRow, SelectedPiece->BoardCol, nullptr);

    SelectedPiece->BoardRow = FMath::RoundToInt(TargetPosition.X);
    SelectedPiece->BoardCol = FMath::RoundToInt(TargetPosition.Y);
    ChessBoardRef->SetPieceAt(SelectedPiece->BoardRow, SelectedPiece->BoardCol, SelectedPiece);

    FVector TargetLocation = ChessBoardRef->GetTileWorldPosition(SelectedPiece->BoardRow, SelectedPiece->BoardCol);

    if (SelectedPiece->PieceMesh)
        SelectedPiece->PieceMesh->SetWorldLocation(TargetLocation + FVector(0, 0, -5.0f));
    else
        SelectedPiece->SetActorLocation(TargetLocation + FVector(0, 0, -5.0f));

    SelectedPiece = nullptr;
    PossibleMoves.Empty();
}
