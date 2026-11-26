#include "analysis_pipeline/unpacker_nalu/utils/builder_bank_decoder.h"
#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error HDSoC nested collector ABI requires little-endian decoding
#endif
namespace naluBuilder {
namespace {
struct Reader {
    const uint8_t *ptr;
    size_t remaining;
    Reader(const uint8_t *p, size_t n) : ptr(p), remaining(n) {
        if (!p && n)
            throw std::runtime_error("Null HDSoC bank data");
    }
    const uint8_t *take(size_t n) {
        if (n > remaining)
            throw std::runtime_error("Truncated HDSoC bank");
        auto p = ptr;
        if (n)
            ptr += n;
        remaining -= n;
        return p;
    }
    template <class T> T read() {
        T value;
        std::memcpy(&value, take(sizeof(T)), sizeof(T));
        return value;
    }
    void prefix(const char *magic) {
        if (std::memcmp(take(4), magic, 4))
            throw std::runtime_error("Invalid HDSoC bank magic");
        if (read<uint16_t>() != 1)
            throw std::runtime_error("Unsupported HDSoC bank version");
    }
    void done() const {
        if (remaining)
            throw std::runtime_error("Trailing HDSoC bank bytes");
    }
};
void require(bool value, const char *reason) {
    if (!value)
        throw std::runtime_error(reason);
}
} // namespace
dataProducts::NaluEvent decodeSourceEvent(const uint8_t *data, size_t size) {
    static_assert(sizeof(dataProducts::NaluEventHeader) == 36);
    static_assert(sizeof(dataProducts::NaluPacket) == 80);
    static_assert(sizeof(dataProducts::NaluTimeData) == 64);
    Reader r(data, size);
    dataProducts::NaluEvent out;
    out.header = r.read<dataProducts::NaluEventHeader>();
    const auto &h = out.header;
    require(h.packet_size == 80 && h.num_windows && !(h.event_info & 15),
            "Invalid nested HD header");
    require(r.remaining == size_t(h.num_packets) * 80 + sizeof(out.footer),
            "Nested HD size mismatch");
    out.packets.packets.reserve(h.num_packets);
    for (unsigned n = 0; n < h.num_packets; ++n) {
        auto packet = r.read<dataProducts::NaluPacket>();
        require(packet.channel < 64 && !(packet.packet_info & 15),
                "Invalid nested HD packet");
        out.packets.packets.push_back(packet);
    }
    out.footer = r.read<dataProducts::NaluEventFooter>();
    r.done();
    out.BuildWaveformsFromPackets();
    return out;
}
dataProducts::NaluBuilderEvent decodeEvent(const uint8_t *data, size_t size) {
    Reader r(data, size);
    r.prefix("HEB1");
    dataProducts::NaluBuilderEvent out;
    out.flags = r.read<uint16_t>();
    out.start_ns = r.read<uint64_t>();
    out.end_ns = r.read<uint64_t>();
    out.source_events = r.read<uint32_t>();
    out.packets = r.read<uint32_t>();
    require(!(out.flags & ~3U) && out.start_ns <= out.end_ns,
            "Invalid HB flags/times");
    require(out.source_events <=
                r.remaining / (12 + sizeof(dataProducts::NaluEventHeader) +
                               sizeof(dataProducts::NaluEventFooter)),
            "Invalid HB source count");
    out.sources.reserve(out.source_events);
    uint64_t packets = 0;
    for (uint32_t n = 0; n < out.source_events; ++n) {
        dataProducts::NaluBuilderSource source;
        source.board = r.read<uint16_t>();
        require(source.board < 2 && r.read<uint16_t>() == 0,
                "Invalid HB source board/reserved");
        const auto bytes = r.read<uint32_t>();
        const auto timing_bytes = r.read<uint32_t>();
        require(timing_bytes == 0 || timing_bytes == 64,
                "Invalid HB source timing size");
        source.event = decodeSourceEvent(r.take(bytes), bytes);
        packets += source.event.header.num_packets;
        source.has_timing = timing_bytes != 0;
        if (source.has_timing)
            source.timing = r.read<dataProducts::NaluTimeData>();
        out.sources.push_back(std::move(source));
    }
    r.done();
    require(packets == out.packets, "HB packet count mismatch");
    return out;
}
dataProducts::NaluCollectionTiming decodeTiming(const uint8_t *data,
                                                size_t size) {
    Reader r(data, size);
    r.prefix("HCT1");
    require(r.read<uint16_t>() == 80 && size == 80, "Invalid HC record size");
    dataProducts::NaluCollectionTiming out;
    out.cycle_id = r.read<uint64_t>();
    out.start_ns = r.read<uint64_t>();
    out.end_ns = r.read<uint64_t>();
    out.input_events = r.read<uint64_t>();
    out.input_packets = r.read<uint64_t>();
    out.output_events = r.read<uint64_t>();
    out.output_packets = r.read<uint64_t>();
    out.processing_ns = r.read<uint64_t>();
    out.flags = r.read<uint64_t>();
    require(out.start_ns <= out.end_ns && !(out.flags & ~1ULL),
            "Invalid HC flags/times");
    require(out.output_events <= out.input_events &&
                out.output_packets <= out.input_packets,
            "Invalid HC counters");
    r.done();
    return out;
}
dataProducts::NaluSelectionSummary decodeSelection(const uint8_t *data,
                                                   size_t size) {
    Reader r(data, size);
    r.prefix("HSS1");
    require(r.read<uint16_t>() == 88 && size == 88, "Invalid HS record size");
    dataProducts::NaluSelectionSummary out;
    out.cycle_id = r.read<uint64_t>();
    const auto *id = reinterpret_cast<const char *>(r.take(32));
    const auto *end = static_cast<const char *>(std::memchr(id, 0, 32));
    require(end != nullptr, "Unterminated HS selection ID");
    out.id.assign(id, end);
    const auto enabled = r.read<uint32_t>();
    require(enabled <= 1, "Invalid HS enable flag");
    out.enabled = enabled;
    out.priority = r.read<int32_t>();
    out.evaluated_events = r.read<uint64_t>();
    out.modified_events = r.read<uint64_t>();
    out.evaluated_packets = r.read<uint64_t>();
    out.removed_packets = r.read<uint64_t>();
    require(out.modified_events <= out.evaluated_events &&
                out.removed_packets <= out.evaluated_packets,
            "Invalid HS counters");
    r.done();
    return out;
}
} // namespace naluBuilder
