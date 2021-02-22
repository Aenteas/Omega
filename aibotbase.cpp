#include "aibotbase.h"

AiBotBase::AiBotBase(GameState& gameState, const QTime* timeLeft):
    QObject(NULL), // can not have a parent as it will be moved to QThread
    gameState{gameState},
    timeLeft{timeLeft},
    moves{gameState.cellNum}
{
    this->moveToThread(&thread);
    connect(&thread, SIGNAL (started()), this, SLOT (updateGameSlot()));
    connect(this, SIGNAL (finished()), &thread, SLOT (quit()));
}

AiBotBase::Moves::Moves(const unsigned int numMoves){
    moves.reserve(numMoves);
    for(unsigned int idx=0; idx<numMoves; ++idx)
        moves.push_back({idx, 0});
}

void AiBotBase::Moves::remove(unsigned int moveIdx){
    for(unsigned int i=moves.size()-1; i>=0; --i){
        if(moves[i].idx == moveIdx){
            takenMoves.push(moves[i]);
            moves.erase(moves.begin()+i);
            return;
        }
    }
}

void AiBotBase::removeMove(unsigned int moveIdx){
    moves.remove(moveIdx);
}

AiBotBase::Moves::Move& AiBotBase::Moves::operator[](unsigned int idx){
    return moves[moves.size()-1-idx];
}

unsigned int AiBotBase::Moves::size() const{
    return moves.size();
}

void AiBotBase::Moves::undo(){
    moves.push_back(takenMoves.top());
    takenMoves.pop();
}

void AiBotBase::Moves::sort(){
    // sort moves by the values in ascendng order
    std::sort(moves.begin(), moves.end(), [](const Move& lhs, const Move& rhs){return lhs.value < rhs.value;});
}

void AiBotBase::Moves::shuffle(){
    std::random_shuffle(moves.begin(), moves.end());
}

void AiBotBase::Moves::reset(const unsigned int numMoves){
    moves.clear();
    moves.reserve(numMoves);
    for(unsigned int idx=0; idx<numMoves; ++idx)
        moves.push_back({idx, 0});
}

unsigned int AiBotBase::getWhiteMove() const{
    return whiteMove;
}

unsigned int AiBotBase::getBlackMove() const{
    return blackMove;
}

void AiBotBase::updateGame(){
    thread.start();
}

void AiBotBase::updateGameSlot(){
    findNextMoves();
    removeMove(whiteMove);
    removeMove(blackMove);
    gameState.update(whiteMove);
    gameState.update(blackMove);
    emit finished();
}
