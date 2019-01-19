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

#include "volumemanager.h"
#include "defines.h"

#include <fstream>
#include <string>
#include <QFileInfo>
#include <QDir>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <QColor>
#include <cstdlib>
#include <math.h>
#include <float.h>

//ITK includes
#include <itkCannyEdgeDetectionImageFilter.h>
#include <itkImportImageFilter.h>
#include <itkRescaleIntensityImageFilter.h>
#include <itkGradientImageFilter.h>
#include <itkDiscreteGaussianImageFilter.h>
#include <itkGradientMagnitudeImageFilter.h>
#include <itkImageToHistogramFilter.h>
#include <itkMinimumMaximumImageCalculator.h>
#include <itkLinearInterpolateImageFunction.h>
#include <itkNearestNeighborInterpolateImageFunction.h>
#include <itkContinuousIndex.h>
#include <vtkImageCast.h>
#include <itkMedianImageFilter.h>
#include <itkBinaryDilateImageFilter.h>
#include <itkBinaryBallStructuringElement.h>
#include <itkSmoothingRecursiveGaussianImageFilter.h>

//VTK includes
#include <vtkImageData.h>
#include <vtkImageShiftScale.h>
#include <vtkImageResize.h>
#include <vtkImageCast.h>
#include <vtkStructuredPointsWriter.h>
#include <vtkImageToStructuredPoints.h>
#include <vtkStructuredPointsReader.h>
#include <vtkStructuredPoints.h>
#include <vtkNrrdReader.h>

using namespace std;

VolumeManager::VolumeManager()
{
    m_data = NULL;
    m_width = m_height = m_depth = 0;
    m_min = m_max = 0.0;
    m_volumeName = new char[256];
    m_histogram.m_nbins = 256;
    m_histogram.m_logFreq = new float[m_histogram.m_nbins];
    m_histogram.m_freq = new float[m_histogram.m_nbins];

    m_cannyEdges = NULL;
    m_gradient = NULL;
}

VolumeManager::~VolumeManager()
{
    delete []m_volumeName;
    if(m_data) delete []m_data;
    if(m_histogram.m_freq) delete []m_histogram.m_freq;
    if(m_histogram.m_logFreq) delete []m_histogram.m_logFreq;
    m_histogram.m_nbins = 0;
    if(m_cannyEdges) delete []m_cannyEdges;
    if(m_gradient) delete []m_gradient;
}

