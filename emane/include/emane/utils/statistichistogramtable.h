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

#ifndef EMANEUTILSSTATISTICHISTOGRAMTABLE_HEADER_
#define EMANEUTILSSTATISTICHISTOGRAMTABLE_HEADER_

#include "emane/statisticregistrar.h"
#include "emane/statistictable.h"

#include <mutex>

namespace EMANE
{
  namespace Utils
  {
    /**
     * @class StatisticHistogramTable
     *
     * @brief Utility class to make a two column statistic table
     * where the first column is the table key and the second column
     * is a count of key instances.
     */
    template<typename Key,typename Counter = std::size_t>
    class StatisticHistogramTable
    {
    public:
      /**
       * Creates a StatisticHistogramTable table
       *
       * @param registrar StatisticRegistrar reference
       * @param sTableName Table name
       * @param labels Table column names
       * @param sDescription Table description
       *
       * @throw RegistrarException when a error occurs during
       * registration.
       */
      StatisticHistogramTable(StatisticRegistrar & registrar,
                              const std::string & sTableName,
                              const StatisticTableLabels & labels,
                              const std::string & sDescription = "");

      /**
       * Increments a table key count
       *
       * @param key Table key (row) to increment
       * @param amount Amount to increment by
       *
       * @throw StatisticTableException when the row key is invalid
       */
      void increment(const Key & key, Counter amount = 1);

    private:
      std::unordered_map<Key,Counter> umap_;
      StatisticTable<Key> * pStatisticTable_;
      std::mutex mutex_;
    };
  }
}

#include "emane/utils/statistichistogramtable.inl"

#endif // EMANEUTILSSTATISTICHISTOGRAMTABLE_HEADER_
