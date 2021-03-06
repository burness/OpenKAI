#include "HM_base.h"

namespace kai
{

HM_base::HM_base()
{
	m_pCAN = NULL;

	m_motorPwmL = 0;
	m_motorPwmR = 0;
	m_motorPwmW = 0;
	m_bSpeaker = 0;

	m_maxSpeedT = 65535;
	m_maxSpeedW = 2500;
	m_ctrlB0 = 0;
	m_ctrlB1 = 0;
}

HM_base::~HM_base()
{
}

bool HM_base::init(void* pKiss)
{
	CHECK_F(!this->ActionBase::init(pKiss));
	Kiss* pK = (Kiss*)pKiss;
	pK->m_pInst = this;

	F_INFO(pK->v("maxSpeedT", &m_maxSpeedT));
	F_INFO(pK->v("maxSpeedW", &m_maxSpeedW));
	F_INFO(pK->v("bSpeaker", &m_bSpeaker));
	F_INFO(pK->v("motorPwmL", &m_motorPwmL));
	F_INFO(pK->v("motorPwmR", &m_motorPwmR));
	F_INFO(pK->v("motorPwmW", &m_motorPwmW));

	return true;
}

bool HM_base::link(void)
{
	CHECK_F(!this->ActionBase::link());
	Kiss* pK = (Kiss*)m_pKiss;

	string iName = "";
	F_INFO(pK->v("_Canbus", &iName));
	m_pCAN = (_Canbus*) (pK->root()->getChildInstByName(&iName));

	return true;
}

void HM_base::updateCAN(void)
{
	NULL_(m_pCAN);

	unsigned long addr = 0x113;
	unsigned char cmd[8];

	m_ctrlB0 = 0;
	m_ctrlB0 |= (m_motorPwmR<0)?(1 << 4):0;
	m_ctrlB0 |= (m_motorPwmL<0)?(1 << 5):0;
	m_ctrlB0 |= (m_motorPwmW<0)?(1 << 6):0;

	uint16_t motorPwmL = abs(constrain(m_motorPwmL, m_maxSpeedT, -m_maxSpeedT));
	uint16_t motorPwmR = abs(constrain(m_motorPwmR, m_maxSpeedT, -m_maxSpeedT));
	uint16_t motorPwmW = abs(constrain(m_motorPwmW, m_maxSpeedW, -m_maxSpeedW));

	m_ctrlB1 = 0;
	m_ctrlB1 |= 1;						//tracktion motor relay
	m_ctrlB1 |= (1 << 1);				//working motor relay
	m_ctrlB1 |= (m_bSpeaker)?(1 << 5):0;//speaker
	m_ctrlB1 |= (1 << 6);				//external command
	m_ctrlB1 |= (1 << 7);				//0:pwm, 1:rpm

	uint16_t bFilter = 0x00ff;

	cmd[0] = m_ctrlB0;
	cmd[1] = m_ctrlB1;
	cmd[2] = motorPwmL & bFilter;
	cmd[3] = (motorPwmL>>8) & bFilter;
	cmd[4] = motorPwmR & bFilter;
	cmd[5] = (motorPwmR>>8) & bFilter;
	cmd[6] = motorPwmW & bFilter;
	cmd[7] = (motorPwmW>>8) & bFilter;

	// send data:  id = 0x00, standrad frame, data len = 8, stmp: data buf
	// CAN.sendMsgBuf(0x113, 0, 8, cmd);

	m_pCAN->send(addr, 8, cmd);
}



}
