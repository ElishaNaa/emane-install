/*
* Copyright (c) 2018 -Elisha Naaman
*/

#include "snmpmodemservice.h"

#include <cstdint>
#include <set>
#include <string>
#include <sstream>
#include <vector>

#include "emane/configureexception.h"
#include "emane/utils/parameterconvert.h"
#include "emane/controls/serializedcontrolmessage.h"
#include "emane/controls/r2rineighbormetriccontrolmessageformatter.h"
#include "emane/controls/r2riqueuemetriccontrolmessageformatter.h"
#include "emane/controls/r2riselfmetriccontrolmessageformatter.h"

namespace
{
	const char * __MODULE__ = "SNMP::ModemService";

	const char * EtherAddrFormat = "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx";

	const char * EtherOUIFormat = "%02hhx:%02hhx:%02hhx";

	const char * _PLACE_ = "_WAVE_FROM_ROOT_";

	template <typename T>
	T clampit(T min, T max, T val)
	{
		if (val < min)
		{
			return min;
		}
		else if (val > max)
		{
			return max;
		}
		else
		{
			return val;
		}
	}

}


EMANE::ModemService::ModemService(NEMId id,
	PlatformServiceProvider * pPlatformService,
	RadioServiceProvider * pRadioService) :
	id_{ id },
	pPlatformService_{ pPlatformService },
	pRadioService_{ pRadioService },
	//pDlepClient_{},
	avgQueueDelayMicroseconds_{},
	fSINRMin_{ 0.0f },
	fSINRMax_{ 20.0f },
	destinationAdvertisementEnable_{ false }
{
	memset(&etherOUI_, 0x0, sizeof(etherOUI_));

	memset(&etherBroadcastAddr_, 0xFF, sizeof(etherBroadcastAddr_));

}



EMANE::ModemService::~ModemService()
{ }


void EMANE::ModemService::configure(const ConfigurationUpdate & update)
{
	LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
		DEBUG_LEVEL,
		"SHIMI %03hu %s::%s", id_, __MODULE__, __func__);

	for (const auto & item : update)
	{
		if (item.first == "macaddress")
		{
			for (const auto & any : item.second)
			{
				const std::string str{ any.asString() };

				const size_t pos{ str.find(" ") };

				if (pos != std::string::npos)
				{
					const std::uint16_t nem{ EMANE::Utils::ParameterConvert(std::string{ str.data(), pos }).toUINT16() };

					EMANE::Utils::EtherAddr eth;

					if (sscanf(str.data() + pos + 1,
						EtherAddrFormat,
						eth.bytes.buff + 0,
						eth.bytes.buff + 1,
						eth.bytes.buff + 2,
						eth.bytes.buff + 3,
						eth.bytes.buff + 4,
						eth.bytes.buff + 5) != 6)
					{
						throw makeException<ConfigureException>(__MODULE__,
							"Invalid configuration item %s, expected <NEMId> <%s>",
							item.first.c_str(),
							EtherAddrFormat);
					}

					if (nemEtherAddrMap_.insert(std::make_pair(nem, eth)).second)
					{
						LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
							INFO_LEVEL,
							"SHIMI %03hu %s::%s %s = %s",
							id_,
							__MODULE__,
							__func__,
							item.first.c_str(),
							str.c_str());

					}
					else
					{
						throw makeException<ConfigureException>(__MODULE__,
							"Invalid configuration item %s, duplicate NEMId %hu",
							item.first.c_str(), nem);
					}
				}
				else
				{
					throw makeException<ConfigureException>(__MODULE__,
						"Invalid configuration item %s, expected <NEMId> <%s>",
						item.first.c_str(),
						EtherAddrFormat);
				}
			}
		}
		else if (item.first == "etheraddroui")
		{
			const std::string str{ item.second[0].asString() };

			if (sscanf(str.data(),
				EtherOUIFormat,
				etherOUI_.bytes.buff + 0,
				etherOUI_.bytes.buff + 1,
				etherOUI_.bytes.buff + 2) != 3)
			{
				throw makeException<ConfigureException>(__MODULE__,
					"Invalid configuration item %s, expected prefix format <%s>",
					item.first.c_str(),
					EtherOUIFormat);
			}
			else
			{
				LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
					INFO_LEVEL,
					"SHIMI %03hu %s::%s %s = %s",
					id_,
					__MODULE__,
					__func__,
					item.first.c_str(),
					str.c_str());
			}
		}
		else if (item.first == "sinrmin")
		{
			fSINRMin_ = item.second[0].asFloat();

			LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
				INFO_LEVEL,
				"SHIMI %03d %s::%s %s = %f dBm",
				id_,
				__MODULE__,
				__func__,
				item.first.c_str(),
				fSINRMin_);
		}
		else if (item.first == "sinrmax")
		{
			fSINRMax_ = item.second[0].asFloat();

			LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
				INFO_LEVEL,
				"SHIMI %03d %s::%s %s = %f dBm",
				id_,
				__MODULE__,
				__func__,
				item.first.c_str(),
				fSINRMax_);
		}
		else if (item.first == "addressRedis")
		{
			addressRedis_ = item.second[0].asINETAddr();

			LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
				INFO_LEVEL,
				"SHIMI %03hu %s::%s %s=%s",
				id_,
				__MODULE__,
				__func__,
				item.first.c_str(),
				addressRedis_.str(false).c_str());
		}
		else
		{
			const std::string str{ item.second[0].asString() };

			LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
				INFO_LEVEL,
				"SHIMI %03hu %s::%s %s = %s",
				id_,
				__MODULE__,
				__func__,
				item.first.c_str(),
				str.c_str());
		}
	}
}


void EMANE::ModemService::start()
{
	LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
		DEBUG_LEVEL,
		"SHIMI %03hu %s::%s", id_, __MODULE__, __func__);

	// sinr saniity check
	if (fSINRMin_ > fSINRMax_)
	{
		throw makeException<ConfigureException>(__MODULE__,
			"sinrmin > ",
			std::to_string(fSINRMin_).c_str(),
			" sinrmax",
			std::to_string(fSINRMax_).c_str());
	}
	else
	{

	}

	std::string str_address = addressRedis_.str(false).c_str();
	std::uint32_t myaddress = inet_addr(str_address.c_str());
	const int NBYTES = 4;
	std::uint8_t octet[NBYTES];
	char ipAddressFinal[16];
	for (int i = 0; i < NBYTES; i++)
	{
		octet[i] = myaddress >> (i * 8);
	}
	sprintf(ipAddressFinal, "%d.%d.%d.%d", octet[0], octet[1], octet[2], octet[3]);
	const char *hostname = ipAddressFinal;
	int port = 6379;
	struct timeval timeout = { 1, 500000 }; // 1.5 seconds
	c = redisConnectWithTimeout(hostname, port, timeout);
	// didnt connected to redis
	if (c == NULL || c->err)
	{
		if (c) {
			LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
				ERROR_LEVEL,
				"SHIMI %03hu %s::%s Connection error: %s",
				id_, __MODULE__, __func__, c->errstr);
			redisFree(c);
		}
		else {
			LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
				ERROR_LEVEL,
				"SHIMI %03hu %s::%s Connection error: can't allocate redis context",
				id_, __MODULE__, __func__);
		}

	}
	else
	{
		LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
			DEBUG_LEVEL,
			"SHIMI %03hu %s::%s Connection Successful",
			id_, __MODULE__, __func__);
	}
}



