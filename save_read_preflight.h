// SR-1: immutable input for the framing that has been proved in Oblivion.
// Included inside dllmain.cpp's private namespace. Unsupported versions and
// created-object tables are explicit results, never silently called protected.
enum SaveReadPreflightResult
{
    kSaveReadAccepted,
    kSaveReadUnsupportedVersion,
    kSaveReadUnsupportedStream,
    kSaveReadUnsupportedCreatedObjects,
    kSaveReadInvalidFraming,
    kSaveReadSnapshotRefused,
    kSaveReadAcquisitionFailed,
    kSaveReadUnexpectedShortRead,
    kSaveReadUnexpectedSeek
};
struct SaveReadLayout
{
    UInt32 pluginStart, changeSection, recordsStart, recordsEnd;
    UInt32 idArraysStart, idArraysEnd, recordCount;
};
struct SaveReadCursor
{
    const UInt8* bytes;
    UInt32 size, position;
    bool Take(UInt32 length, const UInt8** output = NULL)
    {
        if (position > size || length > size - position) return false;
        if (output) *output = bytes + position;
        position += length;
        return true;
    }
    bool Byte(UInt32* output)
    {
        const UInt8* p;
        if (!Take(1, &p)) return false;
        *output = *p;
        return true;
    }
    bool Word(UInt32* output)
    {
        const UInt8* p;
        if (!Take(2, &p)) return false;
        *output = (UInt32)p[0] | ((UInt32)p[1] << 8);
        return true;
    }
    bool Dword(UInt32* output)
    {
        const UInt8* p;
        if (!Take(4, &p)) return false;
        std::memcpy(output, p, 4);
        return true;
    }
    bool WordBlock()
    {
        UInt32 length;
        return Word(&length) && Take(length);
    }
};
static SaveReadPreflightResult SaveReadParse(const UInt8* bytes, UInt32 size,
    UInt32 pluginStart, UInt32 containerBase, UInt8 version, SaveReadLayout* output)
{
    // 45FC60 reads the temp-effects payload directly from version 5Eh onward.
    // Future versions and legacy framing are left explicitly unsupported.
    if (version < 0x5E || version > 0x7D) return kSaveReadUnsupportedVersion;
    if (!bytes || !output || pluginStart > size) return kSaveReadInvalidFraming;
    SaveReadLayout layout = {};
    layout.pluginStart = pluginStart;
    SaveReadCursor c = { bytes, size, pluginStart };
    UInt32 count, length, offset;
    if (!c.Byte(&count)) return kSaveReadInvalidFraming;
    for (UInt32 i = 0; i < count; ++i)
        if (!c.Byte(&length) || !c.Take(length)) return kSaveReadInvalidFraming;
    layout.changeSection = c.position;
    if (!c.Dword(&offset) || !offset || !c.Dword(&layout.recordCount) ||
        containerBase > size || offset > size - containerBase)
        return kSaveReadInvalidFraming;
    layout.idArraysStart = containerBase + offset;
    if (layout.idArraysStart < c.position) return kSaveReadInvalidFraming;
    c.size = layout.idArraysStart;

    // 462B20: five DWORDs and player XYZ; then 45F970's u16 global count.
    // Native global-buffer size narrows 8*count to u16: reject that overflow.
    if (!c.Take(32) || !c.Word(&count) || count > 0xFFFFu / 8 || !c.Take(count * 8))
        return kSaveReadInvalidFraming;
    for (UInt32 i = 0; i < 4; ++i)
        if (!c.WordBlock()) return kSaveReadInvalidFraming;
    if (!c.Take(4) || !c.Dword(&count)) return kSaveReadInvalidFraming;
    // 461310 dispatches created TES records through arbitrary form loaders.
    // Neither 44DCF0 nor 447050 guarantees a common final stream position.
    if (count) return kSaveReadUnsupportedCreatedObjects;
    if (!c.WordBlock()) return kSaveReadInvalidFraming; // quick keys
    if (version >= 0x21 && !c.WordBlock()) return kSaveReadInvalidFraming;
    if (version >= 0x53 && !c.WordBlock()) return kSaveReadInvalidFraming;
    if (version >= 0x79 && !c.WordBlock()) return kSaveReadInvalidFraming;
    layout.recordsStart = c.position;
    if (layout.recordCount > (c.size - c.position) / 12)
        return kSaveReadInvalidFraming;
    for (UInt32 i = 0; i < layout.recordCount; ++i)
    {
        const UInt8* header;
        if (!c.Take(12, &header)) return kSaveReadInvalidFraming;
        UInt32 id, flags;
        std::memcpy(&id, header, 4);
        std::memcpy(&flags, header + 5, 4);
        length = (UInt32)header[10] | ((UInt32)header[11] << 8);
        if (id == 0xFEFFFFFFu)
        {
            // Native sentinel ignores header length and consumes exactly five.
            if (length != 5) return kSaveReadInvalidFraming;
        }
        else
        {
            UInt32 preview = 0;
            if (flags & 2)
            {
                if (flags & 4)
                {
                    if (header[9] < 0x5B) preview = 12;
                    else if (flags & 0x04000000u) preview = 4;
                    else if (flags & 0x02000000u) preview = 6;
                }
                else preview = 36;
            }
            else if (flags & 0x80000000u) preview = 44;
            if (length < preview) return kSaveReadInvalidFraming;
        }
        if (!c.Take(length)) return kSaveReadInvalidFraming;
    }
    layout.recordsEnd = c.position;
    // SaveGame emits temp effects before capturing the ID-array offset.
    if (!c.Dword(&length) || !c.Take(length) || c.position != c.size)
        return kSaveReadInvalidFraming;
    c.size = size;
    for (UInt32 i = 0; i < 2; ++i)
    {
        if (!c.Dword(&count) || count > (c.size - c.position) / 4 || !c.Take(count * 4))
            return kSaveReadInvalidFraming;
    }
    layout.idArraysEnd = c.position;
    *output = layout;
    return kSaveReadAccepted;
}

