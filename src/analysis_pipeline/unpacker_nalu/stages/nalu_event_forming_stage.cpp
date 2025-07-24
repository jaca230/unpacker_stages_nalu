#include "analysis_pipeline/unpacker_nalu/stages/nalu_event_forming_stage.h"

#include "analysis_pipeline/unpacker_nalu/data_products/NaluEvent.h"
#include "analysis_pipeline/unpacker_nalu/data_products/NaluEventHeader.h"
#include "analysis_pipeline/unpacker_nalu/data_products/NaluPacket.h"
#include "analysis_pipeline/unpacker_nalu/data_products/NaluEventFooter.h"

#include <TList.h>
#include <spdlog/spdlog.h>

ClassImp(NaluEventFormingStage)

void NaluEventFormingStage::OnInit() {
    headerName_           = parameters_.value("header_name", "NaluEventHeader");
    footerName_           = parameters_.value("footer_name", "NaluEventFooter");
    packetCollectionName_ = parameters_.value("packet_collection", "NaluPacketCollection");
    outputName_           = parameters_.value("output_product", "NaluEvent");

    spdlog::debug("[{}] Configured with header='{}', footer='{}', packet collection='{}', output='{}'",
                  Name(), headerName_, footerName_, packetCollectionName_, outputName_);
}

void NaluEventFormingStage::Process() {
    auto dpm = getDataProductManager();

    if (!dpm->hasProduct(headerName_)) {
        spdlog::warn("[{}] Missing input product: {}", Name(), headerName_);
        return;
    }
    if (!dpm->hasProduct(footerName_)) {
        spdlog::warn("[{}] Missing input product: {}", Name(), footerName_);
        return;
    }
    if (!dpm->hasProduct(packetCollectionName_)) {
        spdlog::warn("[{}] Missing input product: {}", Name(), packetCollectionName_);
        return;
    }

    auto headerProduct = dpm->extractProduct(headerName_);
    if (!headerProduct) {
        spdlog::error("[{}] Failed to extract product: {}", Name(), headerName_);
        return;
    }

    auto footerProduct = dpm->extractProduct(footerName_);
    if (!footerProduct) {
        spdlog::error("[{}] Failed to extract product: {}", Name(), footerName_);
        return;
    }

    auto packetsProduct = dpm->extractProduct(packetCollectionName_);
    if (!packetsProduct) {
        spdlog::error("[{}] Failed to extract product: {}", Name(), packetCollectionName_);
        return;
    }

    auto headerPtr = dynamic_cast<dataProducts::NaluEventHeader*>(headerProduct->getObject());
    if (!headerPtr) {
        spdlog::error("[{}] Failed to cast product '{}' to NaluEventHeader*", Name(), headerName_);
        return;
    }

    auto footerPtr = dynamic_cast<dataProducts::NaluEventFooter*>(footerProduct->getObject());
    if (!footerPtr) {
        spdlog::error("[{}] Failed to cast product '{}' to NaluEventFooter*", Name(), footerName_);
        return;
    }

    auto packetsCollectionPtr = dynamic_cast<dataProducts::NaluPacketCollection*>(packetsProduct->getObject());
    if (!packetsCollectionPtr) {
        spdlog::error("[{}] Failed to cast product '{}' to NaluPacketCollection*", Name(), packetCollectionName_);
        return;
    }

    dataProducts::NaluEvent event;
    event.header = std::move(*headerPtr);
    event.footer = std::move(*footerPtr);
    event.packets = std::move(*packetsCollectionPtr);

    auto product = std::make_unique<PipelineDataProduct>();
    product->setName(outputName_);
    product->setObject(std::make_unique<dataProducts::NaluEvent>(std::move(event)));
    product->addTag("nalu");
    product->addTag("nalu_event");
    product->addTag("formed_by_nalu_event_forming_stage");

    dpm->addOrUpdate(outputName_, std::move(product));

    spdlog::debug("[{}] Built NaluEvent with {} packets", Name(), event.packets.GetPackets().size());
}


