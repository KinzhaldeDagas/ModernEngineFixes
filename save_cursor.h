#pragma once
#include <cstddef>
#include <cstring>
namespace mef::interop {
// Observed at Oblivion 4534D8/4534E7: +14h is a borrowed source cursor.
// The engine/active read scope owns its storage; this accessor never frees it.
// A full TESSaveLoadGame layout is deliberately not declared here.
inline constexpr std::size_t SaveCursorOffset = 0x14;
static_assert(sizeof(unsigned char*) == 4, "Oblivion cursor width");
inline unsigned char* ReadSaveCursor(const void* owner)
{
    unsigned char* cursor;
    std::memcpy(&cursor, static_cast<const unsigned char*>(owner) + SaveCursorOffset, sizeof(cursor));
    return cursor;
}
inline void WriteSaveCursor(void* owner, unsigned char* cursor)
{
    std::memcpy(static_cast<unsigned char*>(owner) + SaveCursorOffset, &cursor, sizeof(cursor));
}
}
