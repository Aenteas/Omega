#ifndef AIBOTBASE_H
#define AIBOTBASE_H

#include "gamestate.h"

#include <vector>
#include <QTime>
#include <QThread>
#include <QObject>

class AiBotBase : public QObject
{
    Q_OBJECT
    friend class BoardDialog;
public:
    AiBotBase(GameState& gameState, const QTime* timeLeft);
    ~AiBotBase()=default;
    void updateGame();
    unsigned int getWhiteMove() const;
    unsigned int getBlackMove() const;

    virtual void reset()=0;

    void removeMove(unsigned int moveIdx);

    AiBotBase(const AiBotBase&)=delete;
    AiBotBase& operator=(const AiBotBase&)=delete;

signals:
    void finished();

private slots:
    void updateGameSlot();

protected:
    class Moves{
    public:
        struct Move{
            // heuristical value of move
            unsigned int idx;
            int value;
            Move(unsigned int idx, int value): idx{idx}, value{value} {}
        };
        Moves(const unsigned int numMoves);
        unsigned int size() const;
        void remove(unsigned int moveIdx);
        void reset(const unsigned int numMoves);
        void undo();
        void sort();
        void shuffle();
        Move& operator[](unsigned int idx);
    private:
        vector<Move> moves;
        stack<Move> takenMoves;

    };
    unsigned int whiteMove;
    unsigned int blackMove;
    GameState& gameState;
    // moves object depends on gameState and should be declared after gamestate
    Moves moves;
    inline bool isTimeOut() const{
        return timeLeft->toString("m:ss") == "0:00";
    }
    const QTime* timeLeft;

private:
    // function should specify whiteMove and blackMove
    virtual void findNextMoves()=0;
    QThread thread;
};

#endif // AIBOTBASE_H
