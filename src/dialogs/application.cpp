#include "application.h"

#include <QMenu>
#include <QFileDialog>
#include <QStatusBar>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QDialog>
#include <QMovie>
#include <QDesktopWidget>

#include "core/core.h"
#include "models/connectionsmanager.h"

#include "dialogs/connect.h"
#include "dialogs/quickstartdialog.h"

#include "updater/updater.h"
#include "utils/configmanager.h"


MainWin::MainWin(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    qRegisterMetaType<RedisClient::Command>("Command");
    qRegisterMetaType<RedisClient::Command>("RedisClient::Command");
    qRegisterMetaType<RedisClient::Response>("Response");
    qRegisterMetaType<RedisClient::Response>("RedisClient::Response");

    QDesktopWidget* desktop = QApplication::desktop();
    QRect scr = desktop->screenGeometry();
    move(scr.center() - rect().center());

    initConnectionsTreeView();
    initContextMenus();
    initFormButtons();    
    initUpdater();    
    initSystemConsole();      
}

void MainWin::initConnectionsTreeView()
{
    //connection manager
    QString config = ConfigManager::getApplicationConfigPath("connections.xml");

    if (config.isNull()) {
        QMessageBox::warning(this,
            "Settings directory is not writable",
            QString("Program can't save connections file to settings dir."
                    "Please change permissions or restart this program "
                    " with administrative privileges")
            );

        exit(1);
    }

    connections = QSharedPointer<ConnectionsManager>(new ConnectionsManager(config));

    if (connections->size() == 0) {
        QTimer::singleShot(1000, this, SLOT(showQuickStartDialog()));
    }

    ui.serversTreeView->setModel(connections.data());
    ui.serversTreeView->setTabWidget(static_cast<ConnectionsTree::TabWidget*>(ui.tabWidget));
}

void MainWin::initContextMenus()
{
    // TODO: move to custom QMenu class
    connectionsMenu = QSharedPointer<QMenu>(new QMenu());
    connectionsMenu->addAction(QIcon(":/images/import.png"), "Import Connections", this, SLOT(OnImportConnectionsClick()));
    connectionsMenu->addAction(QIcon(":/images/export.png"), "Export Connections", this, SLOT(OnExportConnectionsClick()));
    connectionsMenu->addSeparator();    

    ui.pbImportConnections->setMenu(connectionsMenu.data());
}

void MainWin::initFormButtons()
{
    connect(ui.pbAddServer, SIGNAL(clicked()), SLOT(OnAddConnectionClick()));    
    connect(ui.pbImportConnections, SIGNAL(clicked()), SLOT(OnImportConnectionsClick()));
}

void MainWin::initUpdater()
{
    //set current version
    ui.applicationInfoLabel->setText(
        ui.applicationInfoLabel->text().replace("%VERSION%", QApplication::applicationVersion())
        );

    updater = QSharedPointer<Updater>(new Updater());
    connect(updater.data(), SIGNAL(updateUrlRetrived(QString &)), this, SLOT(OnNewUpdateAvailable(QString &)));
}

void MainWin::initSystemConsole()
{
    QPushButton * systemConsoleActivator = new QPushButton( QIcon(":/images/terminal.png"), "System log", this);
    systemConsoleActivator->setFlat(true);
    systemConsoleActivator->setStyleSheet("border: 0px; margin: 0 5px; font-size: 11px;");

    connect(systemConsoleActivator, SIGNAL(clicked()), this, SLOT(OnConsoleStateChanged()));

    ui.systemConsole->hide();
    ui.statusBar->addPermanentWidget(systemConsoleActivator);
}

void MainWin::showQuickStartDialog()
{
    QScopedPointer<QuickStartDialog> dialog(new QuickStartDialog(this));
    dialog->setWindowState(Qt::WindowActive);
    dialog->exec();
}

void MainWin::OnConsoleStateChanged()
{
    ui.systemConsole->setVisible(!ui.systemConsole->isVisible());
}

void MainWin::OnAddConnectionClick()
{
    QScopedPointer<ConnectionWindow> connectionDialog(new ConnectionWindow(this));    
    connectionDialog->setModal(true);
    connectionDialog->setWindowState(Qt::WindowActive);
#ifdef Q_OS_LINUX
    connectionDialog->move(this->width()/2 + connectionDialog->width() / 3, this->height()/2);
#endif
    connectionDialog->exec();    
}

void MainWin::OnNewUpdateAvailable(QString &url)
{
    QMessageBox::information(this, "New update available", 
        QString("Please download new version of Redis Desktop Manager: %1").arg(url));
}

void MainWin::OnImportConnectionsClick()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Import Connections", "", tr("Xml Files (*.xml)"));

    if (fileName.isEmpty()) {
        return;
    }

    if (connections->ImportConnections(fileName)) {
        QMessageBox::information(this, "Connections imported", "Connections imported from connections file");
    } else {
        QMessageBox::warning(this, "Can't import connections", "Select valid file for import");
    }
}

void MainWin::OnExportConnectionsClick()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Export Connections to xml", "", tr("Xml Files (*.xml)"));

    if (fileName.isEmpty()) {
        return;
    }

    if (connections->SaveConnectionsConfigToFile(fileName)) {
        QMessageBox::information(this, "Connections exported", "Connections exported in selected file");
    } else {
        QMessageBox::warning(this, "Can't export connections", "Select valid file name for export");
    }
}