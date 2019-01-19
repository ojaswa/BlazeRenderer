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

#include "dialog1dtransferfunction.h"
#include "ui_dialog1dtransferfunction.h"

#define SELECTION_TOLERANCE 5
#define EPS 1e-6
#define PLOT_MARGIN_Y 0.1
#define PLOT_MARGIN_X 0.02

inline bool lessThanKey(const QCPItemTracer *t1, const QCPItemTracer *t2)
{
    return (t1->graphKey() < t2->graphKey());
}

Dialog1DTransferFunction::Dialog1DTransferFunction(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog1DTransferFunction)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Tool);
    setFixedSize(this->width(),this->height());
    show(); m_windowPosition = pos(); hide();

    //Attach options menu
    QMenu *optionsMenu = new QMenu(this);
    optionsMenu->addAction("Load 1D TF ...", this, SLOT(loadTF()));
    optionsMenu->addAction("Save 1D TF ...", this, SLOT(saveTF()));
    ui->optionsPushButton->setMenu(optionsMenu);
    connect(ui->optionsPushButton, SIGNAL(clicked()), ui->optionsPushButton, SLOT(showMenu()));

    //Connections
    connect(parent, SIGNAL(volumeDataCreated(VolumeManager*)), this, SLOT(getHistogramData(VolumeManager*)));

    //Setup
    //ui->customPlot->setOpenGl(true); // This is problematic
    ui->customPlot->yAxis->ticker()->setTickCount(1);
    ui->customPlot->xAxis->setRange(-PLOT_MARGIN_X, 1.0 + PLOT_MARGIN_X);
    ui->customPlot->yAxis->setRange(-3*PLOT_MARGIN_Y, 1.0 + PLOT_MARGIN_Y);
    ui->customPlot->xAxis->setVisible(false);
    ui->customPlot->yAxis->setVisible(false);
    ui->customPlot->setInteraction(QCP::iSelectPlottables, true);
    ui->customPlot->setSelectionTolerance(SELECTION_TOLERANCE);
    m_selectedGraph = NULL;
    m_selectedIndex = -1;

    connect(ui->customPlot, SIGNAL(itemDoubleClick(QCPAbstractItem*, QMouseEvent*)), this, SLOT(itemDoubleClicked(QCPAbstractItem*, QMouseEvent*)));
    connect(ui->customPlot, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(graphMouseMove(QMouseEvent*)));
    connect(ui->customPlot, SIGNAL(mouseRelease(QMouseEvent*)), this, SLOT(graphMouseRelease(QMouseEvent*)));
    connect(ui->customPlot, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(graphMousePress(QMouseEvent*)));

    // setup policy and connect slot for context menu popup:
    ui->customPlot->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->customPlot, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuRequest(QPoint)));

    //Data
    m_colorBuffer = new unsigned char[256*4];
    m_histogram.m_nbins = -1;
}

Dialog1DTransferFunction::~Dialog1DTransferFunction()
{
    clearPlots();
    delete [] m_colorBuffer;
    delete ui;
}

void Dialog1DTransferFunction::setupPlots()
{
    // Plot log-histogram
    ui->customPlot->addGraph();
    ui->customPlot->graph(0)->setPen(QPen(Qt::lightGray));
    ui->customPlot->graph(0)->setBrush(QBrush(QColor(224, 224, 224, 255)));
    ui->customPlot->graph(0)->setLineStyle(QCPGraph::lsLine);
    ui->customPlot->graph(0)->setSelectable(QCP::stNone);
    if(m_histogram.m_nbins > 0) { //Set data
        QVector<double> x(m_histogram.m_nbins), y(m_histogram.m_nbins);
        float max_freq = m_histogram.m_logFreq[0];
        for(int i=1; i<m_histogram.m_nbins; i++)
            if(max_freq < m_histogram.m_logFreq[i])
                max_freq = m_histogram.m_logFreq[i];
        for(int i=0; i<m_histogram.m_nbins; i++)
        {
            x[i] = float(i)/float(m_histogram.m_nbins - 1); // [0, 1]
            y[i] = m_histogram.m_logFreq[i]/max_freq;
        }
        ui->customPlot->graph(0)->setData(x, y);
    }

    // Initialize color plot
    ui->customPlot->addGraph();
    ui->customPlot->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 9));
    ui->customPlot->graph(1)->setPen(QPen(Qt::black));
    ui->customPlot->graph(1)->setSelectable(QCP::stSingleData);
    connect(ui->customPlot->graph(1), SIGNAL(selectionChanged(bool)), this, SLOT(graphSelectionChanged(bool)));

    //Initialize alpha nodes
    ui->customPlot->addGraph();
    ui->customPlot->graph(2)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 5));
    ui->customPlot->graph(2)->setPen(QPen(Qt::black));
    ui->customPlot->graph(2)->setSelectable(QCP::stSingleData);
    connect(ui->customPlot->graph(2), SIGNAL(selectionChanged(bool)), this, SLOT(graphSelectionChanged(bool)));
}

