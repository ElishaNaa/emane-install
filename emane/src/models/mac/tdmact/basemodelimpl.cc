/*
 * Copyright (c) 2015-2016 - Adjacent Link LLC, Bridgewater, New Jersey
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 * * Neither the name of Adjacent Link LLC nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "basemodelimpl.h"
#include "emane/models/tdmact/queuemanager.h"

#include "emane/configureexception.h"
#include "emane/controls/frequencyofinterestcontrolmessage.h"
#include "emane/controls/flowcontrolcontrolmessage.h"
#include "emane/controls/serializedcontrolmessage.h"
#include "emane/mactypes.h"

#include "emane/controls/flowcontrolcontrolmessage.h"
#include "emane/controls/r2riselfmetriccontrolmessage.h"
#include "emane/controls/frequencycontrolmessage.h"
#include "emane/controls/frequencycontrolmessageformatter.h"
#include "emane/controls/receivepropertiescontrolmessage.h"
#include "emane/controls/receivepropertiescontrolmessageformatter.h"
#include "emane/controls/timestampcontrolmessage.h"
#include "emane/controls/transmittercontrolmessage.h"
#include "emane/spectrumserviceexception.h"
#include "emane/utils/conversionutils.h"
#include "emane/utils/spectrumwindowutils.h"

#include "txslotinfosformatter.h"
#include "basemodelmessage.h"
#include "managementmessage.h"
#include "priority.h"

#include <random> // for the random generation in self priority
#include <mutex> // lock tools
#include <functional>
#include "devlog.h"

#define TIME_STEP 1000



namespace
{
const std::string QUEUEMANAGER_PREFIX{"queue."};
const std::string SCHEDULER_PREFIX{"scheduler."};
}

EMANE::Models::TDMACT::BaseModel::Implementation::
    Implementation(NEMId id,
                   PlatformServiceProvider *pPlatformServiceProvider,
                   RadioServiceProvider *pRadioServiceProvider,
                   Scheduler *pScheduler,
                   QueueManager *pQueueManager,
                   MACLayerImplementor *pRadioModel) : MACLayerImplementor{id, pPlatformServiceProvider, pRadioServiceProvider},
                                                       pScheduler_{pScheduler},
                                                       pQueueManager_{pQueueManager},
                                                       pRadioModel_{pRadioModel},
                                                       bFlowControlEnable_{},
                                                       u16FlowControlTokens_{},
                                                       u16RetransmissionSlots_{},
                                                       u64FrameDuration_{},
                                                       u64ManagementRetransmissionSlots_{},
                                                       u64ManagementDuration_{},
                                                       u64ManagementEffectiveDuration_{},
                                                       u64NumScheduledSlots_{},
                                                       u64Frequency_Hz_{},
                                                       u64BandwidthHz_{},
                                                       u64DataRate_bps_{},
                                                       managementTimedEventId_{},
                                                       nextFrameTimedEventId_{},
                                                       dPower_dBm_{},
                                                       sPCRCurveURI_{},
                                                       slotOverhead_{},
                                                       u64SequenceNumber_{},
                                                       packetStatusPublisher_{},
                                                       neighborMetricManager_{id},
                                                       queueMetricManager_{id},
                                                       radioMetricTimedEventId_{},
                                                       receiveManager_{id,
                                                                       pRadioModel,
                                                                       &pPlatformServiceProvider->logService(),
                                                                       pRadioServiceProvider,
                                                                       &packetStatusPublisher_,
                                                                       &neighborMetricManager_},
                                                       managementReceiveManager_{id,
                                                                       pRadioModel,
                                                                       &pPlatformServiceProvider->logService(),
                                                                       pRadioServiceProvider,
                                                                       &packetStatusPublisher_,
                                                                       &neighborMetricManager_},
                                                       flowControlManager_{*pRadioModel} {}

EMANE::Models::TDMACT::BaseModel::Implementation::~Implementation()
{
}

void EMANE::Models::TDMACT::BaseModel::Implementation::initialize(Registrar &registrar)
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "MACI %03hu TDMACT::BaseModel::%s",
                          id_,
                          __func__);

  auto &configRegistrar = registrar.configurationRegistrar();

  configRegistrar.registerNumeric<bool>("enablepromiscuousmode",
                                        ConfigurationProperties::DEFAULT |
                                            ConfigurationProperties::MODIFIABLE,
                                        {false},
                                        "Defines whether promiscuous mode is enabled or not."
                                        " If promiscuous mode is enabled, all received packets"
                                        " (intended for the given node or not) that pass the"
                                        " probability of reception check are sent upstream to"
                                        " the transport.");

  configRegistrar.registerNumeric<bool>("flowcontrolenable",
                                        ConfigurationProperties::DEFAULT,
                                        {false},
                                        "Defines whether flow control is enabled. Flow control only works"
                                        " with the virtual transport and the setting must match the setting"
                                        " within the virtual transport configuration.");

  configRegistrar.registerNumeric<std::uint16_t>("flowcontroltokens",
                                                 ConfigurationProperties::DEFAULT,
                                                 {10},
                                                 "Defines the maximum number of flow control tokens"
                                                 " (packet transmission units) that can be processed from the"
                                                 " virtual transport without being refreshed. The number of"
                                                 " available tokens at any given time is coordinated with the"
                                                 " virtual transport and when the token count reaches zero, no"
                                                 " further packets are transmitted causing application socket"
                                                 " queues to backup.");

  configRegistrar.registerNumeric<std::uint16_t>("retransmissionslots",
                                                 ConfigurationProperties::DEFAULT,
                                                 {4},
                                                 "Defines the number of retransmission retransmissionslots in TDMACT");

  configRegistrar.registerNumeric<std::uint64_t>("managementretransmissionslots",
                                                 ConfigurationProperties::DEFAULT,
                                                 {6},
                                                 "Defines the number of retransmission retransmissionslots of priority bits in TDMACT");

  configRegistrar.registerNumeric<std::uint64_t>("managementduration",
                                                 ConfigurationProperties::DEFAULT,
                                                 {100000},
                                                 "The simulated duration in us of management retransmissions");

  configRegistrar.registerNumeric<std::uint64_t>("managementeffectiveduration",
                                                 ConfigurationProperties::DEFAULT,
                                                 {50000},
                                                 "The duration in microseconds as implemented by software of all management retransmissions in TDMACT");

  configRegistrar.registerNumeric<std::uint64_t>("managementbits",
                                                 ConfigurationProperties::DEFAULT,
                                                 {10},
                                                 "How many bits are there in a management message");

  configRegistrar.registerNumeric<std::uint64_t>("defaultclass",
                                                 ConfigurationProperties::REQUIRED,
                                                 {},
                                                 "Default priority class");

  configRegistrar.registerNumeric<std::uint64_t>("numscheduledslots",
                                                 ConfigurationProperties::DEFAULT,
                                                 {100},
                                                 "Defines the number of slots in a schedule message by the winner");

  configRegistrar.registerNumeric<std::uint64_t>("frameduration",
                                                 ConfigurationProperties::DEFAULT,
                                                 {1000000},
                                                 "Defines the duration of the slot (us) in TDMACT");

  configRegistrar.registerNumeric<std::uint64_t>("frequency",
                                                 ConfigurationProperties::REQUIRED,
                                                 {},
                                                 "Defines the frequency for all messages in TDMACT");

  configRegistrar.registerNumeric<double>("power",
                                          ConfigurationProperties::REQUIRED,
                                          {},
                                          "Defines the transmission power for all messages in TDMACT");

  configRegistrar.registerNumeric<std::uint64_t>("bandwidth",
                                                 ConfigurationProperties::REQUIRED,
                                                 {},
                                                 "Defines the bandwidth for all messages in TDMACT (Hz)");

  configRegistrar.registerNumeric<std::uint64_t>("datarate",
                                                 ConfigurationProperties::REQUIRED,
                                                 {},
                                                 "Defines the datarate for all messages in TDMACT (bps)");

  configRegistrar.registerNonNumeric<std::string>("pcrcurveuri",
                                                  ConfigurationProperties::REQUIRED,
                                                  {},
                                                  "Defines the absolute URI of the Packet Completion Rate (PCR) curve"
                                                  " file. The PCR curve file contains probability of reception curves"
                                                  " as a function of Signal to Interference plus Noise Ratio (SINR).");

  configRegistrar.registerNonNumeric<std::string>("devlogfile",
                                                  ConfigurationProperties::DEFAULT,
                                                  {std::string{""}},// dev 
                                                  "Defines the position of the development log.");


  configRegistrar.registerNumeric<std::uint64_t>("rounding",
                                                  ConfigurationProperties::DEFAULT,
                                                  {1},// dev 
                                                  "Discrete minimal time step (in microseconds)");

  configRegistrar.registerNumeric<std::uint16_t>("fragmentcheckthreshold",
                                                 ConfigurationProperties::DEFAULT,
                                                 {2},
                                                 "Defines the rate in seconds a check is performed to see if any packet"
                                                 " fragment reassembly efforts should be abandoned.");

  configRegistrar.registerNumeric<std::uint16_t>("fragmenttimeoutthreshold",
                                                 ConfigurationProperties::DEFAULT,
                                                 {5},
                                                 "Defines the threshold in seconds to wait for another packet fragment"
                                                 " for an existing reassembly effort before abandoning the effort.");
  //taksham dev
  configRegistrar.registerNumeric<bool>("radiometricenable",
                                        ConfigurationProperties::DEFAULT,
                                        {false},
                                        "Defines if radio metrics will be reported up via the Radio to Router Interface"
                                        " (R2RI).");

  configRegistrar.registerNumeric<float>("neighbormetricdeletetime",
                                         ConfigurationProperties::DEFAULT |
                                             ConfigurationProperties::MODIFIABLE,
                                         {60.0f},
                                         "Defines the time in seconds of no RF receptions from a given neighbor"
                                         " before it is removed from the neighbor table.",
                                         1.0f,
                                         3660.0f);

  configRegistrar.registerNumeric<float>("neighbormetricupdateinterval",
                                         ConfigurationProperties::DEFAULT,
                                         {1.0f},
                                         "Defines the neighbor table update interval in seconds.",
                                         0.1f,
                                         60.0f);

  auto &statisticRegistrar = registrar.statisticRegistrar();

  packetStatusPublisher_.registerStatistics(statisticRegistrar);

  slotStatusTablePublisher_.registerStatistics(statisticRegistrar);

  neighborMetricManager_.registerStatistics(statisticRegistrar);

  aggregationStatusPublisher_.registerStatistics(statisticRegistrar);

  pQueueManager_->setPacketStatusPublisher(&packetStatusPublisher_);

  pQueueManager_->initialize(registrar);

  pScheduler_->initialize(registrar);
}

void EMANE::Models::TDMACT::BaseModel::Implementation::configure(const ConfigurationUpdate &update)
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "MACI %03hu TDMACT::BaseModel::%s",
                          id_,
                          __func__);

  ConfigurationUpdate schedulerConfiguration{};
  ConfigurationUpdate queueManagerConfiguration{};

  for (const auto &item : update)
  {
    if (item.first == "enablepromiscuousmode")
    {
      bool bPromiscuousMode{item.second[0].asBool()};

      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              INFO_LEVEL,
                              "MACI %03hu TDMACT::BaseModel::%s: %s = %s",
                              id_,
                              __func__,
                              item.first.c_str(),
                              bPromiscuousMode ? "on" : "off");

      receiveManager_.setPromiscuousMode(bPromiscuousMode);
    }
    else if (item.first == "flowcontrolenable")
    {
      bFlowControlEnable_ = item.second[0].asBool();

      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              INFO_LEVEL,
                              "MACI %03hu TDMACT::BaseModel::%s: %s = %s",
                              id_,
                              __func__,
                              item.first.c_str(),
                              bFlowControlEnable_ ? "on" : "off");
    }
    else if (item.first == "flowcontroltokens")
    {
      u16FlowControlTokens_ = item.second[0].asUINT16();

      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              INFO_LEVEL,
                              "MACI %03hu TDMACT::BaseModel::%s: %s = %hu",
                              id_,
                              __func__,
                              item.first.c_str(),
                              u16FlowControlTokens_);
    }
    else if (item.first == "retransmissionslots")
    {
      u16RetransmissionSlots_ = item.second[0].asUINT16();

      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              INFO_LEVEL,
                              "MACI %03hu TDMACT::BaseModel::%s: %s = %hu",
                              id_,
                              __func__,
                              item.first.c_str(),
                              u16RetransmissionSlots_);
    }
    else if (item.first == "managementretransmissionslots")
    {
      u64ManagementRetransmissionSlots_ = item.second[0].asUINT64();

      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              INFO_LEVEL,
                              "MACI %03hu TDMACT::BaseModel::%s: %s = %lu",
                              id_,
                              __func__,
                              item.first.c_str(),
                              u64ManagementRetransmissionSlots_);
    }
    else if (item.first == "managementeffectiveduration")
    {
      u64ManagementEffectiveDuration_ = item.second[0].asUINT64();

      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              INFO_LEVEL,
                              "MACI %03hu TDMACT::BaseModel::%s: %s = %lu",
                              id_,
                              __func__,
                              item.first.c_str(),
                              u64ManagementEffectiveDuration_);
    }
    else if (item.first == "managementduration")
    {
      u64ManagementDuration_ = item.second[0].asUINT64();

      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              INFO_LEVEL,
                              "MACI %03hu TDMACT::BaseModel::%s: %s = %lu",
                              id_,
                              __func__,
                              item.first.c_str(),
                              u64ManagementRetransmissionSlots_);
    }
    else if (item.first == "managementbits")
    {
      u64ManagementBits_ = item.second[0].asUINT64();

      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              INFO_LEVEL,
                              "MACI %03hu TDMACT::BaseModel::%s: %s = %lu",
                              id_,
                              __func__,
                              item.first.c_str(),
                              u64ManagementBits_);
    }
    else if (item.first == "defaultclass")
    {
      u64DefaultClass_ = item.second[0].asUINT64();

      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              INFO_LEVEL,
                              "MACI %03hu TDMACT::BaseModel::%s: %s = %lu",
                              id_,
                              __func__,
                              item.first.c_str(),
                              u64DefaultClass_);
    }

    else if (item.first == "numscheduledslots")
    {
      u64NumScheduledSlots_ = item.second[0].asUINT64();

      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              INFO_LEVEL,
                              "MACI %03hu TDMACT::BaseModel::%s: %s = %lu",
                              id_,
                              __func__,
                              item.first.c_str(),
                              u64NumScheduledSlots_);
    }
    else if (item.first == "retransmissionslots")
    {
      u16RetransmissionSlots_ = item.second[0].asUINT16();

      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              INFO_LEVEL,
                              "MACI %03hu TDMACT::BaseModel::%s: %s = %hu",
                              id_,
                              __func__,
                              item.first.c_str(),
                              u16RetransmissionSlots_);
    }
    else if (item.first == "frameduration")
    {
      u64FrameDuration_ = item.second[0].asUINT64();

      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              INFO_LEVEL,
                              "MACI %03hu TDMACT::BaseModel::%s: %s = %lu",
                              id_,
                              __func__,
                              item.first.c_str(),
                              u64FrameDuration_);
    }
    else if (item.first == "frequency")
    {
      u64Frequency_Hz_ = item.second[0].asUINT64();

      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              INFO_LEVEL,
                              "MACI %03hu TDMACT::BaseModel::%s: %s = %lu",
                              id_,
                              __func__,
                              item.first.c_str(),
                              u64Frequency_Hz_);
    }
    else if (item.first == "power")
    {
      dPower_dBm_ = item.second[0].asDouble();

      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              INFO_LEVEL,
                              "MACI %03hu TDMACT::BaseModel::%s: %s = %g",
                              id_,
                              __func__,
                              item.first.c_str(),
                              dPower_dBm_);
    }
    else if (item.first == "bandwidth")
    {
      u64BandwidthHz_ = item.second[0].asUINT64();

      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              INFO_LEVEL,
                              "MACI %03hu TDMACT::BaseModel::%s: %s = %lu",
                              id_,
                              __func__,
                              item.first.c_str(),
                              u64BandwidthHz_);
    }
    else if (item.first == "datarate")
    {
      u64DataRate_bps_ = item.second[0].asUINT64();

      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              INFO_LEVEL,
                              "MACI %03hu TDMACT::BaseModel::%s: %s = %lu",
                              id_,
                              __func__,
                              item.first.c_str(),
                              u64DataRate_bps_);
    }
    else if (item.first == "pcrcurveuri")
    {
      sPCRCurveURI_ = item.second[0].asString();

      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              INFO_LEVEL,
                              "MACI %03hu TDMACT::BaseModel::%s: %s = %s",
                              id_,
                              __func__,
                              item.first.c_str(),
                              sPCRCurveURI_.c_str());

      receiveManager_.loadCurves(sPCRCurveURI_);
      managementReceiveManager_.loadCurves(sPCRCurveURI_);
    }
    else if (item.first == "devlogfile")
    {
      std::string sDevLogFile = item.second[0].asString();

      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              INFO_LEVEL,
                              "MACI %03hu TDMACT::BaseModel::%s: %s = %s",
                              id_,
                              __func__,
                              item.first.c_str(),
                              sDevLogFile.c_str());
      CTLOG_INIT(sDevLogFile);
    } 
    else if (item.first == "rounding")
    {
      std::uint64_t rounding = item.second[0].asUINT64();
      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              INFO_LEVEL,
                              "MACI %03hu TDMACT::BaseModel::%s: %s = %lu",
                              id_,
                              __func__,
                              item.first.c_str(),
                              rounding);

      rounder_.setRounding(rounding);
    }
    else if (item.first == "fragmentcheckthreshold")
    {
      std::chrono::seconds fragmentCheckThreshold{item.second[0].asUINT16()};

      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              INFO_LEVEL,
                              "MACI %03hu TDMACT::BaseModel::%s: %s = %lu",
                              id_,
                              __func__,
                              item.first.c_str(),
                              fragmentCheckThreshold.count());

      receiveManager_.setFragmentCheckThreshold(fragmentCheckThreshold);
    }
    else if (item.first == "fragmenttimeoutthreshold")
    {
      std::chrono::seconds fragmentTimeoutThreshold{item.second[0].asUINT16()};

      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              INFO_LEVEL,
                              "MACI %03hu TDMACT::BaseModel::%s: %s = %lu",
                              id_,
                              __func__,
                              item.first.c_str(),
                              fragmentTimeoutThreshold.count());

      receiveManager_.setFragmentTimeoutThreshold(fragmentTimeoutThreshold);
    }//taksham-dev
    else if(item.first == "radiometricenable")
        {
          bRadioMetricEnable_ = item.second[0].asBool();

          LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                  INFO_LEVEL,
                                  "MACI %03hu TDMACT::BaseModel::%s: %s = %s",
                                  id_,
                                  __func__,
                                  item.first.c_str(),
                                  bRadioMetricEnable_ ? "on" : "off");
        }
    else if (item.first == "neighbormetricdeletetime")
    {
      Microseconds neighborMetricDeleteTimeMicroseconds =
          std::chrono::duration_cast<Microseconds>(DoubleSeconds{item.second[0].asFloat()});

      // set the neighbor delete time
      neighborMetricManager_.setNeighborDeleteTimeMicroseconds(neighborMetricDeleteTimeMicroseconds);

      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              DEBUG_LEVEL,
                              "MACI %03hu TDMACT::BaseModel::%s %s = %lf",
                              id_,
                              __func__,
                              item.first.c_str(),
                              std::chrono::duration_cast<DoubleSeconds>(neighborMetricDeleteTimeMicroseconds).count());
    }
    else if (item.first == "neighbormetricupdateinterval")
    {
      neighborMetricUpdateInterval_ =
          std::chrono::duration_cast<Microseconds>(DoubleSeconds{item.second[0].asFloat()});

      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              INFO_LEVEL,
                              "MACI %03hu TDMACT::BaseModel::%s %s = %lf",
                              id_,
                              __func__,
                              item.first.c_str(),
                              std::chrono::duration_cast<DoubleSeconds>(neighborMetricUpdateInterval_).count());
    }
    else
    {
      if (!item.first.compare(0, SCHEDULER_PREFIX.size(), SCHEDULER_PREFIX))
      {
        schedulerConfiguration.push_back(item);
      }
      else if (!item.first.compare(0, QUEUEMANAGER_PREFIX.size(), QUEUEMANAGER_PREFIX))
      {
        queueManagerConfiguration.push_back(item);
      }
      else
      {
        throw makeException<ConfigureException>("TDMACT::BaseModel: "
                                                "Ambiguous configuration item %s.",
                                                item.first.c_str());
      }
    }
  }

  pQueueManager_->configure(queueManagerConfiguration);

  pScheduler_->configure(schedulerConfiguration);
}

void EMANE::Models::TDMACT::BaseModel::Implementation::start()
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "MACI %03hu TDMACT::BaseModel::%s",
                          id_,
                          __func__);

  pQueueManager_->start();

  pScheduler_->start();
}

void EMANE::Models::TDMACT::BaseModel::Implementation::postStart()
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "MACI %03hu TDMACT::BaseModel::%s",
                          id_,
                          __func__);

  pQueueManager_->postStart();

  pScheduler_->postStart();

  // check flow control enabled
  if (bFlowControlEnable_)
  {
    // start flow control
    flowControlManager_.start(u16FlowControlTokens_);

    LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                            DEBUG_LEVEL,
                            "MACI %03hu TDMACT::BaseModel::%s sent a flow control token update,"
                            " a handshake response is required to process packets",
                            id_,
                            __func__);
  }
  //originally was:
  /*pPlatformService_->timerService().
    schedule(std::bind(&NeighborMetricManager::updateNeighborStatus,
                       &neighborMetricManager_),
            Clock::now() + neighborMetricUpdateInterval_,
            neighborMetricUpdateInterval_);*/

  //taksham dev

  radioMetricTimedEventId_ = 
  pPlatformService_->timerService().schedule([this](const TimePoint &, const TimePoint &, const TimePoint &)
          {
            if(!bRadioMetricEnable_)
              {
                neighborMetricManager_.updateNeighborStatus();
              }
            else
              {
                ControlMessages msgs{
                  Controls::R2RISelfMetricControlMessage::create(u64DataRate_bps_,
                                                                u64DataRate_bps_,
                                                                neighborMetricUpdateInterval_),
                  Controls::R2RINeighborMetricControlMessage::create(neighborMetricManager_.getNeighborMetrics()),
                  Controls::R2RIQueueMetricControlMessage::create(queueMetricManager_.getQueueMetrics())};


                  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          INFO_LEVEL, //change to DEBUG
                          "MACI %03hu TDMACT::BaseModel::%s msg logs 1 u64DataRate_bps_=%lu "
                                                                       "neighborMetricUpdateInterval_=%lf",
                          id_,
                          __func__,
                          u64DataRate_bps_,
                          std::chrono::duration_cast<DoubleSeconds>(neighborMetricUpdateInterval_).count());

                sendUpstreamControl(msgs);
              }
          },
          Clock::now() + neighborMetricUpdateInterval_,
          neighborMetricUpdateInterval_);

  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          INFO_LEVEL, //change to DEBUG
                          "MACI %03hu TDMACT::BaseModel::%s added radio metric timed eventId %zu",
                          id_,
                          __func__,
                          radioMetricTimedEventId_);
}

