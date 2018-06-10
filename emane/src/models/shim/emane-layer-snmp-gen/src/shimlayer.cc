/*
 * Copyright (c) 2015 - Adjacent Link LLC, Bridgewater, New Jersey
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
 * * Neither the name of Adjacent Link, LLC nor the names of its
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

#include "emane/controls/serializedcontrolmessage.h"
#include "emane/controls/r2rineighbormetriccontrolmessage.h"
#include "emane/controls/r2riqueuemetriccontrolmessage.h"
#include "emane/controls/r2riselfmetriccontrolmessage.h"
#include "emane/controls/flowcontrolcontrolmessage.h"

#include "shimlayer.h"

namespace
{
  const char * __MODULE__ = "SNMP::ShimLayer";

  EMANE::ControlMessages clone(const EMANE::ControlMessages & msgs)
   {
     EMANE::ControlMessages clones;

     for(const auto & msg : msgs)
       {
         clones.push_back(msg->clone()); 
       }

     return clones;
   }
}


EMANE::R2RI::ShimLayer::ShimLayer(NEMId id,
                                        PlatformServiceProvider * pPlatformService,
                                        RadioServiceProvider * pRadioService) :
  ShimLayerImplementor{id, pPlatformService, pRadioService},
  snmpModemService_{id, pPlatformService, pRadioService}
{ }



EMANE::R2RI::ShimLayer::~ShimLayer() 
{ }



void EMANE::R2RI::ShimLayer::initialize(Registrar & registrar) 
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "SHIMI %03hu %s::%s", id_, __MODULE__, __func__);

  try
   {
    auto & configRegistrar = registrar.configurationRegistrar();

    // sinr min/max
    configRegistrar.registerNumeric<float>("sinrmin",
                                           ConfigurationProperties::DEFAULT,
                                           {0.0},
                                           "min sinr for RLQ calculations");

    configRegistrar.registerNumeric<float>("sinrmax",
                                           ConfigurationProperties::DEFAULT,
                                           {20.0f},
                                           "max sinr for RLQ calculations");

    // nem to mac address mapping
    configRegistrar.registerNonNumeric<std::string>("macaddress",
                                                    ConfigurationProperties::NONE,
                                                    {},
                                                    "Defines a list of NEMId(s) and associated ethernet MAC address"
                                                    "NEMId EtherMacAddress", 
                                                    0,
                                                    255,
                                                    "^\\d+ ([0-9A-Fa-f]{2}[:]){5}([0-9A-Fa-f]{2})$");

    configRegistrar.registerNonNumeric<std::string>("etheraddroui",
                                                    ConfigurationProperties::NONE,
                                                    {},
                                                    "Defines an ethernet MAC address preix to be used along with the NEMId"
                                                    "to construct the full ethernet MAC address",
                                                    0,
                                                    255,
                                                    "^([0-9A-Fa-f]{2}[:]){2}([0-9A-Fa-f]{2})$");

    configRegistrar.registerNonNumeric<INETAddr>("addressRedis",
                                             ConfigurationProperties::NONE,
                                             {},
                                             "IPv4 or IPv6 virutal device address.");

    //const EMANE::ModemService::ConfigParameterMapType & snmpConfiguration{snmpModemService_.getConfigItems()};

    // snmp config items we will be queried for by lib snmp after initialization
    // for(auto & item : snmpConfiguration)
    //   {
    //     auto value = item.second.value;

    //     // the rfid is our NEM id, so NEM #1 is(00,01)
    //     if(item.first == "destinationadvertrfid")
    //       {
    //         value = std::to_string(std::uint8_t((id_ >> 8) & 0xFF)) + "," + std::to_string(std::uint8_t(id_ & 0xFF));
    //       }

    //      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
    //                              DEBUG_LEVEL,
    //                              "SHIMI %03hu %s::%s item %s, default value %s, description %s", 
    //                              id_, __MODULE__, __func__,
    //                              item.first.c_str(), 
    //                              value.c_str(), 
    //                              item.second.description.c_str());

    //      configRegistrar.registerNonNumeric<std::string>(item.first,
    //                                                      ConfigurationProperties::DEFAULT,
    //                                                      {{value}},
    //                                                      item.second.description);
    //   }  
   }
  catch (const EMANE::Exception & ex)
   {
        LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                 ERROR_LEVEL,
                                 "SHIMI %03hu %s::%s caught %s", 
                                 id_, __MODULE__, __func__,
                                 ex.what());
   }
}



void EMANE::R2RI::ShimLayer::configure(const ConfigurationUpdate & update)
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "SHIMI %03hu %s::%s", id_, __MODULE__, __func__);

  for(const auto & item : update)
  {
    if(item.first == "addressRedis")
      {
        addressRedis_ = item.second[0].asINETAddr();

        LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                INFO_LEVEL,
                                "SHIMI %03hu %s::%s %s=%s",
                                id_,
                                __MODULE__,
                                __func__,
                                item.first.c_str(),
                                addressRedis_.str(false).c_str());
      }
     else {

      }
  }

  snmpModemService_.configure(update);
}



void EMANE::R2RI::ShimLayer::start()
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "SHIMI %03hu %s::%s", id_, __MODULE__, __func__);

  snmpModemService_.start();
}



void EMANE::R2RI::ShimLayer::stop()
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "SHIMI %03hu %s::%s", id_, __MODULE__, __func__);
}



void EMANE::R2RI::ShimLayer::destroy()
  throw()
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "SHIMI %03hu %s::%s", id_, __MODULE__, __func__);
}



void EMANE::R2RI::ShimLayer::processUpstreamControl(const ControlMessages & msgs)
{
  EMANE::ControlMessages clones;

  EMANE::ControlMessages controlMessages;

  for(const auto & pMessage : msgs)
    {
      switch(pMessage->getId())
        {
          case EMANE::Controls::R2RISelfMetricControlMessage::IDENTIFIER:
            {
              // self info to the front, consume
              controlMessages.push_front(pMessage);

              LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                      DEBUG_LEVEL, 
                                      "SHIMI %03hu %s::%s consume R2RISelfMetricControlMessage", 
                                      id_, __MODULE__, __func__);
            }
          break;

          case EMANE::Controls::R2RINeighborMetricControlMessage::IDENTIFIER:
            {
              LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                      DEBUG_LEVEL, 
                                      "SHIMI %03hu %s::%s consume R2RINeighborMetricControlMessage", 
                                      id_, __MODULE__, __func__);

              // to the back, consume
              controlMessages.push_back(pMessage);
            }
          break;

          case EMANE::Controls::R2RIQueueMetricControlMessage::IDENTIFIER:
            {
              LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                      DEBUG_LEVEL, 
                                      "SHIMI %03hu %s::%s consume R2RIQueueMetricControlMessage", 
                                      id_, __MODULE__, __func__);

              // to the back, consume
              controlMessages.push_back(pMessage);
            }
          break;

          case EMANE::Controls::FlowControlControlMessage::IDENTIFIER:
            {
              LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                      DEBUG_LEVEL, 
                                      "SHIMI %03hu %s::%s forward and consume FlowControlControlMessage", 
                                      id_, __MODULE__, __func__);

              // transport is expecting these too
              clones.push_back(pMessage->clone()); 

              // to the back
              controlMessages.push_back(pMessage);
            }
          break;

          case EMANE::Controls::SerializedControlMessage::IDENTIFIER:
            {
              const auto pSerializedControlMessage =
                static_cast<const EMANE::Controls::SerializedControlMessage *>(pMessage); 
        
              switch(pSerializedControlMessage->getSerializedId())
                {
                  case EMANE::Controls::R2RISelfMetricControlMessage::IDENTIFIER:
                    {
                       LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                               DEBUG_LEVEL, 
                                               "SHIMI %03hu %s::%s consume R2RISelfMetricControlMessage", 
                                               id_, __MODULE__, __func__);

                      // self info to the front, consume
                      controlMessages.push_front(pMessage);
                    }
                  break;

                  case EMANE::Controls::R2RINeighborMetricControlMessage::IDENTIFIER:
                    {
                       LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                               DEBUG_LEVEL, 
                                               "SHIMI %03hu %s::%s consume R2RINeighborMetricControlMessage", 
                                               id_, __MODULE__, __func__);

                      // to the back, consume
                      controlMessages.push_back(pMessage);
                    }
                  break;

                  case EMANE::Controls::R2RIQueueMetricControlMessage::IDENTIFIER:
                    {
                       LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                               DEBUG_LEVEL, 
                                               "SHIMI %03hu %s::%s consume R2RIQueueMetricControlMessage", 
                                               id_, __MODULE__, __func__);

                      // to the back, consume
                      controlMessages.push_back(pMessage);
                    }
                  break;

                  case EMANE::Controls::FlowControlControlMessage::IDENTIFIER:
                    {
                      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                               DEBUG_LEVEL, 
                                               "SHIMI %03hu %s::%s forward and consume FlowControlControlMessage", 
                                               id_, __MODULE__, __func__);

                      // transport is expecting these too
                      clones.push_back(pMessage->clone()); 

                      // to the back
                      controlMessages.push_back(pMessage);
                    }
                  break;

                  default:
                    clones.push_back(pMessage->clone()); 

                    LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                           DEBUG_LEVEL, 
                                           "SHIMI %03hu %s::%s pass thru unknown msg id %hu", 
                                           id_, __MODULE__, __func__, pMessage->getId());
                }
            }
          break;

          default:
               clones.push_back(pMessage->clone()); 

               LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                      DEBUG_LEVEL, 
                                      "SHIMI %03hu %s::%s pass thru unknown msg id %hu", 
                                      id_, __MODULE__, __func__, pMessage->getId());
      }
   }

  if(! clones.empty())
   {
      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                               DEBUG_LEVEL,
                               "SHIMI %03hu %s::%s pass thru %zu msgs", 
                               id_, __MODULE__, __func__, clones.size());

     // pass thru
     sendUpstreamControl(clones);
   }

   LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                               DEBUG_LEVEL,
                               "SHIMI %03hu %s::%s taksham log here346", 
                               id_, __MODULE__, __func__);

  // now handle control messages in expected order (self first, then nbr/queue/flowctrl)
  snmpModemService_.handleControlMessages(controlMessages);

  sendUpstreamControl(clone(msgs)); // taksham - we need to upstream the msf to the next shim layer!
}



void EMANE::R2RI::ShimLayer::processDownstreamControl(const ControlMessages & msgs)
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "SHIMI %03hu %s::%s pass thru %zu msgs", 
                          id_, __MODULE__, __func__, msgs.size());

  // pass thru
  sendDownstreamControl(clone(msgs));
}



void EMANE::R2RI::ShimLayer::processUpstreamPacket(UpstreamPacket & pkt,
                                                         const ControlMessages & msgs)
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "SHIMI %03hu %s::%s pass thru pkt len %zu and %zu msgs", 
                          id_, __MODULE__, __func__, pkt.length(), msgs.size());

  // pass thru
  sendUpstreamPacket(pkt, clone(msgs));
}



void EMANE::R2RI::ShimLayer::processDownstreamPacket(DownstreamPacket & pkt,
                                                           const ControlMessages & msgs)
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "SHIMI %03hu %s::%s pass thru pkt len %zu and %zu msgs", 
                          id_, __MODULE__, __func__, pkt.length(), msgs.size());

  // pass thru
  sendDownstreamPacket(pkt, clone(msgs));
}


DECLARE_SHIM_LAYER(EMANE::R2RI::ShimLayer);