void EMANE::ModemService::destroy()
{
	LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
		DEBUG_LEVEL,
		"SHIMI %03hu %s::%s", id_, __MODULE__, __func__);

	redisFree(c);
}



void EMANE::ModemService::handleControlMessages(const EMANE::ControlMessages & controlMessages)
{
	try {
		for (const auto & pMessage : controlMessages)
		{
			switch (pMessage->getId())
			{
			case EMANE::Controls::R2RINeighborMetricControlMessage::IDENTIFIER:
			{
				handleMetricMessage_i(reinterpret_cast<const EMANE::Controls::R2RINeighborMetricControlMessage *>(pMessage));
			}
			break;

			case EMANE::Controls::R2RIQueueMetricControlMessage::IDENTIFIER:
			{
				handleMetricMessage_i(reinterpret_cast<const EMANE::Controls::R2RIQueueMetricControlMessage *>(pMessage));
			}
			break;

			case EMANE::Controls::R2RISelfMetricControlMessage::IDENTIFIER:
			{
				handleMetricMessage_i(reinterpret_cast<const EMANE::Controls::R2RISelfMetricControlMessage *>(pMessage));
			}
			break;

			case EMANE::Controls::FlowControlControlMessage::IDENTIFIER:
			{
				handleFlowControlMessage_i();
			}
			break;

			case EMANE::Controls::SerializedControlMessage::IDENTIFIER:
			{
				const auto pSerializedControlMessage =
					static_cast<const EMANE::Controls::SerializedControlMessage *>(pMessage);

				switch (pSerializedControlMessage->getSerializedId())
				{
				case EMANE::Controls::R2RINeighborMetricControlMessage::IDENTIFIER:
				{
					auto p = EMANE::Controls::R2RINeighborMetricControlMessage::create(
						pSerializedControlMessage->getSerialization());

					handleMetricMessage_i(p);

					delete p;
				}
				break;

				case EMANE::Controls::R2RIQueueMetricControlMessage::IDENTIFIER:
				{
					auto p = EMANE::Controls::R2RIQueueMetricControlMessage::create(
						pSerializedControlMessage->getSerialization());

					handleMetricMessage_i(p);

					delete p;
				}
				break;

				case EMANE::Controls::R2RISelfMetricControlMessage::IDENTIFIER:
				{
					auto p = EMANE::Controls::R2RISelfMetricControlMessage::create(
						pSerializedControlMessage->getSerialization());

					handleMetricMessage_i(p);

					delete p;
				}
				break;

				case EMANE::Controls::FlowControlControlMessage::IDENTIFIER:
				{
					handleFlowControlMessage_i();
				}
				break;
				}
			}
			break;
			}
		}
	}

	catch (const std::exception & ex)
	{
		LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
			ERROR_LEVEL,
			"SHIMI %03hu %s::%s caught std exception %s",
			id_,
			__MODULE__,
			__func__,
			ex.what());
	}
}



// self metrics
void EMANE::ModemService::handleMetricMessage_i(const EMANE::Controls::R2RISelfMetricControlMessage * pMessage)
{
	LOGGER_STANDARD_LOGGING_FN_VARGS(pPlatformService_->logService(),
		DEBUG_LEVEL,
		EMANE::Controls::R2RISelfMetricControlMessageFormatter(pMessage),
		"SHIMI %03hu %s::%s R2RISelfMetricControlMessage",
		id_, __MODULE__, __func__);

	redisReply *reply;
	// SET datarate
	std::string mibdr = std::string(_PLACE_) + "CURRENT_DATA_RATE"; // old ---> .1.3.6.1.4.1.16215.1.24.1.4.6"
	std::string strkeydr = std::to_string(id_).c_str() + mibdr;
	const char *mackey = strkeydr.c_str();
	const char *val = (std::to_string(pMessage->getMaxDataRatebps())).c_str();
	reply = static_cast<redisReply*>(redisCommand(c, "SET %s %s", mackey, val));
	LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
		INFO_LEVEL,
		"SHIMI %03hu %s::%s SET datarate: %s, KEY: %s VAL: %s",
		id_, __MODULE__, __func__, reply->str, mackey, val);
	freeReplyObject(reply);

}



