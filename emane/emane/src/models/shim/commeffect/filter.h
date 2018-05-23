/*
 * Copyright (c) 2013 - Adjacent Link LLC, Bridgewater, New Jersey
 * Copyright (c) 2009 - DRS CenGen, LLC, Columbia, Maryland
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
 *
 */

#ifndef EMANEMODELSCOMMEFFECTFILTER_HEADER_
#define EMANEMODELSCOMMEFFECTFILTER_HEADER_

#include "target.h"
#include "emane/events/commeffect.h"

#include <list>
#include <memory>

namespace EMANE
{
  namespace Models
  {
    namespace CommEffect
    {
      class Filter
      {
      public:
        Filter(Target * pTarget, const Events::CommEffect & commEffect);
      
        bool match(const void * buf, std::size_t len) const;
      
        const Events::CommEffect & getEffect() const;
      
      private:
        std::unique_ptr<Target> pTarget_;
        Events::CommEffect commEffect_;
      };
    
      using Filters = std::list<Filter>;
    }
  }
}
#endif // EMANEMODELSCOMMEFFECTFILTER_HEADER_
