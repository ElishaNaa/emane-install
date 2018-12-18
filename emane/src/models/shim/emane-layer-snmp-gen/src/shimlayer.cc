/*
* Copyright (c) 2018 -Elisha Namman
*/

#include "emane/controls/serializedcontrolmessage.h"
#include "emane/controls/r2rineighbormetriccontrolmessage.h"
#include "emane/controls/r2riqueuemetriccontrolmessage.h"
#include "emane/controls/r2riselfmetriccontrolmessage.h"
#include "emane/controls/flowcontrolcontrolmessage.h"

#include "shimlayer.h"

namespace
{
	const char * __MODULE__ = "SNMP::ShimLayer";

	EMANE::ControlMessages clone(const EMANE::ControlMessages & msgs)
	{
		EMANE::ControlMessages clones;

		for (const auto & msg : msgs)
		{
			clones.push_back(msg->clone());
		}

		return clones;
	}
}


EMANE::R2RI::ShimLayer::ShimLayer(NEMId id,
	PlatformServiceProvider * pPlatformService,
	RadioServiceProvider * pRadioService) :
	ShimLayerImplementor{ id, pPlatformService, pRadioService },
	snmpModemService_{ id, pPlatformService, pRadioService }
{ }



EMANE::R2RI::ShimLayer::~ShimLayer()
{ }



void EMANE::R2RI::ShimLayer::initialize(Registrar & registrar)
{
	LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
		DEBUG_LEVEL,
		"SHIMI %03hu %s::%s", id_, __MODULE__, __func__);

	try
	{
		auto & configRegistrar = registrar.configurationRegistrar();

		// sinr min/max
		configRegistrar.registerNumeric<float>("sinrmin",
			ConfigurationProperties::DEFAULT,
			{ 0.0 },
			"min sinr for RLQ calculations");

		configRegistrar.registerNumeric<float>("sinrmax",
			ConfigurationProperties::DEFAULT,
			{ 20.0f },
			"max sinr for RLQ calculations");

		// nem to mac address mapping
		configRegistrar.registerNonNumeric<std::string>("macaddress",
			ConfigurationProperties::NONE,
			{},
			"Defines a list of NEMId(s) and associated ethernet MAC address"
			"NEMId EtherMacAddress",
			0,
			255,
			"^\\d+ ([0-9A-Fa-f]{2}[:]){5}([0-9A-Fa-f]{2})$");

		configRegistrar.registerNonNumeric<std::string>("etheraddroui",
			ConfigurationProperties::NONE,
			{},
			"Defines an ethernet MAC address preix to be used along with the NEMId"
			"to construct the full ethernet MAC address",
			0,
			255,
			"^([0-9A-Fa-f]{2}[:]){2}([0-9A-Fa-f]{2})$");

		configRegistrar.registerNonNumeric<INETAddr>("addressRedis",
			ConfigurationProperties::NONE,
			{},
			"IPv4 or IPv6 virutal device address.");

		configRegistrar.registerNumeric<std::uint64_t>("timeToUpdateRedis",
			ConfigurationProperties::DEFAULT,
		{10000000000},
		"Time to update redis in nanoseconds.");

		configRegistrar.registerNumeric<double>("upperBoundQueue0",
			ConfigurationProperties::DEFAULT,
		{ 0.8 },
		"Send trap (X_OFF), when the packages in the queue cross upper bound for queue-0");

		configRegistrar.registerNumeric<double>("lowerBoundQueue0",
			ConfigurationProperties::DEFAULT,
		{ 0.2 },
		"Send trap (X_ON), when the packages in the queue smallest from lower bound for queue-0");

		configRegistrar.registerNumeric<double>("upperBoundQueue1",
			ConfigurationProperties::DEFAULT,
			{ 0.8 },
			"Send trap (X_OFF), when the packages in the queue cross upper bound for queue-1");

		configRegistrar.registerNumeric<double>("lowerBoundQueue1",
			ConfigurationProperties::DEFAULT,
			{ 0.2 },
			"Send trap (X_ON), when the packages in the queue smallest from lower bound for queue-1");

		configRegistrar.registerNumeric<double>("upperBoundQueue2",
			ConfigurationProperties::DEFAULT,
			{ 0.8 },
			"Send trap (X_OFF), when the packages in the queue cross upper bound for queue-2");

		configRegistrar.registerNumeric<double>("lowerBoundQueue2",
			ConfigurationProperties::DEFAULT,
			{ 0.2 },
			"Send trap (X_ON), when the packages in the queue smallest from lower bound for queue-2");

		configRegistrar.registerNumeric<double>("upperBoundQueue3",
			ConfigurationProperties::DEFAULT,
			{ 0.8 },
			"Send trap (X_OFF), when the packages in the queue cross upper bound for queue-3");

		configRegistrar.registerNumeric<double>("lowerBoundQueue3",
			ConfigurationProperties::DEFAULT,
			{ 0.2 },
			"Send trap (X_ON), when the packages in the queue smallest from lower bound for queue-3");

		configRegistrar.registerNumeric<std::uint64_t>("severity",
			ConfigurationProperties::DEFAULT,
			{ 0 },
			"The trap severity.");

		configRegistrar.registerNumeric<std::uint64_t>("radioTxOverloadOn",
			ConfigurationProperties::DEFAULT,
			{ 1022 },
			"The trap ID code.");

		configRegistrar.registerNumeric<std::uint64_t>("radioTxOverloadOff",
			ConfigurationProperties::DEFAULT,
			{ 1023 },
			"The trap ID code.");

		//const EMANE::ModemService::ConfigParameterMapType & snmpConfiguration{snmpModemService_.getConfigItems()};

		// snmp config items we will be queried for by lib snmp after initialization
		// for(auto & item : snmpConfiguration)
		//   {
		//     auto value = item.second.value;

		//     // the rfid is our NEM id, so NEM #1 is(00,01)
		//     if(item.first == "destinationadvertrfid")
		//       {
		//         value = std::to_string(std::uint8_t((id_ >> 8) & 0xFF)) + "," + std::to_string(std::uint8_t(id_ & 0xFF));
		//       }

		//      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
		//                              DEBUG_LEVEL,
		//                              "SHIMI %03hu %s::%s item %s, default value %s, description %s", 
		//                              id_, __MODULE__, __func__,
		//                              item.first.c_str(), 
		//                              value.c_str(), 
		//                              item.second.description.c_str());

		//      configRegistrar.registerNonNumeric<std::string>(item.first,
		//                                                      ConfigurationProperties::DEFAULT,
		//                                                      {{value}},
		//                                                      item.second.description);
		//   }  
	}
	catch (const EMANE::Exception & ex)
	{
		LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
			ERROR_LEVEL,
			"SHIMI %03hu %s::%s caught %s",
			id_, __MODULE__, __func__,
			ex.what());
	}
}



