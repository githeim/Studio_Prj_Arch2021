
#include "FaceDetector.h"

FaceDetector *FaceDetector::m_Instance = nullptr;

FaceDetector::FaceDetector()
    : embedder(),
      classifier(&embedder)
{
}

FaceDetector::~FaceDetector()
{
}

FaceDetector *FaceDetector::GetInstance()
{
    if (m_Instance == nullptr)
    {
        m_Instance = new FaceDetector();
    }

    return m_Instance;
}

void FaceDetector::GetLabelEncoding(std::vector<std::string> *label_encodings)
{
    classifier.get_label_encoding(label_encodings);
}

void FaceDetector::Embeddings(std::vector<matrix<rgb_pixel>> *faces, std::vector<matrix<float,0,1>> *face_embeddings)
{
    embedder.embeddings(faces, face_embeddings);
}

void FaceDetector::Prediction(std::vector<matrix<float,0,1>> *face_embeddings, std::vector<double> *face_labels)
{
    classifier.prediction(face_embeddings, face_labels);
}

