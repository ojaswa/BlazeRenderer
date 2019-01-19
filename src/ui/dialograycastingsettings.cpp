/***************************************************************************
**                                                                        **
**  BlazeRenderer - An OpenGL based real-time volume renderer             **
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

#include "dialograycastingsettings.h"
#include "ui_dialograycastingsettings.h"

float DialogRaycastingSettings::m_stepSizes[10] = {0.001, 0.002, 0.005, 0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1.0};

DialogRaycastingSettings::DialogRaycastingSettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogRaycastingSettings)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Tool);
    setFixedSize(this->width(),this->height());
    show(); m_windowPosition = pos(); hide();
    ui->checkBoxJittered->setChecked(false);

    //Data
    m_stepSize = m_stepSizes[ui->sliderStepSize->value()];
    setToolTip(QString("%1").arg(m_stepSize));

}

DialogRaycastingSettings::~DialogRaycastingSettings()
{
    delete ui;
}

void DialogRaycastingSettings::on_sliderStepSize_valueChanged(int value)
{
    m_stepSize = m_stepSizes[value];
    emit stepSizeChanged(m_stepSize);
    setToolTip(QString("%1").arg(m_stepSize));
    ui->labelstepSizeValue->setText(QString("%1").arg(m_stepSize));
}

void DialogRaycastingSettings::on_radioButtonInterpolationNN_clicked(bool checked)
{
    if(checked) emit interpolationTypeChanged(InterpolationNearestNeighbour);
}

void DialogRaycastingSettings::on_radioButtonInterpolationLinear_clicked(bool checked)
{
    if(checked) emit interpolationTypeChanged(InterpolationTrilinear);
}

void DialogRaycastingSettings::on_checkBoxJittered_toggled(bool checked)
{
    emit enableJitteredSampling(checked);
}

void DialogRaycastingSettings::on_checkBoxPhongShading_toggled(bool checked)
{
    emit togglePhongShading(checked);
}
