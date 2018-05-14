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

#ifndef EMANEMODELSTDMACTMANAGRECEIVEMANAGER_HEADER_
#define EMANEMODELSTDMACTMANAGRECEIVEMANAGER_HEADER_

#include "emane/types.h"
#include "emane/upstreampacket.h"
#include "emane/downstreamtransport.h"
#include "emane/radioserviceprovider.h"
#include "emane/logserviceprovider.h"
#include "emane/frequencysegment.h"
#include "emane/neighbormetricmanager.h"
#include "emane/models/tdmact/scheduler.h"
#include "emane/utils/randomnumberdistribution.h"

#include "pormanager.h"
#include "managementmessage.h"
#include "emane/models/tdmact/packetstatuspublisher.h"

#include <tuple>
#include <experimental/optional>

namespace EMANE
{
  namespace Models
  {
    namespace TDMACT
    {
      /**
       * @class ReceiveManager
       *
       * @brief Manages all receive side message processing
       *
       * Support aggregate message components and fragmentation
       * reassembly. Handles aggregation and fragmentation of inbound
       * messages even when the radio model is configured to not
       * aggregate or fragment transmissions.
       */
      class ManagementReceiveManager
      {
      public:
        ManagementReceiveManager(NEMId id,
                       DownstreamTransport * pDownstreamTransport,
                       LogServiceProvider * pLogService,
                       RadioServiceProvider * pRadioService,
                       PacketStatusPublisher * pPacketStatusPublisher,
                       NeighborMetricManager *pNeighborMetricManager);

        void loadCurves(const std::string & sPCRFileName);

        void enqueue(ManagementMessage && managementMessage,
                     const PacketInfo & pktInfo,
                     size_t length,
                     const TimePoint & startOfReception,
                     const FrequencySegments & frequencySegments,
                     const Microseconds & span,
                     const TimePoint & beginTime,
                     std::uint64_t u64PacketSequence);

        std::experimental::optional<ManagementMessage>  processRetransmissionSlot(); // to make more efficient

        std::experimental::optional<ManagementMessage>  processFrame(); // to make more efficient

        void setSelfMessage(ManagementMessage selfMessage); // set self message

        void clear();


      private:

        using PendingManagementInfo = std::tuple<ManagementMessage,
                                       PacketInfo,
                                       size_t,
                                       TimePoint, //sor
                                       FrequencySegments,
                                       Microseconds, // span
                                       TimePoint,
                                       std::uint64_t>; // sequence number

        NEMId id_;
        DownstreamTransport * pDownstreamTransport_;
        LogServiceProvider * pLogService_;
        RadioServiceProvider * pRadioService_;
        PacketStatusPublisher * pPacketStatusPublisher_;
        NeighborMetricManager * pNeighborMetricManager_;
        
        std::vector<PendingManagementInfo> pendingManagementInfos_;
        std::experimental::optional<ManagementMessage> pendingManagementWinner_;

        PORManager porManager_;
        Utils::RandomNumberDistribution<std::mt19937,
                                        std::uniform_real_distribution<float>> distribution_;

        bool bPromiscuousMode_;


        ManagementReceiveManager(const ManagementReceiveManager &) = delete;

        ManagementReceiveManager & operator=(const ManagementReceiveManager &) = delete;
      };
    }
  }
}

#endif // EMANEMODELSTDMACTMANAGRECEIVEMANAGER_HEADER_
