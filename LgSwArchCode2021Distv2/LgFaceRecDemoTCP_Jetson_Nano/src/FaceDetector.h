#ifndef FACE_DETECTOR_H
#define FACE_DETECTOR_H

#include <vector>

#include "face_embedder.h"
#include "face_classifier.h"

class FaceDetector {
public:
    ~FaceDetector();
    static FaceDetector *GetInstance();

    void GetLabelEncoding(std::vector<std::string> *label_encodings);
    void Embeddings(std::vector<matrix<rgb_pixel>> *face_chips, std::vector<matrix<float,0,1>> *face_embeddings);
    void Prediction(std::vector<matrix<float,0,1>> *face_embeddings, std::vector<double> *face_labels);

private:
    static FaceDetector *m_Instance;

    face_embedder embedder;                         // deserialize recognition network
    face_classifier classifier;          // train OR deserialize classification SVM's

    FaceDetector();
};

#endif // FACE_DETECTOR_H

