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

#include "snmpmodemservice.h"

#include <cstdint>
#include <set>
#include <string>
#include <sstream>
#include <vector>

#include "hiredisinclude/hiredis.h"

#include "emane/configureexception.h"
#include "emane/utils/parameterconvert.h"
#include "emane/controls/serializedcontrolmessage.h"
#include "emane/controls/r2rineighbormetriccontrolmessageformatter.h"
#include "emane/controls/r2riqueuemetriccontrolmessageformatter.h"
#include "emane/controls/r2riselfmetriccontrolmessageformatter.h"

namespace
{
  const char * __MODULE__ = "SNMP::ModemService";

  const char * EtherAddrFormat = "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx";

  const char * EtherOUIFormat  = "%02hhx:%02hhx:%02hhx";

  template <typename T> 
  T clampit(T min, T max, T val)
   {
      if(val < min)
        {
          return min;
        }
      else if(val > max)
        {
          return max;
        }
      else
        {
          return val;
        }
   }


  // const EMANE::ConfigParameterMapType DefaultDlepConfiguration =
  //   {
  //     // id (no special chars)      alias (special chars ok)   type  value          description
  //     { "acktimeout",              {"ack-timeout",             "i",  "3",           "seconds to wait for ack signals" } },
  //     { "ackprobability",          {"ack-probability",         "i",  "100",         "ack probabilty percent 0-100" } },
  //     { "configfile",              {"config-file",             "f",  "",            "xml config file containing parameter settings" } },
  //     { "heartbeatinterval",       {"heartbeat-interval",      "i",  "10",          "time between sending heartbeat signals" } },
  //     { "heartbeatthreshold",      {"heartbeat-threshold",     "i",  "2",           "number of missed heartbeats to tolerate" } },
  //     { "localtype",               {"local-type",              "s",  "modem",       "which snmp role to play, modem or router?" } },
  //     { "loglevel",                {"log-level",               "i",  "2",           "1=most logging, 5=least" } },
  //     { "logfile",                 {"log-file",                "s",  "snmp.log",    "file to write log messages to" } },
  //     { "peertype",                {"peer-type",               "s",  "emane",       "peer type data item value" } },
  //     { "sendtries",               {"send-tries",              "i",  "3",           "number of times to send a signal before giving up" } },
  //     { "sessionport",             {"session-port",            "i",  "4854",        "tcp port number session connections" } },
  //     { "extensions",              {"extensions",              "iv", "",            "list of extension ids to support" } },

  //     { "protocolconfigfile",      {"protocol-config-file",    "s",  "protocol-config.xml", "xml file containing snmp protocol config." } },
  //     { "protocolconfigschema",    {"protocol-config-schema",  "s",  "protocol-config.xsd", "xml protocol config schema." } },

  //     { "discoveryiface",          {"discovery-iface",         "s",  "eth0",        "interface for the peerdiscovery protocol" } },
  //     { "discoveryinterval",       {"discovery-interval",      "i",  "10",          "time between sending peerdiscovery signals" } },
  //     { "discoverymcastaddress",   {"discovery-mcast-address", "a",  "225.0.0.117", "address to send peerdiscovery signals to" } },
  //     { "discoveryport",           {"discovery-port",          "i",  "4854",        "port to send peerdiscovery signals to" } },
  //     { "discoveryenable",         {"discovery-enable",        "b",  "1",           "should the router run the peerdiscovery protocol?" } },

  //     { "destinationadvertenable",       {"destination-advert-enable",        "b",   "1",           "dest advert enable/disable" } },
  //     { "destinationadvertiface",        {"destination-advert-iface",         "s",   "emane0",      "dest advert interface" } },
  //     { "destinationadvertsendinterval", {"destination-advert-send-interval", "i",   "10",          "dest advert tx interval" } },
  //     { "destinationadvertmcastaddress", {"destination-advert-mcast-address", "a",   "225.0.0.118", "dest advert multicast address" } },
  //     { "destinationadvertport",         {"destination-advert-port",          "i",   "5854",        "dest advert port" } },
  //     { "destinationadvertholdinterval", {"destination-advert-hold-interval", "i",   "0",           "dest advert hold value, 0 = hold" } },
  //     { "destinationadvertexpirecount",  {"destination-advert-expire-count",  "i",   "0",           "dest advert expire count, 0 = hold" } },
  //     { "destinationadvertrfid",         {"destination-advert-rf-id",         "iv",  "",            "dest advert mac id (NEMId)" } },
  //  };

}


EMANE::ModemService::ModemService(NEMId id,
                                              PlatformServiceProvider * pPlatformService,
                                              RadioServiceProvider * pRadioService) :
  id_{id},
  pPlatformService_{pPlatformService},
  pRadioService_{pRadioService},
  //pDlepClient_{},
  avgQueueDelayMicroseconds_{},
  fSINRMin_{0.0f},
  fSINRMax_{20.0f},
  destinationAdvertisementEnable_{false}
{
  memset(&etherOUI_, 0x0, sizeof(etherOUI_)); 

  memset(&etherBroadcastAddr_, 0xFF, sizeof(etherBroadcastAddr_));

  // set default for emane-snmp-demo log file dir
  const std::string logfile = "persist/" + std::to_string(id) + "/radio/var/log/snmp-modem.log";

  //snmpConfiguration_ = DefaultDlepConfiguration;

  //snmpConfiguration_["logfile"].value = logfile;
}



EMANE::ModemService::~ModemService() 
{ }



 //const EMANE::ConfigParameterMapType & EMANE::ModemService::getConfigItems() const
 //{
 //  return snmpConfiguration_;
 //}



