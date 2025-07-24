#include "analysis_pipeline/unpacker_nalu/stages/byte_stream_to_nalu_packet_collection_stage.h"

#include "analysis_pipeline/unpacker_nalu/data_products/NaluPacketCollection.h"
#include "analysis_pipeline/unpacker_nalu/data_products/NaluPacketHeader.h"
#include "analysis_pipeline/unpacker_nalu/data_products/NaluPacketPayload.h"
#include "analysis_pipeline/unpacker_nalu/data_products/NaluPacketFooter.h"

#include <TParameter.h>
#include <TClass.h>
#include <TObject.h>
#include <spdlog/spdlog.h>

ClassImp(ByteStreamToNaluPacketCollectionStage)

void ByteStreamToNaluPacketCollectionStage::OnInit() {
    ByteStreamProcessorStage::OnInit();

    endianness_ = parameters_.value("endianness", "little");
    output_name_ = parameters_.value("output_product", "NaluPacketCollection");
    repeat_count_product_name_ = parameters_.value("repeat_count_product_name", "NaluEventHeader");
    repeat_count_product_member_ = parameters_.value("repeat_count_product_member", "num_packets");

    header_parser_ = std::make_unique<ReflectionBasedParser>("dataProducts::NaluPacketHeader", endianness_);
    payload_parser_ = std::make_unique<ReflectionBasedParser>("dataProducts::NaluPacketPayload", endianness_);
    footer_parser_ = std::make_unique<ReflectionBasedParser>("dataProducts::NaluPacketFooter", endianness_);

    spdlog::info("[{}] Initialized with endianness '{}', output='{}', repeat_count source = '{}.{}'",
                 Name(), endianness_, output_name_, repeat_count_product_name_, repeat_count_product_member_);
}

void ByteStreamToNaluPacketCollectionStage::Process() {
    auto lock = getInputByteStreamLock();
    if (!lock) {
        spdlog::error("[{}] Failed to lock ByteStream '{}'", Name(), input_byte_stream_product_name_);
        return;
    }

    auto base_obj = lock->getSharedObject();
    auto bs = std::dynamic_pointer_cast<dataProducts::ByteStream>(base_obj);
    if (!bs || !bs->data || bs->size == 0) {
        spdlog::warn("[{}] Invalid or empty ByteStream", Name());
        return;
    }

    const uint8_t* buffer = bs->data;
    size_t buffer_size = bs->size;
    size_t offset = (getLastReadIndex() < 0) ? 0 : static_cast<size_t>(getLastReadIndex());

    // Get repeat_count from the referenced product
    int repeat_count = 0;

    if (!repeat_count_product_name_.empty() && !repeat_count_product_member_.empty()) {
        auto repeat_handle = getDataProductManager()->checkoutRead(repeat_count_product_name_);
        if (!repeat_handle) {
            spdlog::error("[{}] Could not lock repeat count product '{}'", Name(), repeat_count_product_name_);
            return;
        }

        auto [ptr, type] = repeat_handle->getMemberPointerAndType(repeat_count_product_member_);
        if (!ptr) {
            spdlog::error("[{}] Member '{}' not found in '{}'", Name(), repeat_count_product_member_, repeat_count_product_name_);
            return;
        }

        if (type == "int" || type == "Int_t") repeat_count = *static_cast<const int*>(ptr);
        else if (type == "unsigned int" || type == "UInt_t" || type == "uint32_t") repeat_count = static_cast<int>(*static_cast<const unsigned int*>(ptr));
        else if (type == "short" || type == "Short_t") repeat_count = static_cast<int>(*static_cast<const short*>(ptr));
        else if (type == "unsigned short" || type == "UShort_t" || type == "uint16_t") repeat_count = static_cast<int>(*static_cast<const unsigned short*>(ptr));
        else if (type == "char" || type == "Char_t") repeat_count = static_cast<int>(*static_cast<const char*>(ptr));
        else if (type == "unsigned char" || type == "UChar_t" || type == "uint8_t") repeat_count = static_cast<int>(*static_cast<const unsigned char*>(ptr));
        else if (type == "long" || type == "Long_t" || type == "int64_t") repeat_count = static_cast<int>(*static_cast<const long*>(ptr));
        else if (type == "unsigned long" || type == "ULong_t" || type == "uint64_t") repeat_count = static_cast<int>(*static_cast<const unsigned long*>(ptr));
        else if (type == "Long64_t") repeat_count = static_cast<int>(*static_cast<const Long64_t*>(ptr));
        else if (type == "ULong64_t") repeat_count = static_cast<int>(*static_cast<const ULong64_t*>(ptr));
        else if (type == "float" || type == "Float_t") repeat_count = static_cast<int>(*static_cast<const float*>(ptr));
        else if (type == "double" || type == "Double_t") repeat_count = static_cast<int>(*static_cast<const double*>(ptr));
        else if (type == "bool" || type == "Bool_t") repeat_count = static_cast<int>(*static_cast<const bool*>(ptr));
        else {
            spdlog::error("[{}] Unsupported repeat count type '{}'", Name(), type);
            return;
        }

        spdlog::debug("[{}] repeat_count = {} from '{}.{}'", Name(), repeat_count, repeat_count_product_name_, repeat_count_product_member_);
    } else {
        spdlog::warn("[{}] No repeat count config given, defaulting to 0", Name());
        return;
    }

    if (repeat_count <= 0) {
        spdlog::warn("[{}] repeat_count = {} <= 0, skipping", Name(), repeat_count);
        return;
    }

    auto collection = std::make_unique<dataProducts::NaluPacketCollection>();

    constexpr size_t header_size = sizeof(dataProducts::NaluPacketHeader);
    constexpr size_t payload_size = sizeof(dataProducts::NaluPacketPayload);
    constexpr size_t footer_size = sizeof(dataProducts::NaluPacketFooter);

    spdlog::debug("[{}] header_size = {}, payload_size = {}, footer_size = {}, total packet_size = {}",
              Name(), header_size, payload_size, footer_size,
              header_size + payload_size + footer_size);
    const size_t packet_size = header_size + payload_size + footer_size;

    for (int i = 0; i < repeat_count; ++i) {
        if (offset + packet_size > buffer_size) {
            spdlog::warn("[{}] Buffer too small for {}th packet", Name(), i);
            break;
        }

        dataProducts::NaluPacketHeader header;
        dataProducts::NaluPacketPayload payload;
        dataProducts::NaluPacketFooter footer;

        // memcpy from buffer to struct
        std::memcpy(&header, buffer + offset, header_size);
        offset += header_size;

        std::memcpy(&payload, buffer + offset, payload_size);
        offset += payload_size;

        std::memcpy(&footer, buffer + offset, footer_size);
        offset += footer_size;

        // TODO: Add endian conversion here per field if buffer endianness != native

        dataProducts::NaluPacket pkt;
        pkt.header = header;
        pkt.payload = payload;
        pkt.footer = footer;

        collection->AddPacket(std::move(pkt));
    }

    if (collection->GetPackets().empty()) {
        spdlog::warn("[{}] No packets parsed", Name());
        return;
    }

    auto output = std::make_unique<PipelineDataProduct>();
    output->setName(output_name_);
    output->setObject(std::move(collection));
    output->addTag("nalu");
    output->addTag("nalu_packets");
    output->addTag("built_by_byte_stream_to_nalu_packet_collection_stage");

    getDataProductManager()->addOrUpdate(output_name_, std::move(output));
    setLastReadIndex(static_cast<int>(offset));

    spdlog::debug("[{}] Parsed {} packets, new offset={}", Name(), repeat_count, offset);
}

