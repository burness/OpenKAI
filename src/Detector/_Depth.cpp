/*
 * _Depth.cpp
 *
 *  Created on: Aug 17, 2015
 *      Author: yankai
 */
/*
#include "_Depth.h"

namespace kai
{
_Depth::_Depth()
{
	_ThreadBase();

	m_pUniverse = NULL;
	m_minObjArea = 0;
	m_maxObjArea = 10000000;

	m_pFlow = NULL;
	m_pStream = NULL;

	m_camFrameID = 0;
}

_Depth::~_Depth()
{
}

bool _Depth::init(void* pKiss)
{
	CHECK_F(!this->_ThreadBase::init(pKiss));
	Kiss* pK = (Kiss*)pKiss;
	pK->m_pInst = this;

	F_INFO(pK->v("areaMin", &m_minObjArea));
	F_INFO(pK->v("areaMax", &m_maxObjArea));

	m_pDepth = new Frame();
	m_camFrameID = 0;

	return true;
}

bool _Depth::link(void)
{
	CHECK_F(!this->_ThreadBase::link());
	Kiss* pK = (Kiss*)m_pKiss;

	//link instance
	string iName = "";
	F_ERROR_F(pK->v("_Stream",&iName));
	m_pStream = (_StreamBase*)(pK->root()->getChildInstByName(&iName));
	F_ERROR_F(pK->v("_Universe",&iName));
	m_pUniverse = (_Universe*)(pK->root()->getChildInstByName(&iName));
	F_ERROR_F(pK->v("_Flow",&iName));
	m_pFlow = (_Flow*)(pK->root()->getChildInstByName(&iName));

	return true;
}

bool _Depth::start(void)
{
	m_bThreadON = true;
	int retCode = pthread_create(&m_threadID, 0, getUpdateThread, this);
	if (retCode != 0)
	{
		LOG(ERROR)<< "Return code: "<< retCode << " in DepthDetector::start().pthread_create()";
		m_bThreadON = false;
		return false;
	}

	return true;
}

void _Depth::update(void)
{
	while (m_bThreadON)
	{
		this->autoFPSfrom();

		detect();

		this->autoFPSto();
	}

}

void _Depth::detect(void)
{
	int i;
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	Rect bb;
	Size imgSize;
	double scaleW;
	double scaleH;
	GpuMat gMat;
	GpuMat gMat2;
	Frame* pFrame;

	NULL_(m_pUniverse);
	NULL_(m_pStream);

	if(m_pStream->getType() == zed)
	{
		gMat = *(m_pStream->depth());
		if(gMat.empty())return;

		//TODO: general multiple layers
//		cuda::divide(gMat,Scalar(50),gMat2);
//		cuda::multiply(gMat2,Scalar(50),gMat);

		cuda::threshold(gMat,gMat2,200,255,cv::THRESH_TOZERO);
		gMat2.download(m_Mat);
	}
	else
	{
		//Use Flow
		if(m_pFlow==NULL)return;

		pFrame = m_pFlow->m_pDepth;
		if(pFrame->empty())return;

		cuda::threshold(*pFrame->getGMat(),gMat,200,255,cv::THRESH_TOZERO);
		gMat.download(m_Mat);
	}

	imgSize = m_pStream->getBGRFrame()->getSize();
	scaleW = ((double)imgSize.width)/((double)m_Mat.cols);
	scaleH = ((double)imgSize.height)/((double)m_Mat.rows);

	// Find contours
	findContours(m_Mat, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
	// Approximate contours to polygons + get bounding rects
	vector<vector<Point> > contours_poly(contours.size());

	for (i = 0; i < contours.size(); i++)
	{
		approxPolyDP(Mat(contours[i]), contours_poly[i], 3, true);
		bb = boundingRect(Mat(contours_poly[i]));

		//Convert bb area position on depth map to those on original image
		bb.x = ((double)bb.x)*scaleW;
		bb.y = ((double)bb.y)*scaleH;
		bb.width = ((double)bb.width)*scaleW-1;
		bb.height = ((double)bb.height)*scaleH-1;

		if (bb.area() < m_minObjArea)continue;
		if (bb.area() > m_maxObjArea)continue;

//		m_pUniverse->addObject(m_pStream->getBGRFrame()->getCMat(), &bb, &contours_poly[i]);
	}


}

}

*/