void EMANE::R2RI::ShimLayer::configure(const ConfigurationUpdate & update)
{
	LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
		DEBUG_LEVEL,
		"SHIMI %03hu %s::%s", id_, __MODULE__, __func__);

	for (const auto & item : update)
	{
		if (item.first == "addressRedis")
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
		else if (item.first == "timeToUpdateRedis")
		{
			timeToUpdateRedis_ = item.second[0].asUINT64();

			LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
				INFO_LEVEL,
				"SHIMI %03hu %s::%s %s=%u",
				id_,
				__MODULE__,
				__func__,
				item.first.c_str(),
				timeToUpdateRedis_);
		}
		else if (item.first == "severity")
		{
			severity_ = item.second[0].asUINT64();

			LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
				INFO_LEVEL,
				"SHIMI %03d %s::%s %s = %u",
				id_,
				__MODULE__,
				__func__,
				item.first.c_str(),
				severity_);
		}
		else if (item.first == "radioTxOverloadOn")
		{
			radio_tx_overload_on_ = item.second[0].asUINT64();

			LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
				INFO_LEVEL,
				"SHIMI %03d %s::%s %s = %u",
				id_,
				__MODULE__,
				__func__,
				item.first.c_str(),
				radio_tx_overload_on_);
		}
		else if (item.first == "radioTxOverloadOff")
		{
			radio_tx_overload_off_ = item.second[0].asUINT64();

			LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
				INFO_LEVEL,
				"SHIMI %03d %s::%s %s = %u",
				id_,
				__MODULE__,
				__func__,
				item.first.c_str(),
				radio_tx_overload_off_);
		}
		else if (item.first == "upperBoundQueue0")
		{
			upperBound_queue_0_ = item.second[0].asDouble();

			LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
				INFO_LEVEL,
				"SHIMI %03d %s::%s %s = %f",
				id_,
				__MODULE__,
				__func__,
				item.first.c_str(),
				upperBound_queue_0_);
		}
		else if (item.first == "lowerBoundQueue0")
		{
			lowerBound_queue_0_ = item.second[0].asDouble();

			LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
				INFO_LEVEL,
				"SHIMI %03d %s::%s %s = %f",
				id_,
				__MODULE__,
				__func__,
				item.first.c_str(),
				lowerBound_queue_0_);
		}
		else if (item.first == "upperBoundQueue1")
		{
			upperBound_queue_1_ = item.second[0].asDouble();

			LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
				INFO_LEVEL,
				"SHIMI %03d %s::%s %s = %f",
				id_,
				__MODULE__,
				__func__,
				item.first.c_str(),
				upperBound_queue_1_);
		}
		else if (item.first == "lowerBoundQueue1")
		{
			lowerBound_queue_1_ = item.second[0].asDouble();

			LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
				INFO_LEVEL,
				"SHIMI %03d %s::%s %s = %f",
				id_,
				__MODULE__,
				__func__,
				item.first.c_str(),
				lowerBound_queue_1_);
		}
		else if (item.first == "upperBoundQueue2")
		{
			upperBound_queue_2_ = item.second[0].asDouble();

			LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
				INFO_LEVEL,
				"SHIMI %03d %s::%s %s = %f",
				id_,
				__MODULE__,
				__func__,
				item.first.c_str(),
				upperBound_queue_2_);
		}
		else if (item.first == "lowerBoundQueue2")
		{
			lowerBound_queue_2_ = item.second[0].asDouble();

			LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
				INFO_LEVEL,
				"SHIMI %03d %s::%s %s = %f",
				id_,
				__MODULE__,
				__func__,
				item.first.c_str(),
				lowerBound_queue_2_);
		}
		else if (item.first == "upperBoundQueue3")
		{
			upperBound_queue_3_ = item.second[0].asDouble();

			LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
				INFO_LEVEL,
				"SHIMI %03d %s::%s %s = %f",
				id_,
				__MODULE__,
				__func__,
				item.first.c_str(),
				upperBound_queue_3_);
		}
		else if (item.first == "lowerBoundQueue3")
		{
			lowerBound_queue_3_ = item.second[0].asDouble();

			LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
				INFO_LEVEL,
				"SHIMI %03d %s::%s %s = %f",
				id_,
				__MODULE__,
				__func__,
				item.first.c_str(),
				lowerBound_queue_3_);
		}
		else {

		}
	}

	snmpModemService_.configure(update);
}