void EMANE::ModemService::configure(const ConfigurationUpdate & update)
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "SHIMI %03hu %s::%s", id_, __MODULE__, __func__);

  for(const auto & item : update)
    {
      if(item.first == "macaddress")
        {
          for(const auto & any : item.second)
            {
              const std::string str{any.asString()};

              const size_t pos{str.find(" ")};
 
              if(pos != std::string::npos)
                {
                  const std::uint16_t nem{EMANE::Utils::ParameterConvert(std::string{str.data(), pos}).toUINT16()};

                  EMANE::Utils::EtherAddr eth;

                  if(sscanf(str.data() + pos + 1, 
                            EtherAddrFormat, 
                            eth.bytes.buff + 0,
                            eth.bytes.buff + 1,
                            eth.bytes.buff + 2,
                            eth.bytes.buff + 3,
                            eth.bytes.buff + 4,
                            eth.bytes.buff + 5) != 6)
                    {
                      throw makeException<ConfigureException>(__MODULE__,
                                                             "Invalid configuration item %s, expected <NEMId> <%s>",
                                                              item.first.c_str(),
                                                              EtherAddrFormat);
                    }

                  if(nemEtherAddrMap_.insert(std::make_pair(nem, eth)).second)
                    {
                       LOGGER_STANDARD_LOGGING(pPlatformService_->logService(), 
                                               INFO_LEVEL,
                                               "SHIMI %03hu %s::%s %s = %s",
                                               id_, 
                                               __MODULE__, 
                                               __func__, 
                                               item.first.c_str(),
                                               str.c_str());

                    }
                  else
                    {
                       throw makeException<ConfigureException>(__MODULE__,
                                                              "Invalid configuration item %s, duplicate NEMId %hu",
                                                               item.first.c_str(), nem);
                    }
                }
              else
                {
                   throw makeException<ConfigureException>(__MODULE__,
                                                          "Invalid configuration item %s, expected <NEMId> <%s>",
                                                           item.first.c_str(),
                                                           EtherAddrFormat);
                }
            }
         }
       else if(item.first == "etheraddroui")
         {
            const std::string str{item.second[0].asString()};

            if(sscanf(str.data(), 
                      EtherOUIFormat,
                      etherOUI_.bytes.buff + 0,
                      etherOUI_.bytes.buff + 1,
                      etherOUI_.bytes.buff + 2) != 3)
              {
                 throw makeException<ConfigureException>(__MODULE__,
                                                        "Invalid configuration item %s, expected prefix format <%s>",
                                                         item.first.c_str(),
                                                         EtherOUIFormat);
              }
            else
              {
                 LOGGER_STANDARD_LOGGING(pPlatformService_->logService(), 
                                         INFO_LEVEL,
                                         "SHIMI %03hu %s::%s %s = %s",
                                         id_, 
                                         __MODULE__, 
                                         __func__, 
                                         item.first.c_str(),
                                         str.c_str());
              }
         }
       else if(item.first == "sinrmin")
         {
            fSINRMin_ = item.second[0].asFloat();

            LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                    INFO_LEVEL,
                                    "SHIMI %03d %s::%s %s = %f dBm",
                                     id_,
                                     __MODULE__,
                                     __func__,
                                     item.first.c_str(),
                                     fSINRMin_);
         }
       else if(item.first == "sinrmax")
         {
            fSINRMax_ = item.second[0].asFloat();

            LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                    INFO_LEVEL,
                                    "SHIMI %03d %s::%s %s = %f dBm",
                                    id_,
                                    __MODULE__,
                                    __func__,
                                    item.first.c_str(),
                                    fSINRMax_);
         }
       else if(item.first == "addressRedis")
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
       else
         {
          //  auto iter = snmpConfiguration_.find(item.first);

          //  if(iter != snmpConfiguration_.end())
             // {
                const std::string str{item.second[0].asString()};

                // if(item.first == "destinationadvertenable")
                //   {
                //      destinationAdvertisementEnable_ = iter->second.value != "0";
                //   }

                 LOGGER_STANDARD_LOGGING(pPlatformService_->logService(), 
                                         INFO_LEVEL,
                                         "SHIMI %03hu %s::%s %s = %s",
                                         id_, 
                                         __MODULE__, 
                                         __func__, 
                                         item.first.c_str(),
                                         str.c_str());
             // }
            /*else
              { 
                throw makeException<ConfigureException>(__MODULE__,
                                                        "Unexpected configuration item %s",
                                                        item.first.c_str());
              }*/
          }
      } 
}


void EMANE::ModemService::start()
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "SHIMI %03hu %s::%s", id_, __MODULE__, __func__);

  // sinr saniity check
  if(fSINRMin_ > fSINRMax_)
    {
      throw makeException<ConfigureException>(__MODULE__,
                                              "sinrmin > ", 
                                              std::to_string(fSINRMin_).c_str(),
                                              " sinrmax", 
                                              std::to_string(fSINRMax_).c_str());
    }
  else
    {
      // configuration complete, create client impl
      //pDlepClient_.reset(new DlepClientImpl{id_, pPlatformService_, snmpConfiguration_});
    }
}



void EMANE::ModemService::handleControlMessages(const EMANE::ControlMessages & controlMessages)
{
  try {
    for(const auto & pMessage : controlMessages)
      {
        switch(pMessage->getId())
          {
            case EMANE::Controls::R2RINeighborMetricControlMessage::IDENTIFIER:
              {
                handleMetricMessage_i(reinterpret_cast<const EMANE::Controls::R2RINeighborMetricControlMessage *>(pMessage));
              }
            break;

            case EMANE::Controls::R2RIQueueMetricControlMessage::IDENTIFIER:
              {
                handleMetricMessage_i(reinterpret_cast<const EMANE::Controls::R2RIQueueMetricControlMessage *>(pMessage));
              }
            break;

            case EMANE::Controls::R2RISelfMetricControlMessage::IDENTIFIER:
              {
                handleMetricMessage_i(reinterpret_cast<const EMANE::Controls::R2RISelfMetricControlMessage *>(pMessage));
              }
            break;

            case EMANE::Controls::FlowControlControlMessage::IDENTIFIER:
              {
                handleFlowControlMessage_i();
              }
            break;

            case EMANE::Controls::SerializedControlMessage::IDENTIFIER:
              {
                const auto pSerializedControlMessage =
                  static_cast<const EMANE::Controls::SerializedControlMessage *>(pMessage); 
        
                switch(pSerializedControlMessage->getSerializedId())
                  {
                    case EMANE::Controls::R2RINeighborMetricControlMessage::IDENTIFIER:
                      {
                        auto p = EMANE::Controls::R2RINeighborMetricControlMessage::create(
                                              pSerializedControlMessage->getSerialization());

                        handleMetricMessage_i(p);

                        delete p;
                      }
                    break;

                    case EMANE::Controls::R2RIQueueMetricControlMessage::IDENTIFIER:
                      {
                        auto p = EMANE::Controls::R2RIQueueMetricControlMessage::create(
                                              pSerializedControlMessage->getSerialization());

                        handleMetricMessage_i(p);

                        delete p;
                      }
                    break;

                    case EMANE::Controls::R2RISelfMetricControlMessage::IDENTIFIER:
                      {
                        auto p = EMANE::Controls::R2RISelfMetricControlMessage::create(
                                              pSerializedControlMessage->getSerialization());

                        handleMetricMessage_i(p);

                        delete p;
                      }
                    break;

                    case EMANE::Controls::FlowControlControlMessage::IDENTIFIER:
                      {
                        handleFlowControlMessage_i();
                      }
                    break;
                  }
              }
            break;
         }
      }
   }
  // catch(const LLSNMP::ProtocolConfig::BadDataItemName & ex)
  //  {
  //     LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
  //                             ERROR_LEVEL,
  //                             "SHIMI %03hu %s::%s caught bad data item name exception %s", 
  //                             id_, 
  //                             __MODULE__, 
  //                             __func__, 
  //                             ex.what());
  //   }

  // catch(const LLSNMP::ProtocolConfig::BadDataItemId & ex)
  //  {
  //     LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
  //                             ERROR_LEVEL,
  //                             "SHIMI %03hu %s::%s caught bad data item id exception %s", 
  //                             id_, 
  //                             __MODULE__, 
  //                             __func__, 
  //                             ex.what());
  //   }


  catch(const std::exception & ex)
   {
      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              ERROR_LEVEL,
                              "SHIMI %03hu %s::%s caught std exception %s", 
                              id_, 
                              __MODULE__, 
                              __func__, 
                              ex.what());
    }
}



