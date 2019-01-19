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

#include "glwidget.h"

#include <QDebug>
#include <QString>
#include <QOpenGLShaderProgram>
#include <QGLFormat>
#include <QMouseEvent>
#include <OpenGLError>

#include <math.h>
#include <time.h>
#include <limits.h>

GLWidget::GLWidget(QWidget *parent) : QOpenGLWidget(parent), m_debugLogger(Q_NULLPTR)
{
    //Widget specific
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    //Connections
    connect(parent, SIGNAL(volumeDataCreated(VolumeManager*)), this, SLOT(createVolume(VolumeManager*)));

    // Initialize the GL context before the window is shown, otherwise we’ll end up with a Compatability Profile
    QSurfaceFormat format;
    format.setRenderableType( QSurfaceFormat::OpenGL ) ;
    format.setProfile( QSurfaceFormat::CoreProfile );
#if GL_DEBUG
    format.setOption(QSurfaceFormat::DebugContext);
#endif
    format.setVersion( 4, 1);
    QSurfaceFormat::setDefaultFormat(format); //This is required to get a valid context!
    setFormat( format );
#if GL_DEBUG
    OpenGLError::pushErrorHandler(this);
#endif

    // Data
    m_nVertices = 0;
    m_trackBall = new TrackBall(1.5);
    m_volumeManager = NULL;
    m_stepSize = 0.01;
    m_interpolationtype = InterpolationTrilinear;
    m_useJittering = 0;
    m_PerformPhongShading = true;
}

GLWidget::~GLWidget()
{
    makeCurrent();
    teardownGL();

    delete m_trackBall;
}

// OpenGL Events

void GLWidget::initializeGL()
{
    //Initialize OpenGL Backend
    initializeOpenGLFunctions();
    connect(this, SIGNAL(frameSwapped()), this, SLOT(update()));
    printContextInformation();

    //Set global information
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);

    glEnable(GL_DEPTH);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //Create GL programs and buffers
    m_program = new QOpenGLShaderProgram();
    m_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/cube.vs");
    m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/cube.fs");
    m_program->link();

    m_VAO.create();
    createCube();

    //Application specific initialization
#if GL_DEBUG
    m_debugLogger = new QOpenGLDebugLogger(context());
    if (m_debugLogger->initialize()) {
        qDebug() << "GL_DEBUG Debug Logger" << m_debugLogger << "\n";
        connect(m_debugLogger, SIGNAL(messageLogged(QOpenGLDebugMessage)), this, SLOT(messageLogged(QOpenGLDebugMessage)));
        m_debugLogger->startLogging();
    }
#endif
}

void GLWidget::resizeGL(int width, int height)
{
    m_screenWidth = width;
    m_screenHeight = height;
    m_projection.setToIdentity();
    m_projection.perspective(45.0f, m_screenWidth/float(m_screenHeight), 0.0f, 1000.0f);
}

void GLWidget::paintGL()
{
    // Clear
    glClear(GL_COLOR_BUFFER_BIT);

    //Render volume
    if(!m_volumeManager) return;

    m_program->bind();
    {

        glActiveTexture(GL_TEXTURE0);
        //glEnable(GL_TEXTURE_3D);
        glBindTexture(GL_TEXTURE_3D, m_textureVol);
        m_program->setUniformValue(m_uTexVol, 0);

        glActiveTexture(GL_TEXTURE1);
        //glEnable(GL_TEXTURE_1D);
        glBindTexture(GL_TEXTURE_1D, m_textureTF1D);
        m_program->setUniformValue(m_uTexTF1D, 1);

        glActiveTexture(GL_TEXTURE2);
        //glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, m_textureNoise);
        m_program->setUniformValue(m_uTexNoise, 2);

        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_3D, m_textureVolNormals);
        m_program->setUniformValue(m_uTexVolNormals, 4);

        m_program->setUniformValue(m_uProjection, m_projection);
        m_view = m_trackBall->getCurrentTransform();
        m_program->setUniformValue(m_uView, m_view);
        m_program->setUniformValue(m_uTime, (float)clock()/CLOCKS_PER_SEC);
        m_program->setUniformValue(m_uStepSize, m_stepSize);
        m_program->setUniformValue(m_uUseJittering, m_useJittering);
        m_program->setUniformValue(m_uBBox, m_bbox);
        m_program->setUniformValue(m_uPerformPhongShading, m_PerformPhongShading?1:0);

        m_VAO.bind();
        glDrawArrays(GL_TRIANGLES, 0, m_nVertices);
        m_VAO.release();

        glBindTexture(GL_TEXTURE_3D, 0);
        glBindTexture(GL_TEXTURE_1D, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    m_program->release();

}

void GLWidget::update()
{
    if(!m_volumeManager) return;

    QOpenGLWidget::update();
}

void GLWidget::teardownGL()
{
    if(!m_volumeManager) return;

    m_VAO.destroy();
    if(m_program) delete m_program;
    glDeleteTextures(1, &m_textureVol);
    glDeleteTextures(1, &m_textureTF1D);
    glDeleteTextures(1, &m_textureNoise);
    glDeleteTextures(1, &m_textureVolNormals);
}

// OpenGL helper functions

void GLWidget::printContextInformation()
{
    QString glType;
    QString glVersion;
    QString glProfile;

    //Get GL version info
    glType = (context()->isOpenGLES())?"OpenGL ES" : "OpenGL";
    glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));

    //Get profile information