void EMANE::R2RI::ShimLayer::start()
{
	LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
		DEBUG_LEVEL,
		"SHIMI %03hu %s::%s", id_, __MODULE__, __func__);

	snmpModemService_.start();
}



void EMANE::R2RI::ShimLayer::stop()
{
	LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
		DEBUG_LEVEL,
		"SHIMI %03hu %s::%s", id_, __MODULE__, __func__);
}



void EMANE::R2RI::ShimLayer::destroy()
throw()
{
	LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
		DEBUG_LEVEL,
		"SHIMI %03hu %s::%s", id_, __MODULE__, __func__);

	snmpModemService_.destroy();
}



void EMANE::R2RI::ShimLayer::processUpstreamControl(const ControlMessages & msgs)
{
	EMANE::ControlMessages clones;

	EMANE::ControlMessages controlMessages;

	for (const auto & pMessage : msgs)
	{
		switch (pMessage->getId())
		{
		case EMANE::Controls::R2RISelfMetricControlMessage::IDENTIFIER:
		{
			// self info to the front, consume
			controlMessages.push_front(pMessage);

			LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
				DEBUG_LEVEL,
				"SHIMI %03hu %s::%s consume R2RISelfMetricControlMessage",
				id_, __MODULE__, __func__);
		}
		break;

		case EMANE::Controls::R2RINeighborMetricControlMessage::IDENTIFIER:
		{
			LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
				DEBUG_LEVEL,
				"SHIMI %03hu %s::%s consume R2RINeighborMetricControlMessage",
				id_, __MODULE__, __func__);

			// to the back, consume
			controlMessages.push_back(pMessage);
		}
		break;

		case EMANE::Controls::R2RIQueueMetricControlMessage::IDENTIFIER:
		{
			LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
				DEBUG_LEVEL,
				"SHIMI %03hu %s::%s consume R2RIQueueMetricControlMessage",
				id_, __MODULE__, __func__);

			// to the back, consume
			controlMessages.push_back(pMessage);
		}
		break;

		case EMANE::Controls::FlowControlControlMessage::IDENTIFIER:
		{
			LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
				DEBUG_LEVEL,
				"SHIMI %03hu %s::%s forward and consume FlowControlControlMessage",
				id_, __MODULE__, __func__);

			// transport is expecting these too
			clones.push_back(pMessage->clone());

			// to the back
			controlMessages.push_back(pMessage);
		}
		break;

		case EMANE::Controls::SerializedControlMessage::IDENTIFIER:
		{
			const auto pSerializedControlMessage =
				static_cast<const EMANE::Controls::SerializedControlMessage *>(pMessage);

			switch (pSerializedControlMessage->getSerializedId())
			{
			case EMANE::Controls::R2RISelfMetricControlMessage::IDENTIFIER:
			{
				LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
					DEBUG_LEVEL,
					"SHIMI %03hu %s::%s consume R2RISelfMetricControlMessage",
					id_, __MODULE__, __func__);

				// self info to the front, consume
				controlMessages.push_front(pMessage);
			}
			break;

			case EMANE::Controls::R2RINeighborMetricControlMessage::IDENTIFIER:
			{
				LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
					DEBUG_LEVEL,
					"SHIMI %03hu %s::%s consume R2RINeighborMetricControlMessage",
					id_, __MODULE__, __func__);

				// to the back, consume
				controlMessages.push_back(pMessage);
			}
			break;

			case EMANE::Controls::R2RIQueueMetricControlMessage::IDENTIFIER:
			{
				LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
					DEBUG_LEVEL,
					"SHIMI %03hu %s::%s consume R2RIQueueMetricControlMessage",
					id_, __MODULE__, __func__);

				// to the back, consume
				controlMessages.push_back(pMessage);
			}
			break;

			case EMANE::Controls::FlowControlControlMessage::IDENTIFIER:
			{
				LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
					DEBUG_LEVEL,
					"SHIMI %03hu %s::%s forward and consume FlowControlControlMessage",
					id_, __MODULE__, __func__);

				// transport is expecting these too
				clones.push_back(pMessage->clone());

				// to the back
				controlMessages.push_back(pMessage);
			}
			break;

			default:
				clones.push_back(pMessage->clone());

				LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
					DEBUG_LEVEL,
					"SHIMI %03hu %s::%s pass thru unknown msg id %hu",
					id_, __MODULE__, __func__, pMessage->getId());
			}
		}
		break;

		default:
			clones.push_back(pMessage->clone());

			LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
				DEBUG_LEVEL,
				"SHIMI %03hu %s::%s pass thru unknown msg id %hu",
				id_, __MODULE__, __func__, pMessage->getId());
		}
	}

	if (!clones.empty())
	{
		LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
			DEBUG_LEVEL,
			"SHIMI %03hu %s::%s pass thru %zu msgs",
			id_, __MODULE__, __func__, clones.size());

		// pass thru
		sendUpstreamControl(clones);
	}

	LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
		DEBUG_LEVEL,
		"SHIMI %03hu %s::%s taksham log here346",
		id_, __MODULE__, __func__);

	// now handle control messages in expected order (self first, then nbr/queue/flowctrl)
	snmpModemService_.handleControlMessages(controlMessages);

	sendUpstreamControl(clone(msgs)); // taksham - we need to upstream the msf to the next shim layer!
}



