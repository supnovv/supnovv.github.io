#include "osi/winpref.h"
#define L_WINDOWS_IMPL
#include "lucycore.h"
static void printlasterror() {
  #define ERRSTRSZ 1024
  TCHAR bf[ERRSTRSZ+1] = {0};
  if (0 == FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, 0, GetLastError(), 0, bf, ERRSTRSZ, 0)) {
    core_loge("formatmessage fail");
    return;
  }
  core_loge("last error: %s", bf);
  #undef ERRSTRSZ
}
void windows_c_test() {
  core_assert(sizeof(core_general_t) >= sizeof(WIN32_FILE_ATTRIBUTE_DATA));
}
/* BOOL WINAPI GetFileAttributesEx(lpFileName, fInfoLevelId, _Out_ lpFileInformation)
   @para[LPCTSTR lpFileName]: name of the file or directory
   @para[GET_FILEEX_INFO_LEVELS fInfoLevelId]: attribute info type to retrieve (can be GetFileExInfoStandard)
   @para[LPVOID lpFileInformation]: out buffer stored the attribute info if the function called success
   @return[BOOL]: zero indicates fail, can call GetLastError to get error code
   @support: windows xp / windows server 2003
   @library: kernel32.lib/dll
   FILETIME contains a 64-bit value representing the number of 100-nanosecond intervals since January 1, 1601 (UTC) */
static coredate_t ToSystemTime(const FILETIME* ft) {
  SYSTEMTIME systime = {0};
  coredate_t date = {0};
  if (0 == FileTimeToSystemTime(ft, &systime)) { LogLastError(); return date; }
  date.year = (thalf)systime.wYear;
  date.month = (byte)systime.wMonth;
  date.dayofweek = (byte)systime.wDayOfWeek;
  date.day = (byte)systime.wDay;
  date.hour = (byte)systime.wHour;
  date.min = (byte)systime.wMinute;
  date.sec = (byte)systime.wSecond;
  return date;
}
bool core_getfileattr(const corestring_t* name, corefileattr_t* fa) {
  if (!name || !fa) return false;
  core_zero(fa, sizeof(corefileattr_t));
  #define MAX_NAME_LEN 1024
  uhalf filename[MAX_NAME_LEN] = {0};
  if (!core_toutf16z(name, filename, MAX_NAME_LEN)) return false;
  #undef MAX_NAME_LEN
  WIN32_FILE_ATTRIBUTE_DATA data = {0};
  if (0 == GetFileAttributesEx((LPCTSTR)filename, GetFileExInfoStandard, &data)) {
    LogLastError();
    return false;
  }
  fa.fsize = (ulong)data.nFileSizeHigh;
  fa.fsize = (fa.fsize << 32) | data.nFileSizeLow;
  fa.ctime = (ulong)data.ftCreationTime.dwHighDateTime;
  fa.ctime = (fa.ctime << 32) | data.ftCreationTime.dwLowDateTime;
  fa.atime = (ulong)data.ftLastAccessTime.dwHighDateTime;
  fa.atime = (fa.atime << 32) | data.ftLastAccessTime.dwLowDateTime;
  fa.mtime = (ulong)data.ftLastWriteTime.dwHighDateTime;
  fa.mtime = (fa.mtime << 32) | data.ftLastWriteTime.dwLowDateTime;
  fa.cdate = ToSystemTime(&data.ftCreationTime);
  fa.adate = ToSystemTime(&data.ftLastAccessTime);
  fa.mdate = ToSystemTime(&data.ftLastWriteTime);
  TIME_ZONE_INFORMATION tz = {0};
  GetTimeZoneInformation(&tz);
  core_logd("windows timezone: %d min %d hour", tz.Bias, tz.Bias / 60);
  return true;
}
void core_plattest() {

}
