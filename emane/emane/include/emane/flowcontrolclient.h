/*
 * Copyright (c) 2013-2014 - Adjacent Link LLC, Bridgewater, New Jersey
 * Copyright (c) 2010 - DRS CenGen, LLC, Columbia, Maryland
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

#ifndef EMANEUTILSFLOWCONTROL_HEADER_
#define EMANEUTILSFLOWCONTROL_HEADER_

#include "emane/upstreamtransport.h"
#include "emane/controls/flowcontrolcontrolmessage.h"

namespace EMANE
{
 /**
  *  
  * @class FlowControlClient
  *
  * @brief FlowControlClient is the token consumer side of
  * nem layer to transport flow contol.
  *
  */
  class FlowControlClient
  { 
  
  public:
    
   /**
    * Creates a FlowControlClient instance
    *
    * @param transport reference to the UpstremTransport
    *
    */
    FlowControlClient(EMANE::UpstreamTransport & transport);
 
   /**
    * Destory an instance
    *
    */
    ~FlowControlClient();

   /**
    * Start starts flow control processing
    *
    */
    void start();

   /**
    * Stop stops flow control processing
    *
    */
    void stop();

   /**
    * Removes a flow control token, blocking until a token is available
    * when flow control is enabled
    *
    * @return Total number of tokens available and a flag indicating success
    */
    std::pair<std::uint16_t,bool>  removeToken();

   /**
    * Handles a flow control update message
    *
    * @param pMsg flow control message containig the number of tokens avaialble
    *
    */
    void processFlowControlMessage(const Controls::FlowControlControlMessage * pMsg);

  private:
    class Implementation;
    std::unique_ptr<Implementation> pImpl_;

    FlowControlClient(const FlowControlClient &) = delete;

    FlowControlClient & 
    operator=(const FlowControlClient &) = delete;
  };
}

#endif // EMANEUTILSFLOWCONTROL_HEADER_
