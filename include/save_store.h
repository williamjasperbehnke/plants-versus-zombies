#ifndef GARDEN_SAVE_STORE_H
#define GARDEN_SAVE_STORE_H
#include "progress.h"
#include "bn_sram.h"
namespace garden
{
class SaveStore
{
public:
    Progress load()
    {
        if(! read_records<SaveRecord>(offset, decode_progress))
        {
            // Migrate the old pair without overwriting either backup until the new save exists.
            if(! read_records<V2SaveRecord>(v2_offset, decode_v2_progress))
            { read_records<LegacySaveRecord>(legacy_offset, decode_legacy_progress); }
            _loaded = false;
        }
#ifdef GARDEN_DEBUG_SAVE
        if(_last.unlocked != level_count) { _loaded = false; _last.unlocked = level_count; }
#endif
        return _last;
    }
    void update(const Progress& progress)
    {
        if(_loaded && progress == _last) { return; }
        _slot = 1 - _slot;
        auto record = encode_progress(progress, ++_sequence);
        bn::sram::write_offset(record, offset + _slot * int(sizeof(SaveRecord)));
        _last = progress;
        _loaded = true;
    }
private:
    template<class Record, class Decoder> bool read_records(int start, Decoder decode)
    {
        Record records[2];
        Progress values[2];
        bool valid[2];
        for(int i = 0; i < 2; ++i)
        {
            bn::sram::read_offset(records[i], start + i * int(sizeof(Record)));
            valid[i] = decode(records[i], values[i]);
        }
        if(! valid[0] && ! valid[1]) { return false; }
        uint32_t distance = save_word(records[1], save_format::sequence_offset) - save_word(records[0], save_format::sequence_offset);
        bool second_newer = valid[1] && (! valid[0] || distance < 0x80000000u);
        _slot = second_newer ? 1 : 0;
        _sequence = save_word(records[_slot], save_format::sequence_offset);
        _last = values[_slot];
        _loaded = true;
        return true;
    }
#ifdef GARDEN_DEBUG_SAVE
    static constexpr int offset = PVZ_SAVE_DEBUG_OFFSET;
    static constexpr int v2_offset = PVZ_SAVE_V2_DEBUG_OFFSET;
    static constexpr int legacy_offset = 128;
#else
    static constexpr int offset = PVZ_SAVE_NORMAL_OFFSET;
    static constexpr int v2_offset = PVZ_SAVE_V2_NORMAL_OFFSET;
    static constexpr int legacy_offset = 0;
#endif
    Progress _last;
    uint32_t _sequence = 0;
    int _slot = 0;
    bool _loaded = false;
};
}
#endif
