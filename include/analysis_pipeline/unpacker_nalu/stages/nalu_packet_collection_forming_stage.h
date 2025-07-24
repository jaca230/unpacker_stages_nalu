#ifndef ANALYSIS_PIPELINE_STAGES_NALU_PACKET_COLLECTION_FORMING_STAGE_H
#define ANALYSIS_PIPELINE_STAGES_NALU_PACKET_COLLECTION_FORMING_STAGE_H

#include "analysis_pipeline/core/stages/base_stage.h"

class NaluPacketCollectionFormingStage : public BaseStage {
public:
    NaluPacketCollectionFormingStage() = default;
    ~NaluPacketCollectionFormingStage() override = default;

    void Process() override;
    std::string Name() const override { return "NaluPacketCollectionFormingStage"; }

    ClassDefOverride(NaluPacketCollectionFormingStage, 1);
protected:
    void OnInit() override;

private:
    std::string headerPrefix_ = "NaluPacketHeader";
    std::string payloadPrefix_ = "NaluPacketPayload";
    std::string footerPrefix_ = "NaluPacketFooter";
    std::string outputName_ = "NaluPacketCollection";
    int maxPackets_ = 62 * 64 + 10; // Default limit
};

#endif // ANALYSIS_PIPELINE_STAGES_NALU_PACKET_COLLECTION_FORMING_STAGE_H