void VolumeManager::readNHDR(const char *filename)
{
    //Read the .nhdr first in text mode
    string line;
    int linecount = 0;
    ifstream fid(filename);
    int vol_typeSize;
    char datafilename[256];
    if(fid) {
        while (getline(fid, line)) {
            if (strncmp(line.c_str(), "encoding", 8) == 0) {
                //Has to be raw
                if(line.find("raw") == string::npos) {
                    fprintf(stderr, "NHDR file not in RAW format!");
                    fid.close();
                    return;
                }
            } else if (strncmp(line.c_str(), "content", 7) == 0) {
                //Name of the volume
                sscanf(line.c_str(), "content: %s", m_volumeName);
            } else if (strncmp(line.c_str(), "type", 4) == 0) {
                char type_str[256];
                sscanf(line.c_str(), "type: %[^\n\r]\n", type_str);
                if (strncmp(type_str, "unsigned char", 13) == 0){
                    vol_typeSize = 1;
                }
                else if (strncmp(type_str, "unsigned short", 14) == 0){
                    vol_typeSize = 2;
                }
                else {
                    fprintf(stderr, "Unknown data type: %s\n", type_str);
                    fid.close();
                    return;
                }
            } else if (strncmp(line.c_str(), "sizes", 5) == 0) {
                sscanf(line.c_str(), "sizes: %d %d %d\n", &m_width, &m_height, &m_depth);
            } else if (strncmp(line.c_str(), "spacings", 8) == 0) {
                sscanf(line.c_str(), "spacings: %f %f %f\n", &m_spacingX, &m_spacingY, &m_spacingZ);
            } else if (strncmp(line.c_str(), "dimension", 9) == 0) {
                int dim;
                sscanf(line.c_str(), "dimension: %d\n", &dim);
                if(dim != 3) {
                    fprintf(stderr, "Not a 3D data!");
                    fid.close();
                    return;
                }
            } else if (strncmp(line.c_str(), "data file", 9) == 0) {
                sscanf(line.c_str(), "data file: %[^\n\r]\n", datafilename);
            }
            linecount++;
        }
    }
    fid.close();

    //Load data from binary raw file
    int nelements = m_width*m_height*m_depth;
    m_data  = new float[nelements];

    QFileInfo nhdr_file(filename);
    QFileInfo raw_file(datafilename);
    QString qdatafile("");
    qdatafile.append(nhdr_file.path()).append("/").append(raw_file.fileName());
    FILE *data_fid = fopen(qdatafile.toStdString().c_str(), "rb");
    if(data_fid) {
#define IO_BLOCK_SIZE 4096
        char *block4k = new char[IO_BLOCK_SIZE*vol_typeSize];
        int elements_read;
        int k=0;
        do {
            elements_read = fread((void*)block4k, vol_typeSize, IO_BLOCK_SIZE, data_fid);
            if(elements_read < 0) break;
            //Convert from NHRD data type to float and store in volume
            if(vol_typeSize == 1)
                for(int i=0; i<elements_read; i++) *(m_data + k++) = (float)*((unsigned char*)block4k + i);
            else if(vol_typeSize == 2)
                for(int i=0; i<elements_read; i++) *(m_data + k++) = (float)*((unsigned short*)block4k + i);
        } while(elements_read == IO_BLOCK_SIZE);

        delete []block4k;
        fclose(data_fid);
    }

    //Find min-max
    m_min = m_data[0];
    m_max = m_data[0];
    for(int i=1; i<nelements; i++) {
        if (m_data[i] < m_min) m_min = m_data[i];
        if (m_data[i] > m_max) m_max = m_data[i];
    }

    //Rescale data to [0, 1]
    for(int i=0; i<nelements; i++) {
        m_data[i] = (m_data[i] - m_min) / (m_max - m_min);
    }

    fprintf(stderr, "Read volume: %s\n", filename);
    fprintf(stderr, "\tName: %s\n", m_volumeName);
    fprintf(stderr, "\tType: %s\n", (vol_typeSize==1)?"unsigned char":(vol_typeSize == 2)?"unsigned short":"unknown");
    fprintf(stderr, "\tSize: %d x %d x %d\n", m_width, m_height, m_depth);
    fprintf(stderr, "\tSpacing: %f x %f x %f\n", m_spacingX, m_spacingY, m_spacingZ);
    fprintf(stderr, "\tData range: [%f, %f] normalized to [0, 1]\n", m_min, m_max);

    computeHistogram();

    emit volumeDataCreated(this);
}

void VolumeManager::preprocess()
{
    //Add any volume preprocessing code here.
    fprintf(stderr, "Processing volume:\n");

#if TIME_PROCESSES
    computeCannyEdges();
    computeGradient();
#else
    //Run independent tasks in seperate threads with QFuture class
    //N.B.: Any task that depends on another must be started after the previous one has finished (use waitForFinished())

    QFuture<void> futureComputeCannyEdges = QtConcurrent::run(this, &VolumeManager::computeCannyEdges);
    QFuture<void> futureComputeGradient = QtConcurrent::run(this, &VolumeManager::computeGradient);

    futureComputeCannyEdges.waitForFinished();
    futureComputeGradient.waitForFinished();
#endif

    emit volumePreprocessCompleted(this);
    fprintf(stderr, "Done.\n");
}

void VolumeManager::readVTK(const char *filename)
{
    //TODO:
}

void VolumeManager::computeHistogram()
{
    int count = m_width*m_height*m_depth;
    for(int i=0; i<m_histogram.m_nbins; i++)
        m_histogram.m_freq[i] = 0.0;

    for(int i=0; i<count; i++)
        m_histogram.m_freq[(int)ceil(m_data[i]*m_histogram.m_nbins)]++;

    for(int i=0; i<m_histogram.m_nbins; i++)
        m_histogram.m_logFreq[i] = log(1.0 + m_histogram.m_freq[i]);
}

