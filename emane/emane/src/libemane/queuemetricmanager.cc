/*
 * Copyright (c) 2013,2016 - Adjacent Link LLC, Bridgewater, New Jersey
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

#include <math.h>
#include <map>

#include "emane/types.h"
#include "emane/queuemetricmanager.h"
#include "emane/constants.h"

#include <mutex>

class EMANE::QueueMetricManager::Implementation
  {
    public:
      Implementation(EMANE::NEMId id) :
       id_{id}
      { }


     ~Implementation()
      { }

     void updateQueueMetric(std::uint16_t u16QueueId,
                            std::uint32_t u32MaxQueueSize,
                            std::uint32_t u32CurrentQueueDepth,
                            std::uint32_t u32NumDiscards,
                            const EMANE::Microseconds & delayMicroseconds)
       {
         std::lock_guard<std::mutex> m(mutex_);

         QueueData & rData = getQueueData_i(u16QueueId);

         // bump num samples
         rData.u32NumSamples_ += 1;

         // bump the delay sum
         rData.sumDelayMicroseconds_ += delayMicroseconds;

         // check max size change
         if(rData.u32MaxQueueSize_ != u32MaxQueueSize)
           {
             // set new max size
             rData.u32MaxQueueSize_ = u32MaxQueueSize;

             // reset high water mark
             rData.u32QueueHighWaterMark_ = 0;
           }

         // check queue depth high water
         if(rData.u32QueueHighWaterMark_ < u32CurrentQueueDepth)
           {
             rData.u32QueueHighWaterMark_ = u32CurrentQueueDepth;
           }

         // check discards high water
         if(rData.u32NumDiscardsHighWaterMark_ < u32NumDiscards)
           {
             rData.u32NumDiscardsHighWaterMark_ = u32NumDiscards;
           }
       }


    EMANE::Controls::R2RIQueueMetrics getQueueMetrics()
      {
        std::lock_guard<std::mutex> m(mutex_);

        EMANE::Controls::R2RIQueueMetrics metrics;

       for(auto & iter : queueDataMap_)
         {
           // get avg delay time
           const EMANE::Microseconds avgDelayMicroseconds{getAvg_i(iter.second.sumDelayMicroseconds_,
                                                                   iter.second.u32NumSamples_)};

           EMANE::Controls::R2RIQueueMetric metric{iter.first,                               // queue id
                                                   iter.second.u32MaxQueueSize_,             // queue max size
                                                   iter.second.u32QueueHighWaterMark_,       // queue current depth high water
                                                   iter.second.u32NumDiscardsHighWaterMark_, // num discards high water
                                                   avgDelayMicroseconds};                    // avg delay

           metrics.push_back(metric);

           // reset data
           iter.second.reset();
         }

         return metrics;
      }


     bool addQueueMetric(std::uint16_t queueId, std::uint32_t u32MaxQueueSize)
      {
        std::lock_guard<std::mutex> m(mutex_);

        return queueDataMap_.insert(std::make_pair(queueId, QueueData(u32MaxQueueSize))).second;
      }


    bool removeQueueMetric(std::uint16_t queueId)
      {
        std::lock_guard<std::mutex> m(mutex_);

        auto iter = queueDataMap_.find(queueId);

        if(iter != queueDataMap_.end())
          {
            // remove entry
            queueDataMap_.erase(iter);

            return true;
          }
        else
         {
            return false;
         }
      }

    private:
      struct QueueData {
        // metrics observed over the report interval
        std::uint32_t        u32MaxQueueSize_;              // queue max size
        std::uint32_t        u32QueueHighWaterMark_;        // queue depth high water
        std::uint32_t        u32NumSamples_;                // num samples
        std::uint32_t        u32NumDiscardsHighWaterMark_;  // num discards high water
        EMANE::Microseconds  sumDelayMicroseconds_;         // total delay

       QueueData() :
         u32MaxQueueSize_{},
         u32QueueHighWaterMark_{},
         u32NumSamples_{},
         u32NumDiscardsHighWaterMark_{},
         sumDelayMicroseconds_{}
        { }

       QueueData(std::uint32_t u32MaxSize) :
         u32MaxQueueSize_(u32MaxSize),
         u32QueueHighWaterMark_{},
         u32NumSamples_{},
         u32NumDiscardsHighWaterMark_{},
         sumDelayMicroseconds_{}
        { }

       void reset()
        {
          u32NumDiscardsHighWaterMark_ = 0;

          sumDelayMicroseconds_ = EMANE::Microseconds::zero();

          u32NumSamples_ = 0;

          u32QueueHighWaterMark_ = 0;
        }
     };

     using QueueDataMap = std::map<std::uint16_t, QueueData>;

     EMANE::NEMId id_;

     QueueDataMap queueDataMap_;

     std::mutex mutex_;


     QueueData & getQueueData_i(std::uint16_t queueId)
       {
         auto iter = queueDataMap_.find(queueId);

         if(iter == queueDataMap_.end())
           {
             // add new queue
             iter = queueDataMap_.insert(std::make_pair(queueId, QueueData())).first;
           }

         return iter->second;
       }


     EMANE::Microseconds getAvg_i(const EMANE::Microseconds & delayMicroseconds, size_t samples)
       {
         if(samples > 1)
           {
             DoubleSeconds doubleSeconds{delayMicroseconds.count() / (USEC_PER_SEC_F * samples)};

             return std::chrono::duration_cast<EMANE::Microseconds>(doubleSeconds);
           }
         else if(samples == 1)
           {
             return delayMicroseconds;
           }
         else
           {
             return EMANE::Microseconds::zero();
           }
       }
 };


EMANE::QueueMetricManager::QueueMetricManager(EMANE::NEMId id) :
  pImpl_{new Implementation{id}}
{ }


EMANE::QueueMetricManager::~QueueMetricManager()
{ }


EMANE::Controls::R2RIQueueMetrics
EMANE::QueueMetricManager::getQueueMetrics()
{
   return pImpl_->getQueueMetrics();
}


void
EMANE::QueueMetricManager::updateQueueMetric(std::uint16_t u16QueueId,
                                             std::uint32_t u32MaxQueueSize,
                                             std::uint32_t u32CurrentQueueDepth,
                                             std::uint32_t u32NumDiscards,
                                             const Microseconds & delayMicroseconds)
{
  pImpl_->updateQueueMetric(u16QueueId,
                            u32MaxQueueSize,
                            u32CurrentQueueDepth,
                            u32NumDiscards,
                            delayMicroseconds);
}


bool
EMANE::QueueMetricManager::addQueueMetric(std::uint16_t u16QueueId, std::uint32_t u32MaxQueueSize)
{
  return pImpl_->addQueueMetric(u16QueueId, u32MaxQueueSize);
}


bool
EMANE::QueueMetricManager::removeQueueMetric(std::uint16_t u16QueueId)
{
  return pImpl_->removeQueueMetric(u16QueueId);
}
