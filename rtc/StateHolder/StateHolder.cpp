// -*- C++ -*-
/*!
 * @file  StateHolder.cpp
 * @brief state holder component
 * $Date$
 *
 * $Id$
 */

#include "StateHolder.h"
#include <rtm/CorbaNaming.h>
#include <hrpUtil/Eigen3d.h>
#include <hrpModel/ModelLoaderUtil.h>

// Module specification
// <rtc-template block="module_spec">
static const char* stateholder_spec[] =
  {
    "implementation_id", "StateHolder",
    "type_name",         "StateHolder",
    "description",       "state holder",
    "version",           "1.0",
    "vendor",            "AIST",
    "category",          "example",
    "activity_type",     "DataFlowComponent",
    "max_instance",      "10",
    "language",          "C++",
    "lang_type",         "compile",
    // Configuration variables

    ""
  };
// </rtc-template>

StateHolder::StateHolder(RTC::Manager* manager)
  : RTC::DataFlowComponentBase(manager),
    // <rtc-template block="initializer">
    m_currentQIn("currentQIn", m_currentQ),
    m_qIn("qIn", m_q),
    m_basePosIn("basePosIn", m_basePos),
    m_baseRpyIn("baseRpyIn", m_baseRpy),
    m_zmpIn("zmpIn", m_zmp),
    m_qOut("qOut", m_q),
    m_basePosOut("basePosOut", m_basePos),
    m_baseRpyOut("baseRpyOut", m_baseRpy),
    m_baseTformOut("baseTformOut", m_baseTform),
    m_basePoseOut("basePoseOut", m_basePose),
    m_zmpOut("zmpOut", m_zmp),
    m_StateHolderServicePort("StateHolderService"),
    m_TimeKeeperServicePort("TimeKeeperService"),
    // </rtc-template>
    m_timeCount(0),
    m_waitSem(0),
    m_timeSem(0),
    dummy(0)
{

  m_service0.setComponent(this);
  m_service1.setComponent(this);
  m_requestGoActual = false;

}

StateHolder::~StateHolder()
{
}



RTC::ReturnCode_t StateHolder::onInitialize()
{
  // <rtc-template block="bind_config">
  // Bind variables and configuration variable
  
  // </rtc-template>

  // Registration: InPort/OutPort/Service
  // <rtc-template block="registration">
  // Set InPort buffers
    addInPort("currentQIn", m_currentQIn);
    addInPort("qIn", m_qIn);
    addInPort("basePosIn", m_basePosIn);
    addInPort("baseRpyIn", m_baseRpyIn);
    addInPort("zmpIn", m_zmpIn);
  
  // Set OutPort buffer
    addOutPort("qOut", m_qOut);
    addOutPort("basePosOut", m_basePosOut);
    addOutPort("baseRpyOut", m_baseRpyOut);
    addOutPort("baseTformOut", m_baseTformOut);
    addOutPort("basePoseOut", m_basePoseOut);
    addOutPort("zmpOut", m_zmpOut);
  
  // Set service provider to Ports
  m_StateHolderServicePort.registerProvider("service0", "StateHolderService", m_service0);
  m_TimeKeeperServicePort.registerProvider("service1", "TimeKeeperService", m_service1);
  
  // Set service consumers to Ports
  
  // Set CORBA Service Ports
  addPort(m_StateHolderServicePort);
  addPort(m_TimeKeeperServicePort);

  // </rtc-template>

  RTC::Properties& prop = getProperties();
  coil::stringTo(m_dt, prop["dt"].c_str());
  std::cout << "StateHolder: dt = " << m_dt << std::endl;
  RTC::Manager& rtcManager = RTC::Manager::instance();
  std::string nameServer = rtcManager.getConfig()["corba.nameservers"];
  int comPos = nameServer.find(",");
  if (comPos < 0){
      comPos = nameServer.length();
  }
  nameServer = nameServer.substr(0, comPos);
  RTC::CorbaNaming naming(rtcManager.getORB(), nameServer.c_str());
  OpenHRP::BodyInfo_var binfo;
  binfo = hrp::loadBodyInfo(prop["model"].c_str(),
                            CosNaming::NamingContext::_duplicate(naming.getRootContext()));
  OpenHRP::LinkInfoSequence_var lis = binfo->links();
  const OpenHRP::LinkInfo& li = lis[0];
  hrp::Vector3 p, axis;
  p << li.translation[0], li.translation[1], li.translation[2];
  axis << li.rotation[0], li.rotation[1], li.rotation[2];
  hrp::Matrix33 R = hrp::rodrigues(axis, li.rotation[3]);
  hrp::Vector3 rpy = hrp::rpyFromRot(R);
  
  m_baseTform.data.length(12);
  double *T = m_baseTform.data.get_buffer();
  T[0] = R(0,0); T[1] = R(0,1); T[ 2] = R(0,2); T[ 3] = p[0];
  T[4] = R(0,0); T[5] = R(0,1); T[ 6] = R(0,2); T[ 7] = p[1];
  T[8] = R(0,0); T[9] = R(0,1); T[10] = R(0,2); T[11] = p[2];
  m_basePos.data.x = m_basePose.data.position.x = p[0];
  m_basePos.data.y = m_basePose.data.position.y = p[1];
  m_basePos.data.z = m_basePose.data.position.z = p[2];
  m_baseRpy.data.r = m_basePose.data.orientation.r = rpy[0];
  m_baseRpy.data.p = m_basePose.data.orientation.p = rpy[1];
  m_baseRpy.data.y = m_basePose.data.orientation.y = rpy[2];
  m_zmp.data.x = m_zmp.data.y = m_zmp.data.z = 0.0;
  
  return RTC::RTC_OK;
}



