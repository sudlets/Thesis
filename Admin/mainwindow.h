#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSql>
#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QTableView>
#include <QMessageBox>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_addRow_clicked();

    void on_pushButton_deleteRow_clicked();

    void on_pushButton_update_clicked();

    void on_pushButtonInactions_update_clicked();

    void on_pushButtonInactions_deleteRow_clicked();

    void on_pushButtonInactions_addRow_clicked();

    void on_pushButtonEmployees_addRow_clicked();

    void on_pushButtonEmployees_deleteRow_clicked();

    void on_pushButtonEmployees_update_clicked();

    void on_pushButtonConnections_addRow_clicked();

    void on_pushButtonConnections_deleteRow_clicked();

    void on_pushButtonConnections_update_clicked();

private:
    Ui::MainWindow *ui;
    QSqlDatabase db;
    QSqlTableModel *modelEmployees;
    QSqlRelationalTableModel *modelInactions;
    QSqlRelationalTableModel *modelConnections;
};
#endif // MAINWINDOW_H