void EMANE::Models::TDMACT::BaseModel::Implementation::stop()
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "MACI %03hu TDMACT::BaseModel::%s",
                          id_,
                          __func__);

  // check flow control enabled
  if (bFlowControlEnable_)
  {
    // stop the flow control manager
    flowControlManager_.stop();
  }

  pQueueManager_->stop();

  pScheduler_->stop();
}

void EMANE::Models::TDMACT::BaseModel::Implementation::destroy() throw()
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "MACI %03hu TDMACT::BaseModel::%s",
                          id_,
                          __func__);

  pQueueManager_->destroy();

  pScheduler_->destroy();
}

void EMANE::Models::TDMACT::BaseModel::Implementation::processUpstreamControl(const ControlMessages &)
{
  /*LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "MACI %03hu TDMACT::BaseModel::%s",
                          id_,
                          __func__);*/
}



void EMANE::Models::TDMACT::BaseModel::Implementation::processDownstreamControl(const ControlMessages &msgs)
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "MACI %03hu TDMACT::BaseModel::%s",
                          id_,
                          __func__);

  for (const auto &pMessage : msgs)
  {
    LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                            DEBUG_LEVEL,
                            "MACI %03hu TDMACT::BaseModel::%s downstream control message id %hu",
                            id_,
                            __func__,
                            pMessage->getId());

    switch (pMessage->getId())
    {
    case Controls::FlowControlControlMessage::IDENTIFIER:
    {
      const auto pFlowControlControlMessage =
          static_cast<const Controls::FlowControlControlMessage *>(pMessage);

      if (bFlowControlEnable_)
      {
        LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                DEBUG_LEVEL,
                                "MACI %03hu TDMACT::BaseModel::%s received a flow control token request/response",
                                id_,
                                __func__);

        flowControlManager_.processFlowControlMessage(pFlowControlControlMessage);
      }
      else
      {
        LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                ERROR_LEVEL,
                                "MACI %03hu TDMACT::BaseModel::%s received a flow control token request but"
                                " flow control is not enabled",
                                id_,
                                __func__);
      }
    }
    break;

    case Controls::SerializedControlMessage::IDENTIFIER:
    {
      const auto pSerializedControlMessage =
          static_cast<const Controls::SerializedControlMessage *>(pMessage);

      switch (pSerializedControlMessage->getSerializedId())
      {
      case Controls::FlowControlControlMessage::IDENTIFIER:
      {
        std::unique_ptr<Controls::FlowControlControlMessage>
            pFlowControlControlMessage{
                Controls::FlowControlControlMessage::create(pSerializedControlMessage->getSerialization())};

        if (bFlowControlEnable_)
        {
          flowControlManager_.processFlowControlMessage(pFlowControlControlMessage.get());
        }
        else
        {
          LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                  ERROR_LEVEL,
                                  "MACI %03hu TDMACT::BaseModel::%s received a flow control token request but"
                                  " flow control is not enabled",
                                  id_,
                                  __func__);
        }
      }
      break;
      }
    }
    }
  }
}

