#pragma once
class KEEN_API_EXPORT cFile
{
public:
	inline static char PathSeparator()
	{
		#ifdef _WIN32
			return '\\';
		#else
			return '/';
		#endif
	}

	enum eMode
	{
		fmRead,
		fmWrite, 
		fmReadWrite,
		fmAppend
	} ;


	cFile(void);

	cFile(const AString & iFileName, eMode iMode);

	~cFile();

	bool Open(const AString & iFileName, eMode iMode);
	void Close(void);
	bool IsOpen(void) const;
	bool IsEOF(void) const;

	int Read(void * a_Buffer, size_t a_NumBytes);

	std::basic_string<std::byte> Read(size_t a_NumBytes);

	int Write(const void * a_Buffer, size_t a_NumBytes);

	int Write(std::string_view a_String)
	{
		return Write(a_String.data(), a_String.size());
	}

	long Seek (int iPosition);

	long Tell (void) const;

	long GetSize(void) const;

	int ReadRestOfFile(AString & a_Contents);

	static bool Exists(const AString & a_FileName);

	static bool Delete(const AString & a_Path);

	static bool DeleteFile(const AString & a_FileName); 

	static bool DeleteFolder(const AString & a_FolderName); 

	static bool DeleteFolderContents(const AString & a_FolderName);

	static bool Rename(const AString & a_OrigPath, const AString & a_NewPath);

	static bool Copy(const AString & a_SrcFileName, const AString & a_DstFileName);

	static bool IsFolder(const AString & a_Path);

	static bool IsFile(const AString & a_Path);

	static long GetSize(const AString & a_FileName);

	static bool CreateFolder(const AString & a_FolderPath);

	static bool CreateFolderRecursive(const AString & a_FolderPath); 

	static AString ReadWholeFile(const AString & a_FileName);

	static AString ChangeFileExt(const AString & a_FileName, const AString & a_NewExt);

	static unsigned GetLastModificationTime(const AString & a_FileName);

	static AString GetPathSeparator();

	static AString GetExecutableExt();

	static AStringVector GetFolderContents(const AString & a_Folder);

	void Flush();

private:
	FILE * m_File;
};





/** A wrapper for file streams that enables exceptions. */
template <class StreamType>
class FileStream final : public StreamType
{
public:

	FileStream(const std::string & Path);
	FileStream(const std::string & Path, const typename FileStream::openmode Mode);
};





using InputFileStream = FileStream<std::ifstream>;
using OutputFileStream = FileStream<std::ofstream>;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-template-vtables"  // http://bugs.llvm.org/show_bug.cgi?id=18733
#endif

extern template class FileStream<std::ifstream>;
extern template class FileStream<std::ofstream>;

#ifdef __clang__
#pragma clang diagnostic pop
#endif
