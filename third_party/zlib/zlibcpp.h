///////////////////////////////////////////////////////////////////////////////
//
//  Module: zlibcpp.h
//
//    Desc: Basic class wrapper for the zlib dll
//
// Copyright (c) 2003 Automatic Data Processing, Inc. All Rights Reserved.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _ZLIBCPP_H_
#define _ZLIBCPP_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#ifndef _WINDOWS
#define _WINDOWS
#endif // !_WINDOWS

#ifndef _zip_H
#include "zip.h"
#include "unzip.h"
#endif // _zip_H

const UINT BUFFERSIZE = 2048;
const UINT MAX_COMMENT = 255;

struct UZ_FileInfo
{
	TCHAR szFileName[MAX_PATH + 1];
	TCHAR szComment[MAX_COMMENT + 1];
	
	DWORD dwVersion;  
	DWORD dwVersionNeeded;
	DWORD dwFlags;	 
	DWORD dwCompressionMethod; 
	DWORD dwDosDate;	
	DWORD dwCRC;   
	DWORD dwCompressedSize; 
	DWORD dwUncompressedSize;
	DWORD dwInternalAttrib; 
	DWORD dwExternalAttrib; 
	bool bFolder;
};

static FILETIME getLastWriteFileTime(CString sFile)
{
   FILETIME          ftLocal = {0};
   HANDLE            hFind;
   WIN32_FIND_DATA   ff32;
   hFind = FindFirstFile(sFile, &ff32);
   if (INVALID_HANDLE_VALUE != hFind)
   {
      FileTimeToLocalFileTime(&(ff32.ftLastWriteTime), &ftLocal);
      FindClose(hFind);        
   }
   return ftLocal;
}

class CZLib  
{
public:
	CZLib()
	{
	   m_zf = 0;
	   m_uzFile = NULL;	//shenrui 08-8-21 没赋值，析购时崩溃
	}
	virtual ~CZLib()
	{
	   if (m_zf)
		  Close();
		if (m_uzFile)
		{
			unzCloseCurrentFile(m_uzFile);
			int nRet = unzClose(m_uzFile);
			m_uzFile = NULL;
		}
	}

	BOOL Open(CString f_file, int f_nAppend = 0)
	{
		USES_CONVERSION;
		char* pFile = T2A(f_file.GetBuffer(0));
	   m_zf = zipOpen(pFile, f_nAppend);
	   return (m_zf != NULL);
	}

	void Close()
	{
	   if (m_zf)
		  zipClose(m_zf, NULL);

	   m_zf = 0;
	}
	
	//
	// !解压缩直接调用此函数，不用Open()和Close()
	//
	BOOL UnZip(CString sFileName, CString sFolder, bool bIgnoreFilePath = FALSE,TCHAR* tcRoot=NULL)
	{
		USES_CONVERSION;
		char* pFileName = T2A(sFileName.GetBuffer(0));
		m_uzFile = unzOpen(pFileName);
		if (m_uzFile)
		{
			if (!CreateFolder(sFolder))
				return FALSE;

			unz_global_info info;
			if (unzGetGlobalInfo(m_uzFile,&info) != UNZ_OK)
			{
				return FALSE;
			}

			if (!GotoFirstFile())
				return FALSE;

			do
			{
				if (!UnzipFile(sFolder, bIgnoreFilePath,tcRoot))
					return FALSE;
			}
			while (GotoNextFile());

		}

		unzCloseCurrentFile(m_uzFile);
		unzClose(m_uzFile);
		m_uzFile = NULL;

		return TRUE;
	}