// neighbor metrics
void EMANE::ModemService::handleMetricMessage_i(const EMANE::Controls::R2RINeighborMetricControlMessage * pMessage)
{
	LOGGER_STANDARD_LOGGING_FN_VARGS(pPlatformService_->logService(),
		DEBUG_LEVEL,
		EMANE::Controls::R2RINeighborMetricControlMessageFormatter(pMessage),
		"SHIMI %03hu %s::%s R2RINeighborMetricControlMessage",
		id_, __MODULE__, __func__);

	const EMANE::Controls::R2RINeighborMetrics metrics{ pMessage->getNeighborMetrics() };
	// for deleting nbrs loop
	const EMANE::Controls::R2RINeighborMetrics delmetrics{ pMessage->getNeighborMetrics() };
	// snmp: if there nbrs to delete, first find them. If there are, delete all nbrs, and then immediatly, update.
	// possible set of nbrs to delete
	auto delNeighbors = currentNeighbors_;
	for (const auto & metric : delmetrics)
	{
		const auto nbr = metric.getId();
		auto iter = currentNeighbors_.find(nbr);
		// update nbr -> do not remove it
		if (!(iter == currentNeighbors_.end()))
		{
			// still active, remove from the candidate delete set
			delNeighbors.erase(nbr);
		}
	}
	uint16_t metricDelIndex = 1; // for snmp DELs. delleting nbrs by numerical order, not by the nbr's id order. for snmmp walk flow
								 // loop on currentNeighbors_ to earase all current nbrs
	if (!delNeighbors.empty())
	{
		for (const auto & nbr : currentNeighbors_)
		{
			LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
				INFO_LEVEL,
				"SHIMI %03hu %s::%s no longer reported, delete nbr %hu",
				id_, __MODULE__, __func__, nbr.first);

			redisReply *reply;
			
			/*
			// del the nbr macaddr mib
			// its enough to delete only that one, because passtest counting the nbrs by this mib
			std::string mibmac = std::string(_PLACE_) + "NEIGHBOUR_MAC_ADDRESS_"; // old ---> ".1.3.6.1.4.1.16215.1.24.1.4.2.1.15.1.1."
			std::string strkeymac = std::to_string(id_).c_str() + mibmac + std::to_string(metricDelIndex).c_str();
			const char *mackey = strkeymac.c_str();
			reply = static_cast<redisReply*>(redisCommand(c, "DEL %s", mackey));
			LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
				INFO_LEVEL,
				"SHIMI %03hu %s::%s DEL NBR MACADDR: %hu, KEY: %s ",
				id_, __MODULE__, __func__, reply->integer, mackey);
			freeReplyObject(reply);


			// del the member macaddr mib
			// its enough to delete only that one, because passtest counting the members by this mib
			mibmac = std::string(_PLACE_) + "MEMBERS_MAC_ADDRESS_"; // old ---> ".1.3.6.1.4.1.16215.1.24.1.4.2.16.1.1."
			strkeymac = std::to_string(id_).c_str() + mibmac + std::to_string(metricDelIndex).c_str();
			mackey = strkeymac.c_str();
			reply = static_cast<redisReply*>(redisCommand(c, "DEL %s", mackey));
			LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
				INFO_LEVEL,
				"SHIMI %03hu %s::%s DEL MEMBER MACADDR: %hu, KEY: %s ",
				id_, __MODULE__, __func__, reply->integer, mackey);
			freeReplyObject(reply);
			*/


			/////////////////////////////////////////////////////////////


			// del the nbr macaddr mib
			// its enough to delete only that one, because passtest counting the nbrs by this mib
			std::string mibmac = std::string(_PLACE_) + "NEIGHBOUR_MAC_ADDRESS_"; // old ---> ".1.3.6.1.4.1.16215.1.24.1.4.2.1.15.1.1."
			std::string strkeymac = std::to_string(id_).c_str() + mibmac + std::to_string(metricDelIndex).c_str();
			const char *nbrmackey = strkeymac.c_str();

			// del the member macaddr mib
			// its enough to delete only that one, because passtest counting the members by this mib
			mibmac = std::string(_PLACE_) + "MEMBERS_MAC_ADDRESS_"; // old ---> ".1.3.6.1.4.1.16215.1.24.1.4.2.16.1.1."
			strkeymac = std::to_string(id_).c_str() + mibmac + std::to_string(metricDelIndex).c_str();
			const char *membermackey = strkeymac.c_str();

			reply = static_cast<redisReply*>(redisCommand(c, "DEL %s, %s", nbrmackey, membermackey));

			LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
				INFO_LEVEL,
				"SHIMI %03hu %s::%s DEL NBR MACADDR: %hu, KEY: %s ",
				id_, __MODULE__, __func__, reply->integer, nbrmackey);

			LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
				INFO_LEVEL,
				"SHIMI %03hu %s::%s DEL MEMBER MACADDR: %hu, KEY: %s ",
				id_, __MODULE__, __func__, reply->integer, membermackey);
			freeReplyObject(reply);



			/////////////////////////////////////////////////////////////

			metricDelIndex++;
		}
		currentNeighbors_.clear();
	}

	uint16_t metricSetIndex = 1; // for snmp SETs. setting nbrs by numerical order, not by the nbr's id order. for snmmp walk flow

	for (const auto & metric : metrics)
	{
		bool isNewNbr = false;

		const auto nbr = metric.getId();

		auto rxDataRate = metric.getRxAvgDataRatebps();

		auto txDataRate = metric.getTxAvgDataRatebps();

		auto iter = currentNeighbors_.find(nbr);

		// new nbr
		if (iter == currentNeighbors_.end())
		{
			NeighborInfo nbrInfo;

			// set is a new nbr
			isNewNbr = true;

			// set ether mac addr
			nbrInfo.macAddress_ = getEthernetAddress_i(nbr);

			// set Rx data rate
			// nbrInfo.lastRxDataRate_ = rxDataRate ? rxDataRate : selfMetrics_.valCurrentDataRateRx;

			// set Tx data rate
			// nbrInfo.lastTxDataRate_ = rxDataRate ? rxDataRate : selfMetrics_.valCurrentDataRateRx;

			LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
				DEBUG_LEVEL,
				"SHIMI %03hu %s::%s new nbr %hu, ether addr %s, txdr %lu, rxdr %lu",
				id_,
				__MODULE__,
				__func__,
				nbr,
				nbrInfo.macAddress_.to_string().c_str(),
				nbrInfo.lastRxDataRate_,
				nbrInfo.lastTxDataRate_);

			// add to the current set
			iter = currentNeighbors_.insert(std::make_pair(nbr, nbrInfo)).first;
		}
		// update nbr
		else
		{
			// still active, remove from the candidate delete set
			delNeighbors.erase(nbr);

			// update Rx data rate
			if (rxDataRate > 0)
			{
				iter->second.lastRxDataRate_ = rxDataRate;
			}

			// update Tx data rate
			if (txDataRate > 0)
			{
				iter->second.lastTxDataRate_ = txDataRate;
			}

			LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
				DEBUG_LEVEL,
				"SHIMI %03hu %s::%s update nbr %hu, txdr %lu, rxdr %lu",
				id_,
				__MODULE__,
				__func__,
				nbr,
				iter->second.lastRxDataRate_,
				iter->second.lastTxDataRate_);
		}

		// update destination metrics
		// iter->second.metrics_.dataRateInfo.valCurrentDataRateRx = iter->second.lastRxDataRate_;

		// iter->second.metrics_.dataRateInfo.valCurrentDataRateTx = iter->second.lastTxDataRate_;

		// iter->second.metrics_.dataRateInfo.valMaxDataRateRx     = selfMetrics_.valMaxDataRateRx;

		// iter->second.metrics_.dataRateInfo.valMaxDataRateTx     = selfMetrics_.valMaxDataRateTx;

		// iter->second.metrics_.valLatency = avgQueueDelayMicroseconds_;

		// iter->second.metrics_.valResources = 50;

		// iter->second.metrics_.valRLQTx =
		//   iter->second.metrics_.valRLQRx = getRLQ_i(metric.getId(),
		//                                             metric.getSINRAvgdBm(),
		//                                             metric.getNumRxFrames(),
		//                                             metric.getNumMissedFrames());

#if 0
		// push remote lan up to router example
		iter->second.metrics_.valIPv4AdvLan =
			LLSNMP::Div_u8_ipv4_u8_t{ LLSNMP::DataItem::IPFlags::add,
			boost::asio::ip::address_v4::from_string("0.0.0.0"),
			32 };
