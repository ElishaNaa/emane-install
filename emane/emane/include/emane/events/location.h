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

#ifndef EMANEEVENTSLOCATION_HEADER_
#define EMANEEVENTSLOCATION_HEADER_

#include "emane/types.h"
#include "emane/position.h"
#include "emane/orientation.h"
#include "emane/velocity.h"

#include <list>
#include <utility>

namespace EMANE
{
  namespace Events
  {
    /**
     * @class Location
     *
     * @brief A location entry holds an NEM Id, that NEM's position 
     * information and optional orientation and velocity.

     *
     * @see Position
     * @see Orientation
     * @see Velocity
     *
     * @note Instances are immutable
     */
    class Location
    {
    public:
      Location(NEMId id,
               const Position & position,
               std::pair<const Orientation &, bool> orientation,
               std::pair<const Velocity &, bool> velocity);
     
      /**
       * Gets the NEM Id
       *
       * @return NEM Id
       */
      NEMId getNEMId() const;
      
      /**
       * Gets the NEM position
       *
       * @return position
       */
      const Position & getPosition() const;
      
      /**
       * Gets the optional NEM orientation
       *
       * @return std::pair, where @c first is the orientation and
       * @c second is a flag indicating whether it is valid (present)
       */
      std::pair<const Orientation &, bool> getOrientation() const;
      
      /**
       * Gets the optional NEM velocity
       *
       * @return std::pair, where @c first is the velocity and
       * @c second is a flag indicating whether it is valid (present)
       */
      std::pair<const Velocity &, bool> getVelocity() const;
      
    private:
      EMANE::NEMId id_;
      Position position_;
      Orientation orientation_;
      Velocity velocity_;
      bool bHasOrientation_;
      bool bHasVelocity_;
    };
    
    using Locations = std::list<Location>;
  }
}

#include "emane/events/location.inl"
  
#endif // EMANEEVENTSLOCATION_HEADER_
