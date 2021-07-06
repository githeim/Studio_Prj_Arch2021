#include <time.h>
#include <boost/filesystem.hpp>
#include <fstream>
#include <dlib/svm_threaded.h>
#include <dlib/svm.h>
#include <vector>
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>

#include "mtcnn.h"
#include "kernels.h"

#include "gstCamera.h"
#include "glDisplay.h"
#include "loadImage.h"
#include "cudaRGB.h"
#include "cudaMappedMemory.h"

#include "face_embedder.h"
#include "face_classifier.h"
#include "alignment.h"
#include "videoSource.h"

#include "NetworkTCP.h"
#include "TcpSendRecvJpeg.h"

#define FEATURE_INTERPOLATE_FACES

#include "ImageHandler.h"
#include "NetworkInterface.h"
#include "PerformanceLogger.h"

#include "FaceDetector.h"

#include <chrono>
#include <list>
#include <mutex>
#include <thread>


// perform face recognition with Raspberry Pi camera
int camera_face_recognition(int argc, char *argv[])
  {
   short              listen_port;
#ifdef FEATURE_INTERPOLATE_FACES
   bool               skipframes_cfg=false;
   int              skipcount_cfg=3; // [0% ~ 100%], setting. 0 = Without skip , 100 = skip all the frames
   int		skipcount_curr=0; // current count, need to compare with the setting
   bool     skipframe_curr=0;
   std::vector<matrix<float,0,1>>  face_embeddings_interpolated;
   std::vector<cv::Rect> rects_interpolated;
   bool usecamera = false;
#endif /* FEATURE_INTERPOLATE_FACES */

    face_embedder embedder;                         // deserialize recognition network
    face_classifier classifier(&embedder);          // train OR deserialize classification SVM's
    if(classifier.need_restart() == 1) return 1;    // small workaround - if svms were trained theres some kind of memory problem when generate mtcnn

    // -------------- Initialization -------------------
    bool user_quit = false;
    int imgWidth ;
    int imgHeight ;

   if (argc <2)
    {
       fprintf(stderr,"usage %s [port] [--input-codec=] [filename]\n", argv[0]);
       exit(0);
    }

    listen_port = atoi(argv[1]);


#ifdef FEATURE_INTERPOLATE_FACES
	fprintf(stderr,"====================================================== \n");
	fprintf(stderr,"usage %s [i=3] [l] [v] [c]\n", argv[0]);
	fprintf(stderr,"i=3 	the number of interpolated frames	\n");
	fprintf(stderr,"l 		enable learning mode	\n");
	fprintf(stderr,"c		intentionally enable camera  \n");
	fprintf(stderr,"v		intentionally enable video	\n");
	fprintf(stderr,"====================================================== \n");	
	for(int i=1 ; i<argc ; i++) {
		//printf("argv[%d] = %s \n", i, argv[i]);
		if(argv[i][0] == 'i')
		{
			skipframes_cfg=true;
			skipcount_cfg =atoi(&argv[i][1]);
			printf("enable face interpolation, interpolate %d frames \n", skipcount_cfg );
		}

		if(argv[i][0] == 'l')
		{
			skipframes_cfg=true;
			skipcount_cfg = 0;
			printf("enable learning mode \n" );
		}

		if(argv[i][0] == 'c')
		{
			usecamera = true;
			printf("intentionally enable camera \n");
		}

		if(argv[i][0] == 'v')
		{
			usecamera = false;
			printf("intentionally enable video \n");
		}
	}
#endif /* FEATURE_INTERPOLATE_FACES */


    ImageHandler::GetInstance()->Initialize(argc, argv, usecamera);

    imgWidth = ImageHandler::GetInstance()->GetImageWidth();
    imgHeight = ImageHandler::GetInstance()->GetImageHeight();

    if (imgWidth == 0 || imgHeight == 0)
    {
        fprintf(stderr, "invalid image source witdh:%d height:%d\n", imgWidth, imgHeight);
        return -1;
    }

     mtcnn finder(imgHeight, imgWidth);              // build OR deserialize TensorRT detection network

    // malloc shared memory for images for access with cpu and gpu without copying data
    // cudaAllocMapped is used from jetson-inference
    uchar* rgb_gpu = NULL;
    uchar* rgb_cpu = NULL;
    cudaAllocMapped( (void**) &rgb_cpu, (void**) &rgb_gpu, imgWidth*imgHeight*3*sizeof(uchar) );
    uchar* cropped_buffer_gpu[2] = {NULL,NULL};
    uchar* cropped_buffer_cpu[2] = {NULL,NULL};
    cudaAllocMapped( (void**) &cropped_buffer_cpu[0], (void**) &cropped_buffer_gpu[0], 150*150*3*sizeof(uchar) );
    cudaAllocMapped( (void**) &cropped_buffer_cpu[1], (void**) &cropped_buffer_gpu[1], 150*150*3*sizeof(uchar) );

    FaceDetector::CreateInstance(embedder, classifier, finder);
    FaceDetector::GetInstance()->Initialize();

    // calculate fps
    double fps = 0.0;
    double fpsCurr = 0.0;
    clock_t clk;
    clock_t now;

    // Detection vars
    int num_dets = 0;
    std::vector<std::string> label_encodings;       // vector for the real names of the classes/persons

    // get the possible class names
    classifier.get_label_encoding(&label_encodings);
    
    NetworkInterface::GetInstance()->Initialize(listen_port);

// ------------------ "Detection" Loop -----------------------
    while(!user_quit){

        if (NetworkInterface::GetInstance()->GetCurrentTransmitQueueSize() > 3)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        clk = clock();              // fps clock
	float* imgOrigin = NULL;    // camera image

        PerformanceLogger::GetInstance()->setStartTimeCapture();

        // the 2nd arg 1000 defines timeout, true is for the "zeroCopy" param what means the image will be stored to shared memory
    imgOrigin = ImageHandler::GetInstance()->GetImageData();
        if (imgOrigin == NULL)
        {
           printf("origin capture image is null\n");
           continue;
        }

        PerformanceLogger::GetInstance()->setEndTimeCapture();

        printf("capture image\n");

        PerformanceLogger::GetInstance()->setStartTimeImgXcoding();

        //since the captured image is located at shared memory, we also can access it from cpu
        // here I define a cv::Mat for it to draw onto the image from CPU without copying data -- TODO: draw from CUDA
//        cudaRGBA32ToBGRA32(  (float4*)imgOrigin,  (float4*)imgOrigin, imgWidth, imgHeight); //ADDED DP
        cv::Mat origin_cpu(imgHeight, imgWidth, CV_32FC4, imgOrigin);

        // the mtcnn pipeline is based on GpuMat 8bit values 3 channels while the captured image is RGBA32
        // i use a kernel from jetson-inference to remove the A-channel and float to uint8
        cudaRGBA32ToRGB8( (float4*)imgOrigin, (uchar3*)rgb_gpu, imgWidth, imgHeight );

        // create GpuMat form the same image thanks to shared memory
        cv::cuda::GpuMat imgRGB_gpu(imgHeight, imgWidth, CV_8UC3, rgb_gpu);

        PerformanceLogger::GetInstance()->setEndTimeImgXcoding();

        PerformanceLogger::GetInstance()->setStartTimeFaceFinder();


#ifdef FEATURE_INTERPOLATE_FACES
		if(skipframes_cfg == true) 
		{
			skipcount_curr++;
			if(skipcount_curr%skipcount_cfg == 0) 
			{
				skipcount_curr=0;
				skipframe_curr=false;
			}
			else skipframe_curr=true;
		}
		else // skipframes_cfg == false
		{
			skipframe_curr=false;
		}

		if(skipframe_curr == false)
		{
#endif /* FEATURE_INTERPOLATE_FACES */

	        // pass the image to the MTCNN and get face detections
	        std::vector<struct Bbox> detections;
	        finder.findFace(imgRGB_gpu, &detections);


	        // check if faces were detected, get face locations, bounding boxes and keypoints
	        std::vector<cv::Rect> rects;
	        std::vector<float*> keypoints;
	        num_dets = get_detections(origin_cpu, &detections, &rects, &keypoints);

	        PerformanceLogger::GetInstance()->setEndTimeFaceFinder();

	        // if faces detected
	        if(num_dets > 0){

	            PerformanceLogger::GetInstance()->setStartTimeCropEmbedding();

	            // crop and align the faces. Get faces to format for "dlib_face_recognition_model" to create embeddings
	            std::vector<matrix<rgb_pixel>> faces;
	            crop_and_align_faces(imgRGB_gpu, cropped_buffer_gpu, cropped_buffer_cpu, &rects, &faces, &keypoints);

	            // generate face embeddings from the cropped faces and store them in a vector
	            std::vector<matrix<float,0,1>> face_embeddings;

	            embedder.embeddings(&faces, &face_embeddings);

	            PerformanceLogger::GetInstance()->setEndTimeCropEmbedding();

	            FaceDetector::GetInstance()->PushClassifyFaces(origin_cpu, num_dets, rects, label_encodings, face_embeddings, fps);
#ifdef FEATURE_INTERPOLATE_FACES
				rects_interpolated.clear();
				rects_interpolated.assign(rects.begin(), rects.end());

				face_embeddings_interpolated.clear();
				face_embeddings_interpolated.assign(face_embeddings.begin(), face_embeddings.end());
#endif /* FEATURE_INTERPOLATE_FACES */
	        }
	        else {
	            std::vector<matrix<float,0,1>> face_embeddings;

	            FaceDetector::GetInstance()->PushClassifyFaces(origin_cpu, num_dets, rects, label_encodings, face_embeddings, fps);
	            //FaceDetector::GetInstance()->ClassifyFaces(origin_cpu, num_dets, rects, label_encodings, face_embeddings, fps);
	        }
		}

#ifdef FEATURE_INTERPOLATE_FACES
	if(skipframes_cfg == true || (num_dets > 0 && skipframe_curr == true))
	{
		FaceDetector::GetInstance()->PushClassifyFaces(origin_cpu, num_dets, rects_interpolated, label_encodings, face_embeddings_interpolated, fps);
	}
#endif /* FEATURE_INTERPOLATE_FACES */

	// smooth FPS to make it readable	
	now = clock();
	    fpsCurr = (1 / ((double)(now-clk)/CLOCKS_PER_SEC));
        fps = (0.90 * fps) + (0.1 * fpsCurr);
        //fps = (0.90 * fps) + (0.1 * (1 / ((double)(clock()-clk)/CLOCKS_PER_SEC)));
	printf("======== fps:%.1lf (%.1lf)  time:%lu ms =======\n",
			fps, fpsCurr, 1000*(now-clk)/CLOCKS_PER_SEC);
    if (ImageHandler::GetInstance()->IsNotStreaming())
        break;

    PerformanceLogger::GetInstance()->PrintTestReport();

    }

    CHECK(cudaFreeHost(rgb_cpu));
    CHECK(cudaFreeHost(cropped_buffer_cpu[0]));
    CHECK(cudaFreeHost(cropped_buffer_cpu[1]));
#if 0 // :x: for test
    CloseTcpConnectedPort(&TcpConnectedPort); // Close network port;
    CloseTcpListenPort(&TcpListenPort);  // Close listen port
#else
    NetworkInterface::GetInstance()->StopWhenEmpty();
#endif // :x: for test

    return 0;
}


    // perform face recognition on test images
    int test_prediction_images(){

        // required format
        int wid = 1280;         // input size width
        int height = 720;       // input size height
        int channel = 3;        // all images rgb
        int face_size = 150;    // cropped faces are always square 150x150 px (required for dlib face embedding cnn)

        using namespace boost::filesystem;
        path p("faces/bbt/testdata/test/");     // directory with bbt-testdata

        // get recognition network and classifier
        face_embedder embedder;
        face_classifier classifier(&embedder);
        if(classifier.need_restart() == 1) return 1;

        // get detection network
        mtcnn finder(height, wid);
        int num_dets = 0;
        int num_images = 0;

        // get the possible class names
        std::vector<std::string> label_encodings;
        classifier.get_label_encoding(&label_encodings);

        // GPU memory for image in MTCNN format
        uchar* rgb_gpu = NULL;
        uchar* rgb_cpu = NULL;
        cudaAllocMapped( (void**) &rgb_cpu, (void**) &rgb_gpu, height*wid*channel*sizeof(uchar) );

        // GPU memory for cropped faces
        uchar* cropped_buffer_gpu[2] = {NULL,NULL};
        uchar* cropped_buffer_cpu[2] = {NULL,NULL};
        cudaAllocMapped( (void**) &cropped_buffer_cpu[0], (void**) &cropped_buffer_gpu[0], face_size*face_size*channel*sizeof(uchar) );
        cudaAllocMapped( (void**) &cropped_buffer_cpu[1], (void**) &cropped_buffer_gpu[1], face_size*face_size*channel*sizeof(uchar) );

        // gpu/cpu memory pointer to load image from disk
        float* imgCPU    = NULL;
        float* imgCUDA   = NULL;
        int    imgWidth  = 0;
        int    imgHeight = 0;

        // read and process images

        try
        {
            if (exists(p))
            {
                if (is_regular_file(p))
                    cout << p << "is a file" << endl;
                else if (is_directory(p))
                {
                    // Iteration over all images in the test directory
                    recursive_directory_iterator dir(p), end;
                    while(dir != end){

                        if(is_directory(dir->path())) {
                            cout << "enter: " << dir->path().filename().string() << endl;
                        }
                        // Handle the images
                        else {

                            // load image from disk to gpu/cpu-mem. (shared mem. for access without copying)
                            if( !loadImageRGBA(dir->path().string().c_str(), (float4**)&imgCPU, (float4**)&imgCUDA, &imgWidth, &imgHeight) )
                                printf("failed to load image '%s'\n", dir->path().filename().string().c_str());

                            // check if size fits
                            if((imgWidth != wid) || (imgHeight != height)){
                                cout << "image has wrong size!" << endl;
                            }else{
                                // create cv::Mat to draw detections from CPU (possible because of shared memory GPU/CPU)
                                cv::Mat origin_cpu(imgHeight, imgWidth, CV_32FC4, imgCPU);
                                // Convert image to format required by MTCNN
                                cudaRGBA32ToRGB8( (float4*)imgCUDA, (uchar3*)rgb_gpu, imgWidth, imgHeight );
                                // create GpuMat which is required by MTCNN pipeline
                                cv::cuda::GpuMat imgRGB_gpu(imgHeight, imgWidth, CV_8UC3, rgb_gpu);
                                std::vector<struct Bbox> detections;
                                // run MTCNN and get bounidng boxes of detected faces
                                finder.findFace(imgRGB_gpu, &detections);
                                std::vector<cv::Rect> rects;
                                std::vector<float*> keypoints;
                                num_dets = get_detections(origin_cpu, &detections, &rects, &keypoints);
                                if(num_dets > 0){
                                    // crop and align faces
                                    std::vector<matrix<rgb_pixel>> faces;
                                    crop_and_align_faces(imgRGB_gpu, cropped_buffer_gpu, cropped_buffer_cpu, &rects, &faces, &keypoints);
                                    // get face embeddings - feature extraction
                                    std::vector<matrix<float,0,1>> face_embeddings;
                                    embedder.embeddings(&faces, &face_embeddings);
                                    // do classification
                                    std::vector<double> face_labels;
                                    classifier.prediction(&face_embeddings, &face_labels);
                                    // draw detection bbox and classification to the image
                                    draw_detections(origin_cpu, &rects, &face_labels, &label_encodings);
                                }
                                CUDA(cudaDeviceSynchronize());

                                // save the predicted image
                                string outputFilename = "faces/bbt/testdata/result/" + to_string(num_images) + ".png";
                                if( !saveImageRGBA(outputFilename.c_str(), (float4*)imgCPU, imgWidth, imgHeight, 255) )
                                    printf("failed saving %ix%i image to '%s'\n", imgWidth, imgHeight, outputFilename.c_str());

                                num_images++;
                            }
                        }
                        CUDA(cudaDeviceSynchronize());
                        // free CUDA space to load next image
                        CUDA(cudaFreeHost(imgCPU));
                        ++dir;
                    }
                } else cout << p << " exists, but is neither a regular file nor a directory\n";
            } else cout << p << " does not exist\n";


        }
        catch (const filesystem_error& ex)
        {
            cout << ex.what() << '\n';
        }

        CHECK(cudaFreeHost(rgb_cpu));
        CHECK(cudaFreeHost(cropped_buffer_cpu[0]));
        CHECK(cudaFreeHost(cropped_buffer_cpu[1]));

        return 0;
    }



int main(int argc, char *argv[])
{

    int state = 0;

    int ret = setpriority(PRIO_PGRP, getpgid(0), -20);
    if (ret == -1)
    {
        printf("setpriorty err:%d\n", errno);
    }

    state = camera_face_recognition( argc, argv );

    //else state = test_prediction_images(); //test prediction at a set of test images


    if(state == 1) cout << "Restart is required! Please type ./main again." << endl;

    return 0;
}