void EMANE::Models::TDMACT::BaseModel::Implementation::processDownstreamPacket(DownstreamPacket &pkt,
                                                                               const ControlMessages &)
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "MACI %03hu TDMACT::BaseModel::%s",
                          id_,
                          __func__);

  // check flow control
  if (bFlowControlEnable_)
  {
    auto status = flowControlManager_.removeToken();

    if (status.second == false)
    {
      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              ERROR_LEVEL,
                              "MACI %03hu TDMACT::BaseModel::%s: failed to remove token, drop packet (tokens:%hu)",
                              id_,
                              __func__,
                              status.first);

      const auto &pktInfo = pkt.getPacketInfo();

      packetStatusPublisher_.outbound(pktInfo.getSource(),
                                      pktInfo.getSource(),
                                      pktInfo.getPriority(),
                                      pkt.length(),
                                      PacketStatusPublisher::OutboundAction::DROP_FLOW_CONTROL);

      // drop
      return;
    }
  }

  std::uint8_t u8Queue{priorityToQueue(pkt.getPacketInfo().getPriority())};

  size_t packetsDropped{pQueueManager_->enqueue(u8Queue, std::move(pkt), getBytesInRetransmissionSlot())};

  // drop, replace token
  if (bFlowControlEnable_)
  {
    for (size_t i = 0; i < packetsDropped; ++i)
    {
      auto status = flowControlManager_.addToken();

      if (!status.second)
      {
        LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                ERROR_LEVEL,
                                "MACI %03hu TDMACT::BaseModel:::%s: failed to add token (tokens:%hu)",
                                id_,
                                __func__,
                                status.first);
      }
    }
  }
}

void EMANE::Models::TDMACT::BaseModel::Implementation::processEvent(const EventId &eventId,
                                                                    const Serialization &serialization)
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "MACI %03hu TDMACT::BaseModel::%s",
                          id_,
                          __func__);

  pScheduler_->processEvent(eventId, serialization);
}

void EMANE::Models::TDMACT::BaseModel::Implementation::processConfiguration(const ConfigurationUpdate &update)
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "MACI %03hu TDMACT::BaseModel::%s",
                          id_,
                          __func__);

  ConfigurationUpdate schedulerConfiguration;

  for (const auto &item : update)
  {
    if (item.first == "enablepromiscuousmode")
    {
      bool bPromiscuousMode{item.second[0].asBool()};

      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              INFO_LEVEL,
                              "MACI %03hu TDMACT::BaseModel::%s: %s = %s",
                              id_,
                              __func__,
                              item.first.c_str(),
                              bPromiscuousMode ? "on" : "off");

      receiveManager_.setPromiscuousMode(bPromiscuousMode);
    }
    else
    {
      schedulerConfiguration.push_back(item);
    }
  }

  pScheduler_->configure(schedulerConfiguration);
}


// legacy function that is implemented because we are a subclass of SchedulerUser
// the whole scheduling system has been completely rewritten and it is no longer needed
// the receival of a schedule is for now an on switch for the whole TDMACT system
void EMANE::Models::TDMACT::BaseModel::Implementation::notifyScheduleChange(const Frequencies &frequencies,
                                                                            std::uint64_t u64BandwidthHz,
                                                                            const Microseconds &slotDuration,
                                                                            const Microseconds &slotOverhead)
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "MACI %03hu TDMACT::BaseModel::%s",
                          id_,
                          __func__);
  // Just to avoid warning of unused arguments
  CTLOG("Received a new schedule %lu %lu %lu %lu", frequencies.size(), u64BandwidthHz, slotDuration.count(), slotOverhead.count())

  slotStatusTablePublisher_.clear();

  lastReceptionAbsoluteFrame_ = 0;


  EMANE::TimePoint now = EMANE::Clock::now();
  rounder_.round(now);
  std::uint64_t nextFrameAbsIndex = absoluteTimeToAbsoluteFrameIndex(now) + 1;
  EMANE::TimePoint nextFrameTime = absoluteFrameIndexToAbsoluteTime(nextFrameAbsIndex); // already rounded, so actually no need
  rounder_.round(nextFrameTime);

  std::time_t t{Clock::to_time_t(nextFrameTime)};
  std::tm ltm;

  localtime_r(&t, &ltm);
  CTLOG("Next slot in %02d:%02d:%02d.%06lu %lu\n", ltm.tm_hour,
        ltm.tm_min,
        ltm.tm_sec,
        std::chrono::duration_cast<Microseconds>(nextFrameTime.time_since_epoch()).count() % 1000000, u64FrameDuration_);

  u64PendingFrame_ = absoluteTimeToAbsoluteFrameIndex(now);
  nextFrameTimedEventId_ = 
      pPlatformService_->timerService().schedule(std::bind(&Implementation::processFrameBegin,
                                                           this,
                                                           nextFrameAbsIndex),
                                                 nextFrameTime);
  slotOverhead_ = Microseconds{0}; // overhead microseconds for each packet. For now it will be fixed to 0
}

void EMANE::Models::TDMACT::BaseModel::Implementation::processSchedulerPacket(DownstreamPacket &pkt)
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "MACI %03hu TDMACT::BaseModel::%s",
                          id_,
                          __func__);

  // enqueue into max priority queue
  pQueueManager_->enqueue(4, std::move(pkt), 0);
}

void EMANE::Models::TDMACT::BaseModel::Implementation::processSchedulerControl(const ControlMessages &msgs)
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "MACI %03hu TDMACT::BaseModel::%s",
                          id_,
                          __func__);

  pRadioModel_->sendDownstreamControl(msgs);
}

EMANE::Models::TDMACT::QueueInfos EMANE::Models::TDMACT::BaseModel::Implementation::getPacketQueueInfo() const
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "MACI %03hu TDMACT::BaseModel::%s",
                          id_,
                          __func__);

  return pQueueManager_->getPacketQueueInfo();
}

