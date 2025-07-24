#ifndef ANALYSIS_PIPELINE_STAGES_NALU_EVENT_FORMING_STAGE_H
#define ANALYSIS_PIPELINE_STAGES_NALU_EVENT_FORMING_STAGE_H

#include "analysis_pipeline/core/stages/base_stage.h"

class NaluEventFormingStage : public BaseStage {
public:
    NaluEventFormingStage() = default;
    ~NaluEventFormingStage() override = default;

    void Process() override;
    std::string Name() const override { return "NaluEventFormingStage"; }

    ClassDefOverride(NaluEventFormingStage, 1);
protected:
    void OnInit() override;

private:
    std::string headerName_ = "NaluEventHeader";
    std::string footerName_ = "NaluEventFooter";
    std::string packetCollectionName_ = "NaluPacketCollection";
    std::string outputName_ = "NaluEventCollection";
};

#endif
