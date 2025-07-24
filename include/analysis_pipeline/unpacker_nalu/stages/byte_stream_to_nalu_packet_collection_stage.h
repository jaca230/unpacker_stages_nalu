#ifndef BYTE_STREAM_TO_NALU_PACKET_COLLECTION_STAGE_H
#define BYTE_STREAM_TO_NALU_PACKET_COLLECTION_STAGE_H

#include "analysis_pipeline/byte_stream_unpacker/stages/byte_stream_processor_stage.h"
#include "analysis_pipeline/byte_stream_unpacker/utils/reflection_based_parser.h"

#include <memory>
#include <string>

class ByteStreamToNaluPacketCollectionStage : public ByteStreamProcessorStage {
public:
    ByteStreamToNaluPacketCollectionStage() = default;
    ~ByteStreamToNaluPacketCollectionStage() override = default;

    void OnInit() override;
    void Process() override;

    std::string Name() const override { return "ByteStreamToNaluPacketCollectionStage"; }

private:

    std::string endianness_ = "little";
    std::string output_name_ = "NaluPacketCollection";
    std::string repeat_count_product_name_ = "NaluEventHeader";
    std::string repeat_count_product_member_ = "num_packets";

    ClassDefOverride(ByteStreamToNaluPacketCollectionStage, 1);
};

#endif  // BYTE_STREAM_TO_NALU_PACKET_COLLECTION_STAGE_H