void EMANE::Models::TDMACT::BaseModel::Implementation::sendDownstreamPacket(double dSlotRemainingRatio)
{
  // calculate the number of bytes allowed in the slot
  size_t bytesAvailable =
      getBytesInRetransmissionSlot() - slotOverhead_.count();
  auto entry = pQueueManager_->dequeue(u64DefaultClass_,
                                       bytesAvailable,
                                       0); // 0 should be any destination
  CTLOG("NEM %d. sendDownstream, requesting %d bytes from %d, the slot is %d microseconds, datarate %zu bps, overhead %d us\n",
        id_,
        bytesAvailable,
        u64DefaultClass_,
        getRetransmissionSlotDuration_us(),
        getEffectiveDataRateBps(),
        slotOverhead_.count());
  MessageComponents &components = std::get<0>(entry);
  size_t totalSize{std::get<1>(entry)};

  if (totalSize)
  {
    if (totalSize <= bytesAvailable)
    {
      float fSeconds{totalSize * 8.0f / getEffectiveDataRateBps()};

      Microseconds duration{std::chrono::duration_cast<Microseconds>(DoubleSeconds{fSeconds})};

      // rounding error corner case mitigation
      rounder_.round_EMANE_microSeconds(duration);
      if (duration >= Microseconds{getRetransmissionSlotDuration_us()})
      {
        duration = Microseconds{getRetransmissionSlotDuration_us()-rounder_.get_rounding_us()};
      }
      NEMId dst{};
      size_t completedPackets{};

      // determine how many components represent completed packets (no fragments remain) and
      // whether to use a unicast or broadcast nem address
      for (const auto &component : components)
      {
        completedPackets += !component.isMoreFragments();

        // if not set, set a destination
        if (!dst)
        {
          dst = component.getDestination();
        }
        else if (dst != NEM_BROADCAST_MAC_ADDRESS)
        {
          // if the destination is not broadcast, check to see if it matches
          // the destination of the current component - if not, set the NEM
          // broadcast address as the dst
          if (dst != component.getDestination())
          {
            dst = NEM_BROADCAST_MAC_ADDRESS;
          }
        }
      }

      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              DEBUG_LEVEL,
                              "MACI %03hu TDMACT::BaseModel::%s sending downstream to %03hu components: %zu",
                              id_,
                              __func__,
                              dst,
                              components.size());

      CTLOG("NEM %d. Sending downstream to %03hu components: %zu\n", id_, dst, components.size());
      if (bFlowControlEnable_ && completedPackets)
      {
        auto status = flowControlManager_.addToken(completedPackets);

        if (!status.second)
        {
          LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                  ERROR_LEVEL,
                                  "MACI %03hu TDMACT::BaseModel::%s: failed to add token (tokens:%hu)",
                                  id_,
                                  __func__,
                                  status.first);
        }
      }

      aggregationStatusPublisher_.update(components);

      BaseModelMessage baseModelMessage{u64PendingFrame_,
                                        0, // first time sending message, so retransmission subslot is 0
                                        u64DataRate_bps_,//getEffectiveDataRateBps(),//juravski change
                                        totalSize,
                                        std::move(components)};

      Serialization serialization{baseModelMessage.serialize()};

      EMANE::TimePoint now = Clock::now();
      rounder_.round(now);

      DownstreamPacket pkt({id_, dst, 0, now}, serialization.c_str(), serialization.size());

      pkt.prependLengthPrefixFraming(serialization.size());

      pRadioModel_->sendDownstreamPacket(CommonMACHeader{REGISTERED_EMANE_MAC_TDMACT, u64SequenceNumber_++},
                                         pkt,
                                         {Controls::FrequencyControlMessage::create(
                                              u64BandwidthHz_,
                                              {{u64Frequency_Hz_, duration}}),
                                          Controls::TimeStampControlMessage::create(absoluteFrameIndexToAbsoluteTime(u64PendingFrame_)),
                                          Controls::TransmitterControlMessage::create({{id_, dPower_dBm_}})});

      slotStatusTablePublisher_.update(1,// bad statistics but they are low priority
                                       1,
                                       1,
                                       SlotStatusTablePublisher::Status::TX_GOOD,
                                       dSlotRemainingRatio);

      neighborMetricManager_.updateNeighborTxMetric(dst,
                                                    getEffectiveDataRateBps(),
                                                    now);
    }
    else
    {
      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              ERROR_LEVEL,
                              "MACI %03hu TDMACT::BaseModel::%s queue dequeue returning %zu bytes than slot has available %zu",
                              id_,
                              __func__,
                              totalSize,
                              bytesAvailable);
    }
  }
  else
  {
    // bad statistics
    slotStatusTablePublisher_.update(1,
                                     1,
                                     1,
                                     SlotStatusTablePublisher::Status::TX_GOOD,
                                     dSlotRemainingRatio);
  }
}

void EMANE::Models::TDMACT::BaseModel::Implementation::processTxOpportunity()
{
  // check also retransmission slot??

  EMANE::TimePoint now = Clock::now();
  rounder_.round(now);

  auto nowFrame = absoluteTimeToAbsoluteFrameIndex(now);

  Microseconds timeRemainingInSlot{Microseconds(getRetransmissionSlotDuration_us()) -
                                   std::chrono::duration_cast<Microseconds>(now -
                                                                            absoluteFrameIndexToAbsoluteTime(u64PendingFrame_))};
  double dSlotRemainingRatio =
      timeRemainingInSlot.count() / static_cast<double>(getRetransmissionSlotDuration_us());

  CTLOG("NEM %d. TXopportunity %g available\n", id_, dSlotRemainingRatio);
  if (nowFrame == u64PendingFrame_)
  {
    // transmit in this slot
    sendDownstreamPacket(dSlotRemainingRatio);
  }
  else
  {
    slotStatusTablePublisher_.update(1,
                                     1,
                                     1,
                                     SlotStatusTablePublisher::Status::TX_MISSED,
                                     dSlotRemainingRatio);
  }

  return;
}

/****************************************************************************************************/
/*********************       start       downstream              ************************************/
/****************************************************************************************************/


void  EMANE::Models::TDMACT::BaseModel::Implementation::sendDownstreamMessage(BaseModelMessage &&bmm, TimePoint txStart)
{
  TimePoint now = Clock::now();
  rounder_.round(now);

  std::uint64_t totalSize = bmm.getTotalSize();

  Serialization serialization{bmm.serialize()};

  DownstreamPacket pkt({id_, NEM_BROADCAST_MAC_ADDRESS, 0, now}, serialization.c_str(), serialization.size());

  pkt.prependLengthPrefixFraming(serialization.size());

  float fSeconds{totalSize * 8.0f / getEffectiveDataRateBps()}; // THERE IS AN ERROR IN THIS RESULT


  Microseconds duration{std::chrono::duration_cast<Microseconds>(DoubleSeconds{fSeconds})};

  CTLOG("NEM %d Sending downstream message size %f sec %lu usec %lu\n", id_, fSeconds, duration.count(), getEffectiveDataRateBps());

  pRadioModel_->sendDownstreamPacket(CommonMACHeader{REGISTERED_EMANE_MAC_TDMACT, u64SequenceNumber_++},
                                     pkt,
                                     {Controls::FrequencyControlMessage::create(
                                          u64BandwidthHz_,
                                          {{u64Frequency_Hz_, duration}}),
                                      Controls::TimeStampControlMessage::create(txStart),
                                      Controls::TransmitterControlMessage::create({{id_, dPower_dBm_}})});
}


void  EMANE::Models::TDMACT::BaseModel::Implementation::sendDownstreamManagement(ManagementMessage &&mm, TimePoint txStart)
{
  TimePoint now = Clock::now();
  rounder_.round(now);
  CTLOG("NEM %d sending management downstream from id %d\n", id_, mm.getSource());

  Serialization serialization{mm.serialize()};

  DownstreamPacket pkt({id_, NEM_BROADCAST_MAC_ADDRESS, 0, now}, serialization.c_str(), serialization.size());

  pkt.prependLengthPrefixFraming(serialization.size());

  float fSeconds{10.0f / u64DataRate_bps_}; // for now we consider 10 bit
  // this is not correct since there is also the schedule but

  Microseconds duration{std::chrono::duration_cast<Microseconds>(DoubleSeconds{fSeconds})};

  pRadioModel_->sendDownstreamPacket(CommonMACHeader{REGISTERED_EMANE_MAC_TDMACT_MANAGEMENT, u64SequenceNumber_++},
                                     pkt,
                                     {Controls::FrequencyControlMessage::create(
                                          u64BandwidthHz_,
                                          {{u64Frequency_Hz_, duration}}),
                                      Controls::TimeStampControlMessage::create(txStart),
                                      Controls::TransmitterControlMessage::create({{id_, dPower_dBm_}})});
  TimePoint end = Clock::now();
  rounder_.round(end);
  CTLOG("NEM: %d %s duration %u\n", id_, __func__, (std::chrono::duration_cast<EMANE::Microseconds>(end-now)).count());

}

/****************************************************************************************************/
/*********************       end       downstream                ************************************/
/****************************************************************************************************/


/****************************************************************************************************/
/*********************       start       process slots           ************************************/
/****************************************************************************************************/
std::mutex  mtxFB;
void EMANE::Models::TDMACT::BaseModel::Implementation::processFrameBegin(std::uint64_t u64FrameToStart)
{
  // 1 - check that u64SlotToStart is consistent with now (if not, error and choose now)
  // 2 - check if it has not been started already (by a message receival)
  
 
  EMANE::TimePoint now = EMANE::Clock::now();
  rounder_.round(now);
  std::uint64_t u64ThisFrameIndexNow = absoluteTimeToAbsoluteFrameIndex(now);
  if (u64ThisFrameIndexNow != u64FrameToStart) {
    LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                            ERROR_LEVEL,
                            "MACI %03hu TDMACT::BaseModel::%s missed slot %lu",
                            id_,
                            __func__,
                            u64FrameToStart);
    
  }

  mtxFB.lock();
  CTLOG("NEM %d -------- slot pending: %lu ---------  tostart: %lu\n", id_, u64PendingFrame_, u64FrameToStart);
 
  if (u64PendingFrame_ < u64FrameToStart)
  {
    if (u64PendingFrame_+1 != u64FrameToStart)
    {
      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              ERROR_LEVEL,
                            "MACI %03hu TDMACT::BaseModel::%s skipped frame %lu",
                            id_,
                            __func__,
                            u64FrameToStart);
    }
    u64PendingFrame_ = u64FrameToStart;

    // 1 process receival
    receiveManager_.processFrame();


    // 2 process management receival
    u64ReceivalPendingManagementRelativeRetransmissionSlot_ = std::experimental::nullopt;
    managementReceiveManager_.processRetransmissionSlot();
    std::experimental::optional<ManagementMessage> lastmm = managementReceiveManager_.processFrame();
    if (lastmm)
    {
      pendingSchedule_ = lastmm->getSchedule();
    }
    else
    {
      CTLOG("NEM %d no management arrived\n",id_);
    }

    // 3 elaborate schedule
    CTLOG("NEM %d the transmitter will be %d\n", id_, pendingSchedule_[u64PendingFrame_].transmitter_);
    if (pendingSchedule_.find(u64PendingFrame_) != pendingSchedule_.end() && pendingSchedule_[u64PendingFrame_].transmitter_ == id_) // look at map....
    {
      pendingType_ = TX;
      processTxOpportunity();
    }
    else
    {
      pendingType_ = RX;
      u64ReceivalPendingRelativeRetransmissionSlot_ = 0;
    }

    // 4 schedule stuff

    // 5 send if needed
    EMANE::TimePoint thisFrameMangementPartBeginTime = absoluteFrameIndexToAbsoluteTime(u64ThisFrameIndexNow) + Microseconds{u64FrameDuration_} - Microseconds{u64ManagementEffectiveDuration_};
    rounder_.round(thisFrameMangementPartBeginTime); // should be already rounded so this is superfluous

    EMANE::TimePoint nextFrameTime = absoluteFrameIndexToAbsoluteTime(u64ThisFrameIndexNow + 1);
    rounder_.round(nextFrameTime); // should be already rounded so this is superfluous

    nextFrameTimedEventId_ =
      pPlatformService_->timerService().schedule(std::bind(&Implementation::processFrameBegin,
                                                           this,
                                                           u64ThisFrameIndexNow + 1),
                                                 nextFrameTime);

    managementTimedEventId_ =
      pPlatformService_->timerService().schedule(std::bind(&Implementation::processManagementPartBegin,
                                                           this),
                                                            thisFrameMangementPartBeginTime);
    
    if(pendingType_ == RX)
    {
        EMANE::TimePoint nextRetransmission = absoluteFrameIndexToAbsoluteTime(u64PendingFrame_) + relativeRetransmissionSlotIndexToRelativeTime(1);
        rounder_.round(nextRetransmission);
        nextRetransmissionTimedEventId_ =
        pPlatformService_->timerService().schedule(std::bind(&Implementation::processRetransmissionSlotBegin,
                                                           this,
                                                           1),
                                                           nextRetransmission);
        u64ReceivalPendingRelativeRetransmissionSlot_ = 0;
    }                                                    
  }
  TimePoint end = Clock::now();
  rounder_.round(end);
  CTLOG("NEM: %d %s duration %u\n", id_, __func__, (std::chrono::duration_cast<EMANE::Microseconds>(end-now)).count());
  mtxFB.unlock();
}

