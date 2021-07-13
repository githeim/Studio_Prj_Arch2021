
#include "alignment.h"
#include "cudaMappedMemory.h"

#include "FaceDetector.h"
#include "PerformanceLogger.h"
#include "NetworkInterface.h"

#define SEPARATE_CLASSIFY_THREAD

#define VERIFY_ACCURACY_RATE
#ifdef VERIFY_ACCURACY_RATE
int current_frame_number = 0;
const std::unordered_map<int, std::vector<std::string>> test_frame_numbers = {
    { 153,  { "Monica", "Extra 1" } },
    { 201,  { "Rachel", "Extra 1", "Unknown" } },
    { 221,  { "Extra 2", "Extra 3" } },
    { 339,  { "Phoebe", "Extra 1", "Unknown" } },
    { 504,  { "Rachel", "Unknown" } },
    { 700,  { "Rachel", "Unknown" } },
    { 910,  { "Monica", "Phoebe", "Rachel" } },
    { 1007, { "Rachel", "Ross" } },
    { 1173, { "Monica", "Phoebe", "Unknown", "Unknown", "Unknown" } },
    { 1279, { } },
    { 1355, { "Extra 4", "Unknown" } },
    { 1397, { "Extra 4", "Unknown" } },
    { 1408, { "Monica" } },
    { 1470, { "Rachel", "Ross", "Phoebe", "Chandler", "Joey" } },
    { 1606, { "Rachel" } },
    { 1687, { "Rachel", "Ross", "Phoebe", "Chandler", "Joey" } },
    { 1711, { "Extra 5", "Unknown" } },
    { 1878, { "Joey", "Chandler", "Phoebe" } },
    { 1895, { "Joey", "Chandler", "Monica", "Phoebe" } },
    { 1969, { "Rachel", "Ross" } },
    { 2256, { "Monica" } },
    { 2440, { "Rachel" } },
    { 2747, { "Phoebe", "Rachel", "Monica" } },
    { 2966, { "Rachel", "Monica" } },
    { 3033, { "Joey" } }
};
#endif

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
    veryfy_detection = false;
    correct_count_frames = 0;
    total_expected_frames = 0;
    exact_found_count = 0;
    wrong_found_count = 0;
    total_expected_count = 0;
    unlink("detection_result.txt");
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
#ifdef SEPARATE_CLASSIFY_THREAD
    g_pThrClassify = std::thread(runClassifier);

    pthread_setname_np(g_pThrClassify.native_handle(), "FaceDetector");
#endif // SEPARATE_CLASSIFY_THREAD

    return 0;
}

void FaceDetector::PushClassifyFaces(
            cv::Mat &origin_cpu,
            int &num_dets,
            std::vector<cv::Rect> &rects,
            std::vector<std::string> &label_encodings,
            std::vector<matrix<float,0,1>> &face_embeddings,
            double &fps,
            const bool &skip_detection
) {
#ifdef SEPARATE_CLASSIFY_THREAD
    std::shared_ptr<ClassifyData> data = std::make_shared<ClassifyData>();
    data->origin_cpu = origin_cpu.clone();
    data->num_dets = num_dets;
    data->rects = rects;
    data->label_encodings = label_encodings;
    data->face_embeddings = face_embeddings;
    data->fps = fps;
    data->skip_detection = skip_detection;

    g_mtxClassify.lock();

    g_listClassify.push_back(data);

    g_CondClassify.notify_all();
    g_mtxClassify.unlock();
#else
    ClassifyFaces(origin_cpu, num_dets, rects, label_encodings, face_embeddings, fps, skip_detection);
#endif // SEPARATE_CLASSIFY_THREAD
}

void FaceDetector::Hwnd_Classifier(    ) {
    SetThreadAffinity("Classifier", 0);


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

        ClassifyFaces(data->origin_cpu, data->num_dets, data->rects, data->label_encodings, data->face_embeddings, data->fps, data->skip_detection);

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    printf("\033[1;33m[%s][%d] :x: Thread End \033[m\n",__FUNCTION__,__LINE__);

}

void FaceDetector::ClassifyFaces(
        cv::Mat &origin_cpu,
        int &num_dets,
        std::vector<cv::Rect> &rects,
        std::vector<std::string> &label_encodings,
        std::vector<matrix<float,0,1>> &face_embeddings,
        double &fps,
        const bool &skip_detection)
{
    if (skip_detection) {
        // draw bounding boxes and labels to the original image
//        draw_detections(origin_cpu, &rects, &face_labels, &label_encodings);
    } else if (num_dets > 0) {
        face_labels.clear();

        PerformanceLogger::GetInstance()->setStartTimeClassifier();

        // feed the embeddings to the pretrained SVM's. Store the predicted labels in a vector

        m_classifier.prediction(&face_embeddings, &face_labels);

        PerformanceLogger::GetInstance()->setEndTimeClassifier();

        PerformanceLogger::GetInstance()->setStartTimeDrawDetection();

        // draw bounding boxes and labels to the original image
        //draw_detections(origin_cpu, &rects, &face_labels, &label_encodings);

        PerformanceLogger::GetInstance()->setEndTimeDrawDetection();
    } else {
        face_labels.clear();
    }

#ifdef VERIFY_ACCURACY_RATE
    current_frame_number++;
    verifyDetection(current_frame_number, rects, label_encodings, face_labels);
    if (current_frame_number == 3033) {
        writeVerifyResult();
    }
#endif

    // :x: write on msg queue
    //NetworkInterface::GetInstance()->PushDataToSend(origin_cpu);
    NetworkInterface::GetInstance()->PushDetectionData(current_frame_number, num_dets, rects, label_encodings, face_labels);
}

void FaceDetector::setVerifyDetection(const bool &value)
{
    veryfy_detection = value;
}

bool FaceDetector::verifyDetection(const int &frame_number, std::vector<cv::Rect> &rects, std::vector<std::string> &label_encodings, std::vector<double> &face_labels)
{
#ifdef VERIFY_ACCURACY_RATE
    if (!veryfy_detection)
        return false;

    auto iter = test_frame_numbers.find(frame_number);
    if (iter == test_frame_numbers.end()) {
        // not a test frame
        return true;
    }

    // detected faces
    std::vector<std::string> detected_labels;
    for (int i = 0; i < rects.size(); ++i) {
        std::string encoding;
        if(face_labels.at(i) >= 0) {
            encoding =  label_encodings.at(face_labels.at(i));
        }else{
            encoding = "Unknown";
        }
        detected_labels.push_back(encoding);
    }

    bool ret = true;
    std::ofstream ofs ("detection_result.txt", std::ofstream::out | std::ofstream::app);

    std::vector<std::string> detected_labels2 = detected_labels;

    const std::vector<std::string> &required_names = iter->second;
    ofs << "frame num:" << frame_number << " required num:" << required_names.size() << " detected num:" << rects.size() << std::endl;
    for (int j = 0; j < required_names.size(); ++j) {
        const std::string &req_name = required_names[j];
        ofs << "    required " << (j+1) << ":" << req_name << std::endl;

        total_expected_count++;

        auto iter = std::find_if(detected_labels2.begin(), detected_labels2.end(), [&](std::string &name)
            {
                return (name == req_name);
            });
        if (iter == detected_labels2.end()) {
            ofs << " **** not found : " << req_name << " ****" << std::endl;
            ret = false;
        } else {
            detected_labels2.erase(iter);
            exact_found_count++;
        }
    }
    for (int i = 0; i < detected_labels.size(); ++i) {
        std::string &encoding = detected_labels[i];

        ofs << "    detected " << (i+1) << ":" << encoding << " (x:" << rects.at(i).x << " y:" << rects.at(i).y << ")" << std::endl;

        auto iter = std::find_if(required_names.begin(), required_names.end(), [&](const std::string &name)
            {
                return (name == encoding);
            });
        if (iter == required_names.end()) {
            wrong_found_count++;
            ofs << " **** wroing found : " << encoding << " ****" << std::endl;
            ret = false;
        }
    }

    total_expected_frames++;
    if (required_names.size() == detected_labels.size()) {
        correct_count_frames++;
    }

    if (total_expected_count > 0) {
        ofs << "====== total correct frame : " << correct_count_frames << " rate:" << ((100 * correct_count_frames) / total_expected_frames) << "%" << std::endl;
        ofs << "====== total faces found   : " << exact_found_count << " rate:" << ((100 * exact_found_count) / total_expected_count) << "%" << std::endl;
//        ofs << "====== total wrong found : " << wrong_found_count << " rate:" << ((100 * wrong_found_count) / total_expected_count) << "%" << std::endl;
    }

    ofs.flush();
    ofs.close();

    return ret;
#else
    return true;
#endif // VERIFY_ACCURACY_RATE
}

void FaceDetector::writeVerifyResult()
{
#ifdef VERIFY_ACCURACY_RATE
    if (!veryfy_detection)
        return;

    std::ofstream ofs ("detection_result.txt", std::ofstream::out | std::ofstream::app);

    ofs << "====================================" << std::endl;
    ofs << "====== total correct frame : " << correct_count_frames << " rate:" << ((100 * correct_count_frames) / total_expected_frames) << "%" << std::endl;
    ofs << "====== total faces found   : " << exact_found_count << " rate:" << ((100 * exact_found_count) / total_expected_count) << "%" << std::endl;
//    ofs << "====== total wrong found : " << wrong_found_count << " rate:" << ((100 * wrong_found_count) / total_expected_count) << "%" << std::endl;
    ofs << "====================================" << std::endl;

    ofs.flush();
    ofs.close();
#endif // VERIFY_ACCURACY_RATE
}
