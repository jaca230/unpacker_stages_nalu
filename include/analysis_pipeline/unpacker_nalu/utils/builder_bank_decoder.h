#pragma once
#include "analysis_pipeline/unpacker_nalu/data_products/NaluBuilderEvent.h"
#include "analysis_pipeline/unpacker_nalu/data_products/NaluCollectionTiming.h"
#include "analysis_pipeline/unpacker_nalu/data_products/NaluSelectionSummary.h"
#include <cstddef>
#include <cstdint>
namespace naluBuilder {
dataProducts::NaluEvent decodeSourceEvent(const uint8_t *data, size_t size);
dataProducts::NaluBuilderEvent decodeEvent(const uint8_t *data, size_t size);
dataProducts::NaluCollectionTiming decodeTiming(const uint8_t *data,
                                                size_t size);
dataProducts::NaluSelectionSummary decodeSelection(const uint8_t *data,
                                                   size_t size);
} // namespace naluBuilder
