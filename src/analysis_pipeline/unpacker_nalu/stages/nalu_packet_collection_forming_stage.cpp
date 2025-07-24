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

        if (!dpm->hasProduct(headerName) || !dpm->hasProduct(payloadName) || !dpm->hasProduct(footerName)) {
            spdlog::debug("[{}] Missing product triplet at index {}, halting early", Name(), index);
            break;
        }

        auto headerProduct = dpm->extractProduct(headerName);
        auto payloadProduct = dpm->extractProduct(payloadName);
        auto footerProduct = dpm->extractProduct(footerName);

        if (!headerProduct || !payloadProduct || !footerProduct) {
            spdlog::error("[{}] Failed to extract all components for index {}", Name(), index);
            break;
        }

        auto headerPtr = dynamic_cast<dataProducts::NaluPacketHeader*>(headerProduct->getObject());
        auto payloadPtr = dynamic_cast<dataProducts::NaluPacketPayload*>(payloadProduct->getObject());
        auto footerPtr = dynamic_cast<dataProducts::NaluPacketFooter*>(footerProduct->getObject());

        if (!headerPtr || !payloadPtr || !footerPtr) {
            spdlog::error("[{}] Type mismatch or null pointer during cast at index {}", Name(), index);
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

    auto product = std::make_unique<PipelineDataProduct>();
    product->setName(outputName_);
    product->setObject(std::move(packetCollection));
    product->addTag("nalu");
    product->addTag("nalu_packets");
    product->addTag("built_by_nalu_packet_collection_forming_stage");

    dpm->addOrUpdate(outputName_, std::move(product));

    spdlog::debug("[{}] Formed {} NaluPacket(s) into '{}'",
                  Name(), packetCollection->GetPackets().size(), outputName_);
}
