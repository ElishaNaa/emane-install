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

#ifndef EMAENMODELSTDMACTBSAEMODELMESSAGE_HEADER_
#define EMAENMODELSTDMACTBSAEMODELMESSAGE_HEADER_

#include "emane/serializable.h"
#include "emane/types.h"
#include "tdmactbasemodelmessage.pb.h"
#include "emane/models/tdmact/messagecomponent.h"

namespace EMANE
{
  namespace Models
  {
    namespace TDMACT
    {
      /**
       * @class BaseModelMessage
       *
       * @brief Message class used to serialize and deserialize %TDMACT
       * radio model messages.
       */
      class BaseModelMessage : public Serializable
      {
      public:
        BaseModelMessage();

        BaseModelMessage(std::uint64_t u64AbsoluteFrameIndex,
                         std::uint64_t u64RelativeRetransmissionSlotIndex,
                         std::uint64_t u64DataRatebps,
                         std::uint64_t u64TotalSize,
                         MessageComponents && messages);

        BaseModelMessage(const void * p, size_t len);

        const MessageComponents & getMessages() const;

        std::uint64_t getAbsoluteFrameIndex() const;

        std::uint64_t getRelativeRetransmissionSlotIndex() const; // message has the index of the retransmission when it should have been sent

        void setRelativeRetransmissionSlotIndex(std::uint64_t relativeRetransmissionSlotIndex); // set the index of retransmission. Useful to create a copy to be retransmitted

        std::uint64_t getDataRate() const;

        std::uint64_t getTotalSize() const;

        Serialization serialize() const override;

      private:
        std::uint64_t u64AbsoluteFrameIndex_;
        std::uint64_t u64RelativeRetransmissionSlotIndex_;
        std::uint64_t u64DataRatebps_;
        std::uint64_t u64TotalSize_;
        MessageComponents messages_;
      };
    }
  }
}

#include "basemodelmessage.inl"

#endif //EMAENMODELSTDMACTBSAEMODELMESSAGE_HEADER_
