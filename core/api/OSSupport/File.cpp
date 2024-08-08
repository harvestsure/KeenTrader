#include "Globals.h"

#include "File.h"
#include <sys/stat.h>
#ifdef _WIN32
	#include <share.h> 
#else
	#include <dirent.h>
#endif  // _WIN32

cFile::cFile(void) :
	m_File(nullptr)
{
	// Nothing needed yet
}

cFile::cFile(const AString & iFileName, eMode iMode) :
	m_File(nullptr)
{
	Open(iFileName, iMode);
}

cFile::~cFile()
{
	if (IsOpen())
	{
		Close();
	}
}

bool cFile::Open(const AString & iFileName, eMode iMode)
{
	ASSERT(!IsOpen());

	if (IsOpen())
	{
		Close();
	}

	const char * Mode = nullptr;
	switch (iMode)
	{
		case fmRead:      Mode = "rb";  break;
		case fmWrite:     Mode = "wb";  break;
		case fmReadWrite: Mode = "rb+"; break;
		case fmAppend:    Mode = "a+";  break;
	}
	if (Mode == nullptr)
	{
		ASSERT(!"Unhandled file mode");
		return false;
	}

	#ifdef _WIN32
		m_File = _fsopen((iFileName).c_str(), Mode, _SH_DENYWR);
	#else
		m_File = fopen((iFileName).c_str(), Mode);
	#endif  // _WIN32

	if ((m_File == nullptr) && (iMode == fmReadWrite))
	{
		#ifdef _WIN32
			m_File = _fsopen((iFileName).c_str(), "wb+", _SH_DENYWR);
		#else
			m_File = fopen((iFileName).c_str(), "wb+");
		#endif  // _WIN32

	}
	return (m_File != nullptr);
}

void cFile::Close(void)
{
	if (!IsOpen())
	{
		return;
	}

	fclose(m_File);
	m_File = nullptr;
}

bool cFile::IsOpen(void) const
{
	return (m_File != nullptr);
}

bool cFile::IsEOF(void) const
{
	ASSERT(IsOpen());

	if (!IsOpen())
	{
		// Unopened files behave as at EOF
		return true;
	}

	return (feof(m_File) != 0);
}

int cFile::Read (void * a_Buffer, size_t a_NumBytes)
{
	ASSERT(IsOpen());

	if (!IsOpen())
	{
		return -1;
	}

	return static_cast<int>(fread(a_Buffer, 1, a_NumBytes, m_File));
}

ContiguousByteBuffer cFile::Read(size_t a_NumBytes)
{
	ASSERT(IsOpen());

	if (!IsOpen())
	{
		return {};
	}

	ContiguousByteBuffer res;
	res.resize(a_NumBytes);
	auto newSize = fread(res.data(), sizeof(std::byte), a_NumBytes, m_File);
	res.resize(newSize);
	return res;
}

int cFile::Write(const void * a_Buffer, size_t a_NumBytes)
{
	ASSERT(IsOpen());

	if (!IsOpen())
	{
		return -1;
	}

	int res = static_cast<int>(fwrite(a_Buffer, 1, a_NumBytes, m_File));
	return res;
}

long cFile::Seek (int iPosition)
{
	ASSERT(IsOpen());

	if (!IsOpen())
	{
		return -1;
	}

	if (fseek(m_File, iPosition, SEEK_SET) != 0)
	{
		return -1;
	}
	return ftell(m_File);
}

long cFile::Tell (void) const
{
	ASSERT(IsOpen());

	if (!IsOpen())
	{
		return -1;
	}

	return ftell(m_File);
}

long cFile::GetSize(void) const
{
	ASSERT(IsOpen());

	if (!IsOpen())
	{
		return -1;
	}

	long CurPos = Tell();
	if (CurPos < 0)
	{
		return -1;
	}
	if (fseek(m_File, 0, SEEK_END) != 0)
	{
		return -1;
	}
	long res = Tell();
	if (fseek(m_File, static_cast<long>(CurPos), SEEK_SET) != 0)
	{
		return -1;
	}
	return res;
}

int cFile::ReadRestOfFile(AString & a_Contents)
{
	ASSERT(IsOpen());

	if (!IsOpen())
	{
		return -1;
	}

	long TotalSize = GetSize();
	if (TotalSize < 0)
	{
		return -1;
	}

	long Position = Tell();
	if (Position < 0)
	{
		return -1;
	}

	auto DataSize = static_cast<size_t>(TotalSize - Position);

	a_Contents.resize(DataSize);
	return Read(a_Contents.data(), DataSize);
}

