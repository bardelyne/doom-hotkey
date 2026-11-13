#include <bcrypt.h>
#include <compressapi.h>
#include <shlobj.h>
#include <wincrypt.h>

#include <array>
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

constexpr wchar_t kLegacyBundleDirectory[] = L"crispy-7.2-freedoom-0.13-2b475f3ecf39";

bool CanRunEngine() {
    SYSTEM_INFO system{};
    GetNativeSystemInfo(&system);
    if (system.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
        return true;
    }

    auto getMachineTypeAttributes = reinterpret_cast<decltype(&GetMachineTypeAttributes)>(
        GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetMachineTypeAttributes"));
    MACHINE_ATTRIBUTES attributes{};
    return getMachineTypeAttributes &&
           SUCCEEDED(getMachineTypeAttributes(IMAGE_FILE_MACHINE_AMD64, &attributes)) &&
           (attributes & UserEnabled);
}

bool IsRegularDirectory(const std::wstring& path) {
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) &&
           !(attributes & FILE_ATTRIBUTE_REPARSE_POINT);
}

bool RegularDirectory(const std::wstring& path) {
    if (!CreateDirectoryW(path.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) {
        return false;
    }
    return IsRegularDirectory(path);
}

bool Matches(BCRYPT_ALG_HANDLE sha, const File& file, const std::vector<UCHAR>& data) {
    std::array<UCHAR, 32> digest{};
    return data.size() == file.size &&
           BCryptHash(sha, nullptr, 0, const_cast<PUCHAR>(data.data()), file.size, digest.data(),
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

bool ImportLegacyFile(const std::wstring& from, const std::wstring& to) {
    const DWORD attributes = GetFileAttributesW(from.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        const DWORD error = GetLastError();
        return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND;
    }
    if (attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) return true;
    if (CopyFileW(from.c_str(), to.c_str(), TRUE)) return true;
    const DWORD error = GetLastError();
    return error == ERROR_FILE_EXISTS || error == ERROR_ALREADY_EXISTS;
}

bool ImportLegacySaves(const std::wstring& legacy, const std::wstring& saveDirectory) {
    WIN32_FIND_DATAW entry{};
    HANDLE search = FindFirstFileW((legacy + L"\\doomsav*.dsg").c_str(), &entry);
    if (search == INVALID_HANDLE_VALUE) return GetLastError() == ERROR_FILE_NOT_FOUND;

    bool imported = true;
    do {
        if (!ImportLegacyFile(legacy + L"\\" + entry.cFileName,
                              saveDirectory + L"\\" + entry.cFileName)) {
            imported = false;
            break;
        }
    } while (FindNextFileW(search, &entry));
    imported = imported && GetLastError() == ERROR_NO_MORE_FILES;
    FindClose(search);
    return imported;
}

bool Prepare(std::wstring& engine, std::wstring& wad, int campaign, std::wstring& profileArguments) {
    if (!CanRunEngine()) {
        Wh_Log(L"The bundled Doom engine requires x64 Windows or Windows 11 with x64 emulation. Set Doom executable to use a compatible engine.");
        return false;
    }

    PWSTR local{};
    if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, &local))) return false;
    const std::wstring root = std::wstring(local) + L"\\Windhawk Doom";
    CoTaskMemFree(local);
    const std::wstring directory = root + L"\\Runtime";
    const std::wstring profile = root + L"\\Profile";
    const std::wstring saves = profile + L"\\Saves";
    if (!RegularDirectory(root) || !RegularDirectory(directory) || !RegularDirectory(profile) ||
        !RegularDirectory(saves)) return false;

    const bool bundledWad = wad.empty();
    const auto wadName = campaign == 2 ? L"freedoom2.wad" : L"freedoom1.wad";
    std::wstring saveDirectory = saves + L"\\" + wadName;
    const std::wstring legacy = root + L"\\" + kLegacyBundleDirectory;

    BCRYPT_ALG_HANDLE sha{};
    if (BCryptOpenAlgorithmProvider(&sha, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) return false;
    if (!bundledWad) {
        std::array<UCHAR, 32> hash{};
        if (BCryptHash(sha, nullptr, 0, reinterpret_cast<PUCHAR>(wad.data()),
                       static_cast<ULONG>(wad.size() * sizeof(wchar_t)), hash.data(),
                       static_cast<ULONG>(hash.size())) != 0) {
            BCryptCloseAlgorithmProvider(sha, 0);
            return false;
        }
        const auto filename = wad.substr(wad.find_last_of(L"\\/") + 1, 40);
        saveDirectory = saves + L"\\custom-" + filename + L"-";
        constexpr wchar_t kHex[] = L"0123456789abcdef";
        for (const UCHAR byte : hash) {
            saveDirectory += kHex[byte >> 4];
            saveDirectory += kHex[byte & 15];
        }
    }

    const auto migrationMarker = profile + L"\\legacy-profile-imported";
    if (IsRegularDirectory(legacy) && GetFileAttributesW(migrationMarker.c_str()) == INVALID_FILE_ATTRIBUTES) {
        bool imported = true;
        for (PCWSTR name : {L"default.cfg", L"crispy-doom.cfg"}) {
            imported = imported && ImportLegacyFile(legacy + L"\\" + name, profile + L"\\" + name);
        }
        const auto legacySaves = profile + L"\\Legacy Saves";
        // Older builds shared save slots between all IWADs. Preserve the files
        // separately because the game each slot belongs to can be ambiguous.
        if (!imported || !RegularDirectory(legacySaves) || !ImportLegacySaves(legacy, legacySaves) ||
            !RegularDirectory(migrationMarker)) {
            BCryptCloseAlgorithmProvider(sha, 0);
            return false;
        }
    }

    if (!RegularDirectory(saveDirectory)) {
        BCryptCloseAlgorithmProvider(sha, 0);
        return false;
    }
    DECOMPRESSOR_HANDLE unpack{};
    bool success = CreateDecompressor(COMPRESS_ALGORITHM_LZMS, nullptr, &unpack) != FALSE;
    if (success) {
        for (const auto& file : kFiles) {
            if (file.campaign && (!bundledWad || file.campaign != campaign)) continue;
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
    if (bundledWad) wad = directory + L"\\" + wadName;
    profileArguments = L" -config \"" + profile + L"\\default.cfg\" -extraconfig \"" + profile +
                       L"\\crispy-doom.cfg\" -savedir \"" + saveDirectory + L"\"";
    return true;
}

}  // namespace DoomBundle