#define CASE(c) case QSurfaceFormat::c: glProfile = #c; break
    switch(format().profile())
    {
        CASE(NoProfile);
        CASE(CoreProfile);
        CASE(CompatibilityProfile);
    }
#undef CASE

    qDebug() <<qPrintable(glType)<<qPrintable(glVersion)<<"("<<qPrintable(glProfile) << ")";

}

void GLWidget::createVolume(VolumeManager *vm)
{
    m_volumeManager = vm;
    float maxdim = fmax(vm->width(), fmax(vm->height(), vm->depth()));
    m_volAspect[0] = vm->width()/maxdim;
    m_volAspect[1] = vm->height()/maxdim;
    m_volAspect[2] = vm->depth()/maxdim;
    //Spacing is usually 1, but can be different in a certain dimension
    m_volSpacing[0] = vm->spacingX();
    m_volSpacing[1] = vm->spacingY();
    m_volSpacing[2] = vm->spacingZ();
    m_bbox = m_volSpacing*m_volAspect;

    m_program->bind(); // Equivalent to glUseProgram

    //Get attribute/uniform locations
    m_uView = m_program->uniformLocation("uView");
    m_uProjection = m_program->uniformLocation("uProjection");
    m_uTexVol = m_program->uniformLocation("uTexVol");
    m_uTexTF1D = m_program->uniformLocation("uTexTF1D");
    m_uTexNoise = m_program->uniformLocation("uTexNoise");
    m_uTexVolNormals = m_program->uniformLocation("uTexVolNormals");
    m_uTime = m_program->uniformLocation("uTime");
    m_uStepSize  = m_program->uniformLocation("uStepSize");
    m_uUseJittering = m_program->uniformLocation("uUseJittering");
    m_uBBox = m_program->uniformLocation("uBBox");
    m_uPerformPhongShading = m_program->uniformLocation("uPerformPhongShading");

    //Prepare texture
    int width = m_volumeManager->width();
    int height = m_volumeManager->height();
    int depth = m_volumeManager->depth();
    glGenTextures(1, &m_textureVol);
    glBindTexture(GL_TEXTURE_3D, m_textureVol);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F,
                 width, height, depth,
                 0, GL_RED, GL_FLOAT, (GLvoid*)m_volumeManager->data());
    glBindTexture(GL_TEXTURE_3D, 0);


    //Create 1D texture for Trasfer function (size: 256)
    glGenTextures(1, &m_textureTF1D);
    glBindTexture(GL_TEXTURE_1D, m_textureTF1D);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA8, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL); //Need to fill data into this texture later.
    glBindTexture(GL_TEXTURE_1D, 0);

    //Create 2D texture for noise (size: 32 x 32)
    int size = 32;
    unsigned char * buffer  = new unsigned char[size*size];
    srand((unsigned)time(NULL));
    for (int i=0; i<(size*size); i++) {
        buffer[i] = 255.*rand()/(float)RAND_MAX;
    }
    glGenTextures(1, &m_textureNoise);
    glBindTexture(GL_TEXTURE_2D, m_textureNoise);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 32, 32, 0, GL_RED, GL_UNSIGNED_BYTE, buffer);
    glBindTexture(GL_TEXTURE_2D, 0);
    delete [] buffer;

    //Generate RGBA normals texture (fill it up with 127: (0.5 in float))
    long nelem = width*height*depth;
    unsigned char *normals = new unsigned char[4*nelem];
    memset(normals, 127, 4*nelem);
    glGenTextures(1, &m_textureVolNormals);
    glBindTexture(GL_TEXTURE_3D, m_textureVolNormals);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8,
                 width, height, depth,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, normals);
    glBindTexture(GL_TEXTURE_3D, 0);
    delete [] normals;

    m_program->release();

    update();
}

void GLWidget::createCube()
{
    //Geometry data: [-1, 1]^3
    GLfloat cube_vertices[] = {1, 1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1, //Front
                   1, 1, -1, -1, 1, -1, -1, -1, -1, 1, -1, -1}; //Back
    GLushort cube_indices[] = {0, 2, 3, 0, 1, 2, //Front
                4, 7, 6, 4, 6, 5, //Back
                5, 2, 1, 5, 6, 2, //Left
                4, 3, 7, 4, 0, 3, //Right
                1, 0, 4, 1, 4, 5, //Top
                2, 7, 3, 2, 6, 7}; //Bottom
    m_nVertices = 6*2*3; //(6 faces) * (2 triangles each) * (3 vertices each)
    GLfloat *expanded_vertices = new GLfloat[m_nVertices*3];

    for(int i=0; i<m_nVertices; i++) {
        expanded_vertices[i*3] = cube_vertices[cube_indices[i]*3];
        expanded_vertices[i*3 + 1] = cube_vertices[cube_indices[i]*3+1];
        expanded_vertices[i*3 + 2] = cube_vertices[cube_indices[i]*3+2];
    }

    m_program->bind(); // Equivalent to glUseProgram

    //Setup VBO
    m_VAO.bind();

    QOpenGLBuffer cube_VBO(QOpenGLBuffer::VertexBuffer);
    cube_VBO.create();
    //cube_VBO.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    cube_VBO.bind();
    cube_VBO.allocate(expanded_vertices, m_nVertices*3*sizeof(GLfloat));

    //Bind VAO and save state

    m_program->enableAttributeArray(0);
    m_program->setAttributeBuffer(0, GL_FLOAT, 0, 3);

    //Release/unbind all
    cube_VBO.release();
    m_VAO.release();

    m_program->release();

    delete[] expanded_vertices;
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
    if(event->buttons() == Qt::LeftButton) {
        float x = event->x();
        float y = event->y();
        normalizeCoordinates(x, y);
        m_trackBall->rotate(x, y);

        update();
    } else if(event->buttons() == Qt::RightButton) {
        float x = event->x();
        float y = event->y();
        normalizeCoordinates(x, y);
        m_trackBall->zoom(x, y);

        update();
    }
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
    if(event->buttons() == Qt::LeftButton) {
        float x = event->x();
        float y = event->y();
        normalizeCoordinates(x, y);
        m_trackBall->beginRotate(x, y);

        update();
    } else if(event->buttons() == Qt::RightButton) {
        float x = event->x();
        float y = event->y();
        normalizeCoordinates(x, y);
        m_trackBall->beginZoom(x, y);

        update();
    }
}

void GLWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->buttons() == Qt::LeftButton) {
        float x = event->x();
        float y = event->y();
        normalizeCoordinates(x, y);
        m_trackBall->endRotate(x, y);

        update();
    } else if(event->buttons() == Qt::RightButton) {
        float x = event->x();
        float y = event->y();
        normalizeCoordinates(x, y);
        m_trackBall->endZoom(x, y);

        update();
    }
}

void GLWidget::keyPressEvent(QKeyEvent *k)
{

}

void GLWidget::normalizeCoordinates(float &x, float &y)
{
  x = -(2.0 * x / (m_screenWidth - 1)  - 1.0);
  if (x < -1.0) x = -1.0;
  if (x >  1.0) x =  1.0;

  y = (2.0 * y / (m_screenHeight - 1.0) - 1.0);
  if (y < -1.0) y = -1.0;
  if (y >  1.0) y =  1.0;
}

void GLWidget::TF1DChanged(unsigned char *colorBuffer)
{
    //colorbuffer has associated colors (i.e., alpha is premultiplied)
    //glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, m_textureTF1D);
    glTexSubImage1D(GL_TEXTURE_1D, 0, 0, 256, GL_RGBA, GL_UNSIGNED_BYTE, colorBuffer);
}

void GLWidget::raycasterStepSizeChanged(float stepSize)
{
    m_stepSize = stepSize;
}

void GLWidget::raycasterInterpolationTypeChanged(RaycastingInterpolationType type)
{
    m_interpolationtype = type;

    if(m_interpolationtype == InterpolationNearestNeighbour) {
        glBindTexture(GL_TEXTURE_3D, m_textureVol);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    else if(m_interpolationtype == InterpolationTrilinear)  {
        glBindTexture(GL_TEXTURE_3D, m_textureVol);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else
        fprintf(stderr, "Unknown texture interpolation mode. Ignoring...\n");

    glBindTexture(GL_TEXTURE_3D, 0);
}

void GLWidget::enableJitteredSampling(bool flag)
{
    m_useJittering = (flag)?1:0;
}

void GLWidget::on_volumeGradientComputed()
{
    float *gradient = m_volumeManager->gradient();

    int width = m_volumeManager->width();
    int height = m_volumeManager->height();
    int depth = m_volumeManager->depth();
    long nelem = width*height*depth;

    //Rescale gradient field to have magnitudes in [0, 1]
    float *gradmag = new float[nelem];
    float gx, gy, gz;
    float gmag_max = std::numeric_limits<float>::min();
    for(long i=0; i<nelem; i++) {
            gx = gradient[3*i];
            gy = gradient[3*i+1];
            gz = gradient[3*i+2];
            gradmag[i] = sqrtf(gx*gx + gy*gy + gz*gz);
            if(gradmag[i] > gmag_max) gmag_max = gradmag[i];
    }
    for(long i=0; i<nelem; i++) {
     gradmag[i] /= gmag_max;
    }

    //Encode rescaled shading normals to a uchar texture
    unsigned char *normals = new unsigned char[4*nelem];
    for(long i=0; i<nelem; i++) {
        gx = gradient[3*i];
        gy = gradient[3*i+1];
        gz = gradient[3*i+2];
        normals[4*i] = (unsigned char)(255.0*(gx/gmag_max + 1.0)/2.0); //Map vector [-1, +1] -> [0, 1]
        normals[4*i+1] = (unsigned char)(255.0*(gy/gmag_max + 1.0)/2.0);
        normals[4*i+2] = (unsigned char)(255.0*(gz/gmag_max + 1.0)/2.0);
        normals[4*i+3] = 0;
    }

    //Upload normals - destroy the old texture and recreate new (Updating the existing 3D texture does not work on MacOS)
    glDeleteTextures(1, &m_textureVolNormals);
    glGenTextures(1, &m_textureVolNormals);
    glBindTexture(GL_TEXTURE_3D, m_textureVolNormals);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, width, height, depth, 0, GL_RGBA, GL_UNSIGNED_BYTE, normals);
    //glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, width, height, depth, GL_RGBA, GL_UNSIGNED_BYTE, normals);
    glBindTexture(GL_TEXTURE_3D, 0);

    //Clean up
    delete []normals;
}

void GLWidget::messageLogged(const QOpenGLDebugMessage &msg)
{
#if GL_DEBUG
    QString error;
    // Format based on severity
    switch (msg.severity())
    {
    case QOpenGLDebugMessage::NotificationSeverity:
        error += "--";
        break;
    case QOpenGLDebugMessage::HighSeverity:
        error += "!!";
        break;
    case QOpenGLDebugMessage::MediumSeverity:
        error += "!~";
        break;
    case QOpenGLDebugMessage::LowSeverity:
        error += "~~";
        break;
    }

    error += " (";

    // Format based on source
#define CASE(c) case QOpenGLDebugMessage::c: error += #c; break
    switch (msg.source())
    {
    CASE(APISource);
    CASE(WindowSystemSource);
    CASE(ShaderCompilerSource);
    CASE(ThirdPartySource);
    CASE(ApplicationSource);
    CASE(OtherSource);
    CASE(InvalidSource);
    }
#undef CASE

    error += " : ";

    // Format based on type
#define CASE(c) case QOpenGLDebugMessage::c: error += #c; break
    switch (msg.type())
    {
    CASE(ErrorType);
    CASE(DeprecatedBehaviorType);
    CASE(UndefinedBehaviorType);
    CASE(PortabilityType);
    CASE(PerformanceType);
    CASE(OtherType);
    CASE(MarkerType);
    CASE(GroupPushType);
    CASE(GroupPopType);
    }
#undef CASE

    error += ")";
    qDebug() << qPrintable(error) << "\n" << qPrintable(msg.message()) << "\n";
#endif
}