std::mutex mtxMP;

void EMANE::Models::TDMACT::BaseModel::Implementation::processManagementPartBegin()
{
    mtxMP.lock();
  CTLOG("NEM %d ---- management part begin frame %lu---\n", id_, u64PendingFrame_)
  // check timings
  EMANE::TimePoint now = EMANE::Clock::now();
  rounder_.round(now);
  std::uint64_t u64ThisFrameIndexNow = absoluteTimeToAbsoluteFrameIndex(now);
  if (u64ThisFrameIndexNow != u64PendingFrame_) {
    LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                            ERROR_LEVEL,
                            "MACI %03hu TDMACT::BaseModel::%s incompatible times for management part, frame %lu"
                            "skipping frame entirely",
                            id_,
                            __func__,
                            u64PendingFrame_);
  }
  else 
  {
    // 1 process receival
    receiveManager_.processRetransmissionSlot(); // last retransmission slot
    u64ReceivalPendingRelativeRetransmissionSlot_ = std::experimental::nullopt;

    // 2 decide priorities
    EMANE::Models::TDMACT::QueueInfos queueInfos = pQueueManager_->getPacketQueueInfo();
    std::uint64_t maxPriority = 0;
    std::uint64_t occupation = 0;
    std::uint64_t randomPart = 0;
    for (auto const &queueInfo : queueInfos)
    {
      //CTLOG("id: %d packets: %lu bytes: %lu\n", queueInfo.u8QueueId_, queueInfo.u64Packets_, queueInfo.u64Bytes_);
      if (queueInfo.u64Packets_ != 0)
      {
        maxPriority = queueInfo.u8QueueId_;
 //     occupancy (congestion) bits is now a single value whether there are messages waiting or not
        occupation = (queueInfo.u64Packets_ > 0) ? 1 : 0;
      }
    }

    
    std::mt19937_64 engine(u64PendingFrame_);
    std::uniform_int_distribution<std::uint64_t> dist;
    randomPart = dist(engine) & 0x7F; // only the last 7 bits
    randomPart = randomPart ^ id_;


    // 3 elaborate schedule
    Schedule newSchedule = pendingSchedule_; 
    // 3.2 remove old entries
    auto it_start = newSchedule.begin(); 
    auto it_this = newSchedule.upper_bound(u64PendingFrame_);
    newSchedule.erase(it_start,it_this);
    std::experimental::optional<std::uint64_t> frameToOverride;

    for (unsigned int i = 1; i< u64NumScheduledSlots_; i++)
    {
      if(pendingSchedule_.find(u64PendingFrame_+i) == pendingSchedule_.end() || 
            (pendingSchedule_[u64PendingFrame_+i].u64Priority_ <  maxPriority && pendingSchedule_[u64PendingFrame_+i].transmitter_ != id_))
      {
        if(!frameToOverride)
          frameToOverride = u64PendingFrame_+i;
      }
      
    }
    newSchedule[*frameToOverride] = {*frameToOverride, id_, maxPriority, 0};

    CTLOG("NEM: %d priorities are (%lu,%lu,%lu)\n", id_, maxPriority, occupation, randomPart);

    // ...
    // find new entry and add myself
    ManagementMessage newManagement{id_,
                                    u64PendingFrame_,
                                    0, // relative retransmission slot
                                    u64DataRate_bps_,
                                    maxPriority,
                                    occupation,
                                    randomPart,
                                    newSchedule};
    
    
    managementReceiveManager_.setSelfMessage(newManagement);
    EMANE::TimePoint thisManagementPart = absoluteFrameIndexToAbsoluteTime(u64PendingFrame_) + relativeManagementRetransmissionSlotIndexToRelativeTime(0);
    rounder_.round(thisManagementPart);
    sendDownstreamManagement(std::move(newManagement), thisManagementPart);
    // send management. if no retransmission the following should not be done
    EMANE::TimePoint nextRetransmission = absoluteFrameIndexToAbsoluteTime(u64PendingFrame_) + relativeManagementRetransmissionSlotIndexToRelativeTime(1);
    rounder_.round(nextRetransmission);
    nextManagementRetransmissionTimedEventId_ =
      pPlatformService_->timerService().schedule(std::bind(&Implementation::processManagementRetransmissionSlotBegin,
                                                            this,
                                                            1),
                                                            nextRetransmission);
    u64ReceivalPendingManagementRelativeRetransmissionSlot_ = 0;

    // 4 schedule stuff


  }
  TimePoint end = Clock::now();
  rounder_.round(end);
  CTLOG("NEM: %d %s duration %u\n", id_, __func__, (std::chrono::duration_cast<EMANE::Microseconds>(end-now)).count());
    mtxMP.unlock();

}


std::mutex mtxRS;

void EMANE::Models::TDMACT::BaseModel::Implementation::processRetransmissionSlotBegin(std::uint64_t u64RelativeRetransmissionSlotIndex)
{
  // can be called by incoming message!!
  // check times ...
  mtxRS.lock();
  TimePoint now = Clock::now();
  rounder_.round(now);
  CTLOG("NEM %d -- Retransmission slot (%lu,%lu), pending %lu \n", id_, u64PendingFrame_, u64RelativeRetransmissionSlotIndex, *u64ReceivalPendingRelativeRetransmissionSlot_);


  EMANE::TimePoint frameStart = absoluteFrameIndexToAbsoluteTime(u64PendingFrame_);
  rounder_.round(frameStart); // probably superfluous
  EMANE::TimePoint retransmissionTx = frameStart + relativeRetransmissionSlotIndexToRelativeTime(u64RelativeRetransmissionSlotIndex);
  rounder_.round(retransmissionTx); // probably superfluous, slotting functions rounded this already
  std::uint64_t relativeRetransmissionNow{relativeTimeToRelativeRetransmissionSlotIndex(std::chrono::duration_cast<EMANE::Microseconds>(now-frameStart))};
  

  if(u64RelativeRetransmissionSlotIndex == relativeRetransmissionNow && u64ReceivalPendingRelativeRetransmissionSlot_)
  { // all is well
    if (u64RelativeRetransmissionSlotIndex > *u64ReceivalPendingRelativeRetransmissionSlot_)
    {
      std::experimental::optional<BaseModelMessage> bmm = receiveManager_.processRetransmissionSlot();
      u64ReceivalPendingRelativeRetransmissionSlot_ = u64RelativeRetransmissionSlotIndex;
      if(bmm) 
      {
        lastReceptionAbsoluteFrame_ = u64PendingFrame_;  
        bmm->setRelativeRetransmissionSlotIndex(u64RelativeRetransmissionSlotIndex);
        sendDownstreamMessage(std::move(*bmm), retransmissionTx);
        std::experimental::optional<BaseModelMessage> bmm = receiveManager_.processRetransmissionSlot();
      }
      if(u64RelativeRetransmissionSlotIndex+1<u16RetransmissionSlots_)
      {
        EMANE::TimePoint nextRetransmission = absoluteFrameIndexToAbsoluteTime(u64PendingFrame_) + relativeRetransmissionSlotIndexToRelativeTime(u64RelativeRetransmissionSlotIndex+1);
        rounder_.round(nextRetransmission);
        nextRetransmissionTimedEventId_ =
            pPlatformService_->timerService().schedule(std::bind(&Implementation::processRetransmissionSlotBegin,
                                                           this,
                                                           u64RelativeRetransmissionSlotIndex+1),
                                                           nextRetransmission);
      }
    }
  }
  else {
    LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          ERROR_LEVEL,
                          "MACI %03hu TDMACT::BaseModel::%s wrong retransmission slot %lu vs %lu",
                          id_,
                          __func__,
                          relativeRetransmissionNow,
                          u64RelativeRetransmissionSlotIndex);
  }

  mtxRS.unlock();
}

std::mutex mtxMRS;