typedef UInt32 (__thiscall* SaveReadNativeReadFn)(UInt8*, void*, UInt32);
typedef void (__thiscall* SaveReadNativeSeekFn)(UInt8*, UInt32, UInt32);
typedef UInt8* (__thiscall* SaveReadNativeDestroyFn)(UInt8*, UInt8);
struct SaveReadServices
{
    void** nativeVtable;
    void* nativeReadCallback;
    FormHeapAllocFn allocate;
    FormHeapFreeFn release;
    bool (*extent)(UInt8*, UInt32*);
    void (*event)(SaveReadPreflightResult);
    UInt32 maximumBytes;
};
struct SaveReadSnapshot
{
    void* table[17]; // RTTI locator followed by the sixteen BSFile entries.
    UInt8* stream;
    UInt8* bytes;
    UInt32 size;
    void** originalVtable;
    SaveReadServices service;
    SaveReadLayout layout;
};
static SaveReadSnapshot* SaveReadSnapshotFor(UInt8* stream)
{
    return (SaveReadSnapshot*)((UInt8*)*(void***)stream - sizeof(void*));
}
static void SaveReadReport(const SaveReadServices& service, SaveReadPreflightResult result)
{
    if (service.event) service.event(result);
}
static UInt32 __fastcall SaveReadSnapshotRead(UInt8* stream, void*, void* destination, UInt32 requested)
{
    SaveReadSnapshot* snapshot = SaveReadSnapshotFor(stream);
    UInt32 position = *(UInt32*)(stream + 0x148);
    UInt32 available = position <= snapshot->size ? snapshot->size - position : 0;
    UInt32 obtained = requested < available ? requested : available;
    if (obtained) std::memcpy(destination, snapshot->bytes + position, obtained);
    *(UInt32*)(stream + 0x148) += obtained;
    if (obtained != requested) SaveReadReport(snapshot->service, kSaveReadUnexpectedShortRead);
    return obtained;
}
static void __fastcall SaveReadSnapshotSeek(UInt8* stream, void*, UInt32 offset, UInt32 origin)
{
    SaveReadSnapshot* snapshot = SaveReadSnapshotFor(stream);
    UInt32* position = (UInt32*)(stream + 0x148);
    if (origin == 0) *position = offset;
    else if (origin == 1) *position += offset;
    else if (origin == 2) *position = snapshot->size - offset; // native END convention
    else { SaveReadReport(snapshot->service, kSaveReadUnexpectedSeek); return; }
    if (*position > snapshot->size) SaveReadReport(snapshot->service, kSaveReadUnexpectedSeek);
}
static UInt32 __fastcall SaveReadSnapshotSize(UInt8* stream, void*)
{
    const UInt32 size = SaveReadSnapshotFor(stream)->size;
    // 430010/430240 expose the same cached size at +150; the direct path
    // updates that cache. Preserve it while using the immutable extent.
    *(UInt32*)(stream + 0x150) = size;
    return size;
}
static UInt8* __fastcall SaveReadSnapshotDestroy(UInt8* stream, void*, UInt8 flags)
{
    SaveReadSnapshot* snapshot = SaveReadSnapshotFor(stream);
    SaveReadNativeDestroyFn destroy = (SaveReadNativeDestroyFn)snapshot->originalVtable[0];
    FormHeapFreeFn release = snapshot->service.release;
    UInt8* bytes = snapshot->bytes;
    *(void***)stream = snapshot->originalVtable;
    // Detach first; no callback can observe a dangling cloned table.
    release(bytes);
    release(snapshot);
    return destroy(stream, flags);
}
static bool SaveReadIsSnapshot(const UInt8* stream)
{
    if (!stream) return false;
    void** table = *(void***)stream;
    // Test the first slot before examining the rest of an unknown vtable.
    if (!table || table[0] != (void*)&SaveReadSnapshotDestroy ||
        table[3] != (void*)&SaveReadSnapshotSeek || table[14] != (void*)&SaveReadSnapshotRead ||
        table[4] != (void*)&SaveReadSnapshotSize || table[7] != (void*)&SaveReadSnapshotSize)
        return false;
    SaveReadSnapshot* snapshot = SaveReadSnapshotFor((UInt8*)stream);
    return snapshot->stream == stream && snapshot->bytes &&
        snapshot->originalVtable == snapshot->service.nativeVtable;
}
static bool SaveReadSnapshotExtent(UInt8* stream, UInt32* output)
{
    if (!output || !SaveReadIsSnapshot(stream)) return false;
    *output = SaveReadSnapshotFor(stream)->size;
    return true;
}
static SaveReadPreflightResult SaveReadBindSnapshot(UInt8* owner, UInt8* stream,
    const SaveReadServices& service, SaveReadLayout* output = NULL)
{
    if (!owner || owner[0x70] || owner[0x71] < 0x5E || owner[0x71] > 0x7D)
        return kSaveReadUnsupportedVersion;
    // ResolveSaveFile's public load argument is 1, but 45F723 passes mode 0
    // to BSFile ctor430970;4309AE stores it at+20.42FE92/42FEAF selects "rb"
    // for that zero mode. The resolver parameter is not the stream mode.
    if (!stream || *(void***)stream != service.nativeVtable ||
        *(void**)(stream + 4) != service.nativeReadCallback ||
        *(UInt32*)(stream + 0x20) != 0 || !stream[0x24] ||
        !*(void**)(stream + 0x1C) || *(UInt32*)(stream + 0x30) != 0xFFFFFFFFu)
        return kSaveReadUnsupportedStream;
    UInt32 size = 0;
    const UInt32 position = *(UInt32*)(stream + 0x148);
    if (!service.extent(stream, &size) || position > size) return kSaveReadAcquisitionFailed;
    if (!size || size > service.maximumBytes) return kSaveReadSnapshotRefused;
    SaveReadSnapshot* snapshot = (SaveReadSnapshot*)service.allocate(sizeof(SaveReadSnapshot));
    if (!snapshot) return kSaveReadSnapshotRefused;
    std::memset(snapshot, 0, sizeof(*snapshot));
    SaveReadPreflightResult result = kSaveReadSnapshotRefused;
    const SaveReadNativeSeekFn seek = (SaveReadNativeSeekFn)service.nativeVtable[3];
    const SaveReadNativeReadFn read = (SaveReadNativeReadFn)service.nativeVtable[14];
    __try
    {
        snapshot->bytes = (UInt8*)service.allocate(size);
        if (snapshot->bytes)
        {
            snapshot->stream = stream;
            snapshot->size = size;
            snapshot->service = service;
            snapshot->originalVtable = service.nativeVtable;
            seek(stream, 0, 0);
            UInt32 copied = 0;
            result = kSaveReadAcquisitionFailed;
            while (copied < size && *(UInt32*)(stream + 0x148) == copied)
            {
                UInt32 chunk = size - copied;
                if (chunk > 0x10000u) chunk = 0x10000u;
                if (read(stream, snapshot->bytes + copied, chunk) != chunk ||
                    *(UInt32*)(stream + 0x148) != copied + chunk) break;
                copied += chunk;
            }
            UInt32 finalSize = 0;
            if (copied == size && service.extent(stream, &finalSize) && finalSize == size)
                result = SaveReadParse(snapshot->bytes, size, position,
                    *(UInt32*)(owner + 0x8C), owner[0x71], &snapshot->layout);
            if (result == kSaveReadAccepted)
            {
                // Acquisition used the original FILE. Every subsequent native
                // callback/virtual seek observes the retained immutable bytes.
                std::memcpy(snapshot->table, service.nativeVtable - 1, sizeof(snapshot->table));
                snapshot->table[1] = (void*)&SaveReadSnapshotDestroy;
                snapshot->table[4] = (void*)&SaveReadSnapshotSeek;
                snapshot->table[5] = (void*)&SaveReadSnapshotSize;
                snapshot->table[8] = (void*)&SaveReadSnapshotSize;
                snapshot->table[15] = (void*)&SaveReadSnapshotRead;
                *(UInt32*)(stream + 0x148) = position;
                *(void***)stream = snapshot->table + 1;
                if (output) *output = snapshot->layout;
                snapshot = NULL;
            }
            else if (result == kSaveReadUnsupportedCreatedObjects)
            {
                // Unsupported shapes retain native behavior, including its
                // outstanding SR-1 risk. Restore the real buffered stream first.
                seek(stream, position, 0);
                if (*(UInt32*)(stream + 0x148) != position) result = kSaveReadAcquisitionFailed;
            }
        }
    }
    __finally
    {
        if (snapshot)
        {
            if (snapshot->bytes) service.release(snapshot->bytes);
            service.release(snapshot);
        }
    }
    SaveReadReport(service, result);
    return result;
}