// self metrics
void EMANE::ModemService::handleMetricMessage_i(const EMANE::Controls::R2RISelfMetricControlMessage * pMessage)
{
   LOGGER_STANDARD_LOGGING_FN_VARGS(pPlatformService_->logService(),
                                               DEBUG_LEVEL, 
                                               EMANE::Controls::R2RISelfMetricControlMessageFormatter(pMessage),
                                               "SHIMI %03hu %s::%s R2RISelfMetricControlMessage",
                                               id_, __MODULE__, __func__);
   
   // update self metrics
  //  selfMetrics_.valMaxDataRateRx     = pMessage->getMaxDataRatebps();

  //  selfMetrics_.valMaxDataRateTx     = pMessage->getMaxDataRatebps();

  //  selfMetrics_.valCurrentDataRateRx = pMessage->getMaxDataRatebps();

  //  selfMetrics_.valCurrentDataRateTx = pMessage->getMaxDataRatebps();

   // now send a peer update
  //  send_peer_update_i();

  redisContext *c;
  redisReply *reply;
  std::string str_address = addressRedis_.str(false).c_str();
  std::uint32_t myaddress = inet_addr(str_address.c_str());
  const int NBYTES = 4;
  std::uint8_t octet[NBYTES];
  char ipAddressFinal[16];
  for(int i = 0 ; i < NBYTES ; i++)
  {
      octet[i] = myaddress >> (i * 8);
  }
  sprintf(ipAddressFinal, "%d.%d.%d.%d", octet[0], octet[1], octet[2], octet[3]);
  const char *hostname = ipAddressFinal;
  int port = 6379;
  struct timeval timeout = { 1, 500000 }; // 1.5 seconds
  c = redisConnectWithTimeout(hostname, port, timeout);
  // didnt connected to redis
  if (c == NULL || c->err) 
  {
      if (c) {
          LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                            ERROR_LEVEL,
                            "SHIMI %03hu %s::%s Connection error: %s", 
                            id_, __MODULE__, __func__, c->errstr);
          redisFree(c);
      } else {
          LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                            ERROR_LEVEL,
                            "SHIMI %03hu %s::%s Connection error: can't allocate redis context", 
                            id_, __MODULE__, __func__);
      }

  }
  // connected to redis successfully
  else{
    // SET datarate
    std::string mibdr = ".1.3.6.1.4.1.16215.1.24.1.4.6";
    std::string strkeydr = std::to_string(id_).c_str() + mibdr;
    const char *mackey = strkeydr.c_str();
    const char *val = (std::to_string(pMessage->getMaxDataRatebps())).c_str();    
    reply = static_cast<redisReply*>(redisCommand(c,"SET %s %s", mackey, val));
    LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                                INFO_LEVEL, 
                                                "SHIMI %03hu %s::%s SET datarate: %s, KEY: %s VAL: %s",
                                                id_, __MODULE__, __func__, reply->str, mackey, val);
    freeReplyObject(reply);
    
    redisFree(c);
  }
  
}



