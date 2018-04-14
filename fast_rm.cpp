static char *fast_rm_id = 
	"@(#)Copyright (C) H.Shirouzu 2004   fast_rm.cpp	Ver0.71";
/* ========================================================================
	Project  Name			: Fast/Force remove files and directories
	Create					: 2004-01-20(Tue)
	Update					: 2004-09-26(Sun)
	Copyright				: H.Shirouzu
	Reference				: 
	======================================================================== */

#ifdef UNICODE
#define main wmain
#endif

#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#define PATH_LOCAL_PREFIX	_T("\\\\?\\")
#define PATH_UNC_PREFIX		_T("\\\\?\\UNC")
#ifdef UNICODE
#define MAX_PATH_EX			32768
#define MakePath			MakePathW
#else
#define MAX_PATH_EX			(MAX_PATH * 2)
#define MakePath			MakePathA
#endif
#define PATH_MAX_PREFIXLEN	7
int		prefix_len			= 0;

int		total_files		= 0;
int		total_dirs		= 0;
_int64	total_sizes		= 0;
int		silent_flg		= 0;
int		truncate_flg	= 0;

int MakePathA(char *dir, int dir_len, const char *file)
{
	BOOL	separetor = TRUE;

	if (dir[dir_len -1] == '\\')	// 表など、2byte目が'\\'で終る文字列対策
	{
		if (dir_len >= 2 && IsDBCSLeadByte(dir[dir_len -2]) == FALSE)
			separetor = FALSE;
		else {
			u_char *p = NULL;
			for (p=(u_char *)dir; *p && p[1]; IsDBCSLeadByte(*p) ? p+=2 : p++)
				;
			if (*p == '\\')
				separetor = FALSE;
		}
	}
	return	dir_len + sprintf(dir + dir_len, "%s%s", separetor ? "\\" : "", file);
}

int MakePathW(WCHAR *dir, int dir_len, const WCHAR *file)
{
	return	dir_len + swprintf(dir + dir_len, L"%s%s", dir[dir_len -1] == L'\\' ? L"" : L"\\" , file);
}

int fast_rm_core(TCHAR *path, int dir_len)
{
	HANDLE	hDir, fh;
	int		ret = 0, new_dir_len;
	int		files = 0;
	_int64	sizes = 0;
	WIN32_FIND_DATA	fdat;
	BOOL	dir_mode = FALSE;

	if ((hDir = ::FindFirstFile(path, &fdat)) == INVALID_HANDLE_VALUE) {
		if (silent_flg == 0)
			_tprintf(_T("Can't access %s\r\n"), path + prefix_len);
		ret = -1;
		goto END;
	}

	do {
		if (_tcscmp(fdat.cFileName, _T(".")) == 0 || _tcscmp(fdat.cFileName, _T("..")) == 0) {
			dir_mode = TRUE;
			continue;
		}
		_tcscpy(path + dir_len, fdat.cFileName);

		// read only の場合、read only flg を落としておく
		if (fdat.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
			::SetFileAttributes(path, fdat.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);

		if (fdat.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			new_dir_len = dir_len + _tcslen(fdat.cFileName);
			_tcscpy(path + new_dir_len++, _T("\\*"));
			if (fast_rm_core(path, new_dir_len) == -1)
				ret = -1;
			path[--new_dir_len] = _T('\0');
			if (::RemoveDirectory(path))
				total_dirs++;
			else if (silent_flg == 0)
				_tprintf(_T("Can't delete %s\r\n"), path + prefix_len);
		}
		else {
			if (truncate_flg) {	// 削除前に truncate する
				if ((fh = ::CreateFile(path, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0)) != INVALID_HANDLE_VALUE) {
					::SetEndOfFile(fh);
					::CloseHandle(fh);
				}
			}
			if (::DeleteFile(path)) {
				files++;
				sizes += (((_int64)fdat.nFileSizeHigh) << 32) + fdat.nFileSizeLow;
			}
			else if (silent_flg == 0) {
				_tprintf(_T("Can't delete %s\r\n"), path + prefix_len);
			}
		}
	} while (::FindNextFile(hDir, &fdat));

	::FindClose(hDir);

	total_files	+= files;
	total_sizes	+= sizes;

	path[dir_len] = _T('\0');
	if (silent_flg == 0 && dir_mode)
		_tprintf(_T("%s (%.1f MB / %d files) removed\r\n"), path + prefix_len, (double)sizes / 1024 / 1024, files);

END:
	return	ret;
}

int fast_rm(TCHAR *dir)
{
	TCHAR	*path = new TCHAR [MAX_PATH_EX], *fname, *prefix=NULL;
	int		ret = -1, len;
	DWORD	attr;

	if (::GetFullPathName(dir, MAX_PATH_EX, path, &fname) > 0) {
#ifdef UNICODE
		prefix = (path[0] == _T('\\')) ? PATH_UNC_PREFIX : PATH_LOCAL_PREFIX;
		prefix_len = _tcslen(prefix);
		// MAX_PATH 越えを許容するパス形式に変換
		// (PATH_UNC_PREFIX の場合、\\server -> \\?\UNC\server にするため、
		//  \\server の頭の \ を一つ潰す)
		memmove(path + prefix_len - (prefix == PATH_UNC_PREFIX ? 1 : 0), path, _tcslen(path) * sizeof(TCHAR));
		memmove(path, prefix, prefix_len * sizeof(TCHAR));
#else
		prefix_len = 0;
#endif
		len = _tcslen(path);
		if ((attr = ::GetFileAttributes(path)) != 0xffffffff && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
			len = MakePath(path, len, _T("*")) -1;
			ret = fast_rm_core(path, len);
			path[len - 1] = 0;
			if (::RemoveDirectory(path))
				total_dirs++;
			else if (silent_flg == 0)
				_tprintf(_T("Can't delete %s\r\n"), path + prefix_len);
		}
		else if (fname) {	// 通常ファイルや */? 等のメタ指定
			len = fname - path + prefix_len - (prefix == PATH_UNC_PREFIX ? 1 : 0);
			ret = fast_rm_core(path, len);
		}

	}
	else
		_tprintf(_T("Invalid dirname %s"), dir);
	delete [] path;
	return	ret;
}

int main(int argc, TCHAR **argv)
{
	int		ret = 0;

	for ( ; argc > 1 && argv[1][0] == _T('-'); argc--, argv++) {
		for (TCHAR *p=argv[1]; *p; p++) {
			switch (*p) {
			case _T('s'): silent_flg = 1; break;
			case _T('t'): truncate_flg = 1; break;
			}
		}
	}

	if (argc < 2)
		return	_tprintf(_T("fast_rm [-s(ilent)][-t(runcate)] target_dirs/files...\r\n")), 0;

	DWORD	startTick = ::GetTickCount();

	while (*++argv) {
//		_tprintf(_T("%s\r\n"), *argv);
		if (fast_rm(*argv) == -1)
			ret = -1;
	}

	_tprintf(_T("total %.1f MB / %d dirs / %d files removed (%.1f sec)\r\n"), (double)total_sizes / 1024 / 1024, total_dirs, total_files, ((double)::GetTickCount() - startTick) / 1000);

	return	ret;
}

