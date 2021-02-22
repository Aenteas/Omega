#include "boarddialog.h"
#include "ui_boarddialog.h"

using namespace std;

BoardDialog::BoardDialog(QWidget *parent, unsigned int boardSize, double radius, double padding, QString mode, QString color, unsigned int time) :
    QDialog(parent),
    ui(new Ui::BoardDialog),
    canvas(new Canvas(this, boardSize, radius, padding)),
    time{time},
    inGame{false},
    gameState{boardSize, GameState::FeatureFlags::FreeNeighbours},
    rTimeWhite{0},
    rTimeBlack{0},
    whiteTimer{NULL},
    blackTimer{NULL}
{
    // setup UI and connections
    ui->setupUi(this);
    this->setWindowTitle(QString("Omega"));
    ui->horizontalLayout->addWidget(canvas);
    setMinimumWidth(canvas->board_width()+200);
    setMinimumHeight(canvas->board_height()+50);
    setMaximumWidth(canvas->board_width()+160);
    setMaximumHeight(canvas->board_height()+50);
    ui->controlPanel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    playerColor = color == "White" ? Color::WHITE : Color::BLACK;
    if(mode == "vs AI"){
        aiBot = new RandomBot(gameState, playerColor == Color::WHITE? &timeBlack : &timeWhite);
        connect(&(aiBot->thread), SIGNAL (finished()), this, SLOT(updateFromAiBot()));
    }
    else aiBot = NULL;
    connect(this, SIGNAL (back_to_main()), parent, SLOT (back_to_main()));
}

BoardDialog::~BoardDialog()
{
    delete ui;
}

void BoardDialog::initTimers(){
    // init time in minutes
    timeWhite = QTime(0, time, 0);
    timeBlack = QTime(0, time, 0);

    rTimeBlack = 0;
    rTimeBlack = 0;
    whiteCounter = 10;
    blackCounter = 10;
    if(whiteTimer) delete whiteTimer;
    if(blackTimer) delete blackTimer;
    whiteTimer = new QTimer(this);
    blackTimer = new QTimer(this);

    ui->whiteCountDown->setText(timeWhite.toString("m:ss"));
    ui->blackCountDown->setText(timeBlack.toString("m:ss"));
    connect(whiteTimer, SIGNAL(timeout()), this, SLOT(updateCountdown()));
    connect(blackTimer, SIGNAL(timeout()), this, SLOT(updateCountdown()));
}

void BoardDialog::startTimer(bool restart){
    if(restart) initTimers();
    if(gameState.getCurrentPlayer() == Color::WHITE){
        if(restart or rTimeWhite == 0) whiteTimer->start(100);
        else whiteTimer->start(rTimeWhite);
    }
    else{
        if(restart or rTimeBlack == 0) blackTimer->start(100);
        else blackTimer->start(rTimeBlack);
    }
}

void BoardDialog::stopTimer(){
    if(gameState.getCurrentPlayer() == Color::WHITE){
        if(whiteTimer){
            rTimeWhite = whiteTimer->remainingTime();
            whiteTimer->stop();
        }
    }
    else if(blackTimer){
        rTimeBlack = blackTimer->remainingTime();
        blackTimer->stop();
    }
}

void BoardDialog::switchTimers(){
    // stop timer of previous player
    if(gameState.getCurrentPlayer() == Color::BLACK){
        rTimeWhite = whiteTimer->remainingTime();
        whiteTimer->stop();
    }
    else{
        rTimeBlack = blackTimer->remainingTime();
        blackTimer->stop();
    }
    startTimer(false);
}

void BoardDialog::on_startButton_clicked()
{
    canvas->active = false;
    if(inGame){
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Restart", "Are you sure want to restart the game?", QMessageBox::Yes|QMessageBox::No);
        if(reply == QMessageBox::Yes){
            // re-init canvas, game state and restart timers
            canvas->reset();
            gameState.reset();
            startTimer(true);
        }
        else{
            startTimer(false);
        }
    }
    else{
        inGame = true;
        ui->startButton->setText("Start New Game");
        startTimer(true);
    }
    updateControlPanel();
    if(aiBot){
        aiBot->reset();
        // if bot starts the game
        if(playerColor != Color::WHITE){
            ui->quitButton->setEnabled(false);
            ui->startButton->setEnabled(false);
            canvas->active = false;
            aiBot->updateGame();
        }
        else canvas->active = true;
    }
    else canvas->active = true;
}

