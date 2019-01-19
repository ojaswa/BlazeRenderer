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

#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>
#include <QOpenGLDebugLogger>
#include <QOpenGLDebugMessage>

#include "trackball.h"
#include "algorithm/volumemanager.h"
#include "defines.h"

class QOpenGLShaderProgram;

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

    //OpenGL events
public:
    GLWidget(QWidget *parent);
    ~GLWidget();

    void initializeGL();
    void resizeGL(int width, int height);
    void paintGL();
    void teardownGL();

protected slots:
    void update();
    void createVolume(VolumeManager *vm);
    void TF1DChanged(unsigned char * colorBuffer);
    void raycasterStepSizeChanged(float stepSize);
    void raycasterInterpolationTypeChanged(RaycastingInterpolationType type);
    void enableJitteredSampling(bool flag);
    void on_volumeGradientComputed();
    void togglePhongShading(bool flag) { m_PerformPhongShading = flag;}
    void messageLogged(const QOpenGLDebugMessage &msg);

protected:
    void mouseMoveEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent *event);
    void keyPressEvent( QKeyEvent *k );

private:
    //OpenGL State information
    QOpenGLVertexArrayObject m_VAO;
    QOpenGLShaderProgram *m_program;
    int m_nVertices;
    int m_screenWidth, m_screenHeight;
    QVector3D m_volAspect, m_volSpacing;
    TrackBall *m_trackBall;
    VolumeManager *m_volumeManager;
    QOpenGLDebugLogger *m_debugLogger;

    //Uniform variables
    GLuint m_textureVol;
    GLuint m_textureTF1D;//1D RGBA texture
    GLuint m_textureNoise;// Texture of random values
    GLuint m_textureVolNormals; // Normals for volumetric phong shading
    QMatrix4x4 m_view, m_projection;
    float m_stepSize;
    RaycastingInterpolationType m_interpolationtype;
    int m_useJittering;
    bool m_PerformPhongShading;
    QVector3D m_bbox;

    //Address to uniform variables
    int m_uTexVol, m_uTexTF1D, m_uTexNoise, m_uTexVolNormals;
    int m_uTime, m_uInterpolationType;
    int m_uView, m_uProjection;
    int m_uStepSize;
    int m_uUseJittering;
    int m_uBBox;
    int m_uPerformPhongShading;

    // private helpers
    void printContextInformation();
    void normalizeCoordinates(float &x, float &y);
    void createCube();
};

#endif // GLWIDGET_H