#endif
		/*
		// send nbr up/update
		//send_destination_update_i(iter->second, isNewNbr);

		redisReply *reply;
		// SET NEIGHBOURS MAC ADDR
		// node_id.PLACE.nbr_id
		std::string mibmac = std::string(_PLACE_) + "NEIGHBOUR_MAC_ADDRESS_"; // old ---> ".1.3.6.1.4.1.16215.1.24.1.4.2.1.15.1.1."
		std::string strkeymac = std::to_string(id_).c_str() + mibmac + std::to_string(metricSetIndex).c_str();
		const char *mackey = strkeymac.c_str();
		const std::string strvalmac = getEthernetAddress_i(nbr).to_string().c_str();

		// convert mac addr to int64
		// now the MAC is 00:00:00:00:00:# (number of node)
		// to get effective converting, the first 4 bytes value will be changed to "CA:FE"

		std::string strvalmacint64;
		std::string cafeaddr = "CA:FE:00:00:00:00";
		unsigned char real[6];
		unsigned char cafe[6];
		int last = -1;
		int rcreal = sscanf(strvalmac.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx%n",
			real + 0, real + 1, real + 2, real + 3, real + 4, real + 5, &last);
		int rccafe = sscanf(cafeaddr.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx%n",
			cafe + 0, cafe + 1, cafe + 2, cafe + 3, cafe + 4, cafe + 5, &last);
		if (rcreal != 6 || strvalmac.size() != last)
		{
			strvalmacint64 = "0";
		}
		else
		{
			uint64_t mac64 = uint64_t(cafe[0]) << 40 |
				uint64_t(cafe[1]) << 32 |
				uint64_t(cafe[0]) << 24 |
				uint64_t(cafe[1]) << 16 |
				uint64_t(real[4]) << 8 |
				uint64_t(real[5]);
			strvalmacint64 = std::to_string(mac64);
		}

		const char *macint64val = strvalmacint64.c_str();

		reply = static_cast<redisReply*>(redisCommand(c, "SET %s %s", mackey, macint64val));

		LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
			INFO_LEVEL,
			"SHIMI %03hu %s::%s SET NEIGHBOURS MACADDR: %s, KEY: %s, VAL: %s",
			id_, __MODULE__, __func__, reply->str, mackey, macint64val);
		freeReplyObject(reply);


		// SET NEIGHBOURS Link Quality
		float LQ = getRLQ_i(metric.getId(),
			metric.getSINRAvgdBm(),
			metric.getNumRxFrames(),
			metric.getNumMissedFrames());
		std::string mibLQ = std::string(_PLACE_) + "NEIGHBOUR_LINK_QUALITY_"; // old ---> ".1.3.6.1.4.1.16215.1.24.1.4.2.1.15.1.2."
		std::string strkeyLQ = std::to_string(id_).c_str() + mibLQ + std::to_string(metricSetIndex).c_str();
		const char *LQkey = strkeyLQ.c_str();
		std::string strvalLQ = (std::to_string(LQ)).c_str();
		const char *LQval = strvalLQ.c_str();

		reply = static_cast<redisReply*>(redisCommand(c, "SET %s %s", LQkey, LQval));

		LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
			INFO_LEVEL,
			"SHIMI %03hu %s::%s SET NEIGHBOURS Link-Quality: %s, KEY: %s, VAL: %s",
			id_, __MODULE__, __func__, reply->str, LQkey, LQval);
		freeReplyObject(reply);


		// SET NEIGHBOURS RSSI (In that implemintation RSSI is SINR)
		std::string mibrssi = std::string(_PLACE_) + "NEIGHBOUR_RSSI_"; // old ---> ".1.3.6.1.4.1.16215.1.24.1.4.2.1.15.1.3."
		std::string strkeyrssi = std::to_string(id_).c_str() + mibrssi + std::to_string(metricSetIndex).c_str();
		const char *rssikey = strkeyrssi.c_str();
		std::string strvalrssi = (std::to_string(metric.getSINRAvgdBm())).c_str();
		const char *rssival = strvalrssi.c_str();

		reply = static_cast<redisReply*>(redisCommand(c, "SET %s %s", rssikey, rssival));

		LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
			INFO_LEVEL,
			"SHIMI %03hu %s::%s SET NEIGHBOURS RSSI: %s, KEY: %s, VAL: %s",
			id_, __MODULE__, __func__, reply->str, rssikey, rssival);
		freeReplyObject(reply);


		// SET NEIGHBOURS SINR
		std::string mibsinr = std::string(_PLACE_) + "NEIGHBOUR_SNR_"; // old ---> ".1.3.6.1.4.1.16215.1.24.1.4.2.1.15.1.4."
		std::string strkeysinr = std::to_string(id_).c_str() + mibsinr + std::to_string(metricSetIndex).c_str();
		const char *sinrkey = strkeysinr.c_str();
		std::string strvalsinr = (std::to_string(metric.getSINRAvgdBm())).c_str();
		const char *sinrval = strvalsinr.c_str();

		reply = static_cast<redisReply*>(redisCommand(c, "SET %s %s", sinrkey, sinrval));

		LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
			INFO_LEVEL,
			"SHIMI %03hu %s::%s SET NEIGHBOURS SINR: %s, KEY: %s, VAL: %s",
			id_, __MODULE__, __func__, reply->str, sinrkey, sinrval);
		freeReplyObject(reply);


		// SET NEIGHBOURS busyRate
		std::string mibbusyrate = std::string(_PLACE_) + "NEIGHBOUR_BUSY_RATE_"; // old ---> ".1.3.6.1.4.1.16215.1.24.1.4.2.1.15.1.5."
		std::string strkeybusyrate = std::to_string(id_).c_str() + mibbusyrate + std::to_string(metricSetIndex).c_str();
		const char *busyratekey = strkeybusyrate.c_str();
		const char *busyrateval = "50";
		reply = static_cast<redisReply*>(redisCommand(c, "SET %s %s", busyratekey, busyrateval));

		LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
			INFO_LEVEL,
			"SHIMI %03hu %s::%s SET NEIGHBOURS busyRate: %s, KEY: %s, VAL: %s",
			id_, __MODULE__, __func__, reply->str, busyratekey, busyrateval);
		freeReplyObject(reply);


		// SET NEIGHBOURS MemberRank
		std::string mibMemberRank = std::string(_PLACE_) + "NEIGHBOUR_MEMBER_RANK_"; // old ---> ".1.3.6.1.4.1.16215.1.24.1.4.2.1.15.1.6."
		std::string strkeyMemberRank = std::to_string(id_).c_str() + mibMemberRank + std::to_string(metricSetIndex).c_str();
		const char *memberRankkey = strkeyMemberRank.c_str();
		const char *memberRankval = "99";
		reply = static_cast<redisReply*>(redisCommand(c, "SET %s %s", memberRankkey, memberRankval));

		LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
			INFO_LEVEL,
			"SHIMI %03hu %s::%s SET NEIGHBOURS MemberRabk: %s, KEY: %s, VAL: %s",
			id_, __MODULE__, __func__, reply->str, memberRankkey, memberRankval);
		freeReplyObject(reply);

		// SET MEMBERS MAC ADDR
		// node_id.PLACE.nbr_id
		mibmac = std::string(_PLACE_) + "MEMBERS_MAC_ADDRESS_"; // old ---> ".1.3.6.1.4.1.16215.1.24.1.4.2.16.1.1."
		strkeymac = std::to_string(id_).c_str() + mibmac + std::to_string(metricSetIndex).c_str();
		mackey = strkeymac.c_str();
		std::string strvalmac2 = getEthernetAddress_i(nbr).to_string().c_str();

		// convert mac addr to int64
		// now the MAC is 00:00:00:00:00:# (number of node)
		// to get effective converting, the first 4 bytes value will be changed to "CA:FE"

		strvalmacint64;
		cafeaddr = "CA:FE:00:00:00:00";
		real[6];
		cafe[6];
		last = -1;
		rcreal = sscanf(strvalmac2.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx%n",
			real + 0, real + 1, real + 2, real + 3, real + 4, real + 5, &last);
		rccafe = sscanf(cafeaddr.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx%n",
			cafe + 0, cafe + 1, cafe + 2, cafe + 3, cafe + 4, cafe + 5, &last);
		if (rcreal != 6 || strvalmac2.size() != last)
		{
			strvalmacint64 = "0";
		}
		else
		{
			uint64_t mac64 = uint64_t(cafe[0]) << 40 |
				uint64_t(cafe[1]) << 32 |
				uint64_t(cafe[0]) << 24 |
				uint64_t(cafe[1]) << 16 |
				uint64_t(real[4]) << 8 |
				uint64_t(real[5]);
			strvalmacint64 = std::to_string(mac64);
		}

		macint64val = strvalmacint64.c_str();

		reply = static_cast<redisReply*>(redisCommand(c, "SET %s %s", mackey, macint64val));

		LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
			INFO_LEVEL,
			"SHIMI %03hu %s::%s SET MEMBERS MACADDR: %s, KEY: %s, VAL: %s",
			id_, __MODULE__, __func__, reply->str, mackey, macint64val);
		freeReplyObject(reply);


		// SET MEMBERS Link Quality
		LQ = getRLQ_i(metric.getId(),
			metric.getSINRAvgdBm(),
			metric.getNumRxFrames(),
			metric.getNumMissedFrames());
		mibLQ = std::string(_PLACE_) + "MEMBERS_RACE_ID_"; // old ---> ".1.3.6.1.4.1.16215.1.24.1.4.2.16.1.2."
		strkeyLQ = std::to_string(id_).c_str() + mibLQ + std::to_string(metricSetIndex).c_str();
		LQkey = strkeyLQ.c_str();
		strvalLQ = (std::to_string(LQ)).c_str();
		LQval = strvalLQ.c_str();

		reply = static_cast<redisReply*>(redisCommand(c, "SET %s %s", LQkey, LQval));

		LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
			INFO_LEVEL,
			"SHIMI %03hu %s::%s SET MEMBERS Link-Quality: %s, KEY: %s, VAL: %s",
			id_, __MODULE__, __func__, reply->str, LQkey, LQval);
		freeReplyObject(reply);


		// SET MEMBERS RSSI (In that implemintation RSSI is SINR)
		mibrssi = std::string(_PLACE_) + "MEMBERS_RSSI_"; // old ---> ".1.3.6.1.4.1.16215.1.24.1.4.2.16.1.3."
		strkeyrssi = std::to_string(id_).c_str() + mibrssi + std::to_string(metricSetIndex).c_str();
		rssikey = strkeyrssi.c_str();
		strvalrssi = (std::to_string(metric.getSINRAvgdBm())).c_str();
		rssival = strvalrssi.c_str();

		reply = static_cast<redisReply*>(redisCommand(c, "SET %s %s", rssikey, rssival));

		LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
			INFO_LEVEL,
			"SHIMI %03hu %s::%s SET MEMBERS RSSI: %s, KEY: %s, VAL: %s",
			id_, __MODULE__, __func__, reply->str, rssikey, rssival);
		freeReplyObject(reply);


		// SET MEMBERS SINR
		mibsinr = std::string(_PLACE_) + "MEMBERS_HOP_"; // old ---> ".1.3.6.1.4.1.16215.1.24.1.4.2.16.1.4."
		strkeysinr = std::to_string(id_).c_str() + mibsinr + std::to_string(metricSetIndex).c_str();
		sinrkey = strkeysinr.c_str();
		strvalsinr = (std::to_string(metric.getSINRAvgdBm())).c_str();
		sinrval = strvalsinr.c_str();

		reply = static_cast<redisReply*>(redisCommand(c, "SET %s %s", sinrkey, sinrval));

		LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
			INFO_LEVEL,
			"SHIMI %03hu %s::%s SET MEMBERS SINR: %s, KEY: %s, VAL: %s",
			id_, __MODULE__, __func__, reply->str, sinrkey, sinrval);
		freeReplyObject(reply);

		reply = static_cast<redisReply*>(redisCommand(c, "DEL mykey"));
		freeReplyObject(reply);
		*/


		////////////////////////////////////////////////////////////////////////////



		// send nbr up/update
		//send_destination_update_i(iter->second, isNewNbr);

		redisReply *reply;
		// SET NEIGHBOURS MAC ADDR
		// node_id.PLACE.nbr_id
		std::string nbrmibmac = std::string(_PLACE_) + "NEIGHBOUR_MAC_ADDRESS_"; // old ---> ".1.3.6.1.4.1.16215.1.24.1.4.2.1.15.1.1."
		std::string nbrstrkeymac = std::to_string(id_).c_str() + nbrmibmac + std::to_string(metricSetIndex).c_str();
		const char *nbrmackey = nbrstrkeymac.c_str();
		const std::string strvalmac = getEthernetAddress_i(nbr).to_string().c_str();

		// convert mac addr to int64
		// now the MAC is 00:00:00:00:00:# (number of node)
		// to get effective converting, the first 4 bytes value will be changed to "CA:FE"

		std::string strvalmacint64;
		std::string cafeaddr = "CA:FE:00:00:00:00";
		unsigned char real[6];
		unsigned char cafe[6];
		int last = -1;
		int rcreal = sscanf(strvalmac.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx%n",
			real + 0, real + 1, real + 2, real + 3, real + 4, real + 5, &last);
		int rccafe = sscanf(cafeaddr.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx%n",
			cafe + 0, cafe + 1, cafe + 2, cafe + 3, cafe + 4, cafe + 5, &last);
		if (rcreal != 6 || strvalmac.size() != last)
		{
			/*LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
			ERROR_LEVEL,
			"SHIMI %03hu %s::%s couldn't conver mac addr %s to int64",
			id_, __MODULE__, __func__, strvalmac);*/
			strvalmacint64 = "0";
		}
		else
		{
			uint64_t mac64 = uint64_t(cafe[0]) << 40 |
				uint64_t(cafe[1]) << 32 |
				uint64_t(cafe[0]) << 24 |
				uint64_t(cafe[1]) << 16 |
				uint64_t(real[4]) << 8 |
				uint64_t(real[5]);
			strvalmacint64 = std::to_string(mac64);
		}

		const char *nbrmacint64val = strvalmacint64.c_str();

		// SET NEIGHBOURS Link Quality
		float LQ = getRLQ_i(metric.getId(),
			metric.getSINRAvgdBm(),
			metric.getNumRxFrames(),
			metric.getNumMissedFrames());
		std::string mibLQ = std::string(_PLACE_) + "NEIGHBOUR_LINK_QUALITY_"; // old ---> ".1.3.6.1.4.1.16215.1.24.1.4.2.1.15.1.2."
		std::string strkeyLQ = std::to_string(id_).c_str() + mibLQ + std::to_string(metricSetIndex).c_str();
		const char *LQkey = strkeyLQ.c_str();
		std::string strvalLQ = (std::to_string(LQ)).c_str();
		const char *LQval = strvalLQ.c_str();

		// SET NEIGHBOURS RSSI (In that implemintation RSSI is SINR)
		std::string mibrssi = std::string(_PLACE_) + "NEIGHBOUR_RSSI_"; // old ---> ".1.3.6.1.4.1.16215.1.24.1.4.2.1.15.1.3."
		std::string strkeyrssi = std::to_string(id_).c_str() + mibrssi + std::to_string(metricSetIndex).c_str();
		const char *rssikey = strkeyrssi.c_str();
		std::string strvalrssi = (std::to_string(metric.getSINRAvgdBm())).c_str();
		const char *rssival = strvalrssi.c_str();

		// SET NEIGHBOURS SINR
		std::string mibsinr = std::string(_PLACE_) + "NEIGHBOUR_SNR_"; // old ---> ".1.3.6.1.4.1.16215.1.24.1.4.2.1.15.1.4."
		std::string strkeysinr = std::to_string(id_).c_str() + mibsinr + std::to_string(metricSetIndex).c_str();
		const char *sinrkey = strkeysinr.c_str();
		std::string strvalsinr = (std::to_string(metric.getSINRAvgdBm())).c_str();
		const char *sinrval = strvalsinr.c_str();

		// SET NEIGHBOURS busyRate
		std::string mibbusyrate = std::string(_PLACE_) + "NEIGHBOUR_BUSY_RATE_"; // old ---> ".1.3.6.1.4.1.16215.1.24.1.4.2.1.15.1.5."
		std::string strkeybusyrate = std::to_string(id_).c_str() + mibbusyrate + std::to_string(metricSetIndex).c_str();
		const char *busyratekey = strkeybusyrate.c_str();
		const char *busyrateval = "50";

		// SET NEIGHBOURS MemberRank
		std::string mibMemberRank = std::string(_PLACE_) + "NEIGHBOUR_MEMBER_RANK_"; // old ---> ".1.3.6.1.4.1.16215.1.24.1.4.2.1.15.1.6."
		std::string strkeyMemberRank = std::to_string(id_).c_str() + mibMemberRank + std::to_string(metricSetIndex).c_str();
		const char *memberRankkey = strkeyMemberRank.c_str();
		const char *memberRankval = "99";



		// SET MEMBERS MAC ADDR
		// node_id.PLACE.nbr_id
		std::string membermibmac = std::string(_PLACE_) + "MEMBERS_MAC_ADDRESS_"; // old ---> ".1.3.6.1.4.1.16215.1.24.1.4.2.16.1.1."
		std::string memberstrkeymac = std::to_string(id_).c_str() + membermibmac + std::to_string(metricSetIndex).c_str();
		const char *membermackey = memberstrkeymac.c_str();
		std::string strvalmac2 = getEthernetAddress_i(nbr).to_string().c_str();

		// convert mac addr to int64
		// now the MAC is 00:00:00:00:00:# (number of node)
		// to get effective converting, the first 4 bytes value will be changed to "CA:FE"

		std::string memberstrvalmacint64;
		//cafeaddr = "CA:FE:00:00:00:00";
		unsigned char real2[6];
		unsigned char cafe2[6];
		int last2 = -1;
		int rcreal2 = sscanf(strvalmac2.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx%n",
			real2 + 0, real2 + 1, real2 + 2, real2 + 3, real2 + 4, real2 + 5, &last2);
		int rccafe2 = sscanf(cafeaddr.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx%n",
			cafe2 + 0, cafe2 + 1, cafe2 + 2, cafe2 + 3, cafe2 + 4, cafe2 + 5, &last2);
		if (rcreal2 != 6 || strvalmac2.size() != last2)
		{
			/*LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
			ERROR_LEVEL,
			"SHIMI %03hu %s::%s couldn't conver mac addr %s to int64",
			id_, __MODULE__, __func__, strvalmac2);*/
			memberstrvalmacint64 = "0";
		}
		else
		{
			uint64_t mac64 = uint64_t(cafe2[0]) << 40 |
				uint64_t(cafe2[1]) << 32 |
				uint64_t(cafe2[0]) << 24 |
				uint64_t(cafe2[1]) << 16 |
				uint64_t(real2[4]) << 8 |
				uint64_t(real2[5]);
			memberstrvalmacint64 = std::to_string(mac64);
		}

		const char *membermacint64val = memberstrvalmacint64.c_str();

		// SET MEMBERS Link Quality
		float memberLQ = getRLQ_i(metric.getId(),
			metric.getSINRAvgdBm(),
			metric.getNumRxFrames(),
			metric.getNumMissedFrames());
		std::string membermibLQ = std::string(_PLACE_) + "MEMBERS_RACE_ID_"; // old ---> ".1.3.6.1.4.1.16215.1.24.1.4.2.16.1.2."
		std::string memberstrkeyLQ = std::to_string(id_).c_str() + membermibLQ + std::to_string(metricSetIndex).c_str();
		const char *memberLQkey = memberstrkeyLQ.c_str();
		std::string memberstrvalLQ = (std::to_string(memberLQ)).c_str();
		const char *memberLQval = memberstrvalLQ.c_str();

		// SET MEMBERS RSSI (In that implemintation RSSI is SINR)
		std::string membermibrssi = std::string(_PLACE_) + "MEMBERS_RSSI_"; // old ---> ".1.3.6.1.4.1.16215.1.24.1.4.2.16.1.3."
		std::string memberstrkeyrssi = std::to_string(id_).c_str() + membermibrssi + std::to_string(metricSetIndex).c_str();
		const char *memberrssikey = memberstrkeyrssi.c_str();
		std::string memberstrvalrssi = (std::to_string(metric.getSINRAvgdBm())).c_str();
		const char *memberrssival = memberstrvalrssi.c_str();

		// SET MEMBERS SINR
		std::string membermibsinr = std::string(_PLACE_) + "MEMBERS_HOP_"; // old ---> ".1.3.6.1.4.1.16215.1.24.1.4.2.16.1.4."
		std::string memberstrkeysinr = std::to_string(id_).c_str() + membermibsinr + std::to_string(metricSetIndex).c_str();
		const char *membersinrkey = memberstrkeysinr.c_str();
		std::string memberstrvalsinr = (std::to_string(metric.getSINRAvgdBm())).c_str();
		const char *membersinrval = memberstrvalsinr.c_str();

		reply = static_cast<redisReply*>(redisCommand(c, "MSET %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s", 
			nbrmackey, nbrmacint64val,
			LQkey, LQval,
			rssikey, rssival, 
			sinrkey, sinrval, 
			busyratekey, busyrateval, 
			memberRankkey, memberRankval, 
			membermackey, membermacint64val, 
			memberLQkey, memberLQval, 
			memberrssikey, memberrssival, 
			membersinrkey, membersinrval));

		LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
			INFO_LEVEL,
			"SHIMI %03hu %s::%s SET NEIGHBOURS MACADDR: %s, KEY: %s, VAL: %s\n"
			"SET NEIGHBOURS Link-Quality: %s, KEY: %s, VAL: %s\n"
			"SET NEIGHBOURS RSSI: %s, KEY: %s, VAL: %s\n"
			"SET NEIGHBOURS SINR: %s, KEY: %s, VAL: %s\n"
			"SET NEIGHBOURS busyRate: %s, KEY: %s, VAL: %s\n"
			"SET NEIGHBOURS MemberRabk: %s, KEY: %s, VAL: %s\n",
			id_, __MODULE__, __func__,
			reply->str, nbrmackey, nbrmacint64val,
			reply->str, LQkey, LQval,
			reply->str, rssikey, rssival,
			reply->str, sinrkey, sinrval,
			reply->str, busyratekey, busyrateval,
			reply->str, memberRankkey, memberRankval);

		LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
			INFO_LEVEL,
			"SHIMI %03hu %s::%s SET MEMBERS MACADDR: %s, KEY: %s, VAL: %s\n"
			"SET MEMBERS Link-Quality: %s, KEY: %s, VAL: %s\n"
			"SET MEMBERS RSSI: %s, KEY: %s, VAL: %s\n"
			"SET MEMBERS SINR: %s, KEY: %s, VAL: %s\n",
			id_, __MODULE__, __func__,
			reply->str, membermackey, membermacint64val,
			reply->str, memberLQkey, memberLQval,
			reply->str, memberrssikey, memberrssival,
			reply->str, membersinrkey, membersinrval);

		freeReplyObject(reply);

		reply = static_cast<redisReply*>(redisCommand(c, "DEL mykey"));
		freeReplyObject(reply);


		////////////////////////////////////////////////////////////////////////////

	metricSetIndex++;
	}
}


