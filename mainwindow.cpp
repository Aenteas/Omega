#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(double padding, double radius) :
    QMainWindow(NULL),
    ui(new Ui::MainWindow),
    padding{padding},
    radius{radius}
{
    ui->setupUi(this);
    this->setWindowTitle(QString("Omega"));
    ui->boardSizeSpinBox->setMinimum(4);
    ui->boardSizeSpinBox->setMaximum(10);
    ui->boardSizeSpinBox->setValue(4);
    ui->colorComboBox->addItem(QString("White"));
    ui->colorComboBox->addItem(QString("Black"));
    ui->modeComboBox->addItem(QString("PvP"));
    ui->modeComboBox->addItem(QString("vs AI"));
    ui->modeComboBox->setCurrentIndex(0);
    ui->timeSlider->setMinimum(1);
    ui->timeSlider->setMaximum(10);
    ui->timeSlider->setValue(1);
    ui->colorComboBox->hide();
    ui->colorLabel->hide();
    board = NULL;

    connect(ui->quitButton, SIGNAL (clicked()), QApplication::instance(), SLOT (quit()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_startButton_clicked()
{
    QString mode = ui->modeComboBox->currentText();
    int boardSize = ui->boardSizeSpinBox->value();
    QString color = ui->colorComboBox->currentText();
    unsigned int time = ui->timeSlider->value();

    board = new BoardDialog(this, boardSize, radius, padding, mode, color, time);
    hide();
    board->show();
}

void MainWindow::on_modeComboBox_currentTextChanged(const QString &text)
{
    // hide other options
    if(text == QString("PvP")){
        ui->colorComboBox->hide();
        ui->colorLabel->hide();
        setMaximumHeight(220);
    }
    // show additional options
    else{
        ui->colorComboBox->show();
        ui->timeSlider->show();
        ui->colorLabel->show();
        ui->timeLabel->show();
        setMaximumHeight(400);
    }
}

void MainWindow::back_to_main(){
    board->close();
    delete board;
    show();
}

void MainWindow::on_timeSlider_sliderMoved(int position)
{
    ui->timeLabel->setText(QString("Time limit: %1 min").arg(position));
}
