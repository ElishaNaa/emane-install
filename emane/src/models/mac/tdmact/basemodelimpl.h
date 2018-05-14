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

#ifndef EMANETDMACTBASEMODELIMPL_HEADER_
#define EMANETDMACTBASEMODELIMPL_HEADER_

#include "emane/maclayerimpl.h"
#include "emane/flowcontrolmanager.h"
#include "emane/neighbormetricmanager.h"
#include "emane/models/tdmact/basemodel.h"
#include "emane/models/tdmact/scheduler.h"
#include "emane/models/tdmact/queuemanager.h"
#include "emane/controls/r2riselfmetriccontrolmessage.h"
#include "emane/queuemetricmanager.h"

#include "slotstatustablepublisher.h"
#include "receivemanager.h"
#include "managementreceivemanager.h"
#include "packetstatuspublisherimpl.h"
#include "aggregationstatuspublisher.h"
#include "rounder.h"

#include <experimental/optional> // to be changed to "optional" when the time comes!!

namespace EMANE
{
  namespace Models
  {
    namespace TDMACT
    {
      /**
       * @class BaseModel::Implementation
       *
       * @brief Implementation of BaseModel
       */
      class BaseModel::Implementation : public MACLayerImplementor,
                                        public SchedulerUser
      {
      public:
        Implementation(NEMId id,
                       PlatformServiceProvider *pPlatformServiceProvider,
                       RadioServiceProvider * pRadioServiceProvider,
                       Scheduler * pScheduler,
                       QueueManager * pQueueManager,
                       MACLayerImplementor * pRadioModel);

        ~Implementation();

        void initialize(Registrar & registrar) override;

        void configure(const ConfigurationUpdate & update) override;

        void start() override;

        void postStart() override;

        void stop() override;

        void destroy() throw() override;

        void processUpstreamControl(const ControlMessages & msgs) override;


        void processUpstreamPacket(const CommonMACHeader & hdr,
                                   UpstreamPacket & pkt,
                                   const ControlMessages & msgs) override;

        void processDownstreamControl(const ControlMessages & msgs) override;


        void processDownstreamPacket(DownstreamPacket & pkt,
                                     const ControlMessages & msgs) override;


        void processEvent(const EventId &, const Serialization &) override;

        void processConfiguration(const ConfigurationUpdate & update) override;

        void notifyScheduleChange(const Frequencies & frequencies,
                                  std::uint64_t u64BandwidthHz,
                                  const Microseconds & slotDuration,
                                  const Microseconds & slotOverhead) override;


        // the following functions are necessary because this is a subclass of SchedulerUser
        void processSchedulerPacket(DownstreamPacket & pkt) override;
        void processSchedulerControl(const ControlMessages & msgs) override;

        QueueInfos getPacketQueueInfo() const override;

      private:
        std::unique_ptr<Scheduler> pScheduler_; // LEGACY. to remove one day but it is connected to other files (see basemodel.cc and radiomodel.cc/h)
        std::unique_ptr<QueueManager> pQueueManager_; // manager of queues of messages to send
        MACLayerImplementor * pRadioModel_;

        bool bFlowControlEnable_;
        bool bRadioMetricEnable_; //dev - Enablr radio metric for R2RI.
        std::uint16_t u16FlowControlTokens_;
        std::uint16_t u16RetransmissionSlots_; // number of retransmissionslots in slot
        std::uint64_t u64FrameDuration_; // frame duration in microseconds from xml file
        std::uint64_t u64ManagementRetransmissionSlots_; // retransmissions of a winnerschedulemessage
        std::uint64_t u64ManagementDuration_; // the duration of the management part that should be
        std::uint64_t u64ManagementEffectiveDuration_; // the effective (in this program) duration of the management part
        std::uint64_t u64NumScheduledSlots_; // how big is the schedule that is sent by the winner
        std::uint64_t u64Frequency_Hz_; // frequency as decided by the xml
        std::uint64_t u64BandwidthHz_; // now decided by the xml
        std::uint64_t u64DataRate_bps_; // data rate of sent messages
        std::uint64_t u64DefaultClass_; // default queue from which to send messages (probably the highest priority one)
        std::uint64_t u64ManagementBits_; // how many bits are there in a management message. Not still implemented

        TimerEventId nextManagementRetransmissionTimedEventId_;
        TimerEventId nextRetransmissionTimedEventId_;
        TimerEventId managementTimedEventId_;
        TimerEventId nextFrameTimedEventId_;

        double dPower_dBm_; // transmission power as decided by the xml
        std::string sPCRCurveURI_; // uri of SINR vs probability of receival curve

        Microseconds slotOverhead_; // now not functional and always 0

        SlotStatusTablePublisher slotStatusTablePublisher_; // for statistics

        std::uint64_t u64SequenceNumber_;  // unique id of each messages sent by me
        Microseconds neighborMetricUpdateInterval_; // for statistics
        PacketStatusPublisherImpl packetStatusPublisher_; // for statistics
        NeighborMetricManager neighborMetricManager_; // neighbors, for statistics
        QueueMetricManager queueMetricManager_; //for dlep
        TimerEventId radioMetricTimedEventId_; //for dlep
        ReceiveManager receiveManager_; // wrapper of vector of received messages
        ManagementReceiveManager managementReceiveManager_; // wrapper of vector of received management messages
        FlowControlManager flowControlManager_; // flow control controller
//        std::uint64_t u64ScheduleIndex_; legacy
        AggregationStatusPublisher aggregationStatusPublisher_; // statistics
        Rounder rounder_; // utility class for rounding times

        // data
        std::uint64_t u64PendingFrame_; // the frame we are in (absolute index)
        std::uint64_t u64PendingCongestionBits_;  
        std::uint64_t u64PendingRandomBits_;
        std::uint64_t u64PendingPriorityBits_;
        Schedule pendingSchedule_; // the schedule we are sending
        FrameType pendingType_; // the type of frame we are in (RX or TX)


        std::experimental::optional<std::uint64_t> u64ReceivalPendingRelativeRetransmissionSlot_; // the retransmission slot we are in, nullopt if in management part
        std::experimental::optional<std::uint64_t> u64ReceivalPendingManagementRelativeRetransmissionSlot_; // the management retransmission slot we are in, nullopt if in message part
        
        std::uint64_t lastReceptionAbsoluteFrame_; // absolute slot at last reception. Used in order not to receive further messages

        // procedures
        void processRetransmissionSlotBegin(std::uint64_t u64RelativeRetransmissionSlotIndex);         // returns whether the message has been received

        void processManagementRetransmissionSlotBegin(std::uint64_t u64RelativeManagementRetransmissionSlotIndex);

        void processFrameBegin(std::uint64_t absoluteSlotIndex);
        
        void processManagementPartBegin();

      

        std::uint64_t relativeTimeToRelativeRetransmissionSlotIndex(Microseconds microseconds); // index of retransmissionslot from timepoint
        Microseconds relativeRetransmissionSlotIndexToRelativeTime(std::uint64_t relativeRetransmissionSlotIndex); // index of retransmissionslot from timepoint
        
        std::uint64_t relativeTimeToRelativeManagementRetransmissionSlotIndex(Microseconds microseconds); // index of retransmissionslot from timepoint
        Microseconds relativeManagementRetransmissionSlotIndexToRelativeTime(std::uint64_t relativeMRetransmissionSlotIndex); // index of retransmissionslot from timepoint

        std::uint64_t getRetransmissionSlotDuration_us(); // duration in microseconds of a retransmission slot

        std::uint64_t getManagementRetransmissionSlotDuration_us(); // duration in microseconds of a m. retransmission slot

        std::uint64_t getMessagesPartDuration_us(); // duration in microseconds of all the message part

        std::uint64_t getEffectiveDataRateBps(); // data rate in bps when considering for the expansion of the management section

        std::uint64_t absoluteTimeToAbsoluteFrameIndex(TimePoint timePoint); // index of retransmissionslot from timepoint
        EMANE::TimePoint absoluteFrameIndexToAbsoluteTime(std::uint64_t absoluteFrameIndex); // index of retransmissionslot from timepoint

        std::uint64_t getBytesInRetransmissionSlot();

        FrameType frameType(std::uint64_t absoluteFrameIndex);


        void sendDownstreamPacket(double dSlotRemainingRatio);

        void sendDownstreamManagement(ManagementMessage &&mm, TimePoint txStart);

        void sendDownstreamMessage(BaseModelMessage &&bmm, TimePoint txStart);


        void processTxOpportunity();
       
        // process messages
        void processUpstreamPacketBaseMessage(const CommonMACHeader &hdr,
                                              UpstreamPacket &pkt,
                                              const ControlMessages &msgs);
        // process management
        void processUpstreamPacketManagement( const CommonMACHeader &hdr,
                                               UpstreamPacket &pkt,
                                               const ControlMessages &msgs);
        
      };
    }
  }
}

#endif // EMANETDMACTBASEMODELIMPL_HEADER_