void BoardDialog::on_quitButton_clicked()
{
    stopTimer();
    if(canvas->active){
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Quit", "Are you sure want to quit?", QMessageBox::Yes|QMessageBox::No);
        if(reply == QMessageBox::Yes)
            emit back_to_main();
        else
            startTimer(false);
    }
    else emit back_to_main();
}

void BoardDialog::updateControlPanel(){
    ui->whiteScore->display(gameState.getPlayerScores()[Color::WHITE]);
    ui->blackScore->display(gameState.getPlayerScores()[Color::BLACK]);
    ui->whiteScore->repaint();
    ui->blackScore->repaint();
    if(gameState.end()){
        freezeControlPanel(gameState.leader());
    }
    else{
        if(aiBot){
            QString description = gameState.getCurrentPlayer() == playerColor ? QString("your") : QString("the AI's");
            ui->description->setText("It's " + description + " turn!");
        }
        else{
            if(gameState.getCurrentPlayer() == Color::WHITE){
                ui->description->setText("It's WHITE's turn!");
            }
            else{
                 ui->description->setText("It's BLACK's turn!");
            }
        }
        ui->description->repaint();
    }
}

void BoardDialog::freezeControlPanel(const Color winner){
    QString description;
    if(aiBot){
        if(winner == playerColor)
            description = "You";
        else if(winner == Color::EMPTY)
            description = "Draw!";
        else
            description = "The AI";
    }
    else{
        if(winner == Color::WHITE)
            description = "White";
        else if(winner == Color::BLACK)
            description = "Black";
        else
            description = "Draw!";
    }
    ui->description->setText(description + " won!");
    canvas->active = false;
    whiteTimer->stop();
    blackTimer->stop();
}

void BoardDialog::updateCountdown()
{
    /*
     * Timers are updated in every 0.1 sec but only the seconds are shown on the UI
     * This way the bot has sub-sec resolution and can better schedule the time for each step
    */
    Color color = gameState.getCurrentPlayer();
    if(color == Color::WHITE){
        timeWhite = timeWhite.addMSecs(-100);
        --whiteCounter;
        if(whiteCounter == 0){
            ui->whiteCountDown->setText(timeWhite.toString("m:ss"));
            whiteCounter = 10;
        }
        if(timeWhite.toString("m:ss:z") == "0:00:0"){
            freezeControlPanel(Color::BLACK);
        }
        else whiteTimer->start(100);
    }
    else{
        timeBlack = timeBlack.addMSecs(-100);
        --blackCounter;
        if(blackCounter == 0){
            ui->blackCountDown->setText(timeBlack.toString("m:ss"));
            blackCounter = 10;
        }
        if(timeBlack.toString("m:ss:z") == "0:00:0"){
            freezeControlPanel(Color::WHITE);
        }
        else blackTimer->start(100);
    }
}

void BoardDialog::updateGameState(unsigned int idx){
    gameState.update(idx);
    // handles the case when game ends
    updateControlPanel();
    if(aiBot and !gameState.end()){
        aiBot->removeMove(idx);
        // aibot's turn
        if(gameState.getCurrentColor() == Color::WHITE){
            canvas->active = false;
            ui->quitButton->setEnabled(false);
            ui->startButton->setEnabled(false);
            switchTimers();
            aiBot->updateGame();
        }
    }
}

void BoardDialog::updateFromAiBot(){
    canvas->aiBotMovedEvent(aiBot->getWhiteMove(), aiBot->getBlackMove());
    switchTimers();
    // handles the case when game ends
    updateControlPanel();
    ui->quitButton->setEnabled(true);
    ui->startButton->setEnabled(true);
    if(!gameState.end()) canvas->active = true;
}

