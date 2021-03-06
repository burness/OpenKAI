/*
 * FCN.h
 *
 *  Created on: Aug 17, 2015
 *      Author: yankai
 */

#ifndef AI_FCN_H_
#define AI_FCN_H_

#include "../Base/common.h"

#ifdef USE_FCN

#include <cuda_runtime.h>
#include <caffe/caffe.hpp>
#include <caffe/blob.hpp>
#include <caffe/common.hpp>
#include <caffe/util/io.hpp>
#include <caffe/proto/caffe.pb.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <algorithm>
#include <iosfwd>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../Base/cv.h"
#include "../Base/_ThreadBase.h"
#include "../Stream/_Stream.h"
#include "../Stream/Frame.h"

namespace kai
{

using namespace caffe;
// NOLINT(build/namespaces)
using caffe::Blob;
using caffe::Caffe;
using caffe::Net;
using caffe::shared_ptr;
using caffe::vector;
using std::string;
using namespace std;
using namespace cv;

class _FCN: public _ThreadBase
{
public:
	_FCN();
	~_FCN();

	bool init(Config* pConfig);
	bool link(void);

	bool start(void);
	bool draw(Frame* pFrame, iVec4* pTextPos);

private:
	void segment(void);
	void WrapInputLayer(std::vector<cv::Mat>* input_channels);
	void Preprocess(const cv::Mat& img, std::vector<cv::Mat>* input_channels);

	void segmentGPU(void);
	void WrapInputLayerGPU(std::vector<cv::cuda::GpuMat>* input_channels);
	void PreprocessGPU(std::vector<cv::cuda::GpuMat>* input_channels);

	void update(void);
	static void* getUpdateThread(void* This)
	{
		((_FCN*) This)->update();
		return NULL;
	}

public:
	_Stream* m_pStream;
	int m_cudaDeviceID;
	Mat m_segment;

private:
	shared_ptr<Net<float> > net_;
	Size m_InputSize;
	int m_NumChannels;
	Mat m_labelColor;
	Ptr<LookUpTable> m_pGpuLUT;

	Frame* m_pFrame;
	Frame* m_pSegment;
	Mat m_frame;

};

}
#endif
#endif /* SRC_DNNCAFFEFCN_H_ */
