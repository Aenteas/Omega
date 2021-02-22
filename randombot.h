#ifndef RANDOMBOT_H
#define RANDOMBOT_H

#include "gamestate.h"
#include "aibotbase.h"

#include <list>
#include <QTime>
#include <QThread>
#include <QObject>

class RandomBot : public AiBotBase
{
    Q_OBJECT
public:
    RandomBot(GameState& gameState, const QTime* timeLeft);

    void reset() override;

private:
    void findNextMoves() override;
};

#endif // RANDOMBOT_H
