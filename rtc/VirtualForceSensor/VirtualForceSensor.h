// -*- C++ -*-
/*!
 * @file  VirtualForceSensor.h
 * @brief null component
 * @date  $Date$
 *
 * $Id$
 */

#ifndef VIRTUAL_FORCE_SENSOR_H
#define VIRTUAL_FORCE_SENSOR_H

#include <rtm/Manager.h>
#include <rtm/DataFlowComponentBase.h>
#include <rtm/CorbaPort.h>
#include <rtm/DataInPort.h>
#include <rtm/DataOutPort.h>
#include <rtm/idl/BasicDataTypeSkel.h>

#include <hrpModel/Body.h>
#include <hrpModel/Link.h>
#include <hrpModel/JointPath.h>
#include <hrpUtil/EigenTypes.h>

#include "IIRFilter.h"

// Service implementation headers
// <rtc-template block="service_impl_h">

// </rtc-template>

// Service Consumer stub headers
// <rtc-template block="consumer_stub_h">

// </rtc-template>

using namespace RTC;

/**
   \brief sample RT component which has one data input port and one data output port
 */
class VirtualForceSensor
  : public RTC::DataFlowComponentBase
{
 public:
  /**
     \brief Constructor
     \param manager pointer to the Manager
  */
  VirtualForceSensor(RTC::Manager* manager);
  /**
     \brief Destructor
  */
  virtual ~VirtualForceSensor();

  // The initialize action (on CREATED->ALIVE transition)
  // formaer rtc_init_entry()
  virtual RTC::ReturnCode_t onInitialize();

  // The finalize action (on ALIVE->END transition)
  // formaer rtc_exiting_entry()
  // virtual RTC::ReturnCode_t onFinalize();

  // The startup action when ExecutionContext startup
  // former rtc_starting_entry()
  // virtual RTC::ReturnCode_t onStartup(RTC::UniqueId ec_id);

  // The shutdown action when ExecutionContext stop
  // former rtc_stopping_entry()
  // virtual RTC::ReturnCode_t onShutdown(RTC::UniqueId ec_id);

  // The activated action (Active state entry action)
  // former rtc_active_entry()
  virtual RTC::ReturnCode_t onActivated(RTC::UniqueId ec_id);

  // The deactivated action (Active state exit action)
  // former rtc_active_exit()
  virtual RTC::ReturnCode_t onDeactivated(RTC::UniqueId ec_id);

  // The execution action that is invoked periodically
  // former rtc_active_do()
  virtual RTC::ReturnCode_t onExecute(RTC::UniqueId ec_id);

  // The aborting action when main logic error occurred.
  // former rtc_aborting_entry()
  // virtual RTC::ReturnCode_t onAborting(RTC::UniqueId ec_id);

  // The error action in ERROR state
  // former rtc_error_do()
  // virtual RTC::ReturnCode_t onError(RTC::UniqueId ec_id);

  // The reset action that is invoked resetting
  // This is same but different the former rtc_init_entry()
  // virtual RTC::ReturnCode_t onReset(RTC::UniqueId ec_id);

  // The state update action that is invoked after onExecute() action
  // no corresponding operation exists in OpenRTm-aist-0.2.0
  // virtual RTC::ReturnCode_t onStateUpdate(RTC::UniqueId ec_id);

  // The action that is invoked when execution context's rate is changed
  // no corresponding operation exists in OpenRTm-aist-0.2.0
  // virtual RTC::ReturnCode_t onRateChanged(RTC::UniqueId ec_id);


 protected:
  // Configuration variable declaration
  // <rtc-template block="config_declare">
  
  // </rtc-template>
  TimedDoubleSeq m_qRef;
  TimedDoubleSeq m_qCurrent;
  TimedDoubleSeq m_tauIn;
  TimedDoubleSeq m_tauOut;
  
  // DataInPort declaration
  // <rtc-template block="inport_declare">
  InPort<TimedDoubleSeq> m_qRefIn;
  InPort<TimedDoubleSeq> m_qCurrentIn;
  InPort<TimedDoubleSeq> m_tauInIn;
  OutPort<TimedDoubleSeq> m_tauOutOut;
  
  // </rtc-template>

  // DataOutPort declaration
  // <rtc-template block="outport_declare">
  std::vector<TimedDoubleSeq> m_force;
  std::vector<OutPort<TimedDoubleSeq> *> m_forceOut;
  
  // </rtc-template>

  // DataOutPort declaration
  // <rtc-template block="outport_declare">
  
  // </rtc-template>

  // CORBA Port declaration
  // <rtc-template block="corbaport_declare">
  
  // </rtc-template>

  // Service declaration
  // <rtc-template block="service_declare">
  //RTC::CorbaPort m_VirtualForceSensorServicePort;
  
  // </rtc-template>

  // Consumer declaration
  // <rtc-template block="consumer_declare">
  //VirtualForceSensorService_impl m_VirtualForceSensorService;
  
  // </rtc-template>

 private:
  struct VirtualForceSensorParam {
    std::string base_name, target_name;
    hrp::Vector3 p;
    hrp::Matrix33 R;
    hrp::JointPathPtr path;
  };
  std::map<std::string, VirtualForceSensorParam> m_sensors;
  std::vector<double> m_error_to_torque_gain;
  std::vector<double> m_error_dead_zone;
  std::vector<double> m_torque_offset;
  double m_dt;
  hrp::BodyPtr m_robot;
  unsigned int m_debugLevel;
  int dummy;
  std::vector<IIRFilter> m_filters;
};


extern "C"
{
  void VirtualForceSensorInit(RTC::Manager* manager);
};

#endif // VIRTUAL_FORCE_SENSOR_H