    BOOL AddFile(CString f_file)
	{
	   BOOL bReturn = FALSE;

	   // Open file being added
	   HANDLE hFile = NULL;
	   hFile = CreateFile(f_file, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	   if (hFile)
	   {
		  // Get file creation date
		  FILETIME       ft = getLastWriteFileTime(f_file);
		  zip_fileinfo   zi = {0};

		  FileTimeToDosDateTime(
			 &ft,                       // last write FILETIME
			 ((LPWORD)&zi.dosDate)+1,   // dos date
			 ((LPWORD)&zi.dosDate)+0);  // dos time

		  // Trim path off file name
		  CString sFileName = f_file.Mid(f_file.ReverseFind(_T('\\')) + 1);

		  // Start a new file in Zip
		  USES_CONVERSION;
		  char* pFileName = T2A(sFileName.GetBuffer(0));
		  if (ZIP_OK == zipOpenNewFileInZip(m_zf, 
											pFileName/*sFileName*/, 
											&zi, 
											NULL, 
											0, 
											NULL, 
											0, 
											NULL, 
											Z_DEFLATED, 
											Z_BEST_COMPRESSION))
		  {
			 // Write file to Zip in 4 KB chunks 
			 const DWORD BUFFSIZE    = 4096;
			 TCHAR buffer[BUFFSIZE]  = _T("");
			 DWORD dwBytesRead       = 0;

			 while (ReadFile(hFile, &buffer, BUFFSIZE, &dwBytesRead, NULL)
					&& dwBytesRead)
			 {
				bReturn = FALSE;
				 if (ZIP_OK == zipWriteInFileInZip(m_zf, buffer, dwBytesRead)
				   /*&& dwBytesRead < BUFFSIZE*/)
				{
				   // Success
				   bReturn = TRUE;
				}
			 }

			 bReturn &= (ZIP_OK == zipCloseFileInZip(m_zf));
		  }
      
		  bReturn &= CloseHandle(hFile);
	   }

	   return bReturn;
	}

	BOOL GetFileByName(LPCTSTR zip,LPCTSTR tcName,LPCTSTR releasepath)
	{
		USES_CONVERSION;
		char* pFileName = T2A((LPTSTR)zip);
		m_uzFile = unzOpen(pFileName);
		if (m_uzFile)
		{
			unz_global_info info;
			if (unzGetGlobalInfo(m_uzFile,&info) != UNZ_OK)
			{
				unzClose(m_uzFile);
				return FALSE;
			}
			UZ_FileInfo fileinfo;
			if (unzGoToFirstFile(m_uzFile) != UNZ_OK)
				return FALSE;
			do 
			{
				if (GetFileInfo(fileinfo))
				{
					OutputDebugString(fileinfo.szFileName);
					if(_tcsstr(fileinfo.szFileName,tcName)!=NULL)
						return UnzipFile(releasepath,TRUE);
				}
			} while(unzGoToNextFile(m_uzFile) == UNZ_OK);
		}
		unzClose(m_uzFile);
		m_uzFile = NULL;
		return FALSE;
	}

	CString GetFirstFileName(LPCTSTR zip)
	{
		CString name(_T(""));
		USES_CONVERSION;
		char* pFileName = T2A((LPTSTR)zip);
		m_uzFile = unzOpen(pFileName);
		if (m_uzFile)
		{
			unz_global_info info;
			if (unzGetGlobalInfo(m_uzFile,&info) != UNZ_OK)
			{
				unzClose(m_uzFile);
				return _T("");
			}
			UZ_FileInfo fileinfo;
			if (unzGoToFirstFile(m_uzFile) == UNZ_OK)
			{
				if (GetFileInfo(fileinfo))
					name=fileinfo.szFileName;
			}
		}
		unzClose(m_uzFile);
		m_uzFile = NULL;
		return name;
	}
private:
	bool GotoFirstFile(LPCTSTR szExt=NULL)
	{
		if (!szExt || !lstrlen(szExt))
			return (unzGoToFirstFile(m_uzFile) == UNZ_OK);

		// else
		if (unzGoToFirstFile(m_uzFile) == UNZ_OK)
		{
			UZ_FileInfo info;

			if (!GetFileInfo(info))
				return FALSE;

			// test extension
			char szFExt[_MAX_EXT];
			USES_CONVERSION;
			char* szFileName = T2A(info.szFileName);
			_splitpath(szFileName, NULL, NULL, NULL, szFExt);

			TCHAR* szFExt2;
#ifdef UNICODE
				szFExt2 = A2W(szFExt);
#else
				szFExt2 = szFExt;
#endif
			if (szFExt)
			{
				if (lstrcmpi(szExt, szFExt2 + 1) == 0)
					return TRUE;
			}

			return GotoNextFile(szExt);
		}

		return FALSE;
	}

	bool GotoNextFile(LPCTSTR szExt = NULL)
	{
		if (!m_uzFile)
			return FALSE;

		if (!szExt || !lstrlen(szExt))
			return (unzGoToNextFile(m_uzFile) == UNZ_OK);

		// else
		UZ_FileInfo info;

		while (unzGoToNextFile(m_uzFile) == UNZ_OK)
		{
			if (!GetFileInfo(info))
				return FALSE;

			// test extension
			char szFExt[_MAX_EXT];
			USES_CONVERSION;
			char* szFileName;
			szFileName = T2A(info.szFileName);
			_splitpath(szFileName, NULL, NULL, NULL, szFExt);

			if (szFExt)
			{
				TCHAR* szFExt2;
#ifdef UNICODE
				szFExt2 = A2W(szFExt);
#else
				szFExt2 = szFExt;
#endif
				if (lstrcmpi(szExt, szFExt2 + 1) == 0)
					return TRUE;
			}
		}

		return FALSE;

	}

	bool CreateFolder(CString sFolder)
	{
		if (sFolder == _T(""))
		{
			return false;
		}
		DWORD dwAttrib = GetFileAttributes(sFolder.GetBuffer(0));

		// already exists ?
		if (dwAttrib != 0xffffffff)
			return ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);

		// recursively create from the top down
		USES_CONVERSION;
		char* pFolder = T2A(sFolder.GetBuffer(0));
		char* szPath = _strdup(pFolder);
		char* p = strrchr(szPath, '\\');

		if (p) 
		{
			// The parent is a dir, not a drive
			*p = '\0';
				
			// if can't create parent
			if (!CreateFolder(szPath))
			{
				free(szPath);
				return FALSE;
			}
			free(szPath);

			if (!CreateDirectory(sFolder.GetBuffer(0), NULL)) 
			{
				if (GetLastError() != ERROR_ALREADY_EXISTS)
				{
					return FALSE;
				}
			}
		}
		
		return TRUE;
	}