/*
RTC::ReturnCode_t StateHolder::onFinalize()
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t StateHolder::onStartup(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t StateHolder::onShutdown(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t StateHolder::onActivated(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t StateHolder::onDeactivated(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/

RTC::ReturnCode_t StateHolder::onExecute(RTC::UniqueId ec_id)
{
    //std::cout << "StateHolder::onExecute(" << ec_id << ")" << std::endl;
    coil::TimeValue coiltm(coil::gettimeofday());
    RTC::Time tm;
    tm.sec = coiltm.sec();
    tm.nsec = coiltm.usec()*1000;

    if (m_currentQIn.isNew()){
        m_currentQIn.read();
    }

    if (m_qIn.isNew()){
        m_qIn.read();
    }
    if (m_requestGoActual || (m_q.data.length() == 0 && m_currentQ.data.length() > 0)){
        m_q = m_currentQ;
    }

    if (m_requestGoActual){
        m_requestGoActual = false;
        m_waitSem.post();
    }

    if (m_basePosIn.isNew()){
        m_basePosIn.read();
    }

    if (m_baseRpyIn.isNew()){
        m_baseRpyIn.read();
    }

    if (m_zmpIn.isNew()){
        m_zmpIn.read();
    }

    double *a = m_baseTform.data.get_buffer();
    a[0] = m_basePos.data.x;
    a[1] = m_basePos.data.y;
    a[2] = m_basePos.data.z;
    hrp::Matrix33 R = hrp::rotFromRpy(m_baseRpy.data.r, 
                                      m_baseRpy.data.p, 
                                      m_baseRpy.data.y); 
    hrp::setMatrix33ToRowMajorArray(R, a, 3);

    m_basePose.data.position = m_basePos.data;
    m_basePose.data.orientation = m_baseRpy.data;

    // put timestamps
    m_q.tm         = tm;
    m_baseTform.tm = tm; 
    m_basePos.tm   = tm; 
    m_baseRpy.tm   = tm; 
    m_zmp.tm       = tm; 
    m_basePose.tm  = tm;

    // write
    if (m_q.data.length() > 0){
        m_qOut.write();
    }
    m_baseTformOut.write();
    m_basePosOut.write();
    m_baseRpyOut.write();
    m_zmpOut.write();
    m_basePoseOut.write();

    if (m_timeCount > 0){
        m_timeCount--;
        if (m_timeCount == 0) m_timeSem.post();
    }

    return RTC::RTC_OK;
}

/*
RTC::ReturnCode_t StateHolder::onAborting(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t StateHolder::onError(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t StateHolder::onReset(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t StateHolder::onStateUpdate(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t StateHolder::onRateChanged(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/


void StateHolder::goActual()
{
    std::cout << "StateHolder::goActual()" << std::endl;
    m_requestGoActual = true;
    m_waitSem.wait();
}

void StateHolder::getCommand(StateHolderService::Command &com)
{
    com.jointRefs.length(m_q.data.length());
    memcpy(com.jointRefs.get_buffer(), m_q.data.get_buffer(), sizeof(double)*m_q.data.length());
    com.baseTransform.length(12);
    com.baseTransform[0] = m_basePos.data.x;
    com.baseTransform[1] = m_basePos.data.y;
    com.baseTransform[2] = m_basePos.data.z;
    hrp::Matrix33 R = hrp::rotFromRpy(m_baseRpy.data.r, m_baseRpy.data.p, m_baseRpy.data.y);
    double *a = com.baseTransform.get_buffer();
    hrp::setMatrix33ToRowMajorArray(R, a, 3);
    com.zmp.length(3);
    com.zmp[0] = m_zmp.data.x; com.zmp[1] = m_zmp.data.y; com.zmp[2] = m_zmp.data.z; 
}

void StateHolder::wait(CORBA::Double tm)
{
    m_timeCount = tm/m_dt;
    m_timeSem.wait();
}
 
extern "C"
{

  void StateHolderInit(RTC::Manager* manager)
  {
    RTC::Properties profile(stateholder_spec);
    manager->registerFactory(profile,
                             RTC::Create<StateHolder>,
                             RTC::Delete<StateHolder>);
  }

};


