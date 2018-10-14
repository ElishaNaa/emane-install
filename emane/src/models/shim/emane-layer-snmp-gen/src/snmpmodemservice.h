#ifndef EMANEMODEL_SNMPLAYER_MODEMSERVICE_HEADER_
#define EMANEMODEL_SNMPLAYER_MODEMSERVICE_HEADER_

#include "emane/shimlayerimpl.h"
#include "emane/utils/netutils.h"
#include "dlepMac.h"

#include "hiredisinclude/hiredis.h"

#include "emane/controls/r2rineighbormetriccontrolmessage.h"
#include "emane/controls/r2riqueuemetriccontrolmessage.h"
#include "emane/controls/r2riselfmetriccontrolmessage.h"
#include "emane/controls/flowcontrolcontrolmessage.h"

#include <chrono>

#include <map>

namespace EMANE
{
	// namespace R2RI
	// {
	//   namespace SNMP
	//   {
	/**
	* @class ModemService
	*
	* @brief Implements EMANE SNMP ModemService
	*/

	class ModemService
	{
	public:
		ModemService(NEMId id,
			PlatformServiceProvider *pPlatformService,
			RadioServiceProvider * pRadioServiceProvider);

		~ModemService();

		void handleControlMessages(const ControlMessages & controlMessages);

		void configure(const ConfigurationUpdate & update);

		void start();

		void destroy();

		//const ConfigParameterMapType & getConfigItems() const;

	private:
		redisContext *c;
		struct oid{
			std::string oidName;
			std::string oidValue;
		};
		std::list<oid> listOids; // cache
		std::uint64_t start_timer;
		std::uint64_t timeToUpdateRedis_; // interval to update redis 
		INETAddr addressRedis_;
		double upperBound_; 
		double lowerBound_; 
		struct NeighborInfo {
			std::uint64_t           lastTxDataRate_;
			std::uint64_t           lastRxDataRate_;

			float                   fSINRlast_;
			float                   fRRlast_;

			// DestinationMetricInfo   metrics_;

			LLSNMP::DlepMac macAddress_;

			NeighborInfo() :
				lastTxDataRate_{},
				lastRxDataRate_{},
				fSINRlast_{ -256.0f },
				fRRlast_{ 0.0f }
			{
				memset(&macAddress_, 0x0, sizeof(macAddress_));
			}
		};

		void UpdateRedis();

		void deleteOidFromList(const char * key);

		void handleMetricMessage_i(const Controls::R2RINeighborMetricControlMessage * pMessage);

		void handleMetricMessage_i(const Controls::R2RIQueueMetricControlMessage * pMessage);

		void handleMetricMessage_i(const Controls::R2RISelfMetricControlMessage * pMessage);

		void handleFlowControlMessage_i();

		void send_peer_update_i();

		void send_destination_update_i(const NeighborInfo & nbrInfo, bool isNewNbr);

		void send_destination_down_i(const NeighborInfo & nbrInfo);

		//void load_datarate_metrics_i(LLSNMP::DataItems & dataItems, const DataRateMetricInfo & values);

		//void load_destination_metrics_i(LLSNMP::DataItems & dataItems, const DestinationMetricInfo & values);

		int getRLQ_i(const std::uint16_t nbr, const float fSINRAvg, const size_t numRxFrames, const size_t numMissedFrames);

		// template <typename T>
		// LLSNMP::DataItem getDataItem_i(const std::string & id, const T & val)
		//  {
		//    return LLSNMP::DataItem{id, LLSNMP::DataItemValue{val}, pDlepClient_->get_protocol_config()};
		//  }


		LLSNMP::DlepMac getEthernetAddress_i(const NEMId id) const;

		// storage for NEM to ether mac addr mapping
		using NEMEtherAddrMap = std::map<NEMId, Utils::EtherAddr>;

		// storage for nbr info
		using NeighborInfoMap = std::map<NEMId, NeighborInfo>;

		NEMId id_;

		PlatformServiceProvider * pPlatformService_;

		RadioServiceProvider * pRadioService_;

		NeighborInfoMap currentNeighbors_;

		NEMEtherAddrMap nemEtherAddrMap_;

		Utils::EtherAddr etherOUI_;

		Utils::EtherAddr etherBroadcastAddr_;

		//std::unique_ptr<DlepClientImpl> pDlepClient_;

		//ConfigParameterMapType snmpConfiguration_;

		// DataRateMetricInfo selfMetrics_;

		std::uint64_t avgQueueDelayMicroseconds_;

		float fSINRMin_;

		float fSINRMax_;

		bool destinationAdvertisementEnable_;
	};
	//   }
	// }
}

#endif