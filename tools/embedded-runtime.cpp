#define DOOM_EMBEDDED_BUILD 1
#include <bcrypt.h>
#include <compressapi.h>
#include <shlobj.h>
#include <wincrypt.h>

#include <array>
#include <cstdint>
#include <vector>

namespace DoomBundle {

struct File {
    PCWSTR name;
    int campaign; // 0 is an engine file.
    DWORD size;
    std::array<UCHAR, 32> hash;
    const char* const* chunks;
    size_t chunkCount;
    size_t packedSize;
};

// EMBEDDED DOOM ASSETS

bool RegularDirectory(const std::wstring& path) {
    if (!CreateDirectoryW(path.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) {
        return false;
    }
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) &&
           !(attributes & FILE_ATTRIBUTE_REPARSE_POINT);
}

bool Matches(BCRYPT_ALG_HANDLE sha, const File& file, std::vector<UCHAR>& data) {
    std::array<UCHAR, 32> digest{};
    return data.size() == file.size &&
           BCryptHash(sha, nullptr, 0, data.data(), file.size, digest.data(),
                      static_cast<ULONG>(digest.size())) == 0 &&
           digest == file.hash;
}

bool AlreadyExtracted(BCRYPT_ALG_HANDLE sha, const File& file, const std::wstring& path) {
    HANDLE handle = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (handle == INVALID_HANDLE_VALUE) return false;
    BY_HANDLE_FILE_INFORMATION info{};
    bool matches = false;
    if (GetFileInformationByHandle(handle, &info) &&
        !(info.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) &&
        !info.nFileSizeHigh && info.nFileSizeLow == file.size) {
        std::vector<UCHAR> data(file.size);
        DWORD read{};
        matches = ReadFile(handle, data.data(), file.size, &read, nullptr) &&
                  read == file.size && Matches(sha, file, data);
    }
    CloseHandle(handle);
    return matches;
}

bool Extract(BCRYPT_ALG_HANDLE sha, DECOMPRESSOR_HANDLE unpack, const File& file,
             const std::wstring& directory) {
    const std::wstring path = directory + L"\\" + file.name;
    if (AlreadyExtracted(sha, file, path)) return true;

    std::vector<UCHAR> packed(file.packedSize);
    size_t offset{};
    for (size_t i = 0; i < file.chunkCount; ++i) {
        DWORD written = static_cast<DWORD>(packed.size() - offset);
        if (!CryptStringToBinaryA(file.chunks[i], 0, CRYPT_STRING_BASE64,
                                  packed.data() + offset, &written, nullptr, nullptr)) return false;
        offset += written;
    }
    if (offset != packed.size()) return false;
    std::vector<UCHAR> data(file.size);
    SIZE_T size{};
    if (!Decompress(unpack, packed.data(), packed.size(), data.data(), data.size(), &size) ||
        size != file.size || !Matches(sha, file, data)) return false;

    wchar_t temporary[MAX_PATH]{};
    if (!GetTempFileNameW(directory.c_str(), L"dm", 0, temporary)) return false;
    HANDLE handle = CreateFileW(temporary, GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                                FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    bool success = false;
    if (handle != INVALID_HANDLE_VALUE) {
        DWORD written{};
        success = WriteFile(handle, data.data(), file.size, &written, nullptr) && written == file.size;
        CloseHandle(handle);
        if (success) success = MoveFileExW(temporary, path.c_str(), MOVEFILE_REPLACE_EXISTING) != FALSE;
    }
    if (!success) DeleteFileW(temporary);
    return success;
}

bool Prepare(std::wstring& engine, std::wstring& wad, int campaign) {
    PWSTR local{};
    if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, &local))) return false;
    const std::wstring root = std::wstring(local) + L"\\Windhawk Doom";
    CoTaskMemFree(local);
    const std::wstring directory = root + L"\\" + kBundleDirectory;
    if (!RegularDirectory(root) || !RegularDirectory(directory)) return false;

    BCRYPT_ALG_HANDLE sha{};
    if (BCryptOpenAlgorithmProvider(&sha, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) return false;
    DECOMPRESSOR_HANDLE unpack{};
    bool success = CreateDecompressor(COMPRESS_ALGORITHM_LZMS, nullptr, &unpack) != FALSE;
    if (success) {
        for (const auto& file : kFiles) {
            if (file.campaign && (!wad.empty() || file.campaign != campaign)) continue;
            if (Extract(sha, unpack, file, directory)) continue;
            Wh_Log(L"Could not prepare bundled Doom file: %ls", file.name);
            success = false;
            break;
        }
        CloseDecompressor(unpack);
    }
    BCryptCloseAlgorithmProvider(sha, 0);
    if (!success) return false;
    engine = directory + L"\\crispy-doom.exe";
    if (wad.empty()) wad = directory + (campaign == 2 ? L"\\freedoom2.wad" : L"\\freedoom1.wad");
    return true;
}

}  // namespace DoomBundle
