

#ifndef EMANEMODELSTDMACTWINNERSCHEDULEMESSAGE_HEADER_
#define EMANEMODELSTDMACTWINNERSCHEDULEMESSAGE_HEADER_

#include "emane/serializable.h"
#include "emane/types.h"
#include "tdmactmanagementmessage.pb.h"

namespace EMANE
{
  namespace Models
  {
    namespace TDMACT
    {
      /**
       * @class ManagementMessage
       *
       * @brief Message class used to serialize and deserialize %TDMACT
       * radio model messages.
       */
      class ManagementMessage : public Serializable
      {
      public:
        ManagementMessage();

        ManagementMessage(NEMId source,
                          std::uint64_t u64AbsoluteFrameIndex,
                          std::uint64_t u64RelativeRetransmissionSlotIndex,
                          std::uint64_t u64DataRate_bps,
                          std::uint64_t u64PriorityBits,
                          std::uint64_t u64CongestionBits,
                          std::uint64_t u64RandomBits,
                          Schedule schedule);

        ManagementMessage(const void * p, size_t len);


        NEMId getSource() const;

        std::uint64_t getAbsoluteFrameIndex() const;

        std::uint64_t getRelativeRetransmissionSlotIndex() const; // message has the index of the retransmission when it should have been sent

        void setRelativeRetransmissionSlotIndex(std::uint64_t relativeRetransmissionSlotIndex); // set the index of retransmission. Useful to create a copy to be retransmitted

        std::uint64_t getPriorityBits() const;

        std::uint64_t getCongestionBits() const;

        std::uint64_t getRandomBits() const;

        std::uint64_t getDataRate() const;

        const Schedule & getSchedule() const;

        Serialization serialize() const override;

      private:
        NEMId source_;
        std::uint64_t u64AbsoluteFrameIndex_;
        std::uint64_t u64RelativeRetransmissionSlotIndex_;
        std::uint64_t u64DataRate_bps_;
        std::uint64_t u64PriorityBits_;
        std::uint64_t u64CongestionBits_;
        std::uint64_t u64RandomBits_;
        Schedule schedule_;
      };
    }
  }
}

#include "managementmessage.inl"

#endif //EMANEMODELSTDMACTWINNERSCHEDULEMESSAGE_HEADER_