void EMANE::Models::TDMACT::BaseModel::Implementation::processManagementRetransmissionSlotBegin(std::uint64_t u64RelativeManagementRetransmissionSlotIndex)
{
  mtxMRS.lock();
  TimePoint now = Clock::now();
  rounder_.round(now);
  CTLOG("NEM %d -- Management retransmission slot (%lu,%lu) pending %lu \n", id_, u64PendingFrame_, u64RelativeManagementRetransmissionSlotIndex, *u64ReceivalPendingManagementRelativeRetransmissionSlot_);
  // can be called by incoming message!!
  
  //std::experimental::optional<ManagementMessage> mm = managementReceiveManager_.processRetransmissionSlot();
  EMANE::TimePoint frameStart = absoluteFrameIndexToAbsoluteTime(u64PendingFrame_);
  rounder_.round(frameStart);
  EMANE::TimePoint retransmissionTx = frameStart + relativeManagementRetransmissionSlotIndexToRelativeTime(u64RelativeManagementRetransmissionSlotIndex);
  rounder_.round(retransmissionTx);
  std::uint64_t relativeManagementRetransmissionNow{relativeTimeToRelativeManagementRetransmissionSlotIndex(std::chrono::duration_cast<EMANE::Microseconds>(now-frameStart))};

  if(u64RelativeManagementRetransmissionSlotIndex == relativeManagementRetransmissionNow && u64ReceivalPendingManagementRelativeRetransmissionSlot_)
  { 
    if (u64RelativeManagementRetransmissionSlotIndex > *u64ReceivalPendingManagementRelativeRetransmissionSlot_)
    {
      std::experimental::optional<ManagementMessage> mm = managementReceiveManager_.processRetransmissionSlot();
      u64ReceivalPendingManagementRelativeRetransmissionSlot_ = u64RelativeManagementRetransmissionSlotIndex;
      if(mm) 
      {
         mm->setRelativeRetransmissionSlotIndex(u64RelativeManagementRetransmissionSlotIndex);
         sendDownstreamManagement(std::move(*mm), retransmissionTx);
      }
      if(u64RelativeManagementRetransmissionSlotIndex+1<u64ManagementRetransmissionSlots_)
      {
        EMANE::TimePoint nextRetransmission = absoluteFrameIndexToAbsoluteTime(u64PendingFrame_) + relativeManagementRetransmissionSlotIndexToRelativeTime(u64RelativeManagementRetransmissionSlotIndex+1);
        rounder_.round(nextRetransmission);
        nextManagementRetransmissionTimedEventId_ =
            pPlatformService_->timerService().schedule(std::bind(&Implementation::processManagementRetransmissionSlotBegin,
                                                           this,
                                                           u64RelativeManagementRetransmissionSlotIndex+1),
                                                           nextRetransmission);
      }
    }
    else
    {
      CTLOG("NEM %d Process management retransmission did nothing\n", id_);
    }
  }
  else {
    LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          ERROR_LEVEL,
                          "MACI %03hu TDMACT::BaseModel::%s wrong retransmission slot %lu vs %lu",
                          id_,
                          __func__,
                          relativeManagementRetransmissionNow,
                          u64RelativeManagementRetransmissionSlotIndex);
  }


  mtxMRS.unlock();
}


/****************************************************************************************************/
/*********************       end         process slots           ************************************/
/****************************************************************************************************/


/****************************************************************************************************/
/*********************       start       receiving           ****************************************/
/****************************************************************************************************/

// process message that will be sent upstream
void EMANE::Models::TDMACT::BaseModel::Implementation::processUpstreamPacketBaseMessage(const CommonMACHeader &hdr,
                                                                                        UpstreamPacket &pkt,
                                                                                        const ControlMessages &msgs)
{

  // time control based on frames
  TimePoint now = Clock::now();
  rounder_.round(now);

  const PacketInfo &pktInfo{pkt.getPacketInfo()};

  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "MACI %03hu TDMACT::BaseModel::%s",
                          id_,
                          __func__);

  size_t len{pkt.stripLengthPrefixFraming()};

  CTLOG("NEM %d #### processing upstream packet from %d, len %lu length %lu\n", id_, pktInfo.getSource(), len, pkt.length());

  if (len && pkt.length() >= len)
  {
    //BaseModelMessage &baseModelMessage (*baseModelMessagePtr);
    BaseModelMessage baseModelMessage{pkt.get(), len};
    const Controls::ReceivePropertiesControlMessage *pReceivePropertiesControlMessage{};

    const Controls::FrequencyControlMessage *pFrequencyControlMessage{};

    // decode receiveproperties and frequencycontrol
    for (auto &pControlMessage : msgs)
    {
      switch (pControlMessage->getId())
      {
      case EMANE::Controls::ReceivePropertiesControlMessage::IDENTIFIER:
      {
        pReceivePropertiesControlMessage =
            static_cast<const Controls::ReceivePropertiesControlMessage *>(pControlMessage);

        LOGGER_VERBOSE_LOGGING_FN_VARGS(pPlatformService_->logService(),
                                        DEBUG_LEVEL,
                                        Controls::ReceivePropertiesControlMessageFormatter(pReceivePropertiesControlMessage),
                                        "MACI %03hu TDMACT::BaseModel::%s Receiver Properties Control Message",
                                        id_,
                                        __func__);
      }
      break;

      case Controls::FrequencyControlMessage::IDENTIFIER:
      {
        pFrequencyControlMessage =
            static_cast<const Controls::FrequencyControlMessage *>(pControlMessage);

        LOGGER_VERBOSE_LOGGING_FN_VARGS(pPlatformService_->logService(),
                                        DEBUG_LEVEL,
                                        Controls::FrequencyControlMessageFormatter(pFrequencyControlMessage),
                                        "MACI %03hu TDMACT::BaseModel::%s Frequency Control Message",
                                        id_,
                                        __func__);
      }

      break;
      }
    }

    if (!pReceivePropertiesControlMessage || !pFrequencyControlMessage ||
        pFrequencyControlMessage->getFrequencySegments().empty())
    {
      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              ERROR_LEVEL,
                              "MACI %03hu TDMACT::BaseModel::%s: phy control "
                              "message not provided from src %hu, drop",
                              id_,
                              __func__,
                              pktInfo.getSource());

      packetStatusPublisher_.inbound(pktInfo.getSource(),
                                     baseModelMessage.getMessages(),
                                     PacketStatusPublisher::InboundAction::DROP_BAD_CONTROL);

      // drop
      return;
    }

    const auto &frequencySegments = pFrequencyControlMessage->getFrequencySegments();

    const FrequencySegment &frequencySegment{*frequencySegments.begin()};

    TimePoint startOfReception{pReceivePropertiesControlMessage->getTxTime() +
                               pReceivePropertiesControlMessage->getPropagationDelay() +
                               frequencySegment.getOffset()};
    rounder_.round(startOfReception);              
    TimePoint endOfReception = startOfReception + frequencySegment.getDuration();
    rounder_.round(endOfReception);
    std::uint64_t eorAbsoluteFrameIndex = absoluteTimeToAbsoluteFrameIndex(endOfReception);

    std::uint64_t eorRelativeRetransmissionSlotIndex = this->relativeTimeToRelativeRetransmissionSlotIndex(std::chrono::duration_cast<EMANE::Microseconds>(endOfReception - absoluteFrameIndexToAbsoluteTime(eorAbsoluteFrameIndex)));
    
    Microseconds frameDuration{u64FrameDuration_};
    

    // if message is too long for slot or for retransmissionslot (EOR slot does not match the SOT slot drop the packet) -> drop
    if (eorAbsoluteFrameIndex != baseModelMessage.getAbsoluteFrameIndex() || eorRelativeRetransmissionSlotIndex != baseModelMessage.getRelativeRetransmissionSlotIndex())
    {
      
      // determine current slot based on now time to update rx slot status table
      std::uint64_t nowAbsoluteFrameIndex = absoluteTimeToAbsoluteFrameIndex(now);

      Microseconds timeRemainingInSlot{};
      

      // ratio calcualtion for slot status tables
      if (nowAbsoluteFrameIndex == baseModelMessage.getAbsoluteFrameIndex())
      {
        timeRemainingInSlot = frameDuration -
                              std::chrono::duration_cast<Microseconds>(now -
                                                                       absoluteFrameIndexToAbsoluteTime(nowAbsoluteFrameIndex));
      }
      else
      {
        timeRemainingInSlot = frameDuration +
                              std::chrono::duration_cast<Microseconds>(now -
                                                                       absoluteFrameIndexToAbsoluteTime(nowAbsoluteFrameIndex));
      }

      double dSlotRemainingRatio =
          timeRemainingInSlot.count() / static_cast<double>(frameDuration.count());

      slotStatusTablePublisher_.update(1, // originally relativeindex
                                       1, // originally relativeframe
                                       1, // originally relativeslot
                                       SlotStatusTablePublisher::Status::RX_TOOLONG,
                                       dSlotRemainingRatio);

      packetStatusPublisher_.inbound(pktInfo.getSource(),
                                     baseModelMessage.getMessages(),
                                     PacketStatusPublisher::InboundAction::DROP_TOO_LONG);

      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              DEBUG_LEVEL,
                              "MACI %03hu TDMACT::BaseModel::%s eor rx slot:"
                              " (%zu,%zu) does not match sot slot: (%zu,%zu), drop long",
                              id_,
                              __func__,
                              eorAbsoluteFrameIndex,
                              eorRelativeRetransmissionSlotIndex,
                              baseModelMessage.getAbsoluteFrameIndex(),
                              baseModelMessage.getRelativeRetransmissionSlotIndex());

      // drop
      return;
    }

    std::uint64_t nowAbsoluteFrameIndex = absoluteTimeToAbsoluteFrameIndex(now); // RX time slot of now
    std::uint64_t nowRelativeRetransmissionSlotIndex = this->relativeTimeToRelativeRetransmissionSlotIndex(std::chrono::duration_cast<EMANE::Microseconds>(now - absoluteFrameIndexToAbsoluteTime(nowAbsoluteFrameIndex)));

    // if the slot or retransmissionslot of now does not match the message drop
    if (nowAbsoluteFrameIndex == baseModelMessage.getAbsoluteFrameIndex() && nowRelativeRetransmissionSlotIndex == baseModelMessage.getRelativeRetransmissionSlotIndex())
    {
      Microseconds timeRemainingInSlot{frameDuration -
                                       std::chrono::duration_cast<Microseconds>(now -
                                                                                absoluteFrameIndexToAbsoluteTime(nowAbsoluteFrameIndex))};

      double dSlotRemainingRatio =
          timeRemainingInSlot.count() / static_cast<double>(frameDuration.count());

      if (frameType(nowAbsoluteFrameIndex) == RX)
      {
        if (u64Frequency_Hz_ == frequencySegment.getFrequencyHz())
        {
          // we are in an RX slot
          
          if (baseModelMessage.getAbsoluteFrameIndex() > lastReceptionAbsoluteFrame_)
          {
            // we did not receive other transmission in this slot
            slotStatusTablePublisher_.update(1, // bad statistics, see before
                                             1,
                                             1,
                                             SlotStatusTablePublisher::Status::RX_GOOD,
                                             dSlotRemainingRatio);

            Microseconds span{pReceivePropertiesControlMessage->getSpan()};


            if (u64PendingFrame_ && baseModelMessage.getAbsoluteFrameIndex() > u64PendingFrame_)
            {
              CTLOG("NEM %d Anticipating whole frame\n", id_);
              processFrameBegin(baseModelMessage.getAbsoluteFrameIndex());
            } // frame should be correct by now
   
            if (u64ReceivalPendingRelativeRetransmissionSlot_ && *u64ReceivalPendingRelativeRetransmissionSlot_ < baseModelMessage.getRelativeRetransmissionSlotIndex())
            {
              CTLOG("NEM %d Anticipating message retransmission slot\n", id_);
              processRetransmissionSlotBegin(baseModelMessage.getRelativeRetransmissionSlotIndex());
              CTLOG("NEM %d relativeretransmissionslot is now %lu\n", id_, *u64ReceivalPendingRelativeRetransmissionSlot_);
            } //  retransmission slot should be correct by now
            // maybe it is needed to verify all is correct before continuing (very extreme cases)
            if (baseModelMessage.getAbsoluteFrameIndex() == u64PendingFrame_ &&
                baseModelMessage.getRelativeRetransmissionSlotIndex() == *u64ReceivalPendingRelativeRetransmissionSlot_ )
            {
              receiveManager_.enqueue(std::move(baseModelMessage),
                                        pktInfo,
                                        pkt.length(),
                                        startOfReception,
                                        frequencySegments,
                                        span,
                                        now,
                                        hdr.getSequenceNumber());
              CTLOG("NEM %d Enqueued to receivemanager len: %lu length: %lu\n", id_, len, pkt.length());

            }
            else
            {
              LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                  DEBUG_LEVEL,
                                  "MACI %03hu TDMACT::BaseModel::%s drop reason rx (frame,slot) incorrect"
                                  " %lu,%lu (message) instead of %lu,%lu (pending)",
                                  id_,
                                  __func__,
                                  baseModelMessage.getAbsoluteFrameIndex(),
                                  baseModelMessage.getRelativeRetransmissionSlotIndex(),
                                  u64PendingFrame_,
                                  *u64ReceivalPendingRelativeRetransmissionSlot_);
            }


         
          }
          else
          {
            CTLOG("NEM %d message already received in this frame \n", id_);
          }
        }
        else
        {
          slotStatusTablePublisher_.update(1, // bad statistics
                                           1,
                                           1,
                                           SlotStatusTablePublisher::Status::RX_WRONGFREQ,
                                           dSlotRemainingRatio);

          packetStatusPublisher_.inbound(pktInfo.getSource(),
                                         baseModelMessage.getMessages(),
                                         PacketStatusPublisher::InboundAction::DROP_FREQUENCY);

          LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                  DEBUG_LEVEL,
                                  "MACI %03hu TDMACT::BaseModel::%s drop reason rx slot correct"
                                  " rframe: %lu  but frequency mismatch expected: %zu got: %zu",
                                  id_,
                                  __func__,
                                  nowAbsoluteFrameIndex,
                                  u64Frequency_Hz_,
                                  frequencySegment.getFrequencyHz());

          // drop
          return;
        }
      }
      else
      {
        // not an rx slot but it is the correct abs slot

        slotStatusTablePublisher_.update(1, // bad statistics
                                         1,
                                         1,
                                         SlotStatusTablePublisher::Status::RX_TX, // crass, we lost the IDLE state (see TDMA)
                                         dSlotRemainingRatio);

        packetStatusPublisher_.inbound(pktInfo.getSource(),
                                       baseModelMessage.getMessages(),
                                       PacketStatusPublisher::InboundAction::DROP_SLOT_NOT_RX);

        LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                DEBUG_LEVEL,
                                "MACI %03hu TDMACT::BaseModel::%s drop reason rx slot correct but %s Message absslot: (%zu,%zu), now absslot %zu, eor absslot %zu"
                                "from %d",
                                id_,
                                __func__,
                                frameType(nowAbsoluteFrameIndex) == TX ?  "in tx" : "?", // we lost IDLE
                                baseModelMessage.getAbsoluteFrameIndex(),
                                baseModelMessage.getRelativeRetransmissionSlotIndex(),
                                nowAbsoluteFrameIndex,
                                eorAbsoluteFrameIndex,
                                pktInfo.getSource());

        // drop
        return;
      }
    }
    else
    {
      Microseconds timeRemainingInSlot{frameDuration +
                                       std::chrono::duration_cast<Microseconds>(now -
                                                                                absoluteFrameIndexToAbsoluteTime(nowAbsoluteFrameIndex))};
      double dSlotRemainingRatio =
          timeRemainingInSlot.count() / static_cast<double>(frameDuration.count());

      // were we supposed to be in rx on the pkt abs slot
      if (frameType(nowAbsoluteFrameIndex) == RX)
      {
        slotStatusTablePublisher_.update(1,
                                         1,
                                         1,
                                         SlotStatusTablePublisher::Status::RX_MISSED,
                                         dSlotRemainingRatio);

        packetStatusPublisher_.inbound(pktInfo.getSource(),
                                       baseModelMessage.getMessages(),
                                       PacketStatusPublisher::InboundAction::DROP_SLOT_MISSED_RX);

        LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                DEBUG_LEVEL,
                                "MACI %03hu TDMACT::BaseModel::%s drop reason slot mismatch pkt: %zu now: %zu ",
                                id_,
                                __func__,
                                baseModelMessage.getAbsoluteFrameIndex(),
                                nowAbsoluteFrameIndex);

        // drop
        return;
      }
      else
      {
        slotStatusTablePublisher_.update(1,
                                         1,
                                         1,
                                         SlotStatusTablePublisher::Status::RX_TX, // we lost IDLE
                                         dSlotRemainingRatio);

        packetStatusPublisher_.inbound(pktInfo.getSource(),
                                       baseModelMessage.getMessages(),
                                       PacketStatusPublisher::InboundAction::DROP_SLOT_NOT_RX);

        LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                DEBUG_LEVEL,
                                "MACI %03hu TDMACT::BaseModel::%s drop reason slot mismatch but %s pkt: %zu now: %zu ",
                                id_,
                                __func__,
                                "in tx", //
                                baseModelMessage.getAbsoluteFrameIndex(),
                                nowAbsoluteFrameIndex);

        // drop
        return;
      }
    }
  }
  else
  {
    LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                            ERROR_LEVEL,
                            "MACI %03hu TDMACT::BaseModel::%s Packet payload length %zu does not match length prefix %zu",
                            id_,
                            __func__,
                            pkt.length(),
                            len);
  }
}


