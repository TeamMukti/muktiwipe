/*
 *	libp - a small library of Win32/C++ helpers
 *
 *	Extended-length path support.
 */
#include "_elpify.h"

//
static
bool is_elp_prefixed(const wstring & path)
{
	// already \\?\-prefixed ?
	if (path.size() >= 4 &&
		path[0] == L'\\' && path[1] == L'\\' &&
		path[2] == L'?'  && path[3] == L'\\')
	{
		return true;
	}

	return false;
}

wstring elpify(const wstring & path)
{
	if (is_elp_prefixed(path))
		return path;

	// UNC path - \\server\share\... -> \\?\UNC\server\share\...
	if (path.size() >= 2 && path[0] == L'\\' && path[1] == L'\\')
		return L"\\\\?\\UNC\\" + path.substr(2);

	return L"\\\\?\\" + path;
}

//
dword elp_api::GetFileAttributesW(const wchar_t * path)
{
	return ::GetFileAttributesW(elpify(path).c_str());
}

bool elp_api::DeleteFileW(const wchar_t * path)
{
	return ::DeleteFileW(elpify(path).c_str());
}

bool elp_api::SetFileAttributesW(const wchar_t * path, dword attrs)
{
	return ::SetFileAttributesW(elpify(path).c_str(), attrs);
}

bool elp_api::RemoveDirectoryW(const wchar_t * path)
{
	return ::RemoveDirectoryW(elpify(path).c_str());
}

//
static elp_api  elp_api_instance;
elp_api       * elp = &elp_api_instance;
