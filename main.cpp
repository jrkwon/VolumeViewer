#include <QCoreApplication>
#include <QDebug>

#include "itkImage.h"
#include "itkImageSeriesReader.h"
#include "itkNumericSeriesFileNames.h"
#include "itkJPEGImageIO.h"
#include "itkJPEGImageIOFactory.h"
#include "itkImageToVTKImageFilter.h"
#include "itkInvertIntensityImageFilter.h"
#include "itkImageFileWriter.h"

const unsigned int g_nDimension = 3;

typedef unsigned char PixelType;
typedef itk::Image<PixelType, g_nDimension> ImageType;
typedef itk::ImageSeriesReader<ImageType> ReaderType;
typedef itk::NumericSeriesFileNames NameGeneratorType;
typedef itk::ImageToVTKImageFilter<ImageType> FilterType;
typedef itk::InvertIntensityImageFilter<ImageType, ImageType> InvertIntensityFilterType;

#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingOpenGL);

#include <vtkFixedPointVolumeRayCastMapper.h>
#include <vtkVolume.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkCylinderSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkSTLWriter.h>
//#include <vtkContourFilter.h>
#include <vtkMarchingCubes.h>
#include <vtkPolyDataNormals.h>
#include <vtkPolyDataWriter.h>
#include <vtkTextMapper.h>
#include <vtkActor2D.h>

FilterType::Pointer m_pConnector;
ReaderType::Pointer m_pReader;
NameGeneratorType::Pointer m_pNameGenerator;
InvertIntensityFilterType::Pointer m_pInvertIntensityFilter;
ImageType::Pointer m_pImageData;

vtkRenderer* m_pRenderer;
vtkRenderWindow* m_pRenderWindow;
vtkRenderWindowInteractor* m_pRenderWindowInteractor;
vtkFixedPointVolumeRayCastMapper *m_pVolumeMapper;
vtkVolume *m_pVolume;
vtkMarchingCubes /*ContourFilter*/ *m_pTissueExtractor;
vtkPolyDataNormals *m_pTissueNormals;
vtkPolyDataMapper *m_pTissueMapper;
vtkActor *m_pTissue;
vtkActor2D* m_pTextActor;
vtkTextMapper *m_pTextMapper;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);


    // new for itk 4.8
    itk::JPEGImageIOFactory::RegisterOneFactory();

    // init
    m_pInvertIntensityFilter = InvertIntensityFilterType::New();
    m_pConnector = FilterType::New();


    //////////////////////////
    // vtk objects
    // QT/VTK interact
    m_pRenderer = vtkRenderer::New();
//    ui.vtkWidget->GetRenderWindow()->AddRenderer(m_pRenderer);
    m_pRenderWindow = vtkRenderWindow::New();
    m_pRenderWindow->AddRenderer(m_pRenderer);
    m_pRenderWindowInteractor = vtkRenderWindowInteractor::New();
    m_pRenderWindowInteractor->SetRenderWindow(m_pRenderWindow);

    m_pTissueExtractor = vtkMarchingCubes::New(); //vtkContourFilter::New();
    m_pTissueNormals = vtkPolyDataNormals::New();
    m_pTissueMapper = vtkPolyDataMapper::New();
    m_pTissue = vtkActor::New();
    m_pTextActor = vtkActor2D::New();
    m_pTextMapper = vtkTextMapper::New();

    // read a volume
    // QString strVolumeIndex;
    //strVolumeIndex.sprintf("../", nZoomOut,
    //										iWidth, iLength, iHeight);
    QString strSeriesFormat;
    strSeriesFormat = QString("K:/Projects/TestVol3/%04d.jpg");

    //qDebug() << strSeriesFormat;

    NameGeneratorType::Pointer pNameGenerator = NameGeneratorType::New();

    pNameGenerator->SetSeriesFormat(strSeriesFormat.toStdString());
    unsigned int nStartIndex = 1;
    pNameGenerator->SetStartIndex(nStartIndex); //0 );
    pNameGenerator->SetEndIndex(256);
    pNameGenerator->SetIncrementIndex( 2 );

    ///////////
    // optional
    //pReader->SetImageIO(itk::JPEGImageIO::New());

    ReaderType::Pointer pReader = ReaderType::New();
    pReader->SetFileNames(pNameGenerator->GetFileNames());

    try
    {
        pReader->Update();
    }
    catch (itk::ExceptionObject & err )
    {
        std::cerr << "ExceptionObject caught !" << std::endl;
        std::cerr << err << std::endl;
        return EXIT_FAILURE;
    }

    m_pImageData = pReader->GetOutput();
