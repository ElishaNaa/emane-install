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

#include "filtermodemservice.h"

#include <cstdint>
#include <set>
#include <string>
#include <vector>


#include "emane/configureexception.h"
#include "emane/utils/parameterconvert.h"
#include "emane/controls/serializedcontrolmessage.h"
#include "emane/controls/r2rineighbormetriccontrolmessageformatter.h"
#include "emane/controls/r2riqueuemetriccontrolmessageformatter.h"
#include "emane/controls/r2riselfmetriccontrolmessageformatter.h"

namespace
{
  const char * __MODULE__ = "FILTER::ModemService";


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

}


EMANE::ModemService::ModemService(NEMId id,
                                              PlatformServiceProvider * pPlatformService,
                                              RadioServiceProvider * pRadioService) :
  id_{id},
  pPlatformService_{pPlatformService},
  pRadioService_{pRadioService}
{

}



EMANE::ModemService::~ModemService() 
{ }


void EMANE::ModemService::configure(const ConfigurationUpdate & update)
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
       else
         {

           LOGGER_STANDARD_LOGGING(pPlatformService_->logService(), 
                         INFO_LEVEL,
                         "SHIMI %03hu %s::%s %s",
                         id_, 
                         __MODULE__, 
                         __func__, 
                         item.first.c_str());
         }
    }
}


void EMANE::ModemService::start()
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "SHIMI %03hu %s::%s", id_, __MODULE__, __func__);

}


bool EMANE::ModemService::filterDataMessages(EMANE::DownstreamPacket & pkt)
{
  //get vector with *base and len of the packet
  auto vIO = pkt.getVectorIO();
  auto v = vIO[0];
  //get ipv4 addr destination of the packet
  std::uint32_t addrV = ((Utils::Ip4Header*) ((Utils::EtherHeader*) v.iov_base + 1))->u32Ipv4dst;


  std::string str_address = address_.str(false).c_str();
  std::uint32_t myaddress = inet_addr(str_address.c_str());

  std::uint32_t myaddressB;
  const char * strr = "10.100.255.255";
  inet_aton(strr, (in_addr*) &myaddressB);


  const int NBYTES = 4;
  std::uint8_t octet[NBYTES];
  char ipAddressFinal[16];
  for(int i = 0 ; i < NBYTES ; i++)
  {
      octet[i] = addrV >> (i * 8);
  }
  sprintf(ipAddressFinal, "%d.%d.%d.%d", octet[3], octet[2], octet[1], octet[0]);

  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "SHIMI %03hu %s::%s packetAddr = %u myaddressA = %u ipAddressFinal = %s myaddressB = %u",
                          id_, __MODULE__, __func__, addrV, myaddress, ipAddressFinal, myaddressB);
                          
  //check if the packets dst addr equl to my addr, if it is - DROP
  if(addrV==myaddress)
  {
    LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "SHIMI %03hu %s::%s packet dropped",
                          id_, __MODULE__, __func__);
    return false;
  }

  return true;
}
