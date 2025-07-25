#ifndef BYTE_STREAM_TO_NALU_TIME_STAGE_H
#define BYTE_STREAM_TO_NALU_TIME_STAGE_H

#include "analysis_pipeline/unpacker_core/stages/byte_stream_processor_stage.h"

class ByteStreamToNaluTimeStage : public ByteStreamProcessorStage {
public:
    ByteStreamToNaluTimeStage();
    ~ByteStreamToNaluTimeStage() override = default;

    void OnInit() override;
    void Process() override;

    std::string Name() const override { return "ByteStreamToNaluTimeStage"; }

private:
    std::string output_product_name_;

    ClassDefOverride(ByteStreamToNaluTimeStage, 1);
};

#endif  // BYTE_STREAM_TO_NALU_TIME_STAGE_H
