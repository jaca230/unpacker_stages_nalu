#pragma once
#include "analysis_pipeline/unpacker_core/stages/byte_stream_processor_stage.h"
class ByteStreamToNaluCollectionTimingStage : public ByteStreamProcessorStage {
  public:
    void OnInit() override;
    void Process() override;
    std::string Name() const override {
        return "ByteStreamToNaluCollectionTimingStage";
    }

  private:
    std::string output_product_name_;
    ClassDefOverride(ByteStreamToNaluCollectionTimingStage, 1);
};
