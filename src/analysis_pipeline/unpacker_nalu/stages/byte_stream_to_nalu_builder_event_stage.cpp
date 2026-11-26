#include "analysis_pipeline/unpacker_nalu/stages/byte_stream_to_nalu_builder_event_stage.h"
#include "analysis_pipeline/core/data/pipeline_data_product.h"
#include "analysis_pipeline/unpacker_core/data_products/ByteStream.h"
#include "analysis_pipeline/unpacker_nalu/utils/builder_bank_decoder.h"
#include <spdlog/spdlog.h>
ClassImp(ByteStreamToNaluBuilderEventStage);
void ByteStreamToNaluBuilderEventStage::OnInit() {
    ByteStreamProcessorStage::OnInit();
    output_product_name_ =
        parameters_.value("output_product_name", "NaluBuilderEvent");
}
void ByteStreamToNaluBuilderEventStage::Process() {
    auto lock = getInputByteStreamLock();
    if (!lock)
        return; // Banks are optional; an absent bank creates no product.
    auto bytes = std::dynamic_pointer_cast<dataProducts::ByteStream>(
        lock->getSharedObject());
    try {
        if (!bytes)
            throw std::runtime_error("Invalid HDSoC byte stream");
        auto result = std::make_unique<dataProducts::NaluBuilderEvent>(
            naluBuilder::decodeEvent(bytes->data, bytes->size));
        auto product = std::make_unique<PipelineDataProduct>();
        product->setName(output_product_name_);
        product->setObject(std::move(result));
        product->addTag("nalu_builder_event");
        getDataProductManager()->addOrUpdate(output_product_name_,
                                             std::move(product));
    } catch (const std::exception &error) {
        spdlog::error("[{}] {}", Name(), error.what());
    }
}
