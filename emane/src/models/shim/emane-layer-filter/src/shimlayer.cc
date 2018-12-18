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

#include "emane/downstreampacket.h"
#include "emane/downstreamtransport.h"
#include "emane/configureexception.h"
#include "emane/startexception.h"

#include <sstream>


#include "shimlayer.h"

namespace
{
  const char * __MODULE__ = "filter::ShimLayer";

  const std::string DEVICE_PREFIX = "";

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


EMANE::Transports::filter::ShimLayer::ShimLayer(NEMId id,
                                        PlatformServiceProvider * pPlatformService,
                                        RadioServiceProvider * pRadioService) :
  ShimLayerImplementor{id, pPlatformService, pRadioService},
  filterModemService_{id, pPlatformService, pRadioService}
{ }



EMANE::Transports::filter::ShimLayer::~ShimLayer() 
{ }



void EMANE::Transports::filter::ShimLayer::initialize(Registrar & registrar) 
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "SHIMI %03hu %s::%s", id_, __MODULE__, __func__);

  try
   {
    auto & configRegistrar = registrar.configurationRegistrar();

    configRegistrar.registerNonNumeric<INETAddr>("address",
                                             ConfigurationProperties::NONE,
                                             {},
                                             "IPv4 or IPv6 virutal device address.");

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



void EMANE::Transports::filter::ShimLayer::configure(const ConfigurationUpdate & update)
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "SHIMI %03hu %s::%s", id_, __MODULE__, __func__);
	for(const auto & item : update)
	{
	  if(item.first == "address")
	    {
	      address_ = item.second[0].asINETAddr();

	      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
	                              INFO_LEVEL,
	                              "SHIMI %03hu %s::%s %s=%s",
	                              id_,
	                              __MODULE__,
	                              __func__,
	                              item.first.c_str(),
	                              address_.str(false).c_str());
	    }
	   else {

	    }
	}

  filterModemService_.configure(update);
}



void EMANE::Transports::filter::ShimLayer::start()
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "SHIMI %03hu %s::%s", id_, __MODULE__, __func__);

  filterModemService_.start();
}



void EMANE::Transports::filter::ShimLayer::stop()
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "SHIMI %03hu %s::%s", id_, __MODULE__, __func__);
}



void EMANE::Transports::filter::ShimLayer::destroy()
  throw()
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "SHIMI %03hu %s::%s", id_, __MODULE__, __func__);
}



void EMANE::Transports::filter::ShimLayer::processUpstreamControl(const ControlMessages & msgs)
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "SHIMI %03hu %s::%s pass thru %zu msgs", 
                          id_, __MODULE__, __func__, msgs.size());

  sendUpstreamControl(clone(msgs)); // taksham - we need to upstream the msf to the next shim layer!
}



void EMANE::Transports::filter::ShimLayer::processDownstreamControl(const ControlMessages & msgs)
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "SHIMI %03hu %s::%s pass thru %zu msgs", 
                          id_, __MODULE__, __func__, msgs.size());

  // pass thru
  sendDownstreamControl(clone(msgs));
}



void EMANE::Transports::filter::ShimLayer::processUpstreamPacket(UpstreamPacket & pkt,
                                                         const ControlMessages & msgs)
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "SHIMI %03hu %s::%s pass thru pkt len %zu and %zu msgs", 
                          id_, __MODULE__, __func__, pkt.length(), msgs.size());
  // pass thru
  sendUpstreamPacket(pkt, clone(msgs));
}


void EMANE::Transports::filter::ShimLayer::processDownstreamPacket(DownstreamPacket & pkt,
                                                           const ControlMessages & msgs)
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "SHIMI %03hu %s::%s pass thru pkt len %zu and %zu msgs", 
                          id_, __MODULE__, __func__, pkt.length(), msgs.size());

  //if dst addr is my addr, drop pkt.
  if(filterModemService_.filterDataMessages(pkt))
  {
    // pass thru
    sendDownstreamPacket(pkt, clone(msgs));
  }
}


DECLARE_SHIM_LAYER(EMANE::Transports::filter::ShimLayer);