void Dialog1DTransferFunction::clearPlots()
{
    ui->customPlot->clearItems();// Remove tracers
    ui->customPlot->clearGraphs();// Remove all graphs
    m_tracers.clear();
    deselect();
}

void Dialog1DTransferFunction::createColorItem(double key, QColor color)
{
    QCPItemTracer *tracer = new QCPItemTracer(ui->customPlot);
    tracer->setStyle(QCPItemTracer::tsCircle);
    tracer->setSize(8);
    tracer->setBrush(QBrush(color));
    tracer->setPen(QPen(color));
    tracer->setGraph(ui->customPlot->graph(1));
    tracer->setGraphKey(key);
    tracer->setSelectable(false);
    m_tracers.push_back(tracer);
}

void Dialog1DTransferFunction::itemDoubleClicked(QCPAbstractItem* item, QMouseEvent* event)
{
    //Bring up the color selection dialogBox
    QColor color = QColorDialog::getColor(((QCPItemTracer*)item)->brush().color(), this);
    if(color.isValid()) { //Set this to the color of the node
        ((QCPItemTracer*)item)->setBrush(QBrush(color));
    }
    deselect();
    ui->customPlot->replot();
    updateColorBuffer();
}

void Dialog1DTransferFunction::getHistogramData(VolumeManager *vm)
{
    m_histogram = vm->histogram();
    setupPlots();

    // Add data to color plot
    QVector<double> x_color(2), y_color(2);
    x_color[0] = 0.0; x_color[1] = 1.0;
    y_color[0] = -2*PLOT_MARGIN_Y; y_color[1] = -2*PLOT_MARGIN_Y;
    ui->customPlot->graph(1)->setData(x_color, y_color);
    createColorItem(0.0, Qt::blue);
    createColorItem(1.0, Qt::red);

    //Add data to alpha plot
    QVector<double> x_alpha(2), y_alpha(2);
    x_color[0] = 0.0; x_color[1] = 1.0;
    y_color[0] = 1.0; y_color[1] = 1.0;
    ui->customPlot->graph(2)->setData(x_color, y_color);

    ui->customPlot->replot();
    updateColorBuffer();
}

void Dialog1DTransferFunction::graphSelectionChanged(bool selected)
{
    if(selected)
    {
        m_selectedGraph = (QCPGraph*)ui->customPlot->selectedPlottables().at(0);
        QCPDataSelection selection = m_selectedGraph->selection();
        m_selectedIndex = selection.dataRanges().at(0).begin();

        //Check selection distance
        double x = ui->customPlot->xAxis->coordToPixel((m_selectedGraph->data()->begin() + m_selectedIndex)->key);
        double y = ui->customPlot->yAxis->coordToPixel((m_selectedGraph->data()->begin() + m_selectedIndex)->value);
        x -= m_lastPosition.x();
        y -= m_lastPosition.y();
        double dist = sqrt(x*x + y*y);
        if(dist > 2*SELECTION_TOLERANCE) { //Unselect and insert a new point
            x = ui->customPlot->xAxis->pixelToCoord(m_lastPosition.x());
            y = ui->customPlot->yAxis->pixelToCoord(m_lastPosition.y());
            if(x<0.0) x = 0.0; if(x > 1.0) x = 1.0;
            if(y<0.0) y = 0.0; if(y > 1.0) y = 1.0;
            if(m_selectedGraph == ui->customPlot->graph(1)) y = -2*PLOT_MARGIN_Y; // Constrain Y for color plot
            m_selectedGraph->addData(x, y);
            if(m_selectedGraph == ui->customPlot->graph(1)) { //Create a colored rectangle beneath
                createColorItem(x, Qt::magenta);
            }

            ui->customPlot->deselectAll();
            m_selectedGraph = NULL;
            m_selectedIndex = -1;
        } else { //Compute mouse move bounds for the key.
            //These ranges are always valid. for first point [0 1], for last [n-2, n-1], else [k-1 k+1].
            x = (m_selectedGraph->data()->begin() + m_selectedIndex)->key;
            m_moveFloorIndex = m_selectedGraph->data()->findBegin(x - EPS, true) - m_selectedGraph->data()->constBegin();
            m_moveCeilIndex = m_selectedGraph->data()->findEnd(x + EPS, true) - m_selectedGraph->data()->constBegin() - 1;
        }
        ui->customPlot->replot();
        updateColorBuffer();
    } else {
        m_selectedGraph = NULL;
        m_selectedIndex = -1;
    }
}

