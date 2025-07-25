#ifndef BYTE_STREAM_TO_NALU_EVENT_STAGE_H
#define BYTE_STREAM_TO_NALU_EVENT_STAGE_H

#include "analysis_pipeline/unpacker_core/stages/byte_stream_processor_stage.h"

class ByteStreamToNaluEventStage : public ByteStreamProcessorStage {
public:
    ByteStreamToNaluEventStage();
    ~ByteStreamToNaluEventStage() override = default;

    void OnInit() override;
    void Process() override;

    std::string Name() const override { return "ByteStreamToNaluEventStage"; }

private:
    std::string output_product_name_ = "NaluEvent";

    ClassDefOverride(ByteStreamToNaluEventStage, 1);
};

#endif // BYTE_STREAM_TO_NALU_EVENT_STAGE_H
