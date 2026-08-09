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
  const DWORD size = static_cast<DWORD>(manifest.size());
  const bool neutral = UpdateResourceW(update, MAKEINTRESOURCEW(24), MAKEINTRESOURCEW(1),
      MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), manifest.data(), size) != FALSE;
  const bool enUs = UpdateResourceW(update, MAKEINTRESOURCEW(24), MAKEINTRESOURCEW(1),
      MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), manifest.data(), size) != FALSE;
  if (!neutral || !enUs) {
    EndUpdateResourceW(update, TRUE);
    return 4;
  }
  return EndUpdateResourceW(update, FALSE) ? 0 : 5;
}
