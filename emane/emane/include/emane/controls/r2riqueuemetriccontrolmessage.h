/*
 * Copyright (c) 2013-2014 - Adjacent Link LLC, Bridgewater, New Jersey
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

#ifndef EMANECONTROLSR2RIQUEUEMETRICCONTROLMESSAGE_HEADER_
#define EMANECONTROLSR2RIQUEUEMETRICCONTROLMESSAGE_HEADER_

#include "emane/controlmessage.h"
#include "emane/controls/controlmessageids.h"
#include "emane/controls/r2riqueuemetric.h"

#include <memory>

namespace EMANE
{
  namespace Controls
  {
    /**
     * @class R2RIQueueMetricControlMessage
     *
     * @brief R2RI Queue Metric Control Message is sent to an NEM's transport layer to
     * convey information about MAC layer queues.
     *
     * @note This control message will be encapsulated in a SerializedControlMessage
     * when delivered as via EMANE::DownstreamTransport::processDownstreamControl() or
     * EMANE::UpstreamTransport::processUpstreamControl().
     *
     * @note Instances are immutable
     */ 
    class R2RIQueueMetricControlMessage : public ControlMessage
    {
    public:
      /**
       * Creates an R2RIQueueMetricControlMessage from a serialization on the heap
       *
       * @param serialization Message serialization
       *
       * @throw SerializationException when a valid message cannot be de-serialized
       *
       * @note Once a control message is passed to another NEM layer using 
       * EMANE::UpstreamTransport::processUpstreamPacket(),
       * EMANE::UpstreamTransport::processUpstreamControl(),
       * EMANE::DownstreamTransport::processDownstreamPacket() or 
       * EMANE::DownstreamTransport::processDownstreamControl() object ownership is
       * transferred to the emulator infrastructure along with deallocation responsibility.
       * It is not valid to use a control message instance after it has been passed to another
       * layer. 
       */
      static
      R2RIQueueMetricControlMessage * create(const Serialization & serialization);
      
      /**
       * Creates an R2RIQueueMetricControlMessage on the heap
       *
       * @param queueMetrics Queue metrics
       *
       * @note Once a control message is passed to another NEM layer using 
       * EMANE::UpstreamTransport::processUpstreamPacket(),
       * EMANE::UpstreamTransport::processUpstreamControl(),
       * EMANE::DownstreamTransport::processDownstreamPacket() or 
       * EMANE::DownstreamTransport::processDownstreamControl() object ownership is
       * transferred to the emulator infrastructure along with deallocation responsibility.
       * It is not valid to use a control message instance after it has been passed to another
       * layer. 
       */
      static
      R2RIQueueMetricControlMessage * create(const R2RIQueueMetrics & queueMetrics);

      /**
       * Clones the control message on the heap
       *
       * @return cloned message
       *
       * @note Caller assumes ownership of the clone
       */
      R2RIQueueMetricControlMessage * clone() const override;
      
      /**
       * Destroys an instance
       */
      ~R2RIQueueMetricControlMessage();
      
      /**
       * Gets the queue metrics
       *
       * @return metrics
       */
      const R2RIQueueMetrics & getQueueMetrics() const;

      /**
       * Serializes the instance
       *
       * @throw SerializationException if the instance cannot be serialized
       */
      std::string serialize() const override;
      
      enum {IDENTIFIER = EMANE_CONTROL_MEASSGE_R2RI_QUEUE_METRIC};
      
    private:
      class Implementation;
      std::unique_ptr<Implementation> pImpl_;
      
      R2RIQueueMetricControlMessage(const  R2RIQueueMetrics & queueMetrics);
      
      R2RIQueueMetricControlMessage(const  R2RIQueueMetricControlMessage &);
      
      R2RIQueueMetricControlMessage & 
      operator=(const R2RIQueueMetricControlMessage &) = delete;
    };
  }
}

#endif // EMANECONTROLR2RIQUEUEMETRICCONTROLMESSAGE_HEADER_
