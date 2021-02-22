#include "gamestate.h"

// ---- (re-)initializations ----

GameState::GameState(int boardSize, FeatureFlags flags):
    boardSize{boardSize},
    flags{flags},
    bitmapSize{sizeof(long int)*8},
    currentColor{WHITE},
    playerScores{{{WHITE,0}, {BLACK,0}}},
    cellNum{computeCellNum(boardSize)},
    currentPlayer{WHITE}
{
    // each player should have equal moves so we divide by 4
    numSteps = cellNum - cellNum%4;
    // we need as much bits to be able to represent each cell on the board
    freeNeighbourBitMapSize = (cellNum+bitmapSize-1)/bitmapSize;
    initCells();
}

void GameState::reset(){
    currentColor = WHITE;
    playerScores = {{WHITE,0}, {BLACK,0}};
    numSteps = cellNum - cellNum%4;
    cells.clear();
    cellVec.clear();
    initCells();
    groups.clear();
}

inline bool GameState::isValidAx(const Ax& ax)
{
    return std::abs(ax.q)<=boardSize-1 and std::abs(ax.r)<=boardSize-1 and std::abs(ax.q + ax.r)<=boardSize-1;
}

void GameState::setNeighbours(Cell& cell){
    vector<Ax> neighbourAxs = {{cell.q-1, cell.r+1},
                               {cell.q-1, cell.r},
                               {cell.q, cell.r-1},
                               {cell.q+1, cell.r-1},
                               {cell.q+1, cell.r},
                               {cell.q, cell.r+1}};
    //clockwise order and consecutive cells should neighbours of each other
    vector<unsigned int> idxs;
    // top edge
    if(cell.q == -boardSize+1 and cell.r > 0){
        idxs = {2,3,4,5,0,1};
    }
    // top right edge
    else if(cell.r == boardSize-1 and cell.q > -boardSize+1){
        idxs = {1,2,3,4,5,0};
    }
    // bottom right edge
    else if(cell.r >=0 and cell.q > 0){
        idxs = {0,1,2,3,4,5};
    }
    // bottom edge
    else if(cell.q == boardSize-1 and cell.r < 0){
        idxs = {5,0,1,2,3,4};
    }
    // bottom left edge
    else if(cell.r == -boardSize+1 and cell.q > 0){
        idxs = {4,5,0,1,2,3};
    }
    // top left edge and interior areas
    else{
        idxs = {3,4,5,0,1,2};
    }

    for(unsigned int idx : idxs){
        Ax ax = neighbourAxs[idx];
        if(isValidAx(ax)) cell.neighbours.push_back(&axToCell(ax));
    }
}

unsigned int GameState::computeCellNum(unsigned int boardSize) const{
    unsigned int numRows = 2*boardSize-1;
    return boardSize*numRows+(numRows-3)/2*((numRows-3)/2+1)+boardSize-1;
}

void GameState::initCells(){
    cells.reserve(2*boardSize-1);
    cellVec.reserve(cellNum);
    unsigned int idx = 0;
    for (int q = -boardSize+1; q < boardSize; q++)
    {
        cells.push_back({});
        // we are setting raw pointers on the container while pushing back the items
        // reserving prevents the push_back operator from changing the address
        cells[q+boardSize-1].reserve(2*boardSize-1 - std::abs(q));
        for (int r = -boardSize+1; r < boardSize; r++)
        {
            if(isValidAx({q, r}))
            {
                cells[q+boardSize-1].push_back({q, r, idx});

                cellVec.push_back(&cells[q+boardSize-1][cells[q+boardSize-1].size()-1]);
                ++idx;
            }
        }
    }
    for(Cell* cell : cellVec) setNeighbours(*cell);
}

// ---- forward updates ----

void GameState::update(unsigned int moveIdx){
    takenMoves.push(moveIdx);
    Cell& cell = idxToCell(moveIdx);
    cell.color = currentColor;
    mergeGroups(cell);
    updateNeighbourBitMaps(cell);
    --numSteps;
    updateColors();
}

void GameState::updateColors(){
    if(currentColor == WHITE)
        currentColor = BLACK;
    else{
        currentPlayer = currentPlayer == WHITE? BLACK : WHITE;
        currentColor = WHITE;
    }
}