// queue metrics
void EMANE::ModemService::handleMetricMessage_i(const EMANE::Controls::R2RIQueueMetricControlMessage * pMessage)
{
	LOGGER_STANDARD_LOGGING_FN_VARGS(pPlatformService_->logService(),
		DEBUG_LEVEL,
		EMANE::Controls::R2RIQueueMetricControlMessageFormatter(pMessage),
		"SHIMI %03hu %s::%s R2RIQueueMetricControlMessage",
		id_, __MODULE__, __func__);


	redisReply *reply;

	// avg queue delay sum
	std::uint64_t delaySum{};

	// bytes in queue sum
	std::uint64_t bytesSum{};

	// packets in queue  sum
	std::uint64_t packetsSum{};

	size_t count{};

	// since there may be multiple Q's, get the overall avg
	for (const auto & metric : pMessage->getQueueMetrics())
	{
		delaySum += metric.getAvgDelay().count(); // in useconds
		packetsSum += metric.getCurrentDepth();

		++count;

		auto depth = metric.getCurrentDepth();
		auto queueid = metric.getQueueId();
		auto capacity = metric.getMaxSize();

		LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
			INFO_LEVEL,
			"SHIMI %03hu %s::%s #Q: %u, Max Size: %u, current depth: %u",
			id_, __MODULE__, __func__, queueid, capacity, depth);

		if(depth/capacity > 0.8 && queueid == 0)
		{
			// Get ip Address, where trap should be sent
			std::string mibpktq = std::string(_PLACE_) + "TRAP_IP";
			std::string strkeypktq = std::to_string(id_).c_str() + mibpktq;
			const char *pktqkey = strkeypktq.c_str();
			reply = static_cast<redisReply*>(redisCommand(c,"GET %s", pktqkey));
			std::string TRAP_IP = reply->str;
			freeReplyObject(reply);
			// Get ip Port, where trap should be sent
			mibpktq = std::string(_PLACE_) + "TRAP_PORT";
			strkeypktq = std::to_string(id_).c_str() + mibpktq;
			pktqkey = strkeypktq.c_str();
			reply = static_cast<redisReply*>(redisCommand(c,"GET %s", pktqkey));
			std::string TRAP_PORT = reply->str;
			freeReplyObject(reply);
			std::stringstream ss;
			ss << "snmptrap -e 0x0102030405 -v 3 -u authOnlyUser -a MD5 -A 'password' -x DES -X 'mypassword' -l authPriv " << TRAP_IP << ":" << TRAP_PORT << " -M SNMPv2-SMI::enterprises SNMPv2-SMI::enterprises int 0";
			std::string str = ss.str();
			const char* command = str.c_str();
			system(command);


			mibpktq = std::string(_PLACE_) + "GLOBAL_X_Off";
			strkeypktq = std::to_string(id_).c_str() + mibpktq;
			pktqkey = strkeypktq.c_str();
			const char *strvalpktq = "0";
			reply = static_cast<redisReply*>(redisCommand(c,"SET %s %s", pktqkey, strvalpktq));
			freeReplyObject(reply);
			LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
				INFO_LEVEL, 
				"SHIMI %03hu %s::%s SET GLOBAL_X_Off: %s to %s:%s",
				id_, __MODULE__, __func__, strvalpktq, TRAP_IP.c_str(), TRAP_PORT.c_str());
		}

		if(depth/capacity < 0.2 && queueid == 0)
		{
			std::string mibpktq = std::string(_PLACE_) + "GLOBAL_X_Off";
			std::string strkeypktq = std::to_string(id_).c_str() + mibpktq;
			const char *pktqkey = strkeypktq.c_str();
			reply = static_cast<redisReply*>(redisCommand(c,"GET %s", pktqkey));
			std::string GLOBAL_X_Off = reply->str;
			freeReplyObject(reply);
			if (!(GLOBAL_X_Off.compare("0")))
			{
				// Get ip Address, where trap should be sent
				mibpktq = std::string(_PLACE_) + "TRAP_IP";
				strkeypktq = std::to_string(id_).c_str() + mibpktq;
				pktqkey = strkeypktq.c_str();
				reply = static_cast<redisReply*>(redisCommand(c,"GET %s", pktqkey));
				std::string TRAP_IP = reply->str;
				freeReplyObject(reply);
				// Get ip Port, where trap should be sent
				mibpktq = std::string(_PLACE_) + "TRAP_PORT";
				strkeypktq = std::to_string(id_).c_str() + mibpktq;
				pktqkey = strkeypktq.c_str();
				reply = static_cast<redisReply*>(redisCommand(c,"GET %s", pktqkey));
				std::string TRAP_PORT = reply->str;
				freeReplyObject(reply);
				std::stringstream ss;
				ss << "snmptrap -e 0x0102030405 -v 3 -u authOnlyUser -a MD5 -A 'password' -x DES -X 'mypassword' -l authPriv " << TRAP_IP << ":" << TRAP_PORT << " -M SNMPv2-SMI::enterprises SNMPv2-SMI::enterprises int 1";
				std::string str = ss.str();
				const char* command = str.c_str();
				system(command);


				mibpktq = std::string(_PLACE_) + "GLOBAL_X_Off";
				strkeypktq = std::to_string(id_).c_str() + mibpktq;
				pktqkey = strkeypktq.c_str();
				const char *strvalpktq = "1";
				reply = static_cast<redisReply*>(redisCommand(c,"SET %s %s", pktqkey, strvalpktq));
				freeReplyObject(reply);
				LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
					INFO_LEVEL, 
					"SHIMI %03hu %s::%s SET GLOBAL_X_Off: %s to %s:%s",
					id_, __MODULE__, __func__, strvalpktq, TRAP_IP.c_str(), TRAP_PORT.c_str());
			}
		}

	}
	if (count)
	{
		avgQueueDelayMicroseconds_ = delaySum / count;
	}
	else
	{
		avgQueueDelayMicroseconds_ = 0;
	}

	// SET packets in Qs
	std::string mibpktq = std::string(_PLACE_) + "PACKETS_IN_TX_QUEUE"; // old ---> ".1.3.6.1.4.1.16215.1.24.1.4.8"
	std::string strkeypktq = std::to_string(id_).c_str() + mibpktq;
	const char *pktqkey = strkeypktq.c_str();
	const char *strvalpktq = (std::to_string(packetsSum)).c_str();

	reply = static_cast<redisReply*>(redisCommand(c, "SET %s %s", pktqkey, strvalpktq));

	LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
		INFO_LEVEL,
		"SHIMI %03hu %s::%s SET packets in Queues: %s, KEY: %s, VAL: %s",
		id_, __MODULE__, __func__, reply->str, pktqkey, strvalpktq);
	freeReplyObject(reply);

}


