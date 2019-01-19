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

#ifndef TRACKBALL_H
#define TRACKBALL_H

#include <QVector3D>
#include <QMatrix4x4>

class TrackBall
{
public:
    TrackBall();
    TrackBall(float radius);
    float& radius() {return m_radius;}
    QVector3D& anchor() {m_anchorRotatePosition.normalize(); return m_anchorRotatePosition;}
    QVector3D& current() {m_currentRotatePosition.normalize(); return m_currentRotatePosition;}
    float& angle() {return m_angle;}
    QVector3D& axis() {m_axis.normalize(); return m_axis;}
    QMatrix4x4 const & getCurrentTransform();
    QMatrix4x4 const & getIncrementalRotation() const {return m_incrementalRotation;}

    void reset();
    void beginRotate(float x, float y);
    void rotate(float x, float y);
    void endRotate(float x, float y);

    void beginZoom(float x, float y);
    void zoom(float x, float y);
    void endZoom(float x, float y);

private:
    float m_radius;
    QVector3D m_anchorRotatePosition;
    QVector3D m_currentRotatePosition;
    QVector3D m_anchorZoomPosition;
    QVector3D m_currentZoomPosition;
    float m_angle;
    QVector3D m_axis;
    QMatrix4x4 m_anchorRotation;
    QMatrix4x4 m_currentRotation;
    QMatrix4x4 m_incrementalRotation;
    QMatrix4x4 m_anchorZoom;
    QMatrix4x4 m_currentZoom;
    QMatrix4x4 m_incrementalZoom;
    QMatrix4x4 m_currentTransform;

    void computeIncrementalRotation();
    void projectOntoSurface(QVector3D &p);
    void computeIncrementalZoom();
};

#endif // TRACKBALL_H
