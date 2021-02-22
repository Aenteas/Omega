#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <vector>
#include <cmath>
#include <map>

#include "cell.h"

struct Ax{
    int q, r;
    Ax(int q, int r): q{q}, r{r} {}
};

class GameState
{
public:
    enum FeatureFlags
    {
        // additional features that the gamestate should compute besides group sizes
        FreeNeighbours = 1,
        Separators = 2,
    };
    GameState(int boardSize, FeatureFlags flags);

    // ---- inline functions for both external and internal usage ----
    inline bool isFlagSet(FeatureFlags flag) const{
        return flags&static_cast<unsigned int>(flag);
    }

    unsigned int computeCellNum(unsigned int boardSize) const;

    void reset();

    const unsigned int cellNum;
    Color getCurrentPlayer() const;
    Color getCurrentColor() const;
    bool end() const;
    Color leader();
    void update(unsigned int idx);
    void undo();
    map<Color, int> getPlayerScores() const;

private:
    // ---- initialization ----
    void initCells();
    void setNeighbours(Cell& cell);

    // ---- forward update ----
    void mergeGroups(Cell& cell);
    void updateColors();
    void updateNeighbourBitMaps(Cell& cell);
    void mergeNeighbourBitMaps(Cell& cell);

    // ---- backward update ----
    void decomposeGroup(Cell& cell);
    void undoColors();
    void undoOppBitMaps(Cell& cell);

    // ---- bit manipulations ----
    unsigned int popCnt64(uint64_t i);

    // --- inline functions for internal usage ---
    inline Cell& idxToCell(unsigned int idx);
    inline Cell& axToCell(Ax ax);
    inline unsigned int axToIdx(Ax ax) const;
    inline bool isValidAx(const Ax& ax);

    // ---- variables ----

    FeatureFlags flags;
    unsigned int numSteps;
    unsigned int freeNeighbourBitMapSize;
    const int boardSize;
    vector<vector<Cell>> cells;
    vector<Cell*> cellVec;
    map<Color, vector<Group>> groups;
    map<Color, int> playerScores;
    size_t bitmapSize;
    Color currentColor;
    Color currentPlayer;
    stack<unsigned int> takenMoves;
};

#endif // GAMESTATE_H