void EMANE::ModemService::handleFlowControlMessage_i()
{
	LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
		DEBUG_LEVEL,
		"SHIMI %03hu %s::%s XXX_TODO credits",
		id_, __MODULE__, __func__);
}



LLSNMP::DlepMac EMANE::ModemService::getEthernetAddress_i(const EMANE::NEMId nbr) const
{
	LLSNMP::DlepMac mac;

	if (destinationAdvertisementEnable_)
	{
		mac.mac_addr.push_back((nbr >> 8) & 0xFF);
		mac.mac_addr.push_back(nbr & 0xFF);
	}
	else
	{
		const auto iter = nemEtherAddrMap_.find(nbr);

		EMANE::Utils::EtherAddr etherAddr;

		// check the specific nbr to ether addr mapping first
		if (iter != nemEtherAddrMap_.end())
		{
			etherAddr = iter->second;
		}
		// otherwise use the OUI prefix (if any)
		else
		{
			etherAddr = etherOUI_;

			etherAddr.words.word3 = ntohs(nbr);
		}

		mac.mac_addr.assign(etherAddr.bytes.buff, etherAddr.bytes.buff + 6);
	}

	return mac;
}



void EMANE::ModemService::send_peer_update_i()
{}


void EMANE::ModemService::send_destination_update_i(
	const EMANE::ModemService::NeighborInfo & nbrInfo,
	bool isNewNbr)
{}



