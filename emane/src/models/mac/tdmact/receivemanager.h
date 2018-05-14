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

#ifndef EMANEMODELSTDMACTRECEIVEMANAGER_HEADER_
#define EMANEMODELSTDMACTRECEIVEMANAGER_HEADER_

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
#include "basemodelmessage.h"
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
      class ReceiveManager
      {
      public:
        ReceiveManager(NEMId id,
                       DownstreamTransport * pDownstreamTransport,
                       LogServiceProvider * pLogService,
                       RadioServiceProvider * pRadioService,
                       PacketStatusPublisher * pPacketStatusPublisher,
                       NeighborMetricManager *pNeighborMetricManager);

        void setFragmentCheckThreshold(const std::chrono::seconds & threshold);

        void setFragmentTimeoutThreshold(const std::chrono::seconds & threshold);

        void setPromiscuousMode(bool bEnable);

        void loadCurves(const std::string & sPCRFileName);

        void enqueue(BaseModelMessage && baseModelMessage,
                     const PacketInfo & pktInfo,
                     size_t length,
                     const TimePoint & startOfReception,
                     const FrequencySegments & frequencySegments,
                     const Microseconds & span,
                     const TimePoint & beginTime,
                     std::uint64_t u64PacketSequence);

        void process(std::uint64_t u64AbsoluteSlotIndex);

        void processFrame();

        void clear();

        std::experimental::optional<BaseModelMessage> processRetransmissionSlot();


      private:

        using PendingInfo = std::tuple<BaseModelMessage,
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
        RadioServiceProvider * pRadioService_; // among other things, spectrum requests
        PacketStatusPublisher * pPacketStatusPublisher_;
        NeighborMetricManager * pNeighborMetricManager_;


        std::vector<PendingInfo> pendingInfos_; // all received messages in the current retransmission slot
        
        std::experimental::optional<PendingInfo> pendingReceived_; // the message (if there one) received in current frame

        PORManager porManager_; // for SINR/POR curve
        Utils::RandomNumberDistribution<std::mt19937,
                                        std::uniform_real_distribution<float>> distribution_;

        bool bPromiscuousMode_;
        std::chrono::seconds fragmentCheckThreshold_;
        std::chrono::seconds fragmentTimeoutThreshold_;

        using FragmentKey = std::tuple<NEMId,Priority,std::uint64_t>;
        using FragmentParts = std::map<size_t,std::vector<std::uint8_t>>;
        using FragmentInfo = std::tuple<std::set<size_t>,
                                        FragmentParts,
                                        TimePoint, // last fragment time
                                        NEMId, // destination
                                        Priority>;
        using FragmentStore = std::map<FragmentKey,FragmentInfo>;
        using FragmentTimeStore = std::map<TimePoint,FragmentKey>;

        FragmentStore fragmentStore_;
        FragmentTimeStore fragmentTimeStore_;
        TimePoint lastFragmentCheckTime_;




        ReceiveManager(const ReceiveManager &) = delete;

        ReceiveManager & operator=(const ReceiveManager &) = delete;
      };
    }
  }
}

#endif // EMANEMODELSTDMACTRECEIVEMANAGER_HEADER_
