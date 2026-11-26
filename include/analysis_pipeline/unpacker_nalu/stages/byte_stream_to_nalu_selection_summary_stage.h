#pragma once
#include "analysis_pipeline/unpacker_core/stages/byte_stream_processor_stage.h"
class ByteStreamToNaluSelectionSummaryStage : public ByteStreamProcessorStage {
  public:
    void OnInit() override;
    void Process() override;
    std::string Name() const override {
        return "ByteStreamToNaluSelectionSummaryStage";
    }

  private:
    std::string output_product_name_;
    ClassDefOverride(ByteStreamToNaluSelectionSummaryStage, 1);
};
