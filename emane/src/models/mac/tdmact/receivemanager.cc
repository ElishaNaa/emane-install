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

#include "receivemanager.h"
#include "emane/utils/spectrumwindowutils.h"
#include "devlog.h"

EMANE::Models::TDMACT::ReceiveManager::ReceiveManager(NEMId id,
                                                      DownstreamTransport *pDownstreamTransport,
                                                      LogServiceProvider *pLogService,
                                                      RadioServiceProvider *pRadioService,
                                                      PacketStatusPublisher *pPacketStatusPublisher,
                                                      NeighborMetricManager *pNeighborMetricManager) : id_{id},
                                                                                                       pDownstreamTransport_{pDownstreamTransport},
                                                                                                       pLogService_{pLogService},
                                                                                                       pRadioService_{pRadioService},
                                                                                                       pPacketStatusPublisher_{pPacketStatusPublisher},
                                                                                                       pNeighborMetricManager_{pNeighborMetricManager},
                                                                                                       pendingInfos_{},
                                                                                                       pendingReceived_{}, 
                                                                                                       distribution_{0.0, 1.0},
                                                                                                       bPromiscuousMode_{},
                                                                                                       fragmentCheckThreshold_{2},
                                                                                                       fragmentTimeoutThreshold_{5} {}

void EMANE::Models::TDMACT::ReceiveManager::setPromiscuousMode(bool bEnable)
{
  bPromiscuousMode_ = bEnable;
}

void EMANE::Models::TDMACT::ReceiveManager::loadCurves(const std::string &sPCRFileName)
{
  porManager_.load(sPCRFileName);
}

void EMANE::Models::TDMACT::ReceiveManager::setFragmentCheckThreshold(const std::chrono::seconds &threshold)
{
  fragmentCheckThreshold_ = threshold;
}

void EMANE::Models::TDMACT::ReceiveManager::setFragmentTimeoutThreshold(const std::chrono::seconds &threshold)
{
  fragmentTimeoutThreshold_ = threshold;
}

void EMANE::Models::TDMACT::ReceiveManager::enqueue(BaseModelMessage &&baseModelMessage,
                                                    const PacketInfo &pktInfo,
                                                    size_t length,
                                                    const TimePoint &startOfReception,
                                                    const FrequencySegments &frequencySegments,
                                                    const Microseconds &span,
                                                    const TimePoint &beginTime,
                                                    std::uint64_t u64PacketSequence)
{
  CTLOG("REM %d Enqueueing message from %d\n", id_, pktInfo.getSource());
  pendingInfos_.push_back(std::make_tuple(std::move(baseModelMessage),
                                          pktInfo,
                                          length,
                                          startOfReception,
                                          frequencySegments,
                                          span,
                                          beginTime,
                                          u64PacketSequence));

}

