/*
 * Module.h
 *
 *  Created on: Nov 22, 2016
 *      Author: root
 */

#ifndef OPENKAI_SRC_CONFIG_MODULE_H_
#define OPENKAI_SRC_CONFIG_MODULE_H_

#include "../Base/common.h"
#include "../Base/BASE.h"
#include "../Script/Kiss.h"
#include "switch.h"

#include "../Stream/_StreamBase.h"
#include "../Stream/_Camera.h"
#include "../Stream/_ZED.h"
#include "../Stream/Frame.h"
#include "../Stream/FrameGroup.h"
#include "../Navigation/_Universe.h"
#include "../Automaton/_Automaton.h"

#include "../IO/SerialPort.h"
#include "../UI/UI.h"
#include "../UI/Window.h"

#include "../Detector/_Bullseye.h"
#include "../Vision/_Flow.h"
#include "../Detector/_Depth.h"
#include "../include/apriltags-cpp/TagDetector.h"
#include "../Detector/_AprilTags.h"
#include "../Tracker/_ROITracker.h"

#include "../Sensor/_Lightware_SF40_sender.h"
#include "../Sensor/_Lightware_SF40.h"

#include "../Autopilot/Action/ActionBase.h"
#include "../Autopilot/Action/HM/HM_base.h"
#include "../Autopilot/Action/HM/HM_follow.h"
#include "../Autopilot/Action/APMcopter/APMcopter_base.h"
#include "../Autopilot/Action/APMcopter/APMcopter_landing.h"
#include "../Autopilot/Action/APMrover/APMrover_base.h"
#include "../Autopilot/Action/APMrover/APMrover_follow.h"
#include "../Autopilot/Action/RC/RC_base.h"
#include "../Autopilot/Action/RC/RC_visualFollow.h"
#include "../Autopilot/_AutoPilot.h"
#include "../IO/File.h"

#include "../Network/_server.h"
#include "../Network/_socket.h"
#include "../Protocol/_Canbus.h"
#include "../Protocol/_Mavlink.h"
#include "../Protocol/_RC.h"

#ifdef USE_SSD
#include "../AI/_SSD.h"
#endif
#ifdef USE_FCN
#include "../AI/_FCN.h"
#endif
#ifdef USE_TENSORRT
#include "../AI/_DetectNet.h"
#endif


#define ADD_MODULE(x) if(pK->m_class == #x){return createInst<x>(pK);}

namespace kai
{

class Module
{
public:
	Module();
	virtual ~Module();
	BASE* createInstance(Kiss* pK);
private:
	template <typename T> BASE* createInst(Kiss* pKiss);
};

}

#endif