// neighbor metrics
void EMANE::ModemService::handleMetricMessage_i(const EMANE::Controls::R2RINeighborMetricControlMessage * pMessage)
{
  LOGGER_STANDARD_LOGGING_FN_VARGS(pPlatformService_->logService(),
                                    DEBUG_LEVEL, 
                                    EMANE::Controls::R2RINeighborMetricControlMessageFormatter(pMessage),
                                    "SHIMI %03hu %s::%s R2RINeighborMetricControlMessage",
                                    id_, __MODULE__, __func__);
 
  const EMANE::Controls::R2RINeighborMetrics metrics{pMessage->getNeighborMetrics()};
  // for deleting nbrs loop
  const EMANE::Controls::R2RINeighborMetrics delmetrics{pMessage->getNeighborMetrics()};
  // snmp: if there nbrs to delete, first find them. If there are, delete all nbrs, and then immediatly, update.
  // possible set of nbrs to delete
  auto delNeighbors = currentNeighbors_;
  for(const auto & metric : delmetrics)
  {
    const auto nbr = metric.getId();
    auto iter = currentNeighbors_.find(nbr);
    // update nbr -> do not remove it
    if(!(iter == currentNeighbors_.end()))
    {
      // still active, remove from the candidate delete set
      delNeighbors.erase(nbr);
    }
  }
  uint16_t metricDelIndex = 1; // for snmp DELs. delleting nbrs by numerical order, not by the nbr's id order. for snmmp walk flow
  // loop on currentNeighbors_ to earase all current nbrs
  if(!delNeighbors.empty())
  {
    for(const auto & nbr : currentNeighbors_)
    {
      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              INFO_LEVEL,
                              "SHIMI %03hu %s::%s no longer reported, delete nbr %hu", 
                              id_, __MODULE__, __func__, nbr.first);

      redisContext *c;
      redisReply *reply;
      std::string str_address = addressRedis_.str(false).c_str();
      std::uint32_t myaddress = inet_addr(str_address.c_str());
      const int NBYTES = 4;
      std::uint8_t octet[NBYTES];
      char ipAddressFinal[16];
      for(int i = 0 ; i < NBYTES ; i++)
      {
        octet[i] = myaddress >> (i * 8);
      }
      sprintf(ipAddressFinal, "%d.%d.%d.%d", octet[0], octet[1], octet[2], octet[3]);
      const char *hostname = ipAddressFinal;
      int port = 6379;
      struct timeval timeout = { 1, 500000 }; // 1.5 seconds
      c = redisConnectWithTimeout(hostname, port, timeout);
      // didnt connected to redis
      if (c == NULL || c->err) 
      {
          if (c) {
              LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                ERROR_LEVEL,
                                "SHIMI %03hu %s::%s Connection error: %s", 
                                id_, __MODULE__, __func__, c->errstr);
              redisFree(c);
          } else {
              LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                ERROR_LEVEL,
                                "SHIMI %03hu %s::%s Connection error: can't allocate redis context", 
                                id_, __MODULE__, __func__);
          }

      }
      // connected to redis successfully
      else
      {
        // del the nbr macaddr mib
        // its enough to delete only that one, because passtest counting the nbrs by this mib
        std::string mibmac = ".1.3.6.1.4.1.16215.1.24.1.4.2.1.15.1.1.";
        std::string strkeymac = std::to_string(id_).c_str() + mibmac + std::to_string(metricDelIndex).c_str();
        const char *mackey = strkeymac.c_str(); 
        reply = static_cast<redisReply*>(redisCommand(c,"DEL %s", mackey));
        LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                                    INFO_LEVEL, 
                                                    "SHIMI %03hu %s::%s DEL NBR MACADDR: %hu, KEY: %s ",
                                                    id_, __MODULE__, __func__, reply->integer, mackey);
        freeReplyObject(reply);


        // del the member macaddr mib
        // its enough to delete only that one, because passtest counting the members by this mib
        mibmac = ".1.3.6.1.4.1.16215.1.24.1.4.2.16.1.1.";
        strkeymac = std::to_string(id_).c_str() + mibmac + std::to_string(metricDelIndex).c_str();
        mackey = strkeymac.c_str(); 
        reply = static_cast<redisReply*>(redisCommand(c,"DEL %s", mackey));
        LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                                    INFO_LEVEL, 
                                                    "SHIMI %03hu %s::%s DEL MEMBER MACADDR: %hu, KEY: %s ",
                                                    id_, __MODULE__, __func__, reply->integer, mackey);
        freeReplyObject(reply);
        

        redisFree(c);
      }

      metricDelIndex ++;
    }
    currentNeighbors_.clear();
  }
     
  uint16_t metricSetIndex = 1; // for snmp SETs. setting nbrs by numerical order, not by the nbr's id order. for snmmp walk flow

  for(const auto & metric : metrics)
    {
      bool isNewNbr = false;

      const auto nbr = metric.getId();

      auto rxDataRate = metric.getRxAvgDataRatebps();

      auto txDataRate = metric.getTxAvgDataRatebps();

      auto iter = currentNeighbors_.find(nbr);

      // new nbr
      if(iter == currentNeighbors_.end())
        {
          NeighborInfo nbrInfo;

          // set is a new nbr
          isNewNbr = true;

          // set ether mac addr
          nbrInfo.macAddress_ = getEthernetAddress_i(nbr);

          // set Rx data rate
          // nbrInfo.lastRxDataRate_ = rxDataRate ? rxDataRate : selfMetrics_.valCurrentDataRateRx;

          // set Tx data rate
          // nbrInfo.lastTxDataRate_ = rxDataRate ? rxDataRate : selfMetrics_.valCurrentDataRateRx;

          LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                  DEBUG_LEVEL,
                                  "SHIMI %03hu %s::%s new nbr %hu, ether addr %s, txdr %lu, rxdr %lu", 
                                  id_, 
                                  __MODULE__, 
                                  __func__, 
                                  nbr, 
                                  nbrInfo.macAddress_.to_string().c_str(),
                                  nbrInfo.lastRxDataRate_,
                                  nbrInfo.lastTxDataRate_);

          // add to the current set
          iter = currentNeighbors_.insert(std::make_pair(nbr, nbrInfo)).first;
        } 
      // update nbr
      else
        {
          // still active, remove from the candidate delete set
          delNeighbors.erase(nbr);

          // update Rx data rate
          if(rxDataRate > 0)
            {
              iter->second.lastRxDataRate_ = rxDataRate;
            }

          // update Tx data rate
          if(txDataRate > 0)
            {
              iter->second.lastTxDataRate_ = txDataRate;
            }

          LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                  DEBUG_LEVEL,
                                  "SHIMI %03hu %s::%s update nbr %hu, txdr %lu, rxdr %lu", 
                                  id_, 
                                  __MODULE__, 
                                  __func__, 
                                  nbr, 
                                  iter->second.lastRxDataRate_,
                                  iter->second.lastTxDataRate_);
        }

      // update destination metrics
      // iter->second.metrics_.dataRateInfo.valCurrentDataRateRx = iter->second.lastRxDataRate_;

      // iter->second.metrics_.dataRateInfo.valCurrentDataRateTx = iter->second.lastTxDataRate_;

      // iter->second.metrics_.dataRateInfo.valMaxDataRateRx     = selfMetrics_.valMaxDataRateRx;

      // iter->second.metrics_.dataRateInfo.valMaxDataRateTx     = selfMetrics_.valMaxDataRateTx;

      // iter->second.metrics_.valLatency = avgQueueDelayMicroseconds_;

      // iter->second.metrics_.valResources = 50;

      // iter->second.metrics_.valRLQTx =
      //   iter->second.metrics_.valRLQRx = getRLQ_i(metric.getId(),
      //                                             metric.getSINRAvgdBm(),
      //                                             metric.getNumRxFrames(),
      //                                             metric.getNumMissedFrames());

#if 0
      // push remote lan up to router example
      iter->second.metrics_.valIPv4AdvLan = 
                      LLSNMP::Div_u8_ipv4_u8_t{LLSNMP::DataItem::IPFlags::add, 
                                              boost::asio::ip::address_v4::from_string("0.0.0.0"),
                                            32};