void GameState::mergeGroups(Cell& cell)
{
    Color color = cell.color;
    unsigned int newGroupSize=1;

    //the array of neighbour super groups
    list<unsigned int> addedNeighbourSuperGroupIds;
    if(groups[color].size()>0){
        addedNeighbourSuperGroupIds = cell.getNeighbourGroupIds(groups[color], color);
    }
    else{
        playerScores[color] = 1;
        goto singleGroup;
    }

    //If there is no neighbour of the same color
    if(addedNeighbourSuperGroupIds.size()==0)
    {
        singleGroup:
        // create a new group with size 1
        unsigned int groupId = groups[color].size();
        cell.groupId = groupId;
        groups[color].push_back({groupId, newGroupSize});
        return;
    }

    // the first is going to be the new supergroup id
    unsigned int moveGroupId = addedNeighbourSuperGroupIds.front();

    //update the groupnum of the cell
    cell.groupId = moveGroupId;

    // update neighbour groups and compute new group size
    for(unsigned int nGroupId: addedNeighbourSuperGroupIds)
    {
        Group& neighbourGroup = groups[color][nGroupId];
        // add the size of the groups to the size of the new group
        newGroupSize += neighbourGroup.size;
        playerScores[color] /= neighbourGroup.size;

        // update neighbour group with new super group id
        neighbourGroup.id = moveGroupId;
    }
    // we only store the other groups
    addedNeighbourSuperGroupIds.pop_front();

    // add the list of group numbers of other groups'
    Group& group = groups[color][moveGroupId];
    group.addedGroupIds[color].push(std::move(addedNeighbourSuperGroupIds));

    group.size = newGroupSize;
    //update the new groupsize and add the addedgroups
    playerScores[color] *= newGroupSize;
}

void GameState::updateNeighbourBitMaps(Cell& cell)
{
    // get color, group, idx of the piece has been placed
    unsigned int groupId = cell.groupId;
    Color color = cell.color;
    Color oppColor = color == WHITE?BLACK:WHITE;
    unsigned int idx = cell.idx;
    Group& group = groups[color][groupId];

    // the index of the long value
    unsigned int bitMapIdx=idx/bitmapSize;
    // the bit position in the long value
    unsigned int bitPos=idx-bitMapIdx*bitmapSize;

    // init neighbourmap of the group
    group.freeNeighbourBitMaps.push(vector<long int>(freeNeighbourBitMapSize));

    if(group.size > 1){
        //merge the bitmaps of the old groups to the new
        mergeNeighbourBitMaps(cell);
    }

    //remove the field of the move from the map as it is taken
    group.freeNeighbourBitMaps.top()[bitMapIdx]&=~(1<<bitPos);

    list<unsigned int> oppNGroupIds = cell.getNeighbourGroupIds(groups[oppColor], oppColor);
    // we keep the values for backward update
    group.addedGroupIds[oppColor].push(oppNGroupIds);
    if(oppNGroupIds.size() > 0){
        // set the bit index of the move to zero in groups of the opposite color
        for(unsigned int oppNeighbourGroupId : group.addedGroupIds[oppColor].top())
            groups[oppColor][oppNeighbourGroupId].freeNeighbourBitMaps.top()[bitMapIdx]&=~(1<<bitPos);
    }

    //add the free neighbours around the cell
    // iterate over the neighbours
    for(const Cell* nCell : cell.neighbours)
    {
        if(nCell->color==EMPTY)
        {
            bitMapIdx=nCell->idx/bitmapSize;
            bitPos=nCell->idx-bitMapIdx*bitmapSize;
            group.freeNeighbourBitMaps.top()[bitMapIdx]|=(1<<bitPos);
        }
    }
}

void GameState::mergeNeighbourBitMaps(Cell& cell)
{
    unsigned int groupId = cell.groupId;
    Color color = cell.color;
    Group& group = groups[color][groupId];
    unsigned int i;
    // we check each connected group neighbour map and use bitwise or (bit value 1 means free neighbour)
    for(unsigned int nGroupId : group.addedGroupIds[color].top())
    {
        // iterate over the long values of the map
        i = 0;
        for(const auto& bMap : groups[color][nGroupId].freeNeighbourBitMaps.top()){
            // bitwise or
            group.freeNeighbourBitMaps.top()[i] |= bMap;
            ++i;
        }
    }
}

