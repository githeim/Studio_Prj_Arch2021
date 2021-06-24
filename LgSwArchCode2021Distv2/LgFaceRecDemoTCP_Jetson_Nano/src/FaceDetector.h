#ifndef FACE_DETECTOR_H
#define FACE_DETECTOR_H

#include "face_embedder.h"
#include "face_classifier.h"
#include "mtcnn.h"

#include <vector>
#include <memory>

struct ClassifyData {
    cv::Mat origin_cpu;
    int num_dets;
    std::vector<cv::Rect> rects;
    std::vector<std::string> label_encodings;
    std::vector<matrix<float,0,1>> face_embeddings;
    double fps;
};

class FaceDetector {
public:
    ~FaceDetector();
    static void CreateInstance(face_embedder &embedder, face_classifier &classifier, mtcnn &finder);
    static FaceDetector *GetInstance();

    int Initialize();

    void Hwnd_Classifier();

    void PushClassifyFaces(
                cv::Mat &origin_cpu,
                int &num_dets,
                std::vector<cv::Rect> &rects,
                std::vector<std::string> &label_encodings,
                std::vector<matrix<float,0,1>> &face_embeddings,
                double &fps
    );

    void ClassifyFaces(
                cv::Mat &origin_cpu,
                int &num_dets,
                std::vector<cv::Rect> &rects,
                std::vector<std::string> &label_encodings,
                std::vector<matrix<float,0,1>> &face_embeddings,
                double &fps);

private:
    static FaceDetector *m_Instance;

    std::condition_variable g_CondClassify;
    std::mutex g_mtxClassify;
    std::list<std::shared_ptr<ClassifyData>> g_listClassify;
    std::thread g_pThrClassify;

    face_embedder &m_embedder;                         // deserialize recognition network
    face_classifier &m_classifier;          // train OR deserialize classification SVM's
    mtcnn &m_finder;

    FaceDetector(face_embedder &embedder, face_classifier &classifier, mtcnn &finder);
};

#endif // FACE_DETECTOR_H
