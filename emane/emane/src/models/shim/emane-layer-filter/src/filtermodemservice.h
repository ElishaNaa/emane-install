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


#ifndef EMANEMODEL_FILTERLAYER_MODEMSERVICE_HEADER_
#define EMANEMODEL_FILTERLAYER_MODEMSERVICE_HEADER_

#include "emane/shimlayerimpl.h"
#include "emane/utils/netutils.h"

#include "emane/controls/r2rineighbormetriccontrolmessage.h"
#include "emane/controls/r2riqueuemetriccontrolmessage.h"
#include "emane/controls/r2riselfmetriccontrolmessage.h"
#include "emane/controls/flowcontrolcontrolmessage.h"

#include "emane/inetaddr.h"

#include <pcap/pcap.h>
#include <sstream>

#include <map>

namespace EMANE
{
  //   namespace filter
  //   {
      /**
       * @class ModemService
       *
       * @brief Implements EMANE filter ModemService
       */

      class ModemService 
      {
      public:
        ModemService(NEMId id,
                     PlatformServiceProvider *pPlatformService,
                     RadioServiceProvider * pRadioServiceProvider);

        ~ModemService();

        void configure(const ConfigurationUpdate & update);

        void start();

        bool filterDataMessages(DownstreamPacket & pkt);
        
      private:
        INETAddr address_;

    
        NEMId id_;

        PlatformServiceProvider * pPlatformService_;
 
        RadioServiceProvider * pRadioService_;

      };
}

#endif