void Dialog1DTransferFunction::graphMouseMove(QMouseEvent *event)
{
    //Bug: Selection color does not change currently; setSelection(...) crashes this code.
    if((event->buttons() & Qt::LeftButton) && m_selectedGraph) {
        double x = ui->customPlot->xAxis->pixelToCoord(event->pos().x());
        double y = ui->customPlot->yAxis->pixelToCoord(event->pos().y());
        if(x<0.0) x = 0.0; if(x > 1.0) x = 1.0;
        if(y<0.0) y = 0.0; if(y > 1.0) y = 1.0;

        //Modify data
        if(m_selectedIndex == 0) x = 0;
        else if((m_selectedGraph->data()->end()  - m_selectedGraph->data()->begin() - 1) == m_selectedIndex) x = 1.0;
        if(m_selectedGraph == ui->customPlot->graph(1)) y = -2*PLOT_MARGIN_Y;
        //Within the floor and ceil range, key x can be freely updated
        double keyFloor = (m_selectedGraph->data()->begin() + m_moveFloorIndex)->key;
        double keyCeil = (m_selectedGraph->data()->begin() + m_moveCeilIndex)->key;
        if((x >= keyFloor) && (x <= keyCeil)) {
            (m_selectedGraph->data()->begin() + m_selectedIndex)->key = x;
            (m_selectedGraph->data()->begin() + m_selectedIndex)->value = y;
        } else if(x < keyFloor) {
            double floor_y = (m_selectedGraph->data()->begin() + m_moveFloorIndex)->value;
            double last_x = (m_selectedGraph->data()->begin() + m_selectedIndex)->key;
            m_selectedGraph->data()->remove(last_x);
            (m_selectedGraph->data()->begin() + m_moveFloorIndex)->value = floor_y;
            m_selectedGraph->addData(x, y);
            m_selectedIndex = m_moveFloorIndex;
            m_moveFloorIndex--;
            m_moveCeilIndex--;
        } else if (x > keyCeil) {
            double ceil_y = (m_selectedGraph->data()->begin() + m_moveCeilIndex)->value;
            double last_x = (m_selectedGraph->data()->begin() + m_selectedIndex)->key;
            m_selectedGraph->data()->remove(last_x);
            (m_selectedGraph->data()->begin() + m_moveCeilIndex - 1)->value = ceil_y;
            m_selectedGraph->addData(x, y);
            m_selectedIndex = m_moveCeilIndex;
            m_moveFloorIndex++;
            m_moveCeilIndex++;
        }
        if(m_selectedGraph == ui->customPlot->graph(1)) {
            int i=0;
            foreach(QCPItemTracer *tracer, m_tracers) {
                tracer->setGraphKey((ui->customPlot->graph(1)->data()->begin() + i++)->key);
            }
        }
        ui->customPlot->replot();
        updateColorBuffer();
    }
}

void Dialog1DTransferFunction::graphMousePress(QMouseEvent *event)
{
    m_lastPosition.setX(event->pos().x());
    m_lastPosition.setY(event->pos().y());
}

void Dialog1DTransferFunction::graphMouseRelease(QMouseEvent *event)
{
    deselect();
    qSort(m_tracers.begin(), m_tracers.end(), lessThanKey);
}

void Dialog1DTransferFunction::contextMenuRequest(QPoint pos)
{
    if(m_selectedGraph && (m_selectedIndex >= 0)) {
        //printStats();

        int lastIndex = (m_selectedGraph->data()->end()  - m_selectedGraph->data()->begin() - 1);
        if((m_selectedIndex == 0) || (m_selectedIndex == lastIndex)) return;

        QMenu *menu = new QMenu(this);
        menu->setAttribute(Qt::WA_DeleteOnClose);
        menu->addAction("Remove selected node", this, SLOT(removeSelectedNode()));
        menu->exec(ui->customPlot->mapToGlobal(pos));
    }
}

