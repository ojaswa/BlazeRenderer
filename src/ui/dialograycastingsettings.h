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

#ifndef DIALOGRAYCASTINGSETTINGS_H
#define DIALOGRAYCASTINGSETTINGS_H

#include <QDialog>
#include "defines.h"

namespace Ui {
class DialogRaycastingSettings;
}

class DialogRaycastingSettings : public QDialog
{
    Q_OBJECT

public:
    explicit DialogRaycastingSettings(QWidget *parent = 0);
    ~DialogRaycastingSettings();

    void enableDirectRenderingToggle();
    QPoint m_windowPosition;

private slots:
    void on_sliderStepSize_valueChanged(int value);
    void on_radioButtonInterpolationNN_clicked(bool checked);
    void on_radioButtonInterpolationLinear_clicked(bool checked);
    void on_checkBoxJittered_toggled(bool checked);
    void on_checkBoxPhongShading_toggled(bool checked);

private:
    Ui::DialogRaycastingSettings *ui;
    static float m_stepSizes[10];
    float m_stepSize;

signals:
    void stepSizeChanged(float);
    void interpolationTypeChanged(RaycastingInterpolationType);
    void enableJitteredSampling(bool);
    void togglePhongShading(bool);
};

#endif // DIALOGRAYCASTINGSETTINGS_H