void EMANE::R2RI::ShimLayer::processDownstreamControl(const ControlMessages & msgs)
{
	LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
		DEBUG_LEVEL,
		"SHIMI %03hu %s::%s pass thru %zu msgs",
		id_, __MODULE__, __func__, msgs.size());

	// pass thru
	sendDownstreamControl(clone(msgs));
}



void EMANE::R2RI::ShimLayer::processUpstreamPacket(UpstreamPacket & pkt,
	const ControlMessages & msgs)
{
	LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
		DEBUG_LEVEL,
		"SHIMI %03hu %s::%s pass thru pkt len %zu and %zu msgs",
		id_, __MODULE__, __func__, pkt.length(), msgs.size());

	// pass thru
	sendUpstreamPacket(pkt, clone(msgs));
}



void EMANE::R2RI::ShimLayer::processDownstreamPacket(DownstreamPacket & pkt,
	const ControlMessages & msgs)
{
	LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
		DEBUG_LEVEL,
		"SHIMI %03hu %s::%s pass thru pkt len %zu and %zu msgs",
		id_, __MODULE__, __func__, pkt.length(), msgs.size());

	// pass thru
	sendDownstreamPacket(pkt, clone(msgs));
}


DECLARE_SHIM_LAYER(EMANE::R2RI::ShimLayer);