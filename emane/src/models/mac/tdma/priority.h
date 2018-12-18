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

#ifndef EMANEMODELSTDMAPRIORITY_HEADER_
#define EMANEMODELSTDMAPRIORITY_HEADER_

#include "emane/types.h"

#include <sstream>
#include <vector>
#include <map>

#include "emane/models/tdma/queuemanager.h"
#include "basemodelimpl.h"

#include "queuemappingsingalton.h"

namespace EMANE
{
  namespace Models
  {
    namespace TDMA
    {
      /**
       * Converts a packet/message priority, usually originating as a
       * dscp code point, to a %TDMA queue index.
       *
       * @param priority Message priority
       *
       * @return %Queue index
       */
	  template<typename Out>
	  inline void split(const std::string &s, char delim, Out result) {
		  std::stringstream ss(s);
		  std::string item;
		  while (std::getline(ss, item, delim)) {
			  *(result++) = item;
		  }
	  }

	  inline std::vector<std::string> split(const std::string &s, char delim) {
		  std::vector<std::string> elems;
		  split(s, delim, std::back_inserter(elems));
		  return elems;
	  }
	  
	  inline std::uint8_t priorityToQueue(Priority priority)
      {
		  std::uint8_t u8Queue{0}; // default
		  std::map<std::uint8_t, std::string> queueMapping_ = queuemappingsingalton::instance()->getQueueMapping();

		  for (std::map<std::uint8_t, std::string>::iterator it = queueMapping_.begin(); it != queueMapping_.end(); ++it)
		  {
			  std::vector<std::string> x;
			  std::vector<std::string> xx = split(it->second, ',');
			  for (auto const& value : xx)
			  {
				  x = split(value, '-');
				  const int numA = stoi(x.front());
				  const int numB = stoi(x.back());
			
				  if (priority >= numA && priority <= numB)
				  {
					  u8Queue = it->first;
					  break;
				  }
			  }
		  }

		  return u8Queue;
	  }
    }
  }
}

#endif // EMANEMODELSTDMAPRIORITY_HEADER_