bool cFile::Exists(const AString & a_FileName)
{
	cFile test(a_FileName, fmRead);
	return test.IsOpen();
}

bool cFile::Delete(const AString & a_Path)
{
	if (IsFolder(a_Path))
	{
		return DeleteFolder(a_Path);
	}
	else
	{
		return DeleteFile(a_Path);
	}
}

bool cFile::DeleteFolder(const AString & a_FolderName)
{
	#ifdef _WIN32
		return (RemoveDirectoryA(a_FolderName.c_str()) != 0);
	#else  // _WIN32
		return (rmdir(a_FolderName.c_str()) == 0);
	#endif  // else _WIN32
}

bool cFile::DeleteFolderContents(const AString & a_FolderName)
{
	auto Contents = cFile::GetFolderContents(a_FolderName);
	for (const auto & item: Contents)
	{
		// Remove the item:
		auto WholePath = a_FolderName + GetPathSeparator() + item;
		if (IsFolder(WholePath))
		{
			if (!DeleteFolderContents(WholePath))
			{
				return false;
			}
			if (!DeleteFolder(WholePath))
			{
				return false;
			}
		}
		else
		{
			if (!DeleteFile(WholePath))
			{
				return false;
			}
		}
	}  // for item - Contents[]

	return true;
}

bool cFile::DeleteFile(const AString & a_FileName)
{
	return (remove(a_FileName.c_str()) == 0);
}

bool cFile::Rename(const AString & a_OrigFileName, const AString & a_NewFileName)
{
	return (rename(a_OrigFileName.c_str(), a_NewFileName.c_str()) == 0);
}

bool cFile::Copy(const AString & a_SrcFileName, const AString & a_DstFileName)
{
	#ifdef _WIN32
		return (CopyFileA(a_SrcFileName.c_str(), a_DstFileName.c_str(), FALSE) != 0);
	#else
		// Other OSs don't have a direct CopyFile equivalent, do it the harder way:
		std::ifstream src(a_SrcFileName.c_str(), std::ios::binary);
		std::ofstream dst(a_DstFileName.c_str(), std::ios::binary);
		if (dst.good())
		{
			dst << src.rdbuf();
			return true;
		}
		else
		{
			return false;
		}
	#endif
}

bool cFile::IsFolder(const AString & a_Path)
{
	#ifdef _WIN32
		DWORD FileAttrib = GetFileAttributesA(a_Path.c_str());
		return ((FileAttrib != INVALID_FILE_ATTRIBUTES) && ((FileAttrib & FILE_ATTRIBUTE_DIRECTORY) != 0));
	#else
		struct stat st;
		return ((stat(a_Path.c_str(), &st) == 0) && S_ISDIR(st.st_mode));
	#endif
}

bool cFile::IsFile(const AString & a_Path)
{
	#ifdef _WIN32
		DWORD FileAttrib = GetFileAttributesA(a_Path.c_str());
		return ((FileAttrib != INVALID_FILE_ATTRIBUTES) && ((FileAttrib & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_DEVICE)) == 0));
	#else
		struct stat st;
		return ((stat(a_Path.c_str(), &st) == 0) && S_ISREG(st.st_mode));
	#endif
}

long cFile::GetSize(const AString & a_FileName)
{
	struct stat st;
	if (stat(a_FileName.c_str(), &st) == 0)
	{
		return static_cast<int>(st.st_size);
	}
	return -1;
}

