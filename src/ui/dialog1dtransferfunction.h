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

#ifndef DIALOG1DTRANSFERFUNCTION_H
#define DIALOG1DTRANSFERFUNCTION_H

#include <QDialog>
#include "qcustomplot.h"
#include "algorithm/volumemanager.h"

class VolumeManager;

namespace Ui {
class Dialog1DTransferFunction;
}
struct Histogram;

class Dialog1DTransferFunction : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog1DTransferFunction(QWidget *parent = 0);
    ~Dialog1DTransferFunction();

    QPoint m_windowPosition;

protected slots:
    void getHistogramData(VolumeManager *vm);
    void itemDoubleClicked(QCPAbstractItem* item, QMouseEvent* event);
    void graphMouseMove(QMouseEvent *);
    void graphMouseRelease(QMouseEvent *);
    void graphMousePress(QMouseEvent *);
    void graphSelectionChanged(bool);
    void contextMenuRequest(QPoint pos);
    void removeSelectedNode();
    void loadTF();
    void saveTF();

private:
    void setupPlots();
    void clearPlots();
    void createColorItem(double key, QColor color);
    void printStats();
    void deselect();
    void updateColorBuffer();
    float* createAutoAlpha();
    Ui::Dialog1DTransferFunction *ui;
    bool m_isDataReady;
    Histogram m_histogram;
    QCPGraph *m_selectedGraph;
    int m_selectedIndex;
    QPointF m_lastPosition;
    int m_moveFloorIndex, m_moveCeilIndex;
    QVector<QCPItemTracer *> m_tracers;
    unsigned char *m_colorBuffer; //RGBA values in a 256 length array extracted from the plots

signals:
    void TFChanged(unsigned char*);
};

#endif // DIALOG1DTRANSFERFUNCTION_H
