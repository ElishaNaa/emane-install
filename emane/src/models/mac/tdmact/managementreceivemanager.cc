#include "managementreceivemanager.h"
#include "emane/utils/spectrumwindowutils.h"
#include "devlog.h"

EMANE::Models::TDMACT::ManagementReceiveManager::ManagementReceiveManager(NEMId id,
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
                                                                                                                           distribution_{0.0, 1.0} {}

void EMANE::Models::TDMACT::ManagementReceiveManager::loadCurves(const std::string &sPCRFileName)
{
  porManager_.load(sPCRFileName);
}

void EMANE::Models::TDMACT::ManagementReceiveManager::enqueue(ManagementMessage &&managementMessage,
                                                              const PacketInfo &pktInfo,
                                                              size_t length,
                                                              const TimePoint &startOfReception,
                                                              const FrequencySegments &frequencySegments,
                                                              const Microseconds &span,
                                                              const TimePoint &beginTime,
                                                              std::uint64_t u64PacketSequence)
{
  pendingManagementInfos_.push_back(std::make_tuple(std::move(managementMessage),
                                                    pktInfo,
                                                    length,
                                                    startOfReception,
                                                    frequencySegments,
                                                    span,
                                                    beginTime,
                                                    u64PacketSequence));
}

std::experimental::optional<EMANE::Models::TDMACT::ManagementMessage> EMANE::Models::TDMACT::ManagementReceiveManager::processRetransmissionSlot()
{
  CTLOG("MRM %d size %lu\n", id_, pendingManagementInfos_.size());
  for (auto i : pendingManagementInfos_)
  {
    double dSINR{};
    double dNoiseFloordB{};

    ManagementMessage managementMessage = std::get<0>(i);
    PacketInfo pktInfo{std::get<1>(i)};
    size_t length{std::get<2>(i)};
    TimePoint &startOfReception = std::get<3>(i);
    FrequencySegments &frequencySegments = std::get<4>(i);
    Microseconds &span = std::get<5>(i);

    auto &frequencySegment = *frequencySegments.begin();

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
                             "MACI %03hu TDMACT::ManagementReceiveManager upstream EOR processing:"
                             " src %hu, dst %hu, max noise %lf, signal in noise %s, SINR %lf",
                             id_,
                             pktInfo.getSource(),
                             pktInfo.getDestination(),
                             dNoiseFloordB,
                             bSignalInNoise ? "yes" : "no",
                             dSINR);

      // get por from sinr

      float fPOR = porManager_.getPOR(managementMessage.getDataRate(), dSINR, length);
      LOGGER_VERBOSE_LOGGING(*pLogService_,
                             DEBUG_LEVEL,
                             "MACI %03hu TDMACT::ManagementReceiveManager upstream EOR processing: src %hu,"
                             " dst %hu, datarate: %ju sinr: %lf length: %lu, por: %f",
                             id_,
                             pktInfo.getSource(),
                             pktInfo.getDestination(),
                             managementMessage.getDataRate(),
                             dSINR,
                             length, fPOR);
      // get random value [0.0, 1.0]
      CTLOG("MRM %d sinr %lf length %lu datarate %lu por %f\n", id_, dSINR, length, managementMessage.getDataRate(), fPOR);
      float fRandom{distribution_()};
      // check por
      if (fPOR < fRandom)
      {
        // TODO statistics (sometime..)
        CTLOG("MRM %d lost\n", id_);
        LOGGER_VERBOSE_LOGGING(*pLogService_,
                               DEBUG_LEVEL,
                               "MACI %03hu TDMACT::ManagementReceiveManager upstream EOR processing: src %hu, dst %hu, "
                               "rxpwr %3.2f dBm, drop, fPOR: %f, fRandom: %f",
                               id_,
                               pktInfo.getSource(),
                               pktInfo.getDestination(),
                               frequencySegment.getRxPowerdBm(),
                               fPOR,
                               fRandom);

      }
      else
      {
        if (!pendingManagementWinner_ ||
            managementMessage.getPriorityBits() > pendingManagementWinner_->getPriorityBits() ||
            (managementMessage.getPriorityBits() == pendingManagementWinner_->getPriorityBits() &&
             managementMessage.getCongestionBits() > pendingManagementWinner_->getCongestionBits()) ||
            (managementMessage.getPriorityBits() == pendingManagementWinner_->getPriorityBits() &&
             managementMessage.getCongestionBits() == pendingManagementWinner_->getCongestionBits() &&
             managementMessage.getRandomBits() > pendingManagementWinner_->getRandomBits()))
        {
          pendingManagementWinner_ = managementMessage; // copy now, to optimize later
          //CTLOG("MRM %d pass\n", id_);
          // LOGGER_VERBOSE_LOGGING(*pLogService_,
          //                      DEBUG_LEVEL,
          //                      "MACI %03hu TDMACT::ManagementReceiveManager upstream EOR processing: src %hu, dst %hu, "
          //                      "rxpwr %3.2f dBm, passed, fPOR: %f, fRandom: %f",
          //                      id_,
          //                      pktInfo.getSource(),
          //                      pktInfo.getDestination(),
          //                      frequencySegment.getRxPowerdBm(),
          //                      fPOR,
          //                      fRandom);

        }
      }
    }
    catch (SpectrumServiceException &exp)
    {
      // TODO statistics (sometime..)

      LOGGER_VERBOSE_LOGGING(*pLogService_,
                             ERROR_LEVEL,
                             "MACI %03hu TDMACT::ManagementReceiveManager upstream EOR processing: src %hu,"
                             " dst %hu, sor %ju, span %ju spectrum service request error: %s",
                             id_,
                             pktInfo.getSource(),
                             pktInfo.getDestination(),
                             std::chrono::duration_cast<Microseconds>(startOfReception.time_since_epoch()).count(),
                             span.count(),
                             exp.what());

      pendingManagementInfos_.clear();
      return pendingManagementWinner_;
    }
  }

  pendingManagementInfos_.clear();
  CTLOG("MRM %d winner is from %d\n", id_, pendingManagementWinner_->getSource());
  return pendingManagementWinner_;
}

std::experimental::optional<EMANE::Models::TDMACT::ManagementMessage> EMANE::Models::TDMACT::ManagementReceiveManager::processFrame()
{
  // return the current max (priority+schedule)
  pendingManagementInfos_.clear();
  std::experimental::optional<EMANE::Models::TDMACT::ManagementMessage> toReturn;
  if (pendingManagementWinner_)
  {
    toReturn = std::move(*pendingManagementWinner_);
    pendingManagementWinner_ = std::experimental::nullopt; // in C++17 reset()
  }
  CTLOG("MRM %d  END FRAME winner is from %d\n", id_, pendingManagementWinner_->getSource());
  return toReturn;
}

void EMANE::Models::TDMACT::ManagementReceiveManager::setSelfMessage(ManagementMessage selfMessage)
{
  pendingManagementWinner_ = selfMessage;
}

void EMANE::Models::TDMACT::ManagementReceiveManager::clear()
{
  pendingManagementInfos_.clear();
  pendingManagementWinner_ = std::experimental::nullopt; // in C++17 reset()
}

/** enqueueings */

/** end enqueueings **/