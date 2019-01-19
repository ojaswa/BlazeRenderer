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

#ifndef VOLUMEMANAGER_H
#define VOLUMEMANAGER_H

#include <QObject> //Need this to use Signal-Slot mechanism

#define TINY 1e-12

struct Histogram {
    int m_nbins;
    float* m_logFreq;
    float *m_freq;
};

#include <itkImage.h>
#include <itkCovariantVector.h>
#include <itkInterpolateImageFilter.h>
#include <vtkImageData.h>

typedef itk::Image<float, 3> FloatImageType;
typedef itk::CovariantVector<float, 3> GradientType;
typedef itk::Image<GradientType, 3> GradientImageType;
typedef itk::InterpolateImageFunction<FloatImageType, float> ImageInterpolatorType;
typedef itk::InterpolateImageFunction<GradientImageType, float> GradientInterpolatorType;

class VolumeManager : public QObject
{
    Q_OBJECT

public:
    VolumeManager();
    ~VolumeManager();
    void readNHDR(const char *filename);
    void readVTK(const char* filename);
    int const & width() const { return m_width;}
    int const & height() const { return m_height;}
    int const & depth() const { return m_depth;}
    float const & spacingX() const { return m_spacingX;}
    float const & spacingY() const { return m_spacingY;}
    float const & spacingZ() const { return m_spacingZ;}
    float* data() { return m_data;}
    unsigned char* getCannyEdges() { return m_cannyEdges;}
    float* gradient() { return m_gradient;}
    itk::Image<float, 3>::Pointer getITKImage();
    Histogram const & histogram() const { return m_histogram; }
    void preprocess(); //Perform preprocessing and data preparation

signals:
    void volumeDataCreated(VolumeManager *vm);
    void volumeEdgesComputed(VolumeManager *vm);
    void volumeGradientComputed(VolumeManager *vm);
    void volumePreprocessCompleted(VolumeManager *vm);

private:
    int m_width, m_height, m_depth;
    float m_spacingX, m_spacingY, m_spacingZ;
    float m_min, m_max; // Min, Max of the original data.
    char* m_volumeName;
    char* filePathName;
    Histogram m_histogram;

    //Derived data
    float *m_data; // Normalized voxel values in the range [0, 1]
    unsigned char *m_cannyEdges;// Edge voxels marked as 255
    float *m_gradient; // Stored as gx, gy, gz, gx, gy, gz, ...

    //Private functions
    void computeCannyEdges(); // Canny edge detection on volume
    void computeGradient();
    void computeHistogram();
};

#endif // VOLUMEMANAGER_H
