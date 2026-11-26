#include "analysis_pipeline/unpacker_nalu/utils/builder_bank_decoder.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
using Bytes = std::vector<uint8_t>;
void check(bool b) {
    if (!b)
        throw std::runtime_error("HDSoC decoder assertion failed");
}
template <class T> void append(Bytes &b, const T &value) {
    auto p = reinterpret_cast<const uint8_t *>(&value);
    b.insert(b.end(), p, p + sizeof(T));
}
void prefix(Bytes &b, const char *magic, uint16_t field) {
    b.insert(b.end(), magic, magic + 4);
    append(b, uint16_t(1));
    append(b, field);
}
template <class F> void rejects(F f) {
    bool failed = false;
    try {
        f();
    } catch (const std::exception &) {
        failed = true;
    }
    check(failed);
}
Bytes source(unsigned windows) {
    Bytes b;
    dataProducts::NaluEventHeader h{};
    h.event_header = 0xbbbb;
    h.event_info = 0x10;
    h.event_index = 17;
    h.event_reference_time = 123;
    h.packet_size = 80;
    h.channel_mask = (1ULL << 31) | 1;
    h.num_windows = windows;
    h.num_packets = windows * 2;
    append(b, h);
    for (unsigned ch : {0U, 31U})
        for (unsigned w = 0; w < windows; ++w) {
            dataProducts::NaluPacket p{};
            p.packet_header = 0xaaaa;
            p.packet_footer = 0xffff;
            p.channel = ch;
            p.trigger_time = 123;
            p.logical_position = w;
            p.window_position = w;
            for (unsigned k = 0; k < 32; ++k) {
                uint16_t adc = 1000 + ch + w * 32 + k;
                p.trace[k] = (adc >> 8) | (adc << 8);
            }
            append(b, p);
        }
    append(b, uint16_t(0xeeee));
    return b;
}
int main() {
    try {
        auto hd = source(61);
        auto event = naluBuilder::decodeSourceEvent(hd.data(), hd.size());
        check(event.waveforms.waveforms.size() == 2);
        for (const auto &wf : event.waveforms.waveforms) {
            check(wf.trace.size() == 1952);
            for (unsigned k = 0; k < 1952; ++k)
                check(wf.trace[k] == 1000 + wf.channel_num + k);
        }
        Bytes hb;
        prefix(hb, "HEB1", 0);
        append(hb, uint64_t(10));
        append(hb, uint64_t(20));
        append(hb, uint32_t(2));
        append(hb, uint32_t(244));
        for (unsigned board = 0; board < 2; ++board) {
            append(hb, uint16_t(board));
            append(hb, uint16_t(0));
            append(hb, uint32_t(hd.size()));
            append(hb, uint32_t(board ? 0 : 64));
            hb.insert(hb.end(), hd.begin(), hd.end());
            if (!board) {
                dataProducts::NaluTimeData t{};
                t.collection_cycle_index = 9;
                append(hb, t);
            }
        }
        auto decoded = naluBuilder::decodeEvent(hb.data(), hb.size());
        check(decoded.sources.size() == 2 && decoded.packets == 244);
        check(decoded.sources[0].board == 0 && decoded.sources[0].has_timing);
        check(decoded.sources[0].timing.collection_cycle_index == 9 &&
              !decoded.sources[1].has_timing);
        for (size_t n = 0; n < hb.size(); ++n) {
            // All truncations before the final source record must fail.
            if (n < 48 || n > hb.size() - 100)
                rejects([&] { naluBuilder::decodeEvent(hb.data(), n); });
        }
        auto bad = hb;
        bad[0] = 'X';
        rejects([&] { naluBuilder::decodeEvent(bad.data(), bad.size()); });
        bad = hb;
        bad[4] = 2;
        rejects([&] { naluBuilder::decodeEvent(bad.data(), bad.size()); });
        bad = hb;
        bad[28]++;
        rejects([&] { naluBuilder::decodeEvent(bad.data(), bad.size()); });
        bad = hb;
        bad[32] = 2;
        rejects([&] { naluBuilder::decodeEvent(bad.data(), bad.size()); });
        bad = hb;
        bad[40] = 1;
        rejects([&] { naluBuilder::decodeEvent(bad.data(), bad.size()); });
        bad = hb;
        bad.push_back(0);
        rejects([&] { naluBuilder::decodeEvent(bad.data(), bad.size()); });
        rejects([&] { naluBuilder::decodeEvent(nullptr, 32); });
        Bytes empty;
        prefix(empty, "HEB1", 1);
        append(empty, uint64_t(20));
        append(empty, uint64_t(21));
        append(empty, uint32_t(0));
        append(empty, uint32_t(0));
        check(naluBuilder::decodeEvent(empty.data(), empty.size())
                  .sources.empty());
        Bytes hc;
        prefix(hc, "HCT1", 80);
        for (uint64_t x : {1, 10, 20, 2, 244, 0, 0, 50, 1})
            append(hc, x);
        auto timing = naluBuilder::decodeTiming(hc.data(), hc.size());
        check(timing.input_packets == 244 && !timing.output_packets);
        for (size_t n = 0; n < hc.size(); ++n)
            rejects([&] { naluBuilder::decodeTiming(hc.data(), n); });
        bad = hc;
        bad[6] = 79;
        rejects([&] { naluBuilder::decodeTiming(bad.data(), bad.size()); });
        Bytes hs;
        prefix(hs, "HSS1", 88);
        append(hs, uint64_t(1));
        char id[32] = "dummy";
        hs.insert(hs.end(), id, id + 32);
        append(hs, uint32_t(1));
        append(hs, int32_t(-10));
        for (uint64_t x : {1, 1, 244, 244})
            append(hs, x);
        auto selection = naluBuilder::decodeSelection(hs.data(), hs.size());
        check(selection.id == "dummy" && selection.enabled &&
              selection.priority == -10 && selection.removed_packets == 244);
        for (size_t n = 0; n < hs.size(); ++n)
            rejects([&] { naluBuilder::decodeSelection(hs.data(), n); });
        bad = hs;
        std::fill(bad.begin() + 16, bad.begin() + 48, 'x');
        rejects([&] { naluBuilder::decodeSelection(bad.data(), bad.size()); });
        bad = hs;
        bad[48] = 2;
        rejects([&] { naluBuilder::decodeSelection(bad.data(), bad.size()); });
        std::cout << "HDSoC decoder: 61-window waveforms, boards, optional "
                     "timing, empty/cut events and malformed banks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