void VolumeManager::computeCannyEdges()
{
    fprintf(stderr, "\tDetecting Canny edges... \n");

    typedef itk::Image<float, 3> InputImageType;
    typedef itk::Image<unsigned char, 3> OutputImageType;

    // Just some good parameters for Canny. Change if required.
    float variance = 1.0;
    float lowerThreshold = 0.05;
    float upperThreshold = 0.1;

    typedef itk::CannyEdgeDetectionImageFilter<InputImageType, InputImageType> FilterType;
    FilterType::Pointer cannyFilter = FilterType::New();
    cannyFilter->SetInput(getITKImage());
    cannyFilter->SetVariance(variance);
    cannyFilter->SetLowerThreshold(lowerThreshold);
    cannyFilter->SetUpperThreshold(upperThreshold);

    typedef itk::RescaleIntensityImageFilter<InputImageType, OutputImageType> RescaleType;
    RescaleType::Pointer rescalar = RescaleType::New();
    rescalar->SetInput(cannyFilter->GetOutput());
    rescalar->SetOutputMinimum(0);
    rescalar->SetOutputMaximum(255);

    //Erode Canny output
    typedef itk::BinaryBallStructuringElement<OutputImageType::PixelType,3> StructuringElementType;
    StructuringElementType structuringElement;
    structuringElement.SetRadius(1);
    structuringElement.CreateStructuringElement();
    typedef itk::BinaryDilateImageFilter <OutputImageType, OutputImageType, StructuringElementType> BinaryDilateImageFilterType;
    BinaryDilateImageFilterType::Pointer dilateFilter = BinaryDilateImageFilterType::New();
    dilateFilter->SetInput(rescalar->GetOutput());
    dilateFilter->SetKernel(structuringElement);

    OutputImageType *output = dilateFilter->GetOutput();
    output->Update();
    OutputImageType::PixelContainer *container;
    container = output->GetPixelContainer();
    container->SetContainerManageMemory(false);
    m_cannyEdges = (unsigned char*) container->GetImportPointer();

    //Signal task completion to the application
    emit volumeEdgesComputed(this);
}

void  VolumeManager::computeGradient() {
    fprintf(stderr, "\tComputing gradient... \n");

    typedef itk::CovariantVector<float, 3> GradientType;
    typedef itk::Image<float, 3> InputImageType;
    typedef itk::Image<GradientType, 3> OutputImageType;

    using FilterType = itk::SmoothingRecursiveGaussianImageFilter< InputImageType, InputImageType >;
    FilterType::Pointer smoothFilter = FilterType::New();
    smoothFilter->SetSigma(2.0);
    smoothFilter->SetInput(getITKImage());

    typedef itk::GradientImageFilter<InputImageType, float, float, OutputImageType> GradientFilterType;
    GradientFilterType::Pointer gradientFilter  = GradientFilterType::New();
    gradientFilter->SetInput(smoothFilter->GetOutput());
    gradientFilter->Update();

    gradientFilter->GetOutput()->GetPixelContainer()->SetContainerManageMemory(false);
    m_gradient = reinterpret_cast<float*>(gradientFilter->GetOutput()->GetPixelContainer()->GetImportPointer());
    gradientFilter->Update();

    //Signal task completion to the application
    emit volumeGradientComputed(this);
}

itk::Image<float, 3>::Pointer VolumeManager::getITKImage()
{
    typedef itk::Image<float, 3> ImageType;
    typedef itk::ImportImageFilter<float, 3>  ImportFilterType;
    ImportFilterType::Pointer importFilter = ImportFilterType::New();

    ImageType::SizeType imsize;
    imsize[0] = m_width;
    imsize[1] = m_height;
    imsize[2] = m_depth;

    ImportFilterType::IndexType start;
    start.Fill(0);
    ImportFilterType::RegionType region;
    region.SetIndex(start);
    region.SetSize(imsize);
    importFilter->SetRegion(region);

    itk::SpacePrecisionType origin[3] = {0.0, 0.0, 0.0};
    importFilter->SetOrigin(origin);

    itk::SpacePrecisionType spacing[3];
    spacing[0] = m_spacingX;
    spacing[1] = m_spacingY;
    spacing[2] = m_spacingZ;

    importFilter->SetSpacing(spacing);

    const unsigned int numberOfVoxels = imsize[0] * imsize[1] * imsize[2];
    importFilter->SetImportPointer(m_data, numberOfVoxels, false);

    itk::Image<float, 3>::Pointer retImg = importFilter->GetOutput();
    retImg->Update();
    return retImg;
}
