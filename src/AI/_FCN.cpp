/*
 * FCN.cpp
 *
 *  Created on: Aug 17, 2015
 *      Author: yankai
 */

#include "_FCN.h"

#ifdef USE_FCN

namespace kai
{

_FCN::_FCN()
{
	_ThreadBase();

	m_NumChannels = 0;
	m_pStream = NULL;
	m_cudaDeviceID = 0;

}

_FCN::~_FCN()
{
}

bool _FCN::init(Config* pConfig)
{
	CHECK_F(!this->_ThreadBase::init(pConfig));
	pConfig->m_pInst = this;

	string modelFile;
	string trainedFile;
	string labelFile;
	string caffeDir = "";

	F_INFO(pConfig->v("dir", &caffeDir));
	F_INFO(pConfig->v("cudaDeviceID", &m_cudaDeviceID));

	F_FATAL_F(pConfig->v("modelFile", &modelFile));
	F_FATAL_F(pConfig->v("trainedFile", &trainedFile));
	F_FATAL_F(pConfig->v("labelFile", &labelFile));

	/* Load the network. */
	net_.reset(new Net<float>(caffeDir+modelFile, TEST));
	net_->CopyTrainedLayersFrom(caffeDir+trainedFile);

	Blob<float>* input_layer = net_->input_blobs()[0];
	m_NumChannels = input_layer->channels();
	m_InputSize = cv::Size(input_layer->width(), input_layer->height());

//	Blob<float>* output_layer = net_->output_blobs()[0];

	m_segment = Mat(m_InputSize.height, m_InputSize.width, CV_8UC3);
	m_pFrame = NULL;//new CamFrame();
	m_pSegment = new Frame();

	m_labelColor = imread(caffeDir+labelFile, 1);
	m_pGpuLUT = cuda::createLookUpTable(m_labelColor);

	return true;
}

bool _FCN::link(void)
{
	NULL_F(m_pConfig);

	//link instance
	string iName = "";
	F_ERROR_F(m_pConfig->v("_Stream",&iName));
	m_pStream = (_Stream*)(m_pConfig->root()->getChildInstByName(&iName));

	return true;
}

bool _FCN::start(void)
{
	m_bThreadON = true;
	int retCode = pthread_create(&m_threadID, 0, getUpdateThread, this);
	if (retCode != 0)
	{
		LOG(ERROR)<< retCode;
		m_bThreadON = false;
		return false;
	}

	return true;
}

void _FCN::update(void)
{
#ifdef CPU_ONLY
	Caffe::set_mode(Caffe::CPU);
#else
	Caffe::set_mode(Caffe::GPU);
#endif
	cuda::setDevice(m_cudaDeviceID);


	while (m_bThreadON)
	{
		this->autoFPSfrom();

		segmentGPU();

		this->autoFPSto();
	}

}

void _FCN::segmentGPU(void)
{
	if(m_pStream==NULL)return;
	m_pFrame = m_pStream->getBGRFrame();

	if(m_pFrame==NULL)return;
	if(m_pFrame->empty())return;

	Blob<float>* input_layer = net_->input_blobs()[0];
	input_layer->Reshape(1, m_NumChannels, m_InputSize.height, m_InputSize.width);

	net_->Reshape();

	std::vector<GpuMat> input_channels;
	WrapInputLayerGPU(&input_channels);
	PreprocessGPU(&input_channels);

	net_->Forward();

	Blob<float>* output_layer = net_->output_blobs()[0];//net_->blob_by_name("argmax").get();
	GpuMat segIdx = GpuMat(m_InputSize.height, m_InputSize.width, CV_32FC1, (void*)output_layer->gpu_data());
	GpuMat uimg,uimg3;
	segIdx.convertTo(uimg, CV_8UC1);
	cv::cuda::cvtColor(uimg, uimg3, CV_GRAY2BGR);
	m_pGpuLUT->transform(uimg3,*m_pSegment->getGMat());
	m_pSegment->updatedGMat();

	m_segment = *m_pSegment->getCMat();
}



/* Wrap the input layer of the network in separate cv::Mat objects
 * (one per channel). This way we save one memcpy operation and we
 * don't need to rely on cudaMemcpy2D. The last preprocessing
 * operation will write the separate channels directly to the input
 * layer. */
void _FCN::WrapInputLayerGPU(std::vector<cv::cuda::GpuMat>* input_channels)
{
	Blob<float>* input_layer = net_->input_blobs()[0];

	float* input_data = input_layer->mutable_gpu_data();
	for (int i = 0; i < input_layer->channels(); ++i)
	{
		cv::cuda::GpuMat channel(m_InputSize.height, m_InputSize.width, CV_32FC1, input_data);
		input_channels->push_back(channel);
		input_data += m_InputSize.width * m_InputSize.height;
	}
}

void _FCN::PreprocessGPU(std::vector<cv::cuda::GpuMat>* input_channels)
{
	/* Convert the input image to the input image format of the network. */
	cv::cuda::GpuMat sample;
	GpuMat* pGMat = m_pFrame->getGMat();

//	m_pCamStream->mutexLock(CAMSTREAM_MUTEX_ORIGINAL);

	if (pGMat->channels() == 3 && m_NumChannels == 1)
		cv::cuda::cvtColor(*pGMat, sample, CV_BGR2GRAY);
	else if (pGMat->channels() == 4 && m_NumChannels == 1)
		cv::cuda::cvtColor(*pGMat, sample, CV_BGRA2GRAY);
	else if (pGMat->channels() == 4 && m_NumChannels == 3)
		cv::cuda::cvtColor(*pGMat, sample, CV_BGRA2BGR);
	else if (pGMat->channels() == 1 && m_NumChannels == 3)
		cv::cuda::cvtColor(*pGMat, sample, CV_GRAY2BGR);
	else
		sample = *pGMat;

	cv::cuda::GpuMat sample_resized;
	if (sample.size() != m_InputSize)
		cv::cuda::resize(sample, sample_resized, m_InputSize);
	else
		sample_resized = sample;

	cv::cuda::GpuMat sample_float;
	if (m_NumChannels == 3)
		sample_resized.convertTo(sample_float, CV_32FC3);
	else
		sample_resized.convertTo(sample_float, CV_32FC1);

//	m_pCamStream->mutexUnlock(CAMSTREAM_MUTEX_ORIGINAL);

	/* This operation will write the separate BGR planes directly to the
	 * input layer of the network because it is wrapped by the cv::Mat
	 * objects in input_channels. */
	cv::cuda::split(sample_float, *input_channels);

}


void _FCN::segment(void)
{
	if(m_frame.empty())return;

	Blob<float>* input_layer = net_->input_blobs()[0];
	input_layer->Reshape(1, m_NumChannels, m_InputSize.height, m_InputSize.width);

	net_->Reshape();

	std::vector<cv::Mat> input_channels;
	WrapInputLayer(&input_channels);
	Preprocess(m_frame, &input_channels);

	net_->ForwardPrefilled();

	Blob<float>* output_layer = net_->output_blobs()[0];//net_->blob_by_name("argmax").get();
	const float* begin = output_layer->cpu_data();
//	const float* end = begin + output_layer->channels() * m_InputSize.area() * 4;
//	vector<float> pOutput = std::vector<float>(begin, end);

	Mat segIdx = Mat(m_InputSize.height, m_InputSize.width, CV_32FC1, (void*)begin);// pOutput.data());

	Mat uimg,uimg3;
	segIdx.convertTo(uimg, CV_8UC1);
	cv::cvtColor(uimg, uimg3, CV_GRAY2BGR);
	cv::LUT(uimg3,m_labelColor, m_segment);

}


/* Wrap the input layer of the network in separate cv::Mat objects
 * (one per channel). This way we save one memcpy operation and we
 * don't need to rely on cudaMemcpy2D. The last preprocessing
 * operation will write the separate channels directly to the input
 * layer. */
void _FCN::WrapInputLayer(std::vector<cv::Mat>* input_channels)
{
	Blob<float>* input_layer = net_->input_blobs()[0];

	float* input_data = input_layer->mutable_cpu_data();
	for (int i = 0; i < input_layer->channels(); ++i)
	{
		cv::Mat channel(m_InputSize.height, m_InputSize.width, CV_32FC1, input_data);
		input_channels->push_back(channel);
		input_data += m_InputSize.width * m_InputSize.height;
	}
}

void _FCN::Preprocess(const cv::Mat& img,
		std::vector<cv::Mat>* input_channels)
{
	/* Convert the input image to the input image format of the network. */
	cv::Mat sample;
	if (img.channels() == 3 && m_NumChannels == 1)
		cv::cvtColor(img, sample, CV_BGR2GRAY);
	else if (img.channels() == 4 && m_NumChannels == 1)
		cv::cvtColor(img, sample, CV_BGRA2GRAY);
	else if (img.channels() == 4 && m_NumChannels == 3)
		cv::cvtColor(img, sample, CV_BGRA2BGR);
	else if (img.channels() == 1 && m_NumChannels == 3)
		cv::cvtColor(img, sample, CV_GRAY2BGR);
	else
		sample = img;
//		cv::cvtColor(img, sample, CV_BGR2RGB);

	cv::Mat sample_resized;
	if (sample.size() != m_InputSize)
		cv::resize(sample, sample_resized, m_InputSize);
	else
		sample_resized = sample;

	cv::Mat sample_float;
	if (m_NumChannels == 3)
		sample_resized.convertTo(sample_float, CV_32FC3);
	else
		sample_resized.convertTo(sample_float, CV_32FC1);

	/* This operation will write the separate BGR planes directly to the
	 * input layer of the network because it is wrapped by the cv::Mat
	 * objects in input_channels. */
	cv::split(sample_float, *input_channels);

//	CHECK(reinterpret_cast<float*>(input_channels->at(0).data)
//			== net_->input_blobs()[0]->cpu_data())
//															<< "Input channels are not wrapping the input layer of the network.";
}

bool _FCN::draw(Frame* pFrame, vInt4* pTextPos)
{
	if (pFrame == NULL)
		return false;

	if(m_segment.empty())return false;

	Mat fcnMat, showMat;
	Mat* pMat = pFrame->getCMat();

	putText(*pMat, "FCN FPS: " + i2str(getFrameRate()),
			cv::Point(pTextPos->m_x, pTextPos->m_y), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0), 1);

	pTextPos->m_y += pTextPos->m_w;

	cv::resize(m_segment, fcnMat, Size(pMat->cols, pMat->rows));
	cv::addWeighted(*pMat, 1.0, fcnMat, 0.25, 0.0, showMat);
	pFrame->update(&showMat);

	return true;
}




}

#endif