	bool GetFileInfo(UZ_FileInfo& info)
	{
		if (!m_uzFile)
			return FALSE;

		unz_file_info uzfi;

		ZeroMemory(&info, sizeof(info));
		ZeroMemory(&uzfi, sizeof(uzfi));

		USES_CONVERSION;
		char szFileName[MAX_PATH];// = W2A(info.szFileName);
		char szComment[MAX_PATH];// = W2A(info.szComment);
		if (UNZ_OK != unzGetCurrentFileInfo(m_uzFile, &uzfi, szFileName, MAX_PATH, NULL, 0, szComment, MAX_COMMENT))
			return FALSE;

		// copy across
		info.dwVersion = uzfi.version;	
		info.dwVersionNeeded = uzfi.version_needed;
		info.dwFlags = uzfi.flag;	
		info.dwCompressionMethod = uzfi.compression_method; 
		info.dwDosDate = uzfi.dosDate;  
		info.dwCRC = uzfi.crc;	 
		info.dwCompressedSize = uzfi.compressed_size; 
		info.dwUncompressedSize = uzfi.uncompressed_size;
		info.dwInternalAttrib = uzfi.internal_fa; 
		info.dwExternalAttrib = uzfi.external_fa; 

		CString sTmp;
		sTmp.Format(_T("%s"),A2W(szFileName));
		memcpy(info.szFileName,sTmp,sTmp.GetLength()*sizeof(TCHAR));
		sTmp.Format(_T("%s"),A2W(szComment));
		memcpy(info.szComment,sTmp,sTmp.GetLength()*sizeof(TCHAR));

		// replace filename forward slashes with backslashes
		int nLen = lstrlen(info.szFileName);

		while (nLen--)
		{
			if (info.szFileName[nLen] == '/')
				info.szFileName[nLen] = '\\';
		}

		// is it a folder?
		info.bFolder = ((info.dwExternalAttrib & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);

		return TRUE;
	}

	bool CreateFilePath(CString sFilePath)
	{
		USES_CONVERSION;
		char* szPath = _strdup(T2A(sFilePath.GetBuffer(0)));
		char* p = strrchr(szPath,'\\');

		bool bRes = FALSE;

		if (p)
		{
			*p = '\0';

			bRes = CreateFolder(szPath);
		}

		free(szPath);

		return bRes;
	}


	bool UnzipFile(CString sFolder, bool bIgnoreFilePath,TCHAR* tcRoot=NULL)
	{
		if (!m_uzFile)
			return FALSE;

		if (sFolder == _T(""))
		{
			return FALSE;
		}

		if (!CreateFolder(sFolder))
			return FALSE;

		UZ_FileInfo info;
		GetFileInfo(info);

		USES_CONVERSION;
		// if the item is a folder then create it and return 'TRUE'
		if (info.bFolder)
		{
			char szFolderPath[MAX_PATH];
			_makepath(szFolderPath, NULL, T2A(sFolder.GetBuffer(0)), T2A(info.szFileName), NULL);
			if(tcRoot!=NULL && _tcslen(tcRoot)==0)
			{
				//first
				_tcscpy(tcRoot,info.szFileName);
			}
			return CreateFolder(szFolderPath);
		}

		// build the output filename
		char szFilePath[MAX_PATH];
		if (bIgnoreFilePath)
		{
			char szFName[_MAX_FNAME], szExt[_MAX_EXT];
            char* pFileName = T2A(info.szFileName);
			_splitpath(pFileName, NULL, NULL, szFName, szExt);
			_makepath(pFileName, NULL, NULL, szFName, szExt);
			_makepath(szFilePath, NULL, T2A(sFolder.GetBuffer(0)), pFileName, NULL);
		}
		else
			_makepath(szFilePath, NULL, T2A(sFolder.GetBuffer(0)), T2A(info.szFileName), NULL);
//		TRACE(_T("Unzip: %s\n"),A2W(szFilePath));

		// open the input and output files
		if (!CreateFilePath(szFilePath))
			return FALSE;

		HANDLE hOutputFile = ::CreateFileA(szFilePath, 
											GENERIC_WRITE,
											0,
											NULL,
											CREATE_ALWAYS,
											FILE_ATTRIBUTE_NORMAL,
											NULL);

		if (INVALID_HANDLE_VALUE == hOutputFile)
			return FALSE;

		if (unzOpenCurrentFile(m_uzFile) != UNZ_OK)
			return FALSE;

		// read the file and output
		int nRet = UNZ_OK;
		char pBuffer[BUFFERSIZE];

		do
		{
			nRet = unzReadCurrentFile(m_uzFile, pBuffer, BUFFERSIZE);

			if (nRet > 0)
			{
				// output
				DWORD dwBytesWritten = 0;

				if (!::WriteFile(hOutputFile, pBuffer, nRet, &dwBytesWritten, NULL) ||
					dwBytesWritten != (DWORD)nRet)
				{
					nRet = UNZ_ERRNO;
					break;
				}
			}
		}
		while (nRet > 0);

		CloseHandle(hOutputFile);
		unzCloseCurrentFile(m_uzFile);

		//if (nRet == UNZ_OK)
		//	SetFileModTime(szFilePath, info.dwDosDate);

		return (nRet == UNZ_OK);
	}

protected:
	zipFile m_zf;
	void* m_uzFile;
};

#endif // !_ZLIBCPP_H