void EMANE::Models::TDMACT::BaseModel::Implementation::processUpstreamPacketManagement(const CommonMACHeader &hdr,
                                                                                        UpstreamPacket &pkt,
                                                                                         const ControlMessages &msgs)
{
    // time control based on frames
  TimePoint now = Clock::now();
  rounder_.round(now);

  CTLOG("NEM %d ### processing management upstream\n", id_);

  const PacketInfo &pktInfo{pkt.getPacketInfo()};

  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "MACI %03hu TDMACT::BaseModel::%s",
                          id_,
                          __func__);

  size_t len{pkt.stripLengthPrefixFraming()};

  if (len && pkt.length() >= len)
  {
    //BaseModelMessage &baseModelMessage (*baseModelMessagePtr);
    ManagementMessage managementMessage{pkt.get(), len};
    const Controls::ReceivePropertiesControlMessage *pReceivePropertiesControlMessage{};

    const Controls::FrequencyControlMessage *pFrequencyControlMessage{};

    // decode receiveproperties and frequencycontrol
    for (auto &pControlMessage : msgs)
    {
      switch (pControlMessage->getId())
      {
      case EMANE::Controls::ReceivePropertiesControlMessage::IDENTIFIER:
      {
        pReceivePropertiesControlMessage =
            static_cast<const Controls::ReceivePropertiesControlMessage *>(pControlMessage);

        LOGGER_VERBOSE_LOGGING_FN_VARGS(pPlatformService_->logService(),
                                        DEBUG_LEVEL,
                                        Controls::ReceivePropertiesControlMessageFormatter(pReceivePropertiesControlMessage),
                                        "MACI %03hu TDMACT::BaseModel::%s Receiver Properties Control Message",
                                        id_,
                                        __func__);
      }
      break;

      case Controls::FrequencyControlMessage::IDENTIFIER:
      {
        pFrequencyControlMessage =
            static_cast<const Controls::FrequencyControlMessage *>(pControlMessage);

        LOGGER_VERBOSE_LOGGING_FN_VARGS(pPlatformService_->logService(),
                                        DEBUG_LEVEL,
                                        Controls::FrequencyControlMessageFormatter(pFrequencyControlMessage),
                                        "MACI %03hu TDMACT::BaseModel::%s Frequency Control Message",
                                        id_,
                                        __func__);
      }

      break;
      }
    }

    if (!pReceivePropertiesControlMessage || !pFrequencyControlMessage ||
        pFrequencyControlMessage->getFrequencySegments().empty())
    {
      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              ERROR_LEVEL,
                              "MACI %03hu TDMACT::BaseModel::%s: phy control "
                              "message not provided from src %hu, drop",
                              id_,
                              __func__,
                              pktInfo.getSource());

      // drop
      return;
    }

    const auto &frequencySegments = pFrequencyControlMessage->getFrequencySegments();

    const FrequencySegment &frequencySegment{*frequencySegments.begin()};

    TimePoint startOfReception{pReceivePropertiesControlMessage->getTxTime() +
                               pReceivePropertiesControlMessage->getPropagationDelay() +
                               frequencySegment.getOffset()};

    rounder_.round(startOfReception);

    TimePoint endOfReception = startOfReception + frequencySegment.getDuration();
    rounder_.round(endOfReception);

    std::uint64_t eorAbsoluteFrameIndex = absoluteTimeToAbsoluteFrameIndex(endOfReception);

    std::uint64_t eorRelativeRetransmissionSlotIndex = this->relativeTimeToRelativeManagementRetransmissionSlotIndex(std::chrono::duration_cast<EMANE::Microseconds>(endOfReception - absoluteFrameIndexToAbsoluteTime(eorAbsoluteFrameIndex)));
    
    Microseconds frameDuration{u64FrameDuration_};
    // if message is too long for slot or for retransmissionslot (EOR slot does not match the SOT slot drop the packet) -> drop
    if (eorAbsoluteFrameIndex != managementMessage.getAbsoluteFrameIndex() || eorRelativeRetransmissionSlotIndex != managementMessage.getRelativeRetransmissionSlotIndex())
    {
      // determine current slot based on now time to update rx slot status table
      std::uint64_t nowAbsoluteFrameIndex = absoluteTimeToAbsoluteFrameIndex(now);

      Microseconds timeRemainingInSlot{};
      

      // ratio calcualtion for slot status tables
      if (nowAbsoluteFrameIndex == managementMessage.getAbsoluteFrameIndex())
      {
        timeRemainingInSlot = frameDuration -
                              std::chrono::duration_cast<Microseconds>(now -
                                                                       absoluteFrameIndexToAbsoluteTime(nowAbsoluteFrameIndex));
      }
      else
      {
        timeRemainingInSlot = frameDuration +
                              std::chrono::duration_cast<Microseconds>(now -
                                                                       absoluteFrameIndexToAbsoluteTime(nowAbsoluteFrameIndex));
      }


      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              DEBUG_LEVEL,
                              "MACI %03hu TDMACT::BaseModel::%s eor rx slot:"
                              " (%zu,%zu) does not match sot slot: (%zu,%zu), drop long length %lu",
                              id_,
                              __func__,
                              eorAbsoluteFrameIndex,
                              eorRelativeRetransmissionSlotIndex,
                              managementMessage.getAbsoluteFrameIndex(),
                              managementMessage.getRelativeRetransmissionSlotIndex(),
                              frequencySegment.getDuration().count());

      // drop
      return;
    }

    std::uint64_t nowAbsoluteFrameIndex = absoluteTimeToAbsoluteFrameIndex(now); // RX time slot of now
    std::uint64_t nowRelativeRetransmissionSlotIndex = this->relativeTimeToRelativeManagementRetransmissionSlotIndex(std::chrono::duration_cast<EMANE::Microseconds>(now - absoluteFrameIndexToAbsoluteTime(nowAbsoluteFrameIndex)));

    // if the slot or retransmissionslot of now does not match the message drop
    if (nowAbsoluteFrameIndex == managementMessage.getAbsoluteFrameIndex() && nowRelativeRetransmissionSlotIndex == managementMessage.getRelativeRetransmissionSlotIndex())
    {
      Microseconds timeRemainingInSlot{frameDuration -
                                       std::chrono::duration_cast<Microseconds>(now -
                                                                                absoluteFrameIndexToAbsoluteTime(nowAbsoluteFrameIndex))};

      
        if (u64Frequency_Hz_ == frequencySegment.getFrequencyHz())
        {

            Microseconds span{pReceivePropertiesControlMessage->getSpan()};

            if (u64PendingFrame_ && managementMessage.getAbsoluteFrameIndex() > u64PendingFrame_)
            {
              CTLOG("NEM %d Anticipating whole frame\n", id_);
              processFrameBegin(managementMessage.getAbsoluteFrameIndex());
            } // frame should be correct by now
            if (!u64ReceivalPendingManagementRelativeRetransmissionSlot_) {
              CTLOG("NEM %d Anticipating management part\n", id_);
              processManagementPartBegin();
            } // management should be correct by now
            if (u64ReceivalPendingManagementRelativeRetransmissionSlot_ && u64ReceivalPendingManagementRelativeRetransmissionSlot_ < managementMessage.getRelativeRetransmissionSlotIndex())
            {
              CTLOG("NEM %d Anticipating management retransmission slot\n", id_);
              processManagementRetransmissionSlotBegin(managementMessage.getRelativeRetransmissionSlotIndex());
            } // management retransmission slot should be correct by now
            // maybe it is needed to verify all is correct before continuing (very extreme cases)
            if (managementMessage.getAbsoluteFrameIndex() == u64PendingFrame_ &&
                managementMessage.getRelativeRetransmissionSlotIndex() == u64ReceivalPendingManagementRelativeRetransmissionSlot_ )
            {
              managementReceiveManager_.enqueue(std::move(managementMessage),
                                        pktInfo,
                                        pkt.length(),
                                        startOfReception,
                                        frequencySegments,
                                        span,
                                        now,
                                        hdr.getSequenceNumber());
            }
            else
            {
              LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                  DEBUG_LEVEL,
                                  "MACI %03hu TDMACT::BaseModel::%s drop reason rx (frame,slot) incorrect",
                                  id_,
                                  __func__);
            }

        }
        else
        {

          LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                  DEBUG_LEVEL,
                                  "MACI %03hu TDMACT::BaseModel::%s drop reason rx slot correct"
                                  " rframe: %lu  but frequency mismatch expected: %zu got: %zu",
                                  id_,
                                  __func__,
                                  nowAbsoluteFrameIndex,
                                  u64Frequency_Hz_,
                                  frequencySegment.getFrequencyHz());

          // drop
          return;
        }
      

    }
    else
    {
      Microseconds timeRemainingInSlot{frameDuration +
                                       std::chrono::duration_cast<Microseconds>(now -
                                                                                absoluteFrameIndexToAbsoluteTime(nowAbsoluteFrameIndex))};

      // were we supposed to be in rx on the pkt abs slot
      if (frameType(nowAbsoluteFrameIndex) == RX)
      {

        LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                DEBUG_LEVEL,
                                "MACI %03hu TDMACT::BaseModel::%s drop reason slot mismatch pkt: %zu now: %zu ",
                                id_,
                                __func__,
                                managementMessage.getAbsoluteFrameIndex(),
                                nowAbsoluteFrameIndex);

        // drop
        return;
      }
      else
      {
        LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                DEBUG_LEVEL,
                                "MACI %03hu TDMACT::BaseModel::%s drop reason slot mismatch but %s pkt: %zu now: %zu ",
                                id_,
                                __func__,
                                "in tx", //
                                managementMessage.getAbsoluteFrameIndex(),
                                nowAbsoluteFrameIndex);

        // drop
        return;
      }
    }
  }
  else
  {
    LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                            ERROR_LEVEL,
                            "MACI %03hu TDMACT::BaseModel::%s Packet payload length %zu does not match length prefix %zu",
                            id_,
                            __func__,
                            pkt.length(),
                            len);
  }
}