void EMANE::ModemService::send_destination_down_i(const EMANE::ModemService::NeighborInfo & nbrInfo)
{}



int EMANE::ModemService::getRLQ_i(const std::uint16_t nbr,
	const float fSINRAvg,
	const size_t numRxFrames,
	const size_t numMissedFrames)
{
	const auto numRxAndMissedFrames = numRxFrames + numMissedFrames;
	
	float fNewSINR{};

	float fReceiveRatio{};

	int iRLQ{};

	// lets find some history on this nbr 
	// even if this is the first report of this nbr it should 
	// be stored with initial values of (RR = 0, and SINR = -256)
	const auto & iter = currentNeighbors_.find(nbr);

	if (iter != currentNeighbors_.end())
	{
		// we have no pkt info for this interval
		if (numRxAndMissedFrames == 0.0f)
		{
			// reduce sinr by 3db
			fNewSINR = iter->second.fSINRlast_ - 3.0f;

			fReceiveRatio = iter->second.fRRlast_;
		}
		else
		{
			fNewSINR = fSINRAvg;

			fReceiveRatio = (float)numRxFrames / (float)numRxAndMissedFrames;
		}

		// check sinr is above min configured value 
		if (fNewSINR > fSINRMin_)
		{
			const auto fDeltaConfigSINR = fSINRMax_ - fSINRMin_;

			// the min to avg sinr delta 
			const auto fDeltaSINR = fNewSINR - fSINRMin_;

			// calculate rlq
			const auto fValue = 100.0f * (fDeltaSINR / fDeltaConfigSINR) * fReceiveRatio;

			// clamp between 0 and 100
			iRLQ = clampit(0.0f, 100.0f, fValue);
		}

		// save the sinr
		iter->second.fSINRlast_ = fNewSINR;

		// save the rr
		iter->second.fRRlast_ = fReceiveRatio;
	}

	LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
		DEBUG_LEVEL,
		"SHIMI %03hu %s::%s nbr %hu. sinr %f, receive ratio %f, RLQ %d",
		id_, __MODULE__, __func__,
		nbr,
		fNewSINR,
		fReceiveRatio,
		iRLQ);

	return iRLQ;
}