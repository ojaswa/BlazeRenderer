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
**       Reference: http://image.diku.dk/research/trackballs/index.html   **
****************************************************************************/

#include "trackball.h"

#include <QQuaternion>
#include <math.h>

TrackBall::TrackBall() : m_radius(1.0)
{
    reset();
}

TrackBall::TrackBall(float radius) : m_radius(radius)
{
    reset();
}

QMatrix4x4 const & TrackBall::getCurrentTransform()
{
        m_currentRotation = m_anchorRotation * m_incrementalRotation;
        m_currentZoom  = m_anchorZoom * m_incrementalZoom;
        m_currentTransform = m_currentRotation*m_currentZoom;
        return m_currentTransform;
}

void TrackBall::reset()
{
    for(int i=0; i<3; i++) {
    m_anchorRotatePosition[i] = 0.0;
    m_currentRotatePosition[i] = 0.0;
    m_axis[i] = 0.0;
    }

    m_angle  = 0.0;
    m_anchorRotation.setToIdentity();
    m_anchorRotation.lookAt(QVector3D(0, 0, 2.5), QVector3D(0, 0, 0), QVector3D(0, 1, 0));
    m_currentRotation.setToIdentity();
    m_incrementalRotation.setToIdentity();

    m_anchorZoom.setToIdentity();
    m_currentZoom.setToIdentity();
    m_incrementalZoom.setToIdentity();
    m_currentTransform.setToIdentity();

    projectOntoSurface(m_anchorRotatePosition);
    projectOntoSurface(m_currentRotatePosition);
}

void TrackBall::beginRotate(float x, float y)
{
    m_angle  = 0.0;
    m_axis[0] = 0.0; m_axis[1] = 0.0; m_axis[2] = 0.0;

    m_anchorRotation = m_currentRotation;
    m_incrementalRotation.setToIdentity();
    m_currentRotation.setToIdentity();

    m_anchorRotatePosition[0] = x; m_anchorRotatePosition[1] = y; m_anchorRotatePosition[2] = 0;
    projectOntoSurface(m_anchorRotatePosition);
    m_currentRotatePosition[0] = x; m_currentRotatePosition[1] = y; m_currentRotatePosition[2] = 0;
    projectOntoSurface(m_currentRotatePosition);
}

void TrackBall::rotate(float x, float y)
{
    m_currentRotatePosition[0] = x; m_currentRotatePosition[1] = y; m_currentRotatePosition[2] = 0;
    projectOntoSurface(m_currentRotatePosition);
    computeIncrementalRotation();
}

void TrackBall::endRotate(float x, float y)
{
    m_currentRotatePosition[0] = x; m_currentRotatePosition[1] = y; m_currentRotatePosition[2] = 0;
    projectOntoSurface(m_currentRotatePosition);
    computeIncrementalRotation();
}

void TrackBall::computeIncrementalRotation()
{
    QQuaternion q_anchor (0, m_anchorRotatePosition.normalized());
    QQuaternion q_current(0, m_currentRotatePosition.normalized());
    QQuaternion q_rot = -q_anchor * q_current;

    m_axis = q_rot.vector().normalized();
    m_angle = atan2(q_rot.vector().length(), q_rot.scalar());

    QMatrix3x3 rot = q_rot.toRotationMatrix();

    for(int row=0; row<3; row++)
        for(int col=0; col<3; col++)
            m_incrementalRotation(row,col) = rot(row,col);

}

void TrackBall::projectOntoSurface(QVector3D &p)
{
    float radius2 = m_radius * m_radius;
    float length2 = p[0]*p[0] + p[1]*p[1];

    if(length2 <= radius2/2.0)
        p[2] = sqrt(radius2 - length2);
    else {
        p[2] = radius2 / (2.0*sqrt(length2));
        //p.normalize();
    }
    p.normalize();

    //Convert to world space
    QVector4D p_world = m_anchorRotation.inverted()*QVector4D(p);
    p = p_world.toVector3D().normalized();
}

void TrackBall::beginZoom(float x, float y)
{
    m_anchorZoom = m_currentZoom;
    m_incrementalZoom.setToIdentity();
    m_currentZoom.setToIdentity();

    m_anchorZoomPosition[0] = x; m_anchorZoomPosition[1] = y; m_anchorZoomPosition[2] = 0;
    //projectOntoSurface(m_anchorZoomPosition);
    m_currentZoomPosition[0] = x; m_currentZoomPosition[1] = y; m_currentZoomPosition[2] = 0;
    //projectOntoSurface(m_currentZoomPosition);
}

void TrackBall::zoom(float x, float y)
{
    m_currentZoomPosition[0] = x; m_currentZoomPosition[1] = y; m_currentZoomPosition[2] = 0;
    //projectOntoSurface(m_currentZoomPosition);
    computeIncrementalZoom();
}

void TrackBall::endZoom(float x, float y)
{
    m_currentZoomPosition[0] = x; m_currentZoomPosition[1] = y; m_currentZoomPosition[2] = 0;
    //projectOntoSurface(m_currentZoomPosition);
    computeIncrementalZoom();
}

void TrackBall::computeIncrementalZoom()
{
    float dy = m_currentZoomPosition.y() - m_anchorZoomPosition.y();
    float scale  = 2.0/(1.0 + exp(0.1*dy)); //Apply sigmoid function
    m_incrementalZoom.scale(scale);
}