void EMANE::Models::TDMACT::BaseModel::Implementation::processUpstreamPacket(const CommonMACHeader &hdr,
                                                                             UpstreamPacket &pkt,
                                                                             const ControlMessages &msgs)
{
  const PacketInfo &pktInfo{pkt.getPacketInfo()};

  // if header is not TDMACT, drop
  if (hdr.getRegistrationId() != REGISTERED_EMANE_MAC_TDMACT && hdr.getRegistrationId() != REGISTERED_EMANE_MAC_TDMACT_MANAGEMENT)
  {
    LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                            ERROR_LEVEL,
                            "MACI %03hu TDMACT::BaseModel::%s: MAC Registration Id %hu does not match our Id %hu, drop.",
                            id_,
                            __func__,
                            hdr.getRegistrationId(),
                            REGISTERED_EMANE_MAC_TDMACT);

    packetStatusPublisher_.inbound(pktInfo.getSource(),
                                   pktInfo.getSource(),
                                   pktInfo.getPriority(),
                                   pkt.length(),
                                   PacketStatusPublisher::InboundAction::DROP_REGISTRATION_ID);

    // drop
    return;
  }

  if (hdr.getRegistrationId() == REGISTERED_EMANE_MAC_TDMACT_MANAGEMENT)
  {
    processUpstreamPacketManagement(hdr, pkt, msgs);
  }
  else if (hdr.getRegistrationId() == REGISTERED_EMANE_MAC_TDMACT)
  {
    processUpstreamPacketBaseMessage(hdr, pkt, msgs);
  }
}


/*******************************************************************************/
/********************* start of scheduler/slotter ******************************/
/*******************************************************************************/

std::uint64_t EMANE::Models::TDMACT::BaseModel::Implementation::relativeTimeToRelativeRetransmissionSlotIndex(EMANE::Microseconds microseconds)
{
  // everything uses integer division
  std::uint64_t relativeRetransmissionSlotIndex{microseconds.count() / getRetransmissionSlotDuration_us()};
  return relativeRetransmissionSlotIndex;
}


EMANE::Microseconds EMANE::Models::TDMACT::BaseModel::Implementation::relativeRetransmissionSlotIndexToRelativeTime(std::uint64_t relativeRetransmissionSlotIndex)
{
  EMANE::Microseconds microseconds{getRetransmissionSlotDuration_us() * relativeRetransmissionSlotIndex};
  return microseconds;
}

std::uint64_t EMANE::Models::TDMACT::BaseModel::Implementation::relativeTimeToRelativeManagementRetransmissionSlotIndex(EMANE::Microseconds microseconds)
{
  // everything uses integer division
  std::uint64_t relativeManagementRetransmissionSlotIndex{(microseconds.count() - getMessagesPartDuration_us()) / getManagementRetransmissionSlotDuration_us()};
  return relativeManagementRetransmissionSlotIndex;
}


EMANE::Microseconds EMANE::Models::TDMACT::BaseModel::Implementation::relativeManagementRetransmissionSlotIndexToRelativeTime(std::uint64_t relativeManagementRetransmissionSlotIndex)
{
  EMANE::Microseconds microseconds{getMessagesPartDuration_us() + getManagementRetransmissionSlotDuration_us() * relativeManagementRetransmissionSlotIndex};
  return microseconds;
}


std::uint64_t EMANE::Models::TDMACT::BaseModel::Implementation::getRetransmissionSlotDuration_us()
{
  // rounded retransmission slot duration
  std::uint64_t slotDuration_us = getMessagesPartDuration_us() / u16RetransmissionSlots_;
  rounder_.round_microSeconds(slotDuration_us);
  return slotDuration_us;
}

std::uint64_t EMANE::Models::TDMACT::BaseModel::Implementation::getEffectiveDataRateBps()
{
 //                      round up                     simulated retransmission slot time                                                 effective retransmission slot time (included rounding effect)   
  return (std::uint64_t) std::ceil((u64DataRate_bps_)*( (((double) u64FrameDuration_-u64ManagementDuration_) / u16RetransmissionSlots_) / getRetransmissionSlotDuration_us()));
}

std::uint64_t EMANE::Models::TDMACT::BaseModel::Implementation::absoluteTimeToAbsoluteFrameIndex(TimePoint timePoint)
{
  std::uint64_t count = std::chrono::duration_cast<Microseconds>(timePoint.time_since_epoch()).count();
  return count / u64FrameDuration_;
}

EMANE::TimePoint EMANE::Models::TDMACT::BaseModel::Implementation::absoluteFrameIndexToAbsoluteTime(std::uint64_t absoluteFrameIndex)
{
  TimePoint tp = EMANE::TimePoint{EMANE::Microseconds{absoluteFrameIndex * u64FrameDuration_}};
  rounder_.round(tp);
  return tp;
}

EMANE::Models::TDMACT::FrameType EMANE::Models::TDMACT::BaseModel::Implementation::frameType(std::uint64_t absoluteFrameIndex)
{
  if (pendingSchedule_[absoluteFrameIndex].transmitter_ != id_)
    return RX;
  else
    return TX;
}

std::uint64_t EMANE::Models::TDMACT::BaseModel::Implementation::getManagementRetransmissionSlotDuration_us()
{
  std::uint64_t managementSlotDuration_us = u64ManagementEffectiveDuration_ / u64ManagementRetransmissionSlots_;
  rounder_.round_microSeconds(managementSlotDuration_us);
  return managementSlotDuration_us;
}

std::uint64_t EMANE::Models::TDMACT::BaseModel::Implementation::getMessagesPartDuration_us()
{
  // unrounded!
  return u64FrameDuration_ - u64ManagementEffectiveDuration_;
}


std::uint64_t EMANE::Models::TDMACT::BaseModel::Implementation::getBytesInRetransmissionSlot()
{
  return (std::uint64_t)((u64DataRate_bps_/(8.0*1e6))*((u64FrameDuration_ - u64ManagementDuration_)/u16RetransmissionSlots_));
}

/************ end of scheduler/slotter **************/