#endif

      // send nbr up/update
      //send_destination_update_i(iter->second, isNewNbr);

  redisContext *c;
  redisReply *reply;
  std::string str_address = addressRedis_.str(false).c_str();
  std::uint32_t myaddress = inet_addr(str_address.c_str());
  const int NBYTES = 4;
  std::uint8_t octet[NBYTES];
  char ipAddressFinal[16];
  for(int i = 0 ; i < NBYTES ; i++)
  {
      octet[i] = myaddress >> (i * 8);
  }
  sprintf(ipAddressFinal, "%d.%d.%d.%d", octet[0], octet[1], octet[2], octet[3]);
  const char *hostname = ipAddressFinal;
  int port = 6379;
  struct timeval timeout = { 1, 500000 }; // 1.5 seconds
  c = redisConnectWithTimeout(hostname, port, timeout);
  // didnt connect to redis
  if (c == NULL || c->err) 
  {
      if (c) {
          LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                            ERROR_LEVEL,
                            "SHIMI %03hu %s::%s Connection error: %s", 
                            id_, __MODULE__, __func__, c->errstr);
          redisFree(c);
      } else {
          LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                            ERROR_LEVEL,
                            "SHIMI %03hu %s::%s Connection error: can't allocate redis context", 
                            id_, __MODULE__, __func__);
      }

  }
  // connected to redis successfully
  else
  {
    // SET NEIGHBOURS MAC ADDR
    // node_id.PLACE.nbr_id
    std::string mibmac = ".1.3.6.1.4.1.16215.1.24.1.4.2.1.15.1.1.";
    std::string strkeymac = std::to_string(id_).c_str() + mibmac + std::to_string(metricSetIndex).c_str();
    const char *mackey = strkeymac.c_str();
    const std::string strvalmac = getEthernetAddress_i(nbr).to_string().c_str();

    // convert mac addr to int64
    // now the MAC is 00:00:00:00:00:# (number of node)
    // to get effective converting, the first 4 bytes value will be changed to "CA:FE"

    std::string strvalmacint64;
    std::string cafeaddr = "CA:FE:00:00:00:00";
    unsigned char real[6];
    unsigned char cafe[6];
    int last = -1;
    int rcreal = sscanf(strvalmac.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx%n",
                    real + 0, real + 1, real + 2, real + 3, real + 4, real + 5, &last);
    int rccafe = sscanf(cafeaddr.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx%n",
                    cafe + 0, cafe + 1, cafe + 2, cafe + 3, cafe + 4, cafe + 5, &last);
    if(rcreal != 6 || strvalmac.size() != last)
    {
      /*LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                                ERROR_LEVEL, 
                                                "SHIMI %03hu %s::%s couldn't conver mac addr %s to int64",
                                                id_, __MODULE__, __func__, strvalmac);*/
      strvalmacint64 = "0";
    }
    else
    {
      uint64_t mac64 = uint64_t(cafe[0]) << 40 |
                      uint64_t(cafe[1]) << 32 |
                      uint64_t(cafe[0]) << 24 |
                      uint64_t(cafe[1]) << 16 |
                      uint64_t(real[4]) << 8 |
                      uint64_t(real[5]);
      strvalmacint64 = std::to_string(mac64);
    }

    const char *macint64val = strvalmacint64.c_str();

    reply = static_cast<redisReply*>(redisCommand(c,"SET %s %s", mackey, macint64val));

    LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                                INFO_LEVEL, 
                                                "SHIMI %03hu %s::%s SET NEIGHBOURS MACADDR: %s, KEY: %s, VAL: %s",
                                                id_, __MODULE__, __func__, reply->str, mackey, macint64val);
    freeReplyObject(reply);


    // SET NEIGHBOURS Link Quality
    float LQ = getRLQ_i(metric.getId(),
                          metric.getSINRAvgdBm(),
                          metric.getNumRxFrames(),
                          metric.getNumMissedFrames());
    std::string mibLQ = ".1.3.6.1.4.1.16215.1.24.1.4.2.1.15.1.2.";
    std::string strkeyLQ = std::to_string(id_).c_str() + mibLQ + std::to_string(metricSetIndex).c_str();
    const char *LQkey = strkeyLQ.c_str();
    std::string strvalLQ = (std::to_string(LQ)).c_str();
    const char *LQval = strvalLQ.c_str();

    reply = static_cast<redisReply*>(redisCommand(c,"SET %s %s", LQkey, LQval));

    LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                                INFO_LEVEL, 
                                                "SHIMI %03hu %s::%s SET NEIGHBOURS Link-Quality: %s, KEY: %s, VAL: %s",
                                                id_, __MODULE__, __func__, reply->str, LQkey, LQval);
    freeReplyObject(reply);


    // SET NEIGHBOURS RSSI (In that implemintation RSSI is SINR)
    std::string mibrssi = ".1.3.6.1.4.1.16215.1.24.1.4.2.1.15.1.3.";
    std::string strkeyrssi = std::to_string(id_).c_str() + mibrssi + std::to_string(metricSetIndex).c_str();
    const char *rssikey = strkeyrssi.c_str();
    std::string strvalrssi = (std::to_string(metric.getSINRAvgdBm())).c_str();
    const char *rssival = strvalrssi.c_str();

    reply = static_cast<redisReply*>(redisCommand(c,"SET %s %s", rssikey, rssival));

    LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                                INFO_LEVEL, 
                                                "SHIMI %03hu %s::%s SET NEIGHBOURS RSSI: %s, KEY: %s, VAL: %s",
                                                id_, __MODULE__, __func__, reply->str, rssikey, rssival);
    freeReplyObject(reply);


    // SET NEIGHBOURS SINR
    std::string mibsinr = ".1.3.6.1.4.1.16215.1.24.1.4.2.1.15.1.4.";
    std::string strkeysinr = std::to_string(id_).c_str() + mibsinr + std::to_string(metricSetIndex).c_str();
    const char *sinrkey = strkeysinr.c_str();
    std::string strvalsinr = (std::to_string(metric.getSINRAvgdBm())).c_str();
    const char *sinrval = strvalsinr.c_str();

    reply = static_cast<redisReply*>(redisCommand(c,"SET %s %s", sinrkey, sinrval));

    LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                                INFO_LEVEL, 
                                                "SHIMI %03hu %s::%s SET NEIGHBOURS SINR: %s, KEY: %s, VAL: %s",
                                                id_, __MODULE__, __func__, reply->str, sinrkey, sinrval);
    freeReplyObject(reply);


    // SET NEIGHBOURS busyRate
    std::string mibbusyrate = ".1.3.6.1.4.1.16215.1.24.1.4.2.1.15.1.5.";
    std::string strkeybusyrate = std::to_string(id_).c_str() + mibbusyrate + std::to_string(metricSetIndex).c_str();
    const char *busyratekey = strkeybusyrate.c_str();
    const char *busyrateval = "50";
    reply = static_cast<redisReply*>(redisCommand(c,"SET %s %s", busyratekey, busyrateval));

    LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                                INFO_LEVEL, 
                                                "SHIMI %03hu %s::%s SET NEIGHBOURS busyRate: %s, KEY: %s, VAL: %s",
                                                id_, __MODULE__, __func__, reply->str, busyratekey, busyrateval);
    freeReplyObject(reply);


    // SET NEIGHBOURS MemberRank
    std::string mibMemberRank = ".1.3.6.1.4.1.16215.1.24.1.4.2.1.15.1.6.";
    std::string strkeyMemberRank = std::to_string(id_).c_str() + mibMemberRank + std::to_string(metricSetIndex).c_str();
    const char *memberRankkey = strkeyMemberRank.c_str();
    const char *memberRankval = "99";
    reply = static_cast<redisReply*>(redisCommand(c,"SET %s %s", memberRankkey, memberRankval));

    LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                                INFO_LEVEL, 
                                                "SHIMI %03hu %s::%s SET NEIGHBOURS MemberRabk: %s, KEY: %s, VAL: %s",
                                                id_, __MODULE__, __func__, reply->str, memberRankkey, memberRankval);
    freeReplyObject(reply);
    
    // SET MEMBERS MAC ADDR
    // node_id.PLACE.nbr_id
    mibmac = ".1.3.6.1.4.1.16215.1.24.1.4.2.16.1.1.";
    strkeymac = std::to_string(id_).c_str() + mibmac + std::to_string(metricSetIndex).c_str();
    mackey = strkeymac.c_str();
    std::string strvalmac2 = getEthernetAddress_i(nbr).to_string().c_str();

    // convert mac addr to int64
    // now the MAC is 00:00:00:00:00:# (number of node)
    // to get effective converting, the first 4 bytes value will be changed to "CA:FE"

    strvalmacint64;
    cafeaddr = "CA:FE:00:00:00:00";
    real[6];
    cafe[6];
    last = -1;
    rcreal = sscanf(strvalmac2.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx%n",
                    real + 0, real + 1, real + 2, real + 3, real + 4, real + 5, &last);
    rccafe = sscanf(cafeaddr.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx%n",
                    cafe + 0, cafe + 1, cafe + 2, cafe + 3, cafe + 4, cafe + 5, &last);
    if(rcreal != 6 || strvalmac2.size() != last)
    {
      /*LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                                ERROR_LEVEL, 
                                                "SHIMI %03hu %s::%s couldn't conver mac addr %s to int64",
                                                id_, __MODULE__, __func__, strvalmac2);*/
      strvalmacint64 = "0";
    }
    else
    {
      uint64_t mac64 = uint64_t(cafe[0]) << 40 |
                      uint64_t(cafe[1]) << 32 |
                      uint64_t(cafe[0]) << 24 |
                      uint64_t(cafe[1]) << 16 |
                      uint64_t(real[4]) << 8 |
                      uint64_t(real[5]);
      strvalmacint64 = std::to_string(mac64);
    }

    macint64val = strvalmacint64.c_str();

    reply = static_cast<redisReply*>(redisCommand(c,"SET %s %s", mackey, macint64val));

    LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                                INFO_LEVEL, 
                                                "SHIMI %03hu %s::%s SET MEMBERS MACADDR: %s, KEY: %s, VAL: %s",
                                                id_, __MODULE__, __func__, reply->str, mackey, macint64val);
    freeReplyObject(reply);


    // SET MEMBERS Link Quality
    LQ = getRLQ_i(metric.getId(),
                          metric.getSINRAvgdBm(),
                          metric.getNumRxFrames(),
                          metric.getNumMissedFrames());
    mibLQ = ".1.3.6.1.4.1.16215.1.24.1.4.2.16.1.2.";
    strkeyLQ = std::to_string(id_).c_str() + mibLQ + std::to_string(metricSetIndex).c_str();
    LQkey = strkeyLQ.c_str();
    strvalLQ = (std::to_string(LQ)).c_str();
    LQval = strvalLQ.c_str();

    reply = static_cast<redisReply*>(redisCommand(c,"SET %s %s", LQkey, LQval));

    LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                                INFO_LEVEL, 
                                                "SHIMI %03hu %s::%s SET MEMBERS Link-Quality: %s, KEY: %s, VAL: %s",
                                                id_, __MODULE__, __func__, reply->str, LQkey, LQval);
    freeReplyObject(reply);


    // SET MEMBERS RSSI (In that implemintation RSSI is SINR)
    mibrssi = ".1.3.6.1.4.1.16215.1.24.1.4.2.16.1.3.";
    strkeyrssi = std::to_string(id_).c_str() + mibrssi + std::to_string(metricSetIndex).c_str();
    rssikey = strkeyrssi.c_str();
    strvalrssi = (std::to_string(metric.getSINRAvgdBm())).c_str();
    rssival = strvalrssi.c_str();

    reply = static_cast<redisReply*>(redisCommand(c,"SET %s %s", rssikey, rssival));

    LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                                INFO_LEVEL, 
                                                "SHIMI %03hu %s::%s SET MEMBERS RSSI: %s, KEY: %s, VAL: %s",
                                                id_, __MODULE__, __func__, reply->str, rssikey, rssival);
    freeReplyObject(reply);


    // SET MEMBERS SINR
    mibsinr = ".1.3.6.1.4.1.16215.1.24.1.4.2.16.1.4.";
    strkeysinr = std::to_string(id_).c_str() + mibsinr + std::to_string(metricSetIndex).c_str();
    sinrkey = strkeysinr.c_str();
    strvalsinr = (std::to_string(metric.getSINRAvgdBm())).c_str();
    sinrval = strvalsinr.c_str();

    reply = static_cast<redisReply*>(redisCommand(c,"SET %s %s", sinrkey, sinrval));

    LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                                INFO_LEVEL, 
                                                "SHIMI %03hu %s::%s SET MEMBERS SINR: %s, KEY: %s, VAL: %s",
                                                id_, __MODULE__, __func__, reply->str, sinrkey, sinrval);
    freeReplyObject(reply);

    reply = static_cast<redisReply*>(redisCommand(c,"DEL mykey"));
    freeReplyObject(reply);

    
    redisFree(c);
    }

    metricSetIndex ++;
    }
}


