/*
 * Copyright (c) 2013-2014,2016 - Adjacent Link LLC, Bridgewater,
 * New Jersey
 * Copyright (c) 2008 - DRS CenGen, LLC, Columbia, Maryland
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
 * * Neither the name of DRS CenGen, LLC nor the names of its
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

#ifndef EMANESHIMLAYER_HEADER_
#define EMANESHIMLAYER_HEADER_

#include "nemqueuedlayer.h"

#include <memory>

namespace EMANE
{
  /**
   * @class ShimLayer
   *
   * @brief Bridge for a shim NEM layer.  Decouples a ShimLayerImplementor
   * implementation from the NEM to allow for interface modification
   * and encapsulation.
   */
  class ShimLayer : public NEMQueuedLayer
  {
  public:
    ShimLayer(NEMId id,
              NEMLayer * pImplementor,
              PlatformServiceProvider *pPlatformService);

    ~ShimLayer();

    void initialize(Registrar & registrar) override;

    void configure(const ConfigurationUpdate & update) override;

    void start() override;

    void stop() override;

    void postStart() override;

    void destroy() throw() override;

    void setDownstreamTransport(DownstreamTransport *) override;

    void setUpstreamTransport(UpstreamTransport *) override;

  private:
    std::unique_ptr<NEMLayer> pImplementor_;

    std::unique_ptr<PlatformServiceProvider> pPlatformService_;

    void doProcessConfiguration(const ConfigurationUpdate &) override;

    void doProcessUpstreamControl(const ControlMessages &) override;

    void doProcessUpstreamPacket(UpstreamPacket &,
                                 const ControlMessages &) override;

    void doProcessDownstreamControl(const ControlMessages &) override;

    void doProcessDownstreamPacket(DownstreamPacket &,
                                   const ControlMessages &) override;

    void doProcessEvent(const EventId & , const Serialization &) override;

    void doProcessTimedEvent(TimerEventId eventId,
                             const TimePoint & expireTime,
                             const TimePoint & scheduleTime,
                             const TimePoint & fireTime,
                             const void * arg) override;
  };
}

#endif //EMANESHIMLAYER_HEADER_