bool cFile::CreateFolder(const AString & a_FolderPath)
{
	#ifdef _WIN32
		return (CreateDirectoryA(a_FolderPath.c_str(), nullptr) != 0);
	#else
		return (mkdir(a_FolderPath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == 0);
	#endif
}

bool cFile::CreateFolderRecursive(const AString & a_FolderPath)
{
	if (a_FolderPath.empty())
	{
		return false;
	}

	auto len = a_FolderPath.length();
	for (decltype(len) i = 0; i < len; i++)
	{
	#ifdef _WIN32
		if ((a_FolderPath[i] == '\\') || (a_FolderPath[i] == '/'))
	#else
		if (a_FolderPath[i] == '/')
	#endif
		{
			CreateFolder(a_FolderPath.substr(0, i));
		}
	}
	CreateFolder(a_FolderPath);

	return IsFolder(a_FolderPath);
}

AStringVector cFile::GetFolderContents(const AString & a_Folder)
{
	AStringVector AllFiles;

	#ifdef _WIN32
		AString FileFilter = a_Folder;
		if (
			!FileFilter.empty() &&
			(FileFilter[FileFilter.length() - 1] != '\\') &&
			(FileFilter[FileFilter.length() - 1] != '/')
		)
		{
			FileFilter.push_back('\\');
		}

		FileFilter.append("*.*");
		HANDLE hFind;
		WIN32_FIND_DATAA FindFileData;
		if ((hFind = FindFirstFileA(FileFilter.c_str(), &FindFileData)) != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (
					(strcmp(FindFileData.cFileName, ".") == 0) ||
					(strcmp(FindFileData.cFileName, "..") == 0)
				)
				{
					continue;
				}
				AllFiles.push_back(FindFileData.cFileName);
			} while (FindNextFileA(hFind, &FindFileData));
			FindClose(hFind);
		}

	#else  // _WIN32

		DIR * dp;
		AString Folder = a_Folder;
		if (Folder.empty())
		{
			Folder = ".";
		}
		if ((dp = opendir(Folder.c_str())) == nullptr)
		{
			LOGERROR("Error (%i) opening directory \"%s\"\n", errno, Folder.c_str());
		}
		else
		{
			struct dirent *dirp;
			while ((dirp = readdir(dp)) != nullptr)
			{
				if (
					(strcmp(dirp->d_name, ".") == 0) ||
					(strcmp(dirp->d_name, "..") == 0)
				)
				{
					continue;
				}
				AllFiles.push_back(dirp->d_name);
			}
			closedir(dp);
		}

	#endif  // else _WIN32

	return AllFiles;
}

AString cFile::ReadWholeFile(const AString & a_FileName)
{
	cFile f;
	if (!f.Open(a_FileName, fmRead))
	{
		return "";
	}
	AString Contents;
	f.ReadRestOfFile(Contents);
	return Contents;
}

AString cFile::ChangeFileExt(const AString & a_FileName, const AString & a_NewExt)
{
	auto res = a_FileName;

	#if defined(_MSC_VER)
		auto LastPathSep = res.find_last_of("/\\");
	#elif defined(_WIN32)
		auto LastPathSep = res.rfind('\\');
	#else
		auto LastPathSep = res.rfind('/');
	#endif
	if ((LastPathSep != AString::npos) && (LastPathSep + 1 == res.size()))
	{
		return res;
	}

	auto DotPos = res.rfind('.');
	if (
		(DotPos == AString::npos) ||
		((LastPathSep != AString::npos) && (LastPathSep > DotPos))
	)
	{
		if (!a_NewExt.empty() && (a_NewExt[0] != '.'))
		{
			res.push_back('.');
		}
		res.append(a_NewExt);
	}
	else
	{
		if (!a_NewExt.empty() && (a_NewExt[0] != '.'))
		{
			res.erase(DotPos + 1, AString::npos);
		}
		else
		{
			res.erase(DotPos, AString::npos);
		}
		res.append(a_NewExt);
	}
	return res;
}

unsigned cFile::GetLastModificationTime(const AString & a_FileName)
{
	struct stat st;
	if (stat(a_FileName.c_str(), &st) < 0)
	{
		return 0;
	}
	#if defined(_WIN32)
		return static_cast<unsigned>(st.st_mtime);
	#else
		return static_cast<unsigned>(mktime(localtime(&st.st_mtime)));
	#endif
}

AString cFile::GetPathSeparator()
{
	#ifdef _WIN32
		return "\\";
	#else
		return "/";
	#endif
}

AString cFile::GetExecutableExt()
{
	#ifdef _WIN32
		return ".exe";
	#else
		return "";
	#endif
}

void cFile::Flush()
{
	fflush(m_File);
}


template <class StreamType>
FileStream<StreamType>::FileStream(const std::string & Path)
{
	FileStream::exceptions(FileStream::failbit | FileStream::badbit);

	FileStream::open(Path);

	FileStream::exceptions(FileStream::badbit);
}


template <class StreamType>
FileStream<StreamType>::FileStream(const std::string & Path, const typename FileStream::openmode Mode)
{
	FileStream::exceptions(FileStream::failbit | FileStream::badbit);

	FileStream::open(Path, Mode);

	FileStream::exceptions(FileStream::badbit);
}





#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-template-vtables"  // http://bugs.llvm.org/show_bug.cgi?id=18733
#endif

// Instantiate the templated wrapper for input and output:
template class FileStream<std::ifstream>;
template class FileStream<std::ofstream>;

#ifdef __clang__
#pragma clang diagnostic pop
#endif
