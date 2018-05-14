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

#ifndef EMANECONTROLSANTENNAPROFILECONTROLMESSAGE_HEDAER_
#define EMANECONTROLSANTENNAPROFILECONTROLMESSAGE_HEDAER_

#include "emane/types.h"
#include "emane/controlmessage.h"
#include "emane/controls/controlmessageids.h"

#include <memory>

namespace EMANE
{
  namespace Controls
  {
    /**
     * @class AntennaProfileControlMessage
     *
     * @brief Antenna Profile Control Message is sent to the emulator physical
     * layer to update antenna profile selection and/or antenna pointing
     * information.
     *
     * @note Instances are immutable
     *
     * @note Will result in the emulator physical layer publishing an 
     * Events::AntennaProfileEvent to inform all NEMs of the change.
     */
    class AntennaProfileControlMessage : public ControlMessage
    {
    public:
      /**
       *  Creates an AntennaProfileControlMessage instance on the heap
       *
       * @param id Antenna profile Id
       * @param dAntennaAzimuthDegrees Antenna point azimuth in degrees
       * @param dAntennaElevationDegrees Antenna pointing elevation in degrees
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
      AntennaProfileControlMessage * create(AntennaProfileId id,
                                            double dAntennaAzimuthDegrees,
                                            double dAntennaElevationDegrees);

      /**
       * Clones the control message on the heap
       *
       * @return cloned message
       *
       * @note Caller assumes ownership of the clone
       */
      AntennaProfileControlMessage * clone() const override;

      /**
       * Destroys an instance
       */
      ~AntennaProfileControlMessage();

      /**
       * Gets the antenna profile Id
       *
       * @return Profile id
       */
      AntennaProfileId getAntennaProfileId() const;
      
      /**
       * Gets the antenna pointing azimuth in degrees
       *
       * @return azimuth
       */
      double getAntennaAzimuthDegrees() const;
      
      /**
       * Gets the antenna pointing elevation in degrees
       *
       * @return elevation
       */
      double getAntennaElevationDegrees() const;
      
      enum {IDENTIFIER = EMANE_CONTROL_MEASSGE_ANTENNA_PROFILE};
      
    private:
      class Implementation;
      std::unique_ptr<Implementation> pImpl_;
      
      AntennaProfileControlMessage(AntennaProfileId id,
                                   double dAntennaAzimuthDegrees,
                                   double dAntennaElevationDegrees);
      
      AntennaProfileControlMessage(const AntennaProfileControlMessage &);

      AntennaProfileControlMessage & 
      operator=(const AntennaProfileControlMessage &) = delete;
    };
  }
}

#endif // EMANECONTROLSANTENNAPROFILECONTROLMESSAGE_HEDAER_
