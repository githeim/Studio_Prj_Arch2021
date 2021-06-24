
#include "alignment.h"
#include "cudaMappedMemory.h"

#include "FaceDetector.h"
#include "PerformanceLogger.h"
#include "NetworkInterface.h"

void runClassifier()
{
    FaceDetector::GetInstance()->Hwnd_Classifier();
}

FaceDetector *FaceDetector::m_Instance = nullptr;

FaceDetector::FaceDetector(face_embedder &embedder, face_classifier &classifier, mtcnn &finder)
    : m_embedder(embedder),
      m_classifier(classifier),
      m_finder(finder)
{
}

FaceDetector::~FaceDetector()
{
}

void FaceDetector::CreateInstance(face_embedder &embedder, face_classifier &classifier, mtcnn &finder)
{
    if (m_Instance == nullptr)
    {
        m_Instance = new FaceDetector(embedder, classifier, finder);
    }
}

FaceDetector *FaceDetector::GetInstance()
{
    return m_Instance;
}

int FaceDetector::Initialize()
{

    g_pThrClassify = std::thread(runClassifier);

    return 0;
}

void FaceDetector::PushClassifyFaces(
            cv::Mat &origin_cpu,
            int &num_dets,
            std::vector<cv::Rect> &rects,
            std::vector<std::string> &label_encodings,
            std::vector<matrix<float,0,1>> &face_embeddings,
            double &fps
) {
    std::shared_ptr<ClassifyData> data = std::make_shared<ClassifyData>();
    data->origin_cpu = origin_cpu;
    data->num_dets = num_dets;
    data->rects = rects;
    data->label_encodings = label_encodings;
    data->face_embeddings = face_embeddings;
    data->fps = fps;
    
    g_mtxClassify.lock();

    g_listClassify.push_back(data);

    g_CondClassify.notify_all();
    g_mtxClassify.unlock();
}

void FaceDetector::Hwnd_Classifier(    ) {
    std::shared_ptr<ClassifyData> data;
    while (1) {

        printf("\033[1;36m[%s][%d] :x: size = [%lu] \033[m\n",__FUNCTION__,__LINE__,g_listClassify.size());

        {
            std::unique_lock<std::mutex> lck(g_mtxClassify);

            if (g_listClassify.size() == 0 )
            {                    
                g_CondClassify.wait(lck);
                continue;
            }
            data = g_listClassify.front();
            g_listClassify.pop_front();
        }

        ClassifyFaces(data->origin_cpu, data->num_dets, data->rects, data->label_encodings, data->face_embeddings, data->fps);
    }

    printf("\033[1;33m[%s][%d] :x: Thread End \033[m\n",__FUNCTION__,__LINE__);

}

void FaceDetector::ClassifyFaces(
        cv::Mat &origin_cpu,
        int &num_dets,
        std::vector<cv::Rect> &rects,
        std::vector<std::string> &label_encodings,
        std::vector<matrix<float,0,1>> &face_embeddings,
        double &fps)
{
    if(num_dets > 0) {

        PerformanceLogger::GetInstance()->setStartTimeClassifier();

        // feed the embeddings to the pretrained SVM's. Store the predicted labels in a vector
        std::vector<double> face_labels;

        m_classifier.prediction(&face_embeddings, &face_labels);

        PerformanceLogger::GetInstance()->setEndTimeClassifier();

        PerformanceLogger::GetInstance()->setStartTimeDrawDetection();

        // draw bounding boxes and labels to the original image
        draw_detections(origin_cpu, &rects, &face_labels, &label_encodings);

        PerformanceLogger::GetInstance()->setEndTimeDrawDetection();
    }
    
    char str[256];
    sprintf(str, "TensorRT  %.1lf FPS", fps);               // print the FPS to the bar

    cv::putText(origin_cpu, str , cv::Point(0,20),
            cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, cv::Scalar(0,0,0,255), 1 );

    // :x: write on msg queue
    NetworkInterface::GetInstance()->PushDataToSend(origin_cpu);
}