// ---- backward updates ----

void GameState::undo()
{
    // we expect that the caller do not call when there is no taken cells
    Cell& cell = idxToCell(takenMoves.top());

    undoOppBitMaps(cell);
    decomposeGroup(cell);

    ++numSteps;
    undoColors();
    takenMoves.pop();
}

void GameState::undoColors(){
    if(currentColor == WHITE){
        currentColor = BLACK;
        currentPlayer = currentPlayer == WHITE? BLACK : WHITE;
    }
    else
        currentColor = WHITE;
}

void GameState::decomposeGroup(Cell& cell)
{
    Color color = cell.color;
    Color oppColor = color == WHITE?BLACK:WHITE;
    unsigned int groupId = cell.groupId;
    Group& group = groups[color][groupId];

    // no connection, just remove the group
    if(group.size==1)
    {
        groups[color].pop_back();
        return;
    }
    playerScores[color] /= group.size;

    //decrease the size with the removed field
    --group.size;

    // group was already existing, we remove the last bitmap
    group.freeNeighbourBitMaps.pop();

    // restore component groups
    for(unsigned int cGroupId : group.addedGroupIds[color].top()){
        Group component = groups[color][cGroupId];
        playerScores[color] *= component.size;
        group.size -= component.size;
        component.id = cGroupId;
    }
    // group was already existing multiply back with the original size
    playerScores[color] *= group.size;

    group.addedGroupIds[color].pop();
    group.addedGroupIds[oppColor].pop();
}

void GameState::undoOppBitMaps(Cell& cell)
{
    // get color, group, idx of the piece has been placed
    unsigned int groupId = cell.groupId;
    Color color = cell.color;
    Color oppColor = color == WHITE?BLACK:WHITE;
    unsigned int idx = cell.idx;

    Group& group = groups[color][groupId];

    // the index of the long value
    unsigned int bitMapIdx=idx/bitmapSize;
    // the bit position in the long value
    unsigned int bitPos=idx-bitMapIdx*bitmapSize;

    //set the bit of the move on the map of each group of the opposite color
    for(unsigned int oppNeighbourGroupId : group.addedGroupIds[oppColor].top())
    {
        Group& oppGroup = groups[oppColor][oppNeighbourGroupId];
        oppGroup.freeNeighbourBitMaps.top()[bitMapIdx]|=(1<<bitPos);
    }
}

// ---- bit manipulations ----

unsigned int GameState::popCnt64(uint64_t i) {
    // SWAR algorithm
    i = i - ((i >> 1) & 0x5555555555555555);
    i = (i & 0x3333333333333333) + ((i >> 2) & 0x3333333333333333);
    return (((i + (i >> 4)) & 0x0F0F0F0F0F0F0F0F) *
            0x0101010101010101) >> 56;
}

// ---- inline internal functions ----

inline Cell& GameState::idxToCell(unsigned int idx){
    return *cellVec[idx];
}

inline Cell& GameState::axToCell(Ax ax){
    return cells[ax.q+boardSize-1][ax.q >= 0? ax.r+boardSize-1: ax.r+boardSize-1+ax.q];
}

inline unsigned int GameState::axToIdx(Ax ax) const{
    return cells[ax.q+boardSize-1][ax.q >= 0? ax.r+boardSize-1: ax.r+boardSize-1+ax.q].idx;
}

// ---- queries ----

Color GameState::leader(){
    if(playerScores[Color::WHITE] > playerScores[Color::BLACK])
        return Color::WHITE;
    if(playerScores[Color::WHITE] < playerScores[Color::BLACK])
        return BLACK;
    return EMPTY;
}

Color GameState::getCurrentColor() const{
    return currentColor;
}

bool GameState::end() const{
    return numSteps == 0;
}

map<Color, int> GameState::getPlayerScores() const{
    return playerScores;
}

Color GameState::getCurrentPlayer() const{
    return currentPlayer;
}

// ---- operators ----

inline unsigned int operator|(GameState::FeatureFlags first, GameState::FeatureFlags second){
    return static_cast<unsigned int>(first) | static_cast<unsigned int>(second);
}