// queue metrics
void EMANE::ModemService::handleMetricMessage_i(const EMANE::Controls::R2RIQueueMetricControlMessage * pMessage)
{
  LOGGER_STANDARD_LOGGING_FN_VARGS(pPlatformService_->logService(),
                                    DEBUG_LEVEL, 
                                    EMANE::Controls::R2RIQueueMetricControlMessageFormatter(pMessage),
                                    "SHIMI %03hu %s::%s R2RIQueueMetricControlMessage",
                                    id_, __MODULE__, __func__);
  
  // avg queue delay sum
  std::uint64_t delaySum{};

  // bytes in queue sum
  std::uint64_t bytesSum{};

  // packets in queue  sum
  std::uint64_t packetsSum{};

  size_t count{};

  // since there may be multiple Q's, get the overall avg
  for(const auto & metric : pMessage->getQueueMetrics())
    {
      delaySum += metric.getAvgDelay().count(); // in useconds
      packetsSum += metric.getCurrentDepth();
      
      ++count;

      auto depth = metric.getCurrentDepth();
      auto queueid = metric.getQueueId();
      auto capacity = metric.getMaxSize();

      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              INFO_LEVEL,
                              "SHIMI %03hu %s::%s #Q: %u, Max Size: %u, current depth: %u", 
                              id_, __MODULE__, __func__, queueid, capacity, depth);

      if(depth/capacity > 0.8)
        {
          std::stringstream ss;
          ss << "snmptrap -e 0x0102030405 -v 3 -u authOnlyUser -a MD5 -A password -x DES -X mypassword -l authPriv -c public 10.100." << id_ << ".254 42 .1.3.6.1.4.1.8072.2.255.1.0  SNMPv2-MIB::sysLocation.0 s \"Queue #" << queueid <<" Over 80%\"" ;
          std::string str = ss.str();
          const char* command = str.c_str();
          system(command);
        }

    }
  if(count)
    {
      avgQueueDelayMicroseconds_ = delaySum / count;
    }
  else
    {
      avgQueueDelayMicroseconds_ = 0;
    }

    redisContext *c;
    redisReply *reply;
    std::string str_address = addressRedis_.str(false).c_str();
    std::uint32_t myaddress = inet_addr(str_address.c_str());
    const int NBYTES = 4;
    std::uint8_t octet[NBYTES];
    char ipAddressFinal[16];
    for(int i = 0 ; i < NBYTES ; i++)
    {
      octet[i] = myaddress >> (i * 8);
    }
    sprintf(ipAddressFinal, "%d.%d.%d.%d", octet[0], octet[1], octet[2], octet[3]);
    const char *hostname = ipAddressFinal;
    int port = 6379;
    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    c = redisConnectWithTimeout(hostname, port, timeout);
    // didnt connect to redis
    if (c == NULL || c->err) 
    {
        if (c) {
            LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              ERROR_LEVEL,
                              "SHIMI %03hu %s::%s Connection error: %s", 
                              id_, __MODULE__, __func__, c->errstr);
            redisFree(c);
        } else {
            LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                              ERROR_LEVEL,
                              "SHIMI %03hu %s::%s Connection error: can't allocate redis context", 
                              id_, __MODULE__, __func__);
        }

    }
    // connected to redis successfully
    else{
      // SET packets in Qs
      std::string mibpktq = ".1.3.6.1.4.1.16215.1.24.1.4.8";
      std::string strkeypktq = std::to_string(id_).c_str() + mibpktq;
      const char *pktqkey = strkeypktq.c_str();
      const char *strvalpktq = (std::to_string(packetsSum)).c_str();

      reply = static_cast<redisReply*>(redisCommand(c,"SET %s %s", pktqkey, strvalpktq));

      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                                  INFO_LEVEL, 
                                                  "SHIMI %03hu %s::%s SET packets in Queues: %s, KEY: %s, VAL: %s",
                                                  id_, __MODULE__, __func__, reply->str, pktqkey, strvalpktq);
      freeReplyObject(reply);

      redisFree(c);
    }
}


void EMANE::ModemService::handleFlowControlMessage_i()
{
   LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                           DEBUG_LEVEL, 
                           "SHIMI %03hu %s::%s XXX_TODO credits",
                           id_, __MODULE__, __func__);
}

 

LLSNMP::DlepMac EMANE::ModemService::getEthernetAddress_i(const EMANE::NEMId nbr) const
{
  LLSNMP::DlepMac mac;

  if(destinationAdvertisementEnable_)
    {
      mac.mac_addr.push_back((nbr >> 8) & 0xFF);
      mac.mac_addr.push_back(nbr & 0xFF);
    }
  else
    {
      const auto iter = nemEtherAddrMap_.find(nbr);

      EMANE::Utils::EtherAddr etherAddr;

      // check the specific nbr to ether addr mapping first
      if(iter != nemEtherAddrMap_.end())
       {
          etherAddr = iter->second;
       } 
      // otherwise use the OUI prefix (if any)
      else
       {
         etherAddr = etherOUI_;

         etherAddr.words.word3 = ntohs(nbr);
       }

      mac.mac_addr.assign(etherAddr.bytes.buff, etherAddr.bytes.buff + 6); 
   }

  return mac;
}



void EMANE::ModemService::send_peer_update_i()
{
  // if(pDlepClient_)
  //   {
  //     LLSNMP::DataItems metrics{};

  //     load_datarate_metrics_i(metrics, selfMetrics_);

  //     const bool result = pDlepClient_->send_peer_update(metrics);

  //     LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
  //                             DEBUG_LEVEL,
  //                             "SHIMI %03hu %s::%s entries %zu, result %s", 
  //                             id_, __MODULE__, __func__, 
  //                             metrics.size(), 
  //                             result ? "success" : "failed");
  //   }
  // else
  //   {
  //     LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
  //                             ERROR_LEVEL,
  //                             "SHIMI %03hu %s::%s DlepClient not ready", 
  //                             id_, __MODULE__, __func__);
  //   }
}


