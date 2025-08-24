// Fill out your copyright notice in the Description page of Project Settings.


#include "ChessGameMode.h"
#include "ChessPlayerController.h" 

AChessGameMode::AChessGameMode()
{
PlayerControllerClass = AChessPlayerController::StaticClass();
}
