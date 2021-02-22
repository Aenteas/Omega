#include "randombot.h"

RandomBot::RandomBot(GameState& gameState, const QTime* timeLeft):
    AiBotBase(gameState, timeLeft)
{
    moves.shuffle();
}

void RandomBot::reset(){
    moves.reset(gameState.cellNum);
    moves.shuffle();
}

void RandomBot::findNextMoves(){
    whiteMove = moves[0].idx;
    blackMove = moves[1].idx;
}