void EMANE::ModemService::send_destination_update_i(
     const EMANE::ModemService::NeighborInfo & nbrInfo, 
     bool isNewNbr)
{
  // if(pDlepClient_)
  //   {
  //     LLSNMP::DataItems metrics{};

  //     load_destination_metrics_i(metrics, nbrInfo.metrics_);

  //     bool result{};

  //     if(isNewNbr)
  //       {
  //         // dest up
  //         result = pDlepClient_->send_destination_up(nbrInfo.macAddress_, metrics);
  //       }
  //     else
  //       {
  //         // dest update
  //         result = pDlepClient_->send_destination_update(nbrInfo.macAddress_, metrics);
  //       }

  //     LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
  //                             DEBUG_LEVEL,
  //                             "SHIMI %03hu %s::%s %s mac %s, entries %zu, result %s", 
  //                             id_, __MODULE__, __func__, 
  //                             isNewNbr ? "new" : "existing",
  //                             nbrInfo.macAddress_.to_string().c_str(),
  //                             metrics.size(), 
  //                             result ? "success" : "failed");
  //   }
  // else
  //   {
  //     LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
  //                             ERROR_LEVEL,
  //                             "SHIMI %03hu %s::%s DlepClient not ready", 
  //                             id_, __MODULE__, __func__);
  //   }
}



void EMANE::ModemService::send_destination_down_i(const EMANE::ModemService::NeighborInfo & nbrInfo)
{
  // if(pDlepClient_)
  //   {
  //     const bool result = pDlepClient_->send_destination_down(nbrInfo.macAddress_);

  //     LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
  //                             DEBUG_LEVEL,
  //                             "SHIMI %03hu %s::%s mac %s, result %s", 
  //                             id_, __MODULE__, __func__, 
  //                             nbrInfo.macAddress_.to_string().c_str(),
  //                             result ? "success" : "failed");
  //   }
  // else
  //   {
  //     LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
  //                             ERROR_LEVEL,
  //                             "SHIMI %03hu %s::%s DlepClient not ready", 
  //                             id_, __MODULE__, __func__);
  //   }
}



// void EMANE::ModemService::load_datarate_metrics_i(LLSNMP::DataItems & dataItems, const EMANE::DataRateMetricInfo & values)
// {
//    // max data rate Rx
//    dataItems.push_back(getDataItem_i(values.idMaxDataRateRx, values.valMaxDataRateRx));

//    // max data rate Tx
//    dataItems.push_back(getDataItem_i(values.idMaxDataRateTx, values.valMaxDataRateTx));

//    // curr data rate Rx
//    dataItems.push_back(getDataItem_i(values.idCurrentDataRateRx, values.valCurrentDataRateRx));

//    // curr data rate Tx
//    dataItems.push_back(getDataItem_i(values.idCurrentDataRateTx, values.valCurrentDataRateTx));

//    for(auto & item : dataItems)
//     {
//       LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
//                        DEBUG_LEVEL,
//                        "SHIMI %03hu %s::%s item metric [%s]", 
//                        id_, __MODULE__, __func__, 
//                        item.to_string().c_str());
//     }
// }



// void EMANE::ModemService::load_destination_metrics_i(LLSNMP::DataItems & dataItems, const EMANE::DestinationMetricInfo & values)
// {
//    // load data rate values
//    load_datarate_metrics_i(dataItems, values.dataRateInfo);

//    // latency
//    dataItems.push_back(getDataItem_i(values.idLatency, values.valLatency));

//    // resources 
//    dataItems.push_back(getDataItem_i(values.idResources, values.valResources));

//    // rlq Rx
//    dataItems.push_back(getDataItem_i(values.idRLQRx, values.valRLQRx));

//    // rlq Tx
//    dataItems.push_back(getDataItem_i(values.idRLQTx, values.valRLQTx));

// #if 0
//    // adv lan example
//    dataItems.push_back(getDataItem_i(values.idIPv4AdvLan, values.valIPv4AdvLan));
// #endif
   
//    for(auto & item : dataItems)
//     {
//        LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
//                                DEBUG_LEVEL,
//                                "SHIMI %03hu %s::%s item metric [%s]", 
//                                id_, __MODULE__, __func__, 
//                                item.to_string().c_str());
//     }
// }


int EMANE::ModemService::getRLQ_i(const std::uint16_t nbr, 
                                              const float fSINRAvg, 
                                              const size_t numRxFrames, 
                                              const size_t numMissedFrames)
{
  const auto numRxAndMissedFrames = numRxFrames + numMissedFrames;

  float fNewSINR{};

  float fReceiveRatio{};

  int iRLQ{};

  // lets find some history on this nbr 
  // even if this is the first report of this nbr it should 
  // be stored with initial values of (RR = 0, and SINR = -256)
  const auto & iter = currentNeighbors_.find(nbr);

  if(iter != currentNeighbors_.end())
    {
      // we have no pkt info for this interval
      if(numRxAndMissedFrames == 0.0f)
       {
         // reduce sinr by 3db
         fNewSINR = iter->second.fSINRlast_ - 3.0f;

         fReceiveRatio = iter->second.fRRlast_;
       }
      else
       {
         fNewSINR = fSINRAvg;

         fReceiveRatio = (float) numRxFrames / (float) numRxAndMissedFrames;
       }

      // check sinr is above min configured value 
      if(fNewSINR > fSINRMin_)
        {
          const auto fDeltaConfigSINR = fSINRMax_ - fSINRMin_;

          // the min to avg sinr delta 
          const auto fDeltaSINR = fNewSINR - fSINRMin_;

          // calculate rlq
          const auto fValue = 100.0f * (fDeltaSINR / fDeltaConfigSINR) * fReceiveRatio;

          // clamp between 0 and 100
          iRLQ = clampit(0.0f, 100.0f, fValue);
        }

      // save the sinr
      iter->second.fSINRlast_ = fNewSINR;

      // save the rr
      iter->second.fRRlast_ = fReceiveRatio;
   }

  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "SHIMI %03hu %s::%s nbr %hu. sinr %f, receive ratio %f, RLQ %d", 
                          id_, __MODULE__, __func__, 
                          nbr, 
                          fNewSINR, 
                          fReceiveRatio, 
                          iRLQ); 

  return iRLQ;
}