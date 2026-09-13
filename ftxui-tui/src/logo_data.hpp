#pragma once
#include <string>
#include <vector>
// Generated from assets/logo.png by build tooling. Half-block brand emblem.
namespace logo {
enum Tone { N, L, R };
struct EmblemCell { std::string glyph; Tone fg; Tone bg; };
// Emblem16: 16x8 cells (square)
inline const std::vector<std::vector<EmblemCell>> Emblem16 = {
  {{" ",N,N}, {" ",N,N}, {" ",N,N}, {" ",N,N}, {" ",N,N}, {" ",N,N}, {" ",N,N}, {" ",N,N}, {"▄",L,N}, {" ",N,N}, {" ",N,N}, {" ",N,N}, {" ",N,N}, {" ",N,N}, {" ",N,N}, {" ",N,N}},
  {{" ",N,N}, {" ",N,N}, {" ",N,N}, {" ",N,N}, {" ",N,N}, {" ",N,N}, {"▄",L,N}, {"▄",L,N}, {"█",L,N}, {"▀",L,N}, {"█",L,N}, {"▄",L,N}, {" ",N,N}, {" ",N,N}, {" ",N,N}, {" ",N,N}},
  {{" ",N,N}, {"▄",R,N}, {"▄",R,N}, {"▄",R,N}, {"▄",L,N}, {"█",L,N}, {"█",L,N}, {" ",N,N}, {" ",N,N}, {"▄",L,N}, {"▄",L,N}, {"▀",L,N}, {"█",L,N}, {"█",L,N}, {"▄",L,N}, {" ",N,N}},
  {{" ",N,N}, {" ",N,N}, {" ",N,N}, {"▄",R,N}, {"█",L,N}, {"█",L,N}, {"▀",L,N}, {"▄",L,N}, {"▄",L,N}, {" ",N,N}, {"█",L,N}, {"█",L,N}, {"█",L,N}, {"█",L,N}, {"█",L,N}, {"█",L,N}},
  {{" ",N,N}, {"▀",R,N}, {"█",R,N}, {"▀",R,N}, {"█",L,N}, {"█",L,N}, {"▀",L,N}, {" ",N,N}, {" ",N,N}, {" ",N,N}, {" ",N,N}, {"▀",L,N}, {"▄",L,N}, {" ",N,N}, {"█",L,N}, {"█",L,N}},
  {{" ",N,N}, {" ",N,N}, {" ",N,N}, {" ",N,N}, {"█",L,N}, {"█",L,N}, {"▄",L,N}, {" ",N,N}, {" ",N,N}, {"█",L,N}, {"█",L,N}, {"▄",L,N}, {" ",N,N}, {"█",L,N}, {"█",L,N}, {"▀",L,N}},
  {{" ",N,N}, {" ",N,N}, {" ",N,N}, {" ",N,N}, {" ",N,N}, {"▀",L,N}, {"█",L,N}, {"█",L,N}, {"▄",L,N}, {"▄",L,N}, {"█",L,N}, {"█",L,N}, {"█",L,N}, {"█",L,N}, {"▀",L,N}, {" ",N,N}},
  {{" ",N,N}, {" ",N,N}, {" ",N,N}, {" ",N,N}, {" ",N,N}, {" ",N,N}, {" ",N,N}, {"▀",L,N}, {"▀",L,N}, {"▀",L,N}, {"▀",L,N}, {" ",N,N}, {"█",L,N}, {"█",L,N}, {"█",L,N}, {" ",N,N}},
};

// Emblem12: 12x6 cells (square)
inline const std::vector<std::vector<EmblemCell>> Emblem12 = {
  {{" ",N,N}, {" ",N,N}, {" ",N,N}, {" ",N,N}, {" ",N,N}, {" ",N,N}, {"▄",L,N}, {"▄",L,N}, {" ",N,N}, {" ",N,N}, {" ",N,N}, {" ",N,N}},
  {{" ",N,N}, {" ",N,N}, {"▄",R,N}, {" ",N,N}, {"▄",L,N}, {"▀",L,N}, {"▀",L,N}, {"▄",L,N}, {"█",L,N}, {"▄",L,N}, {"▄",L,N}, {" ",N,N}},
  {{" ",N,N}, {" ",N,N}, {" ",N,N}, {"█",L,N}, {"█",L,N}, {" ",N,N}, {" ",N,N}, {"▀",L,N}, {"█",L,N}, {"█",L,N}, {"█",L,N}, {"█",L,N}},
  {{" ",N,N}, {"▀",R,N}, {"▀",R,N}, {"█",L,N}, {"█",L,N}, {" ",N,N}, {"▄",L,N}, {"▄",L,N}, {"▀",L,N}, {" ",N,N}, {"█",L,N}, {"█",L,N}},
  {{" ",N,N}, {" ",N,N}, {" ",N,N}, {"▀",L,N}, {"█",L,N}, {"▄",L,N}, {"▄",L,N}, {"█",L,N}, {"█",L,N}, {"▄",L,N}, {"█",L,N}, {"▀",L,N}},
  {{" ",N,N}, {" ",N,N}, {" ",N,N}, {" ",N,N}, {"▀",L,N}, {"▀",L,N}, {"▀",L,N}, {"█",L,N}, {"▀",L,N}, {"█",L,N}, {"█",L,N}, {"▄",L,N}},
};

// Emblem8: 8x4 cells (square)
inline const std::vector<std::vector<EmblemCell>> Emblem8 = {
  {{" ",N,N}, {" ",N,N}, {" ",N,N}, {"▄",L,N}, {"█",L,N}, {"▄",L,N}, {" ",N,N}, {" ",N,N}},
  {{" ",N,N}, {"▀",R,N}, {"█",L,N}, {"▀",L,N}, {" ",N,N}, {"█",L,N}, {"█",L,N}, {"█",L,N}},
  {{" ",N,N}, {"▀",R,N}, {"█",L,N}, {" ",N,N}, {"▄",L,N}, {"▄",L,N}, {"▄",L,N}, {"█",L,N}},
  {{" ",N,N}, {" ",N,N}, {"▀",L,N}, {"█",L,N}, {"█",L,N}, {"█",L,N}, {"█",L,N}, {"▄",L,N}},
};
} // namespace logo