//    m_pImageData->DisconnectPipeline();

    // execute pipeline
    m_pInvertIntensityFilter->SetInput(m_pImageData);
    m_pConnector->SetInput(m_pInvertIntensityFilter->GetOutput()); //m_pReader->GetOutput());
#if VTK_MAJOR_VERSION <= 5
    m_pTissueExtractor->SetInput(m_pConnector->GetOutput());
#else
    m_pConnector->Update();
    m_pTissueExtractor->SetInputData(m_pConnector->GetOutput());
#endif
    //int nIsoSurface = ui.lineEditValue->text().toInt();
    //if(nIsoSurface == 0)
    int nIsoSurface = 155;
    m_pTissueExtractor->SetValue(0, nIsoSurface);

#if VTK_MAJOR_VERSION <= 5
    m_pTissueNormals->SetInput(m_pTissueExtractor->GetOutput());
#else
    m_pTissueNormals->SetInputConnection(m_pTissueExtractor->GetOutputPort());
#endif
    //m_pTissueNormals->SetFeatureAngle(60.0);
#if VTK_MAJOR_VERSION <= 5
    m_pTissueMapper->SetInput(m_pTissueNormals->GetOutput());
#else
    m_pTissueMapper->SetInputConnection(m_pTissueNormals->GetOutputPort());
#endif
    m_pTissueMapper->ScalarVisibilityOff();
    m_pTissue->SetMapper(m_pTissueMapper);

        //if(!m_pVolumeMapper)
        //{
        //	m_pVolumeMapper = vtkFixedPointVolumeRayCastMapper::New();
        //	m_pVolumeMapper->SetInput(m_pConnector->GetOutput());

        //	m_pVolume = vtkVolume::New();
        //	m_pVolume->SetMapper(m_pVolumeMapper);
        //	m_pVolume->Update();

        //	//m_pRenderer->AddActor(m_pVolume);

    m_pRenderer->AddActor(m_pTissue);

    m_pRenderer->SetBackground(0.0,0.0,0.4);
    m_pRenderer->ResetCamera();
    m_pRenderer->GetRenderWindow()->Render();

    m_pRenderWindowInteractor->Start();

    // save to vtk
    vtkPolyDataWriter* pWriter = vtkPolyDataWriter::New();
#if VTK_MAJOR_VERSION <= 5
    pWriter->SetInput(m_pTissueNormals->GetOutput());
#else
    pWriter->SetInputConnection(m_pTissueNormals->GetOutputPort());
#endif
    QString strVTKFileName;
    strVTKFileName.sprintf("test2.vtk");
    pWriter->SetFileName(strVTKFileName.toStdString().c_str());
    pWriter->SetFileTypeToBinary();
    pWriter->Write();

    // save to stl
    vtkSTLWriter* pSTLWriter = vtkSTLWriter::New();
#if VTK_MAJOR_VERSION <= 5
    pSTLWriter->SetInput(m_pTissueNormals->GetOutput());
#else
    pSTLWriter->SetInputConnection(m_pTissueNormals->GetOutputPort());
#endif
    QString strSTLFileName;
    strSTLFileName.sprintf("test2.stl");
    pSTLWriter->SetFileName(strSTLFileName.toStdString().c_str());
    pSTLWriter->SetFileTypeToBinary();
    pSTLWriter->Write();

    // finalize
    m_pRenderer->Delete();

    m_pTissueExtractor->Delete();
    m_pTissueNormals->Delete();
    m_pTissueMapper->Delete();
    m_pTissue->Delete();
    m_pTextActor->Delete();;
    m_pTextMapper->Delete();

    // TODO: there are missing pointers to be deleted. Add them.

    return a.exec();
}
