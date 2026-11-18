#ifndef GARDEN_TEXT_WRAP_H
#define GARDEN_TEXT_WRAP_H
#include <array>

namespace garden {
// Fixed storage keeps text layout allocation-free on the GBA.
template <int Columns, int Capacity>
class WrappedText {
public:
    explicit WrappedText(const char* text) {
        while (*text) {
            while (*text && *text <= ' ') {
                ++text;
            }
            if (!*text) {
                break;
            }
            int length = 0;
            while (text[length] > ' ') {
                ++length;
            }
            if (_width && _width + 1 + length <= Columns) {
                append(' ');
            } else if (_width) {
                if (!next_line()) {
                    return;
                }
            }
            for (int i = 0; i < length; ++i) {
                if (_width == Columns && !next_line()) {
                    return;
                }
                append(text[i]);
            }
            text += length;
        }
    }

    int size() const {
        return _count;
    }

    const char* operator[](int line) const {
        return _lines[line].data();
    }

    bool truncated() const {
        return _truncated;
    }

private:
    void append(char value) {
        if (!_count) {
            _count = 1;
        }
        _lines[_count - 1][_width++] = value;
    }

    bool next_line() {
        if (_count == Capacity) {
            _truncated = true;
            int start = _width < Columns - 3 ? _width : Columns - 3;
            for (int i = start; i < start + 3; ++i) {
                _lines[Capacity - 1][i] = '.';
            }
            _lines[Capacity - 1][start + 3] = '\0';
            return false;
        }
        ++_count;
        _width = 0;
        return true;
    }

    static_assert(Columns >= 3 && Capacity > 0);
    std::array<std::array<char, Columns + 1>, Capacity> _lines{};
    int _count = 0;
    int _width = 0;
    bool _truncated = false;
};
} // namespace garden
#endif
