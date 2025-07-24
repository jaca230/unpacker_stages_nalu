#include "analysis_pipeline/unpacker_nalu/stages/nalu_packet_collection_forming_stage.h"

#include "analysis_pipeline/unpacker_nalu/data_products/NaluPacket.h"
#include "analysis_pipeline/unpacker_nalu/data_products/NaluPacketHeader.h"
#include "analysis_pipeline/unpacker_nalu/data_products/NaluPacketPayload.h"
#include "analysis_pipeline/unpacker_nalu/data_products/NaluPacketFooter.h"
#include "analysis_pipeline/unpacker_nalu/data_products/NaluPacketCollection.h"

#include <spdlog/spdlog.h>

ClassImp(NaluPacketCollectionFormingStage)

void NaluPacketCollectionFormingStage::OnInit() {
    headerPrefix_ = parameters_.value("header_prefix", "NaluPacketHeader");
    payloadPrefix_ = parameters_.value("payload_prefix", "NaluPacketPayload");
    footerPrefix_ = parameters_.value("footer_prefix", "NaluPacketFooter");
    outputName_ = parameters_.value("output_product", "NaluPacketCollection");
    maxPackets_ = parameters_.value("max_packets", 62 * 64 + 10);

    spdlog::debug("[{}] Configured with header='{}', payload='{}', footer='{}', output='{}', max_packets={}",
                  Name(), headerPrefix_, payloadPrefix_, footerPrefix_, outputName_, maxPackets_);
}

void NaluPacketCollectionFormingStage::Process() {
    auto dpm = getDataProductManager();
    auto packetCollection = std::make_unique<dataProducts::NaluPacketCollection>();

    for (int index = 0; index < maxPackets_; ++index) {
        const std::string headerName = headerPrefix_ + "_" + std::to_string(index);
        const std::string payloadName = payloadPrefix_ + "_" + std::to_string(index);
        const std::string footerName = footerPrefix_ + "_" + std::to_string(index);

        if (!dpm->hasProduct(headerName)) {
            spdlog::debug("[{}] Missing header product '{}' at index {}, halting", Name(), headerName, index);
            break;
        }
        if (!dpm->hasProduct(payloadName)) {
            spdlog::debug("[{}] Missing payload product '{}' at index {}, halting", Name(), payloadName, index);
            break;
        }
        if (!dpm->hasProduct(footerName)) {
            spdlog::debug("[{}] Missing footer product '{}' at index {}, halting", Name(), footerName, index);
            break;
        }

        auto headerProduct = dpm->extractProduct(headerName);
        if (!headerProduct) {
            spdlog::error("[{}] Failed to extract header product '{}' at index {}", Name(), headerName, index);
            break;
        }
        auto payloadProduct = dpm->extractProduct(payloadName);
        if (!payloadProduct) {
            spdlog::error("[{}] Failed to extract payload product '{}' at index {}", Name(), payloadName, index);
            break;
        }
        auto footerProduct = dpm->extractProduct(footerName);
        if (!footerProduct) {
            spdlog::error("[{}] Failed to extract footer product '{}' at index {}", Name(), footerName, index);
            break;
        }

        auto headerPtr = dynamic_cast<dataProducts::NaluPacketHeader*>(headerProduct->getObject());
        if (!headerPtr) {
            spdlog::error("[{}] Failed to cast header product '{}' to NaluPacketHeader* at index {}", Name(), headerName, index);
            break;
        }
        auto payloadPtr = dynamic_cast<dataProducts::NaluPacketPayload*>(payloadProduct->getObject());
        if (!payloadPtr) {
            spdlog::error("[{}] Failed to cast payload product '{}' to NaluPacketPayload* at index {}", Name(), payloadName, index);
            break;
        }
        auto footerPtr = dynamic_cast<dataProducts::NaluPacketFooter*>(footerProduct->getObject());
        if (!footerPtr) {
            spdlog::error("[{}] Failed to cast footer product '{}' to NaluPacketFooter* at index {}", Name(), footerName, index);
            break;
        }

        dataProducts::NaluPacket pkt;
        pkt.header = std::move(*headerPtr);
        pkt.payload = std::move(*payloadPtr);
        pkt.footer = std::move(*footerPtr);

        packetCollection->AddPacket(std::move(pkt));
    }

    if (packetCollection->GetPackets().empty()) {
        spdlog::warn("[{}] No complete NaluPacket triplets found", Name());
        return;
    }

    size_t packetCount = packetCollection->GetPackets().size();

    auto product = std::make_unique<PipelineDataProduct>();
    product->setName(outputName_);
    product->setObject(std::move(packetCollection));
    product->addTag("nalu");
    product->addTag("nalu_packets");
    product->addTag("built_by_nalu_packet_collection_forming_stage");

    dpm->addOrUpdate(outputName_, std::move(product));

    spdlog::debug("[{}] Formed {} NaluPacket(s) into '{}'", Name(), packetCount, outputName_);
}