void Dialog1DTransferFunction::removeSelectedNode()
{
    if(m_selectedGraph && (m_selectedIndex >= 0)) {
        double key = (m_selectedGraph->data()->begin() + m_selectedIndex)->key;
        if(m_selectedGraph == ui->customPlot->graph(1)) { //Remove the associated tracer first, if color graph
            QCPItemTracer *tracer = m_tracers[m_selectedIndex];
            tracer->setGraph(0);
            ui->customPlot->removeItem(tracer);
            m_tracers.remove(m_selectedIndex);
        }
        m_selectedGraph->data()->remove(key);
        deselect();
        ui->customPlot->replot();
        updateColorBuffer();
    }
}

void Dialog1DTransferFunction::deselect()
{
    ui->customPlot->deselectAll();
    m_selectedGraph = NULL;
    m_selectedIndex = -1;
}

void Dialog1DTransferFunction::printStats()
{
    QSharedPointer<QCPGraphDataContainer> data = ui->customPlot->graph(1)->data();
    fprintf(stderr, "\nGraph data:\n");
    for (int i=0; i< data->size(); i++) {
        fprintf(stderr, "%d: %f\n", i, (data->begin() + i)->key);
    }
    fprintf(stderr, "\nTracer data:\n");
    for(int i= 0; i<m_tracers.size(); i++) {
        fprintf(stderr, "%d -> %f\n", i, m_tracers[i]->graphKey());
    }
}

void Dialog1DTransferFunction::updateColorBuffer()
{
    //Extract plot data for color and alpha and populate the RGBA buffer
    QSharedPointer<QCPGraphDataContainer> colorData = ui->customPlot->graph(1)->data();
    QSharedPointer<QCPGraphDataContainer> alphaData = ui->customPlot->graph(2)->data();
    double key, k1, k2;
    QColor c1, c2;
    double a1, a2;
    int i1, i2;
    double frac, alpha;
    for(int i=0; i<256; i++) {
        key = double(i)/255.0;
        //1. Interpolate alpha
       i1 = alphaData->findBegin(key, true) - alphaData->constBegin();
        i2 = alphaData->findEnd(key, true) - alphaData->constBegin() - 1;
        k1 = (alphaData->begin() + i1)->key;
        k2 = (alphaData->begin() + i2)->key;
        frac = (key - k1)/(k2 - k1);
        a1 = (alphaData->begin() + i1)->value;
        a2 = (alphaData->begin() + i2)->value;
        alpha = a1 * (1-frac) + a2 * frac;
        m_colorBuffer[i*4 + 3] = 255.0 * alpha;

        //1. Interpolate color
        //These ranges are always valid. for first point [0 1], for last [n-2, n-1], else [k-1 k+1].
        i1 = colorData->findBegin(key, true) - colorData->constBegin();
        i2 = colorData->findEnd(key, true) - colorData->constBegin() - 1;
        k1 = (colorData->begin() + i1)->key;
        k2 = (colorData->begin() + i2)->key;
        frac = (key - k1)/(k2 - k1);
        c1 = m_tracers[i1]->brush().color();
        c2 = m_tracers[i2]->brush().color();
        //Save colors as premultiplied by alpha (a.k.a. associated colors)
        m_colorBuffer[i*4 + 0] = (c1.red() * (1-frac) + c2.red() * frac)*alpha;
        m_colorBuffer[i*4 + 1] = (c1.green() * (1-frac) + c2.green() * frac)*alpha;
        m_colorBuffer[i*4 + 2] = (c1.blue() * (1-frac) + c2.blue() * frac)*alpha;
    }
    emit TFChanged(m_colorBuffer);
}

float* Dialog1DTransferFunction::createAutoAlpha()
{
    int n = m_histogram.m_nbins;
    float *freq = m_histogram.m_freq;
    float *autoAlpha = new float[n];

    float maxFreq = freq[0]; //Find max freq.
    for (int i=1; i< n; i++)
        if(maxFreq < freq[i]) maxFreq = freq[i];

    float val, sum = 0.0;
    for (int i=1; i< n; i++) { //Normalize freq
        val = (float)(1.0 - 1.5*pow(freq[i]/maxFreq, 0.2));
        if(val > 0.0) sum += val;
        autoAlpha[i] = val;
    }
    sum /= n; //Mean histogram height

    float *temp = new float[n];
    for (int i=0; i< n; i++) { //Lowpass filtering
        int im2 = (i>1)?(i-2):0;
        int im1 = (i>0)?(i-1):0;
        int ip1 = (i<(n-1))?(i+1):(n-1);
        int ip2 = (i<(n-2))?(i+2):(n-1);
        val = (autoAlpha[ip2] + autoAlpha[ip1] + autoAlpha[i] + autoAlpha[im1] + autoAlpha[im2])*0.2;
        val += 0.5 - sum;
        temp[i] = val;
    }

    for (int i=0; i< n; i++) { //Clip to valid range
        autoAlpha[i] = fmin(fmax(temp[i], 0.0), 1.0);
    }
    delete []temp;

    //Convert this into a simple polyline (i.e, simplify the autoAlpha graph)
    //TODO:
    return autoAlpha;
}

void Dialog1DTransferFunction::loadTF()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Load 1D TF"),
                                                    QDir::currentPath(), "1D Color Transfer Function (*.tf1)");
    if(filename.isEmpty()) return;

    QFile file(filename);
    if(!file.open(QFile::ReadOnly | QFile::Text)) {
        fprintf(stderr, "Could not open 1D TF.\n");
        return;
    }

    QXmlStreamReader xmlReader(file.readAll());
    file.close();

    //Clear current graphs
    clearPlots();
    //Initialize graphs
    setupPlots();

    double key, alpha;
    QColor color;
    while(!xmlReader.atEnd()) {
        xmlReader.readNext();
        if(xmlReader.isStartElement()) {
            if(xmlReader.name() == "Alpha") {
                QVector<double> xdata, ydata;
                while(!xmlReader.atEnd()) {
                    xmlReader.readNext();
                    if(xmlReader.isStartElement()) {
                        if(xmlReader.name() == "Node") {
                            key = -1; alpha = -1;
                            foreach(const QXmlStreamAttribute &attr, xmlReader.attributes()) {
                                if(attr.name().toString() == "Key")
                                    key = attr.value().toDouble();
                                if(attr.name().toString() == "Value")
                                    alpha = attr.value().toDouble();
                            }
                            xdata.push_back(key); ydata.push_back(alpha);
                        } else break;
                    }
                }
                ui->customPlot->graph(2)->setData(xdata, ydata);
            }
            if(xmlReader.name() == "Color") {
                while(!xmlReader.atEnd()) {
                    xmlReader.readNext();
                    if(xmlReader.isStartElement()) {
                        if(xmlReader.name() == "Node") {
                            key = -1; color = -1;
                            foreach(const QXmlStreamAttribute &attr, xmlReader.attributes()) {
                                if(attr.name().toString() == "Key")
                                    key = attr.value().toDouble();
                                if(attr.name().toString() == "Value")
                                    color = attr.value().toString();
                            }
                            ui->customPlot->graph(1)->addData(key, -2*PLOT_MARGIN_Y);
                            createColorItem(key, color);
                        } else break;
                    }
                }
            }
        }
    }

    ui->customPlot->replot();
    updateColorBuffer();
}

void Dialog1DTransferFunction::saveTF()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Save 1D TF"),
                                         QDir::currentPath(), "1D Color Transfer Function (*.tf1)");
    if(filename.isEmpty()) return;

    QSharedPointer<QCPGraphDataContainer> colorData = ui->customPlot->graph(1)->data();
    QSharedPointer<QCPGraphDataContainer> alphaData = ui->customPlot->graph(2)->data();
    QFile file(filename);
    double key, alpha;
    QColor color;

    file.open(QIODevice::WriteOnly);
    QXmlStreamWriter xmlWriter(&file);
    xmlWriter.setAutoFormatting(true);
    xmlWriter.writeStartDocument();
    xmlWriter.writeStartElement("TransferFunction1D");
    xmlWriter.writeStartElement("Alpha");
    for(int i=0; i<alphaData->size(); i++) {
        key = (alphaData->begin() + i)->key;
        alpha = (alphaData->begin() + i)->value;
        xmlWriter.writeStartElement("Node");
        xmlWriter.writeAttribute("Key", QString("%1").arg(key));
        xmlWriter.writeAttribute("Value", QString("%1").arg(alpha));
        xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();//Alpha
    xmlWriter.writeStartElement("Color");
    for(int i=0; i<colorData->size(); i++) {
        key = (colorData->begin() + i)->key;
        color = m_tracers[i]->brush().color();
        xmlWriter.writeStartElement("Node");
        xmlWriter.writeAttribute("Key", QString("%1").arg(key));
        xmlWriter.writeAttribute("Value", color.name());
        xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();//Color
    xmlWriter.writeEndDocument();
    file.close();
}
