#include "defines.h"
#include "memoryCommands.h"
#include "moduleBase.h"
#include "util.h"
#include "logger.h"
#include <cstring>

namespace MemoryCommands {
	using namespace SbbLog;
    using namespace Util;
    using namespace ModuleBase;

    /**
     * @brief Read memory from the specified offset and size into the buffer.
     * @param The memory offset.
     * @param The number of bytes to read.
     * @param[out] Output buffer for the read data.
     */
    void Vision::peek(u64 offset, u64 size, std::vector<char>& buffer) {
        u64 total = 0;
        u64 remainder = size;
        buffer.resize(size);

        while (remainder > 0) {
            u64 receive = remainder > MAX_LINE_LENGTH ? MAX_LINE_LENGTH : remainder;
            remainder -= receive;
            Result rc = readMem(buffer, offset + total, receive);
            if (R_FAILED(rc)) {
                Logger::instance().log("peek() readMem() failed. Offset=" + std::to_string(offset + total) + ", Size=" + std::to_string(receive), std::to_string(R_DESCRIPTION(rc)));
                buffer.assign(size, 0);
                break;
            }
            total += receive;
        }

        if (g_enableBackwardsCompat && !Utils::isUSB()) {
            Utils::hexify(buffer);
        }
    }

    /**
     * @brief Read multiple memory regions into the buffer.
     * @param Vector of memory offsets.
     * @param Vector of sizes for each region.
     * @param[out] Output buffer for the read data.
     */
    void Vision::peekMulti(const std::vector<u64>& offsets, const std::vector<u64>& sizes, std::vector<char>& buffer) {
        u64 ofs = 0;
        u64 totalSize = 0;
        int size = (int)sizes.size();
        for (int i = 0; i < size; i++) {
            totalSize += sizes[i];
        }

        buffer.resize(totalSize * sizeof(u8));
        int count = (int)offsets.size();
        for (int i = 0; i < count; i++) {
            Result rc = readMem(buffer, offsets[i], sizes[i], ofs);
            if (R_FAILED(rc)) {
                Logger::instance().log("peekMulti() readMem() failed. Offset=" + std::to_string(offsets[i]) + ", Size=" + std::to_string(sizes[i]), std::to_string(R_DESCRIPTION(rc)));
                buffer.assign(totalSize * sizeof(u8), 0);
                break;
            }
            ofs += sizes[i];
        }

        if (g_enableBackwardsCompat && !Utils::isUSB()) {
            Utils::hexify(buffer);
        }
    }

    /**
     * @brief Write memory to the specified offset.
     * @param The memory offset.
     * @param The number of bytes to write.
     * @param Input buffer containing data to write.
     */
    void Vision::poke(u64 offset, u64 size, const std::vector<char>& buffer) {
        writeMem(offset, size, buffer);
    }

    /**
     * @brief Follow a pointer chain starting from main, applying jumps, and return the final address.
     * @param The base pointer offset.
     * @param Vector of jumps to follow.
     * @param[out] Buffer used for intermediate reads.
     * @return The final address after following the pointer chain.
     */
    u64 Vision::followMainPointer(const s64& main, const std::vector<s64>& jumps, std::vector<char>& buffer) {
        u64 offset = 0;
        u64 size = sizeof(u64);
        buffer.resize(size);

        Result rc = readMem(buffer, m_metaData.main_nso_base + main, size);
        if (R_FAILED(rc)) {
            Logger::instance().log("followMainPointer() initial readMem() failed. Main=" + std::to_string(main), std::to_string(R_DESCRIPTION(rc)));
            return 0;
        }

        std::memcpy(&offset, buffer.data(), size);
        int count = (int)jumps.size();
        for (int i = 0; i < count; i++) {
            rc = readMem(buffer, offset + jumps[i], size);
            if (R_FAILED(rc)) {
                Logger::instance().log("followMainPointer() readMem() failed. Offset=" + std::to_string(offset) + ", Jump=" + std::to_string(jumps[i]), std::to_string(R_DESCRIPTION(rc)));
                return 0;
            }

            std::memcpy(&offset, buffer.data(), size);
            if (offset == 0) {
                break;
            }
        }

        return offset;
    }

    /**
     * @brief Read memory from the debugged process.
     * @param Buffer to store the read data.
     * @param The memory offset.
     * @param The number of bytes to read.
     * @param Offset into the buffer for multi-read (default 0).
     */
    Result Vision::readMem(const std::vector<char>& buffer, u64 offset, u64 size, u64 multi) {
        attach();
        Result rc = svcReadDebugProcessMemory((void*)(buffer.data() + multi), m_debugHandle, offset, size);
        detach();
        return rc;
    }

    /**
     * @brief Write memory to the debugged process.
     * @param The memory offset.
     * @param The number of bytes to write.
     * @param Buffer containing data to write.
     */
    void Vision::writeMem(u64 offset, u64 size, const std::vector<char>& buffer) {
        attach();
        Result rc = svcWriteDebugProcessMemory(m_debugHandle, (void*)buffer.data(), offset, size);
        if (R_FAILED(rc)) {
            Logger::instance().log("writeMem() svcWriteDebugProcessMemory() failed. Offset=" + std::to_string(offset) + ", Size=" + std::to_string(size), std::to_string(R_DESCRIPTION(rc)));
        }
        detach();
    }
}
