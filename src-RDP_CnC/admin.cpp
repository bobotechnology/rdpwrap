#include <windows.h>
#include <shellapi.h>

int WINAPI rdpMain(HINSTANCE, HINSTANCE, LPWSTR, int);

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous,
                    LPWSTR commandLine, int show) {
  BOOL elevated = FALSE;
  SID_IDENTIFIER_AUTHORITY authority = SECURITY_NT_AUTHORITY;
  PSID administrators = nullptr;
  if (AllocateAndInitializeSid(&authority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                               DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                               &administrators)) {
    CheckTokenMembership(nullptr, administrators, &elevated);
    FreeSid(administrators);
  }

  if (!elevated) {
    wchar_t executable[MAX_PATH]{};
    GetModuleFileNameW(nullptr, executable, MAX_PATH);
    const HINSTANCE result = ShellExecuteW(nullptr, L"runas", executable,
                                           commandLine, nullptr, show);
    return reinterpret_cast<INT_PTR>(result) > 32 ? 0 : 1;
  }
  return rdpMain(instance, previous, commandLine, show);
}
