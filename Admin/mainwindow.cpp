#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("127.0.0.1");
    db.setDatabaseName("mydb");
    db.setUserName("root");
    db.setPassword("Ag~147Wy");

    bool ok = db.open();
    if (!ok)
    {
        QMessageBox msgBox;
        msgBox.setText("Ошибка: невозможно подключиться к базе данных");
        msgBox.setInformativeText("Проверьте подключение к базе данных, её работоспособность");
        msgBox.setStandardButtons(QMessageBox::Cancel);
        msgBox.exec();
    }
    qDebug() << "Успешное подключение";

    modelEmployees = new QSqlTableModel(parent, db);
    modelEmployees->setTable("Employees");
    modelEmployees->setEditStrategy(QSqlTableModel::OnRowChange);
    modelEmployees->setHeaderData(0, Qt::Horizontal, tr("Id"));
    modelEmployees->setHeaderData(1, Qt::Horizontal, tr("Имя аккаунта"));
    modelEmployees->setHeaderData(2, Qt::Horizontal, tr("Полное имя пользователя"));
    modelEmployees->setHeaderData(3, Qt::Horizontal, tr("Время начала работы"));
    modelEmployees->setHeaderData(4, Qt::Horizontal, tr("Время окончания работы"));
    modelEmployees->setHeaderData(5, Qt::Horizontal, tr("Время начала обеда"));
    modelEmployees->setHeaderData(6, Qt::Horizontal, tr("Время окончания обеда"));
    modelEmployees->select();

    modelInactions = new QSqlRelationalTableModel(parent, db);
    modelInactions->setTable("Inactions");
    modelInactions->setEditStrategy(QSqlTableModel::OnRowChange);
    modelInactions->setHeaderData(0, Qt::Horizontal, tr("Id"));
    modelInactions->setHeaderData(1, Qt::Horizontal, tr("Время обнаружения бездействия"));
    modelInactions->setHeaderData(2, Qt::Horizontal, tr("Имя простаивающего аккаунта"));
    modelInactions->setRelation(2, QSqlRelation("Employees", "idEmployees", "username"));
    modelInactions->select();

    modelConnections = new QSqlRelationalTableModel(parent, db);
    modelConnections->setTable("Connections");
    modelConnections->setEditStrategy(QSqlTableModel::OnRowChange);
    modelConnections->setHeaderData(0, Qt::Horizontal, tr("Id"));
    modelConnections->setHeaderData(1, Qt::Horizontal, tr("Время подключения"));
    modelConnections->setHeaderData(2, Qt::Horizontal, tr("Время отключения"));
    modelConnections->setHeaderData(3, Qt::Horizontal, tr("Имя аккаунта"));
    modelConnections->setRelation(3, QSqlRelation("Employees", "idEmployees", "username"));
    modelConnections->select();

    //===========//

    ui->tableView_Employees->setModel(modelEmployees);
    ui->tableView_Employees->setItemDelegate(new QSqlRelationalDelegate(ui->tableView_Employees));
    ui->tableView_Employees->setWindowTitle("Таблица сотрудников");
    ui->tableView_Employees->hideColumn(0);
    ui->tableView_Employees->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView_Employees->show();

    ui->tableView_Inactions->setModel(modelInactions);
    ui->tableView_Inactions->setItemDelegate(new QSqlRelationalDelegate(ui->tableView_Inactions));
    ui->tableView_Inactions->setWindowTitle("Таблица бездействия");
    ui->tableView_Inactions->hideColumn(0);
    ui->tableView_Inactions->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView_Inactions->show();

    ui->tableView_Connections->setModel(modelConnections);
    ui->tableView_Connections->setItemDelegate(new QSqlRelationalDelegate(ui->tableView_Connections));
    ui->tableView_Connections->setWindowTitle("Таблица соединений");
    ui->tableView_Connections->hideColumn(0);
    ui->tableView_Connections->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView_Connections->show();

}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_addRow_clicked()
{

}

void MainWindow::on_pushButton_deleteRow_clicked()
{

}

void MainWindow::on_pushButton_update_clicked()
{

}

void MainWindow::on_pushButtonInactions_addRow_clicked()
{
    const QModelIndex index =  ui->tableView_Inactions->selectionModel()->currentIndex();
    QAbstractItemModel *model =  ui->tableView_Inactions->model();

    if (!model->insertRow(index.row()+1, index.parent()))
        return;
}

void MainWindow::on_pushButtonInactions_deleteRow_clicked()
{
    const QModelIndex index = ui->tableView_Inactions->selectionModel()->currentIndex();
    QAbstractItemModel *model = ui->tableView_Inactions->model();
    if (model->removeRow(index.row(), index.parent()))
        ;//updateActions();

    ui->tableView_Inactions->update();
}

void MainWindow::on_pushButtonInactions_update_clicked()
{
    modelInactions->select();
}

void MainWindow::on_pushButtonEmployees_addRow_clicked()
{
    const QModelIndex index =  ui->tableView_Employees->selectionModel()->currentIndex();
    QAbstractItemModel *model =  ui->tableView_Employees->model();

    if (!model->insertRow(index.row()+1, index.parent()))
        return;
}

void MainWindow::on_pushButtonEmployees_deleteRow_clicked()
{
    const QModelIndex index = ui->tableView_Employees->selectionModel()->currentIndex();
    QAbstractItemModel *model = ui->tableView_Employees->model();
    if (model->removeRow(index.row(), index.parent()))
        ;//updateActions();

    ui->tableView_Employees->update();
}

void MainWindow::on_pushButtonEmployees_update_clicked()
{
    modelEmployees->select();
}

void MainWindow::on_pushButtonConnections_addRow_clicked()
{
    const QModelIndex index =  ui->tableView_Connections->selectionModel()->currentIndex();
    QAbstractItemModel *model =  ui->tableView_Connections->model();

    if (!model->insertRow(index.row()+1, index.parent()))
        return;
}

void MainWindow::on_pushButtonConnections_deleteRow_clicked()
{
    const QModelIndex index = ui->tableView_Connections->selectionModel()->currentIndex();
    QAbstractItemModel *model = ui->tableView_Connections->model();
    if (model->removeRow(index.row(), index.parent()))
        ;//updateActions();

    ui->tableView_Connections->update();
}

void MainWindow::on_pushButtonConnections_update_clicked()
{
    modelConnections->select();
}
