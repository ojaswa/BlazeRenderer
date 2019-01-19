/***************************************************************************
**                                                                        **
**  Blaze - a volume rendering and analytics program                      **
**  Copyright (C) 2016-2018 Graphics Research Group, IIIT Delhi           **
**                                                                        **
**  This program is free software: you can redistribute it and/or modify  **
**  it under the terms of the GNU General Public License as published by  **
**  the Free Software Foundation, either version 3 of the License, or     **
**  (at your option) any later version.                                   **
**                                                                        **
**  This program is distributed in the hope that it will be useful,       **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of        **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
**  GNU General Public License for more details.                          **
**                                                                        **
**  You should have received a copy of the GNU General Public License     **
**  along with this program.  If not, see http://www.gnu.org/licenses/.   **
**                                                                        **
****************************************************************************
**           Author: Ojaswa Sharma                                        **
**           E-mail: ojaswa@iiitd.ac.in                                   **
**           Date  : 14.12.2016                                           **
****************************************************************************/

#include "mainwindow.h"
#include "ui_mainwindow.h"

//Qt includes
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //this->setUnifiedTitleAndToolBarOnMac(true);
    m_volumeManager = new VolumeManager();
    m_1DTFDialog = new Dialog1DTransferFunction(this);
    m_raycastingSettingsDialog = new DialogRaycastingSettings(this);

    //Enable/disable actions
    ui->action_Read->setEnabled(true);
    ui->action1D_TF->setEnabled(false);
    ui->actionRaycasting_settings->setEnabled(false);

    //Connections
    connect(m_1DTFDialog, SIGNAL(finished(int)), ui->action1D_TF, SLOT(toggle()));
    connect(m_1DTFDialog, SIGNAL(TFChanged(unsigned char*)), ui->centralWidget, SLOT(TF1DChanged(unsigned char*)));
    connect(m_raycastingSettingsDialog, SIGNAL(finished(int)), ui->actionRaycasting_settings, SLOT(toggle()));
    connect(m_raycastingSettingsDialog, SIGNAL(stepSizeChanged(float)), ui->centralWidget, SLOT(raycasterStepSizeChanged(float)));
    connect(m_raycastingSettingsDialog, SIGNAL(interpolationTypeChanged(RaycastingInterpolationType)), ui->centralWidget, SLOT(raycasterInterpolationTypeChanged(RaycastingInterpolationType)));
    connect(m_raycastingSettingsDialog, SIGNAL(enableJitteredSampling(bool)), ui->centralWidget, SLOT(enableJitteredSampling(bool)));
    connect(m_raycastingSettingsDialog, SIGNAL(togglePhongShading(bool)), ui->centralWidget, SLOT(togglePhongShading(bool)));
    connect(m_volumeManager, SIGNAL(volumeDataCreated(VolumeManager *)), this, SLOT(on_volumeReadFinished()));
    connect(m_volumeManager, SIGNAL(volumeEdgesComputed(VolumeManager*)), this, SLOT(on_volumeEdgesComputed()));
    connect(m_volumeManager, SIGNAL(volumePreprocessCompleted(VolumeManager*)), this, SLOT(on_volumePreprocessCompleted()));
    connect(m_volumeManager, SIGNAL(volumeGradientComputed(VolumeManager*)), ui->centralWidget, SLOT(on_volumeGradientComputed()));
}

MainWindow::~MainWindow()
{
    delete m_raycastingSettingsDialog;
    delete m_1DTFDialog;
    delete m_volumeManager;
    delete ui;
}

void MainWindow::on_volumeReadFinished()
{
    //Begin preprocessing asynchronously while allowing user to explore the volume render.
    QtConcurrent::run(m_volumeManager, &VolumeManager::preprocess); //Preprocess volume
    //m_volumeManager->preprocess();
    //Resume UI thread jobs
    emit volumeDataCreated(m_volumeManager);
    ui->action1D_TF->setEnabled(true);
    ui->actionRaycasting_settings->setEnabled(true);
}

void MainWindow::on_volumePreprocessCompleted()
{
    emit volumePreprocessCompleted(m_volumeManager);
}

void MainWindow::on_volumeEdgesComputed()
{
    emit volumeEdgesComputed(m_volumeManager);
}

void MainWindow::on_action_Read_triggered()
{
    QString selfilter = tr("NRRD (*.nhdr *.nrrd)");
    QString filename = QFileDialog::getOpenFileName(this, "Open a volume", QCoreApplication::applicationDirPath(),
                                              tr("Supported formats (*.nhdr *.nrrd *.vtk);;NRRD (*.nhdr *.nrrd);;VTK (*.vtk)"),
                                              &selfilter);
    if(filename.isEmpty() || filename.isNull())
        return;

    readVolume(filename);
    ui->action_Read->setEnabled(false);
}

bool MainWindow::readVolume(QString filename)
{
    //Before  reading a new volume file, clear old contents of the process dir.
    QDir dir( "./process");
    if(dir.exists()) {
        dir.setFilter( QDir::NoDotAndDotDot | QDir::Files );
        foreach( QString dirItem, dir.entryList() )
            dir.remove( dirItem );

        dir.setFilter( QDir::NoDotAndDotDot | QDir::Dirs );
        foreach( QString dirItem, dir.entryList() )
        {
            QDir subDir( dir.absoluteFilePath( dirItem ) );
            subDir.removeRecursively();
        }
    }
    else
        QDir().mkdir(dir.path());

    //Return true of false appropriatey if the volume was successfully read or not.
    QFileInfo fi(filename);
    QString filetype = fi.suffix();

    fprintf(stderr, "Reading %s from disk...\n", filename.toStdString().c_str());
    if (filetype.compare("nhdr")==0 | filetype.compare("nrrd")==0) //Load NRRD file
    {
        m_volumeManager->readNHDR(filename.toStdString().c_str());
    }
    else if (filetype.compare("vtk") == 0) // Load VTK file
    {
        //TODO:
    }

    return true;
}


void MainWindow::on_action_Quit_triggered()
{
    QApplication::quit();
}

void MainWindow::on_action1D_TF_toggled(bool arg1)
{
    if(arg1) {
        m_1DTFDialog->move(m_1DTFDialog->m_windowPosition);
        m_1DTFDialog->show();
    }
    else {
        m_1DTFDialog->m_windowPosition = m_1DTFDialog->pos();
        m_1DTFDialog->hide();
    }
}

void MainWindow::on_actionRaycasting_settings_toggled(bool arg1)
{
    if(arg1) {
        m_raycastingSettingsDialog->move(m_raycastingSettingsDialog->m_windowPosition);
        m_raycastingSettingsDialog->show();
    }
    else {
        m_raycastingSettingsDialog->m_windowPosition = m_raycastingSettingsDialog->pos();
        m_raycastingSettingsDialog->hide();
    }
}

void MainWindow::on_actionSave_screenshot_triggered()
{
    QString filename = QFileDialog::getSaveFileName(this,
                                            tr("Save volume render"), QCoreApplication::applicationDirPath(),
                                            tr("PNG (*.png)"));
    if(filename.isEmpty() || filename.isNull())
        return;
    QFileInfo fi(filename);

    QImage img = ui->centralWidget->grabFramebuffer();
    img.save(filename);
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, tr("About BlazeRenderer"),
            tr("BlazeRenderer: a real-time high quality GPU volume renderer\n\nVersion 2.0\n\nCopyright \u00A9 2016-2018 Graphics Research Group\nIIIT Delhi"));
}
