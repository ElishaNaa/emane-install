

#include "tdmactmanagementmessage.pb.h"

inline
EMANE::Models::TDMACT::ManagementMessage::ManagementMessage()
{}

inline
EMANE::Models::TDMACT::ManagementMessage::ManagementMessage(
                  EMANE::NEMId source,
                  std::uint64_t u64AbsoluteFrameIndex,
                  std::uint64_t u64RelativeRetransmissionSlotIndex,
                  std::uint64_t u64DataRate_bps,
                  std::uint64_t u64PriorityBits,
                  std::uint64_t u64CongestionBits,
                  std::uint64_t u64RandomBits,
                  Schedule schedule) :
                  source_{source},
                  u64AbsoluteFrameIndex_{u64AbsoluteFrameIndex},
                  u64RelativeRetransmissionSlotIndex_{u64RelativeRetransmissionSlotIndex},
                  u64DataRate_bps_{u64DataRate_bps},
                  u64PriorityBits_{u64PriorityBits},
                  u64CongestionBits_{u64CongestionBits},
                  u64RandomBits_{u64RandomBits},
                  schedule_{schedule}
                  {}

inline
EMANE::Models::TDMACT::ManagementMessage::ManagementMessage(const void *p, size_t len)
{
  EMANEMessage::TDMACTManagementMessage message;

  if(!message.ParseFromArray(p, len))
    {
      throw SerializationException("unable to deserialize TDMACTManagementMessage");
    }

  u64AbsoluteFrameIndex_ = message.absframeindex();
  source_ = message.source();
  u64RelativeRetransmissionSlotIndex_ = message.relretransmissionslotindex();
  u64PriorityBits_ = message.prioritybits();
  u64CongestionBits_ = message.congestionbits();
  u64RandomBits_ = message.randombits();
  u64DataRate_bps_ = message.dataratebps();

  for(const auto & frm : message.proposedschedule())
  {
      SchedulingFrameInfo frameInfo = {};
      frameInfo.u64AbsoluteFrameIndex_ = frm.absframeindex();
      frameInfo.transmitter_ = frm.transmitter();
      frameInfo.u64Priority_ = frm.priority();
      frameInfo.u64Congestion_ = frm.congestion();
      schedule_[frm.absframeindex()] = std::move(frameInfo);
  }
}

inline
std::uint64_t EMANE::Models::TDMACT::ManagementMessage::getAbsoluteFrameIndex() const
{
  return u64AbsoluteFrameIndex_;
}

inline
EMANE::NEMId EMANE::Models::TDMACT::ManagementMessage::getSource() const
{
  return source_;
}


inline
std::uint64_t EMANE::Models::TDMACT::ManagementMessage::getRelativeRetransmissionSlotIndex() const
{
  return u64RelativeRetransmissionSlotIndex_;
}

inline
void EMANE::Models::TDMACT::ManagementMessage::setRelativeRetransmissionSlotIndex(std::uint64_t relativeRetransmissionSlotIndex)
{
  u64RelativeRetransmissionSlotIndex_ = relativeRetransmissionSlotIndex;
}

inline
std::uint64_t EMANE::Models::TDMACT::ManagementMessage::getPriorityBits() const
{
  return u64PriorityBits_;
}

inline
std::uint64_t EMANE::Models::TDMACT::ManagementMessage::getCongestionBits() const
{
  return u64CongestionBits_;
}

inline
std::uint64_t EMANE::Models::TDMACT::ManagementMessage::getRandomBits() const
{
  return u64RandomBits_;
}

inline
std::uint64_t EMANE::Models::TDMACT::ManagementMessage::getDataRate() const
{
  return u64DataRate_bps_;
}

inline
const EMANE::Models::TDMACT::Schedule & EMANE::Models::TDMACT::ManagementMessage::getSchedule() const
{
 return schedule_;
}

inline
EMANE::Serialization EMANE::Models::TDMACT::ManagementMessage::serialize() const
{
  Serialization serialization{};

  EMANEMessage::TDMACTManagementMessage message{};

  message.set_source(source_);
  message.set_absframeindex(u64AbsoluteFrameIndex_);
  message.set_relretransmissionslotindex(u64RelativeRetransmissionSlotIndex_);
  message.set_dataratebps(u64DataRate_bps_);
  message.set_prioritybits(u64PriorityBits_);
  message.set_congestionbits(u64CongestionBits_);
  message.set_randombits(u64RandomBits_);

  for(const auto & schedule_item : schedule_)
    {
      auto pProposedSchedule = message.add_proposedschedule();

      pProposedSchedule->set_absframeindex(schedule_item.first); // it should be the same as schedule_item.second.u64AbsoluteFrameIndex_
      pProposedSchedule->set_transmitter(schedule_item.second.transmitter_);
      pProposedSchedule->set_priority(schedule_item.second.u64Priority_);
      pProposedSchedule->set_congestion(schedule_item.second.u64Congestion_);
    }

  if(!message.SerializeToString(&serialization))
    {
      throw SerializationException("unable to serialize TDMACTBaseModelMessage");
    }


  return serialization;
}
