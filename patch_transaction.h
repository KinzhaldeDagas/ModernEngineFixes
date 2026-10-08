#pragma once

#include <windows.h>
#include <cstdint>
#include <cstring>

namespace mef
{
// Installation runs synchronously during OBSE startup, before these engine
// paths execute. This stages writes; it is not a concurrent hot-patching API.
class PatchTransaction
{
public:
    typedef BOOL (WINAPI* ProtectFn)(LPVOID, SIZE_T, DWORD, PDWORD);
    typedef BOOL (WINAPI* FlushFn)(HANDLE, LPCVOID, SIZE_T);
    static const unsigned MaxWrites = 192;
    static const unsigned MaxBytes = 256;
    static const unsigned MaxPages = 192;

    explicit PatchTransaction(ProtectFn protect = VirtualProtect,
        FlushFn flush = FlushInstructionCache)
        : m_protect(protect), m_flush(flush), m_writeCount(0), m_pageCount(0),
          m_finished(false), m_failed(false), m_rollbackIncomplete(false), m_error("none")
    {
        SYSTEM_INFO info = {};
        GetSystemInfo(&info);
        m_pageSize = info.dwPageSize;
    }

    bool Queue(std::uintptr_t address, const void* replacement, unsigned length)
    {
        if (m_finished || m_failed || !address || !replacement || !length ||
            length > MaxBytes || m_writeCount == MaxWrites ||
            address + length < address)
            return Fail("invalid patch plan or capacity exceeded");

        for (unsigned i = 0; i < m_writeCount; ++i)
        {
            const Write& previous = m_writes[i];
            if (address < previous.address + previous.length &&
                previous.address < address + length)
                return Fail("overlapping patch writes");
        }

        // Record each page separately: VirtualProtect reports only the first
        // page's previous protection when a range crosses a page boundary.
        for (std::uintptr_t page = address - address % m_pageSize;
            page < address + length; page += m_pageSize)
        {
            // A later Queue can revisit a page whose protection changed since
            // the first write was staged. Check before reading any source bytes.
            MEMORY_BASIC_INFORMATION info = {};
            if (!VirtualQuery(reinterpret_cast<void*>(page), &info, sizeof(info)) ||
                info.State != MEM_COMMIT || (info.Protect & (PAGE_NOACCESS | PAGE_GUARD)))
                return Fail("patch page is unavailable");
            unsigned i = 0;
            while (i < m_pageCount && m_pages[i].address != page)
                ++i;
            if (i == m_pageCount)
            {
                if (m_pageCount == MaxPages)
                    return Fail("patch page is unavailable");
                Page& entry = m_pages[m_pageCount++];
                entry.address = page;
                entry.protection = info.Protect;
                entry.writable = false;
            }
            else if (info.Protect != m_pages[i].protection)
                return Fail("patch page changed while staging patches");
            if (address + length - page <= m_pageSize)
                break;
        }

        Write& write = m_writes[m_writeCount++];
        write.address = address;
        write.length = length;
        std::memcpy(write.original, reinterpret_cast<void*>(address), length);
        std::memcpy(write.replacement, replacement, length);
        return true;
    }

    bool Commit()
    {
        if (m_finished || m_failed || !m_writeCount)
            return Fail("empty or finished patch transaction");
        m_finished = true;
        // Staging does not hold pages writable. Revalidate their commitment and
        // protection before memcmp, so a stale page fails without an access
        // violation or undoing another caller's deliberate protection change.
        // Installation still requires the documented non-concurrent startup.
        for (unsigned i = 0; i < m_pageCount; ++i)
        {
            MEMORY_BASIC_INFORMATION info = {};
            const Page& page = m_pages[i];
            if (!VirtualQuery(reinterpret_cast<void*>(page.address), &info, sizeof(info)) ||
                info.State != MEM_COMMIT || info.Protect != page.protection)
                return Fail("patch page changed while staging patches");
        }
        for (unsigned i = 0; i < m_writeCount; ++i)
        {
            const Write& write = m_writes[i];
            if (std::memcmp(reinterpret_cast<void*>(write.address),
                write.original, write.length) != 0)
                return Fail("engine bytes changed while staging patches");
        }
        if (!MakeWritable())
        {
            // No engine bytes have changed. A failed permission restore is
            // reported, but cannot leave a detour into an unloaded module.
            if (!RestoreProtection())
                m_error = "patch preflight failed; page protection restore also failed";
            return false;
        }

        CopyBytes(false);
        if (!m_flush(GetCurrentProcess(), NULL, 0))
        {
            m_error = "instruction cache flush failed";
            Rollback();
            return false;
        }
        if (!RestoreProtection())
        {
            m_error = "patch page protection restore failed";
            Rollback();
            return false;
        }
        return true;
    }

    const char* Error() const { return m_error; }
    bool RollbackIncomplete() const { return m_rollbackIncomplete; }
    unsigned WriteCount() const { return m_writeCount; }

private:
    struct Write
    {
        std::uintptr_t address;
        unsigned length;
        unsigned char original[MaxBytes];
        unsigned char replacement[MaxBytes];
    };
    struct Page
    {
        std::uintptr_t address;
        DWORD protection;
        bool writable;
    };
    ProtectFn m_protect;
    FlushFn m_flush;
    Write m_writes[MaxWrites];
    Page m_pages[MaxPages];
    unsigned m_writeCount;
    unsigned m_pageCount;
    DWORD m_pageSize;
    bool m_finished;
    bool m_failed;
    bool m_rollbackIncomplete;
    const char* m_error;

    bool Fail(const char* message) { m_error = message; m_failed = true; return false; }

    bool MakeWritable()
    {
        for (unsigned i = 0; i < m_pageCount; ++i)
        {
            Page& page = m_pages[i];
            if (page.writable)
                continue;
            DWORD previous = 0;
            if (!m_protect(reinterpret_cast<void*>(page.address), m_pageSize,
                PAGE_EXECUTE_READWRITE, &previous))
                return Fail("patch page protection preflight failed");
            page.writable = true;
        }
        return true;
    }

    bool RestoreProtection()
    {
        bool restored = true;
        for (unsigned i = m_pageCount; i > 0; --i)
        {
            Page& page = m_pages[i - 1];
            if (!page.writable)
                continue;
            DWORD previous = 0;
            if (m_protect(reinterpret_cast<void*>(page.address), m_pageSize,
                page.protection, &previous))
                page.writable = false;
            else
                restored = false;
        }
        return restored;
    }

    void CopyBytes(bool original)
    {
        for (unsigned i = 0; i < m_writeCount; ++i)
        {
            const Write& write = m_writes[i];
            std::memcpy(reinterpret_cast<void*>(write.address),
                original ? write.original : write.replacement, write.length);
        }
    }

    void Rollback()
    {
        if (!MakeWritable())
        {
            // The caller must retain its module reference if the OS prevents
            // restoring bytes; unloading would strand live code pointers.
            m_rollbackIncomplete = true;
            RestoreProtection();
            return;
        }
        CopyBytes(true);
        if (!m_flush(GetCurrentProcess(), NULL, 0))
            m_rollbackIncomplete = true;
        if (!RestoreProtection())
            m_error = "patches rolled back; page protection restore failed";
    }
};
}