// returns a message between 
std::experimental::optional<EMANE::Models::TDMACT::BaseModelMessage> EMANE::Models::TDMACT::ReceiveManager::processRetransmissionSlot()
{
  std::experimental::optional<EMANE::Models::TDMACT::BaseModelMessage> toReturn;
  toReturn = std::experimental::nullopt;

  for (auto &&i : pendingInfos_)
  {
    double dSINR{};
    double dNoiseFloordB{};
    
    BaseModelMessage &baseModelMessage = std::get<0>(i);
    PacketInfo pktInfo{std::get<1>(i)};
    size_t length{std::get<2>(i)};
    TimePoint &startOfReception = std::get<3>(i);
    FrequencySegments &frequencySegments = std::get<4>(i);
    Microseconds &span = std::get<5>(i);
    std::uint64_t u64SequenceNumber{std::get<7>(i)};

    auto &frequencySegment = *frequencySegments.begin();

    CTLOG("REC %d process infos, from %d\n", id_, pktInfo.getSource());

    // get sinr and noisefloor
    try
    {
      auto window = pRadioService_->spectrumService().request(frequencySegment.getFrequencyHz(),
                                                              span,
                                                              startOfReception);

      bool bSignalInNoise{};

      std::tie(dNoiseFloordB, bSignalInNoise) =
          Utils::maxBinNoiseFloor(window, frequencySegment.getRxPowerdBm());

      dSINR = frequencySegment.getRxPowerdBm() - dNoiseFloordB;

      LOGGER_VERBOSE_LOGGING(*pLogService_,
                             DEBUG_LEVEL,
                             "MACI %03hu TDMACT::ReceiveManager upstream EOR processing 1 :"
                             " src %hu, dst %hu, rxpwr: %f, max noise %lf, signal in noise %s, SINR %lf",
                             id_,
                             pktInfo.getSource(),
                             pktInfo.getDestination(),
                             frequencySegment.getRxPowerdBm(),
                             dNoiseFloordB,
                             bSignalInNoise ? "yes" : "no",
                             dSINR);
    }
    catch (SpectrumServiceException &exp)
    {

      // to do statistics...

      LOGGER_VERBOSE_LOGGING(*pLogService_,
                             ERROR_LEVEL,
                             "MACI %03hu TDMACT::ReceiveManager upstream EOR processing: src %hu,"
                             " dst %hu, sor %ju, span %ju spectrum service request error: %s",
                             id_,
                             pktInfo.getSource(),
                             pktInfo.getDestination(),
                             std::chrono::duration_cast<Microseconds>(startOfReception.time_since_epoch()).count(),
                             span.count(),
                             exp.what());
      
      return toReturn;
    }

    // get por from sinr
    float fPOR = porManager_.getPOR(baseModelMessage.getDataRate(), dSINR, length);

    LOGGER_VERBOSE_LOGGING(*pLogService_,
                           DEBUG_LEVEL,
                           "MACI %03hu TDMACT::ReceiveManager upstream EOR processing 2 : src %hu,"
                           " dst %hu, datarate: %ju sinr: %lf length: %lu, por: %f",
                           id_,
                           pktInfo.getSource(),
                           pktInfo.getDestination(),
                           baseModelMessage.getDataRate(),
                           dSINR,
                           length,
                           fPOR);

    // get random value [0.0, 1.0]
    float fRandom{distribution_()};
    // check por
    if (fPOR < fRandom)
    {
      // update statistics
      LOGGER_VERBOSE_LOGGING(*pLogService_,
                             DEBUG_LEVEL,
                             "MACI %03hu TDMACT::ReceiveManager upstream EOR processing 3 : src %hu, dst %hu, "
                             "rxpwr %3.2f dBm, dropProcessRetransmissionSlot, fPOR: %f, fRandom: %f",
                             id_,
                             pktInfo.getSource(),
                             pktInfo.getDestination(),
                             frequencySegment.getRxPowerdBm(),
                             fPOR,
                             fRandom);
      // drop

    }
    else // we received the message
    {
      LOGGER_VERBOSE_LOGGING(*pLogService_,
                             DEBUG_LEVEL,
                             "MACI %03hu TDMACT::ReceiveManager upstream EOR processing 4 : src %hu, dst %hu, "
                             "rxpwr %3.2f dBm, receivedProcessRetransmissionSlot, fPOR: %f, fRandom: %f",
                             id_,
                             pktInfo.getSource(),
                             pktInfo.getDestination(),
                             frequencySegment.getRxPowerdBm(),
                             fPOR,
                             fRandom);
      if(!pendingReceived_) {
        pendingReceived_ = std::move(i);
        if(!toReturn) {
          toReturn = std::get<0>(*pendingReceived_);
        }
      }

      pNeighborMetricManager_->updateNeighborRxMetric(pktInfo.getSource(), // nbr (src)
                                                      u64SequenceNumber,   // sequence number
                                                      pktInfo.getUUID(),
                                                      dSINR,                           // sinr in dBm
                                                      dNoiseFloordB,                   // noise floor in dB
                                                      startOfReception,                // rx time
                                                      frequencySegment.getDuration(),  // duration
                                                      baseModelMessage.getDataRate()); // data rate bps
    }
  }
  pendingInfos_.clear();

  return toReturn;
}

