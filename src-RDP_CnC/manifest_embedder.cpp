#include <windows.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

int wmain(int argc, wchar_t** argv) {
  if (argc != 3) return 1;
  std::ifstream input(std::filesystem::path(argv[2]), std::ios::binary);
  std::vector<char> manifest((std::istreambuf_iterator<char>(input)), {});
  if (manifest.empty()) return 2;
  HANDLE update = BeginUpdateResourceW(argv[1], FALSE);
  if (!update) return 3;
  if (!UpdateResourceW(update, MAKEINTRESOURCEW(24), MAKEINTRESOURCEW(1),
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                       manifest.data(), static_cast<DWORD>(manifest.size()))) {
    EndUpdateResourceA(update, TRUE);
    return 4;
  }
  return EndUpdateResourceA(update, FALSE) ? 0 : 5;
}