void EMANE::Models::TDMACT::ReceiveManager::processFrame()
{
  CTLOG("REC %d processFrame\n", id_);
  if (pendingReceived_)
  {
    BaseModelMessage &baseModelMessage = std::get<0>(*pendingReceived_);
    PacketInfo pktInfo{std::get<1>(*pendingReceived_)};

    auto now = Clock::now();

    // deal with fragments
    for (const auto &message : baseModelMessage.getMessages())
    {
      NEMId dst{message.getDestination()};
      Priority priority{message.getPriority()};

      if (bPromiscuousMode_ ||
          (dst == id_) ||
          (dst == NEM_BROADCAST_MAC_ADDRESS))
      {
        const auto &data = message.getData();

        if (message.isFragment())
        {
          LOGGER_VERBOSE_LOGGING(*pLogService_,
                                DEBUG_LEVEL,
                                "MACI %03hu TDMACT::ReceiveManager upstream EOR processing:"
                                " src %hu, dst %hu, findex: %zu foffset: %zu fbytes: %zu"
                                " fmore: %s",
                                id_,
                                pktInfo.getSource(),
                                pktInfo.getDestination(),
                                message.getFragmentIndex(),
                                message.getFragmentOffset(),
                                data.size(),
                                message.isMoreFragments() ? "yes" : "no");

          auto key = std::make_tuple(pktInfo.getSource(),
                                    priority,
                                    message.getFragmentSequence());

          auto iter = fragmentStore_.find(key);

          if (iter != fragmentStore_.end())
          {
            auto &indexSet = std::get<0>(iter->second);
            auto &parts = std::get<1>(iter->second);
            auto &lastFragmentTime = std::get<2>(iter->second);

            if (indexSet.insert(message.getFragmentIndex()).second)
            {
              parts.insert(std::make_pair(message.getFragmentOffset(), message.getData()));

              lastFragmentTime = now;

              // check that all previous fragments have been received
              if (indexSet.size() == message.getFragmentIndex() + 1)
              {
                if (!message.isMoreFragments())
                {
                  Utils::VectorIO vectorIO{};

                  for (const auto &part : parts) {
                    vectorIO.push_back(Utils::make_iovec(const_cast<std::uint8_t *>(&part.second[0]),
                                                        part.second.size()));
                  }

                  UpstreamPacket pkt{{pktInfo.getSource(),
                                      dst,
                                      priority,
                                      pktInfo.getCreationTime(),
                                      pktInfo.getUUID()},
                                      vectorIO};

                  pPacketStatusPublisher_->inbound(pktInfo.getSource(),
                                                  dst,
                                                  priority,
                                                  pkt.length(),
                                                  PacketStatusPublisher::InboundAction::ACCEPT_GOOD);

                  if (message.getType() == MessageComponent::Type::DATA)
                  {
                    CTLOG("REC %d Sending upstream to transport\n", id_);
                    pDownstreamTransport_->sendUpstreamPacket(pkt);
                  }
                  else
                  {
                    // this should not happen, we have no control basemessages
                  }

                  fragmentStore_.erase(iter);
                }
              }
              else
              {
                // missing a fragment - record all bytes received and discontinue assembly
                size_t totalBytes{message.getData().size()};

                for (const auto &part : parts)
                {
                  totalBytes += part.second.size();
                }

                pPacketStatusPublisher_->inbound(pktInfo.getSource(),
                                                dst,
                                                priority,
                                                totalBytes,
                                                PacketStatusPublisher::InboundAction::DROP_MISS_FRAGMENT);

                // fragment was not received, abandon reassembly
                fragmentStore_.erase(iter);
              }
            }
          }
          else
          {
            // if the first fragment receieved is not index 0, fragments
            // were lost, so don't bother trying to reassemble
            if (!message.getFragmentIndex())
            {
              fragmentStore_.insert(std::make_pair(key,
                                                  std::make_tuple(std::set<size_t>{message.getFragmentIndex()},
                                                                  FragmentParts{{message.getFragmentOffset(),
                                                                                  message.getData()}},
                                                                  now,
                                                                  dst,
                                                                  priority)));
            }
            else
            {
              pPacketStatusPublisher_->inbound(pktInfo.getSource(),
                                              message,
                                              PacketStatusPublisher::InboundAction::DROP_MISS_FRAGMENT);
            }
          }
        }
        else
        {
          LOGGER_VERBOSE_LOGGING(*pLogService_,
                                DEBUG_LEVEL,
                                "MACI %03hu TDMACT::ReceiveManager upstream EOR processing:"
                                " src %hu, dst %hu, forward upstream",
                                id_,
                                pktInfo.getSource(),
                                pktInfo.getDestination());

          auto data = message.getData();

          UpstreamPacket pkt{{pktInfo.getSource(),
                              dst,
                              priority,
                              pktInfo.getCreationTime(),
                              pktInfo.getUUID()},
                            &data[0],
                            data.size()};

          pPacketStatusPublisher_->inbound(pktInfo.getSource(),
                                          message,
                                          PacketStatusPublisher::InboundAction::ACCEPT_GOOD);

          if (message.getType() == MessageComponent::Type::DATA)
          {
            CTLOG("REC %d Sending upstream to transport\n", id_);
            pDownstreamTransport_->sendUpstreamPacket(pkt);
          }
          else
          {
            // no control basemessages
          }
        }
      }
      else
      {
        pPacketStatusPublisher_->inbound(pktInfo.getSource(),
                                        message,
                                        PacketStatusPublisher::InboundAction::DROP_DESTINATION_MAC);
      }
    }
    // send upstream

    // check to see if there are fragment assemblies to abandon
    if (lastFragmentCheckTime_ + fragmentCheckThreshold_ <= now)
    {
      for (auto iter = fragmentStore_.begin(); iter != fragmentStore_.end();)
      {
        auto &parts = std::get<1>(iter->second);
        auto &lastFragmentTime = std::get<2>(iter->second);
        auto &dst = std::get<3>(iter->second);
        auto &priority = std::get<4>(iter->second);

        if (lastFragmentTime + fragmentTimeoutThreshold_ <= now)
        {
          size_t totalBytes{};

          for (const auto &part : parts)
          {
            totalBytes += part.second.size();
          }

          pPacketStatusPublisher_->inbound(std::get<0>(iter->first),
                                          dst,
                                          priority,
                                          totalBytes,
                                          PacketStatusPublisher::InboundAction::DROP_MISS_FRAGMENT);

          fragmentStore_.erase(iter++);
        }
        else
        {
          ++iter;
        }
      }
    }
    pendingReceived_ = std::experimental::nullopt; // in C++17 it can be simply .reset()
  }

  pendingInfos_.clear();
}

void EMANE::Models::TDMACT::ReceiveManager::clear()
{
  pendingInfos_.clear();
  pendingReceived_ = std::experimental::nullopt; // in C++17 it can be simply .reset()
}
