/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                                 *
 * Copyright (c) 2019, William C. Lenthe                                           *
 * All rights reserved.                                                            *
 *                                                                                 *
 * Redistribution and use in source and binary forms, with or without              *
 * modification, are permitted provided that the following conditions are met:     *
 *                                                                                 *
 * 1. Redistributions of source code must retain the above copyright notice, this  *
 *    list of conditions and the following disclaimer.                             *
 *                                                                                 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,    *
 *    this list of conditions and the following disclaimer in the documentation    *
 *    and/or other materials provided with the distribution.                       *
 *                                                                                 *
 * 3. Neither the name of the copyright holder nor the names of its                *
 *    contributors may be used to endorse or promote products derived from         *
 *    this software without specific prior written permission.                     *
 *                                                                                 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"     *
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE       *
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE  *
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE    *
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL      *
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR      *
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER      *
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,   *
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE   *
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.            *
 *                                                                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#ifndef _mmap_h_
#define _mmap_h_

#include <string>
#include <cstdint>

namespace memorymap {
	//wraper for type of memory map access pattern hints
	enum class Hint {
		Normal    ,//no special treatment
		Sequential,//expect sequential access (modify caching behavior accordingly)
		Random     //expect random access (modify caching behavior accordingly)
	};

	//@brief: cross platform memory mapped file interface
	class File {
		public:

			//@brief: open or create a memory mapped file
			//@param filename: name of file to open with memory map
			//@param hint: access pattern hint
			//@param write: true to allow write access, false for read only (true + a non existent file -> write only)
			//@param size: size to create or resize file to (or 0 to use current file size, ignored if no write access is requested)
			File(std::string filename, Hint hint = Hint::Normal, const bool write = false, const uint64_t size = 0);

			//@brief: close the memory mapped file / cleanup on destruction (+write changes to disk if needed)
			~File();

			//@brief: get read only raw pointer to memory mapped file
			//@return: raw pointer
			//@note: writing to the pointer (e.g. by const casting) will cause undefined behavior if the file wasn't opened with write access (writeAccess() == false)
			char const * constData() const {return (char const*)fileBuffer;}

			//@brief: get read/write raw pointer to memory mapped file
			//@return: raw pointer
			char* data() {
				if(!canWrite) throw std::runtime_error("write access to read only memory map isn't allowed");
				return (char*)fileBuffer;
			}

			//@brief: get size of mapped file in bytes
			//@return: size of mapped file in bytes
			std::uint64_t size() const {return fileBytes;}

			//@brief: check if the file as write access or is read only
			//@return: true for read/write, false for read only
			bool writeAccess() const {return canWrite;}

		private:
			void*         fileBuffer;//pointer to raw data in memory map
			std::uint64_t fileBytes ;//size of file in bytes
			bool          canWrite  ;//flag for if the file is open for read only or read/write access

			//for pointer based file handles (windows)
			void*         fileHandle;//handle to opened file
			void*         fileMap   ;//handle to memory map
			
			//for integer based file handles (*nix)
			int           fileId    ;//opened file number

			//disable copying
			File(File const &) = delete;
			void operator=(File const &) = delete;
	};
}

//the two main interfaces to memory maps are provided by windows and unix style interfaces, this block handles the includes / #defines to switch between them at compile time as needed
#define _MMAP_WIN_ 1
#define _MMAP_NIX_ 2
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
	//define flag for subsequent os dependent code
	#define _MMAP_API_TYPE_ _MMAP_WIN_

	//limit windows includes
	#ifndef NOMINMAX//don't redefine
		#define NOMINMAX//the min/max macros can cause issues with std::min/max
	#endif
	#ifndef WIN32_LEAN_AND_MEAN//don't redefine
		#define WIN32_LEAN_AND_MEAN//we don't need much, don't bother including it all the extra bits
	#endif

	//define windows version (for GetFileSizeEx)
	#ifndef WINVER//don't redefine
		#define WINVER 0x0501//this is windows xp so it shouldn't be asking for too much...
	#endif
	#include <windows.h>
#elif __APPLE__ || __linux__ || __unix__ || defined(_POSIX_VERSION)
	//define flag for subsequent os dependent code
	#define _MMAP_API_TYPE_ _MMAP_NIX_

	#include <fcntl.h>    //file access flags
	#include <errno.h>    //error state
	#include <string.h>   //strerror
	#include <sys/mman.h> //memory map
	#include <sys/stat.h> //stat struct
	#include <sys/types.h>//fstat
	#include <unistd.h>   //close

	//___64 is now deprecated on mac but should still be preferred on linux
	#if __APPLE__
		#include <Availability.h>//version information
		#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 1050//1050 is __MAC_10_5, the first version that stat64/mmap64 are deprecated
			#define __64_FUNCS_DEPRECATED_
		#endif
	#endif
	#ifdef __64_FUNCS_DEPRECATED_//___64 functions are deprecated
		#define STAT stat
		#define MMAP mmap
	#else//explicitely use 64 bit functions
		#define STAT stat64
		#define MMAP mmap64
	#endif
#else
	//if the platform isn't windows, apple, linix, or unix we're in trouble
	#define _MMAP_API_TYPE_ 0//I'll use 0 as a placeholder for unknown
#endif

namespace memorymap {
	//first we need some functions for wrapping os dependent calls / values that memory maps depend on (errors + hints)
	namespace detail {
		//@brief: os independent wrapper to get the error as a string
		//@return: error message as std::string (empty string if no error message was set)
		std::string getErrorMessage() {
			#if _MMAP_API_TYPE_ == _MMAP_WIN_//windows
				DWORD errorCode = GetLastError();//get the current error code
				if(errorCode == 0) return std::string();//if there isn't an error we're done
				LPSTR buff = NULL;
				size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&buff, 0, NULL);//get message
				std::string message(buff, size);//copy to string
				LocalFree(buff);//free buffer
				return message;
			#elif _MMAP_API_TYPE_ == _MMAP_NIX_// *nix
				return std::string(strerror(errno));
			#else//unknown platform
				static_assert(false, "memorymap::detail::getErrorMessage isn't implemented for this platform");
			#endif
		}

		//@brief: os independent wrapper function to convert from Hint enum to #defined flags for memory map hints
		//@param hint: cross platform Hint enum to translate
		//@return: os specific #defined hint code
		std::uint32_t translateHint(const Hint& hint) {
			#if _MMAP_API_TYPE_ == _MMAP_WIN_//windows
				switch(hint) {
					case Hint::Normal    : return (std::uint32_t) FILE_ATTRIBUTE_NORMAL    ;
					case Hint::Sequential: return (std::uint32_t) FILE_FLAG_SEQUENTIAL_SCAN;
					case Hint::Random    : return (std::uint32_t) FILE_FLAG_RANDOM_ACCESS  ;
				}
			#elif 2==_MMAP_API_TYPE_// *nix
				switch(hint) {
					case Hint::Normal    : return (std::uint32_t) MADV_NORMAL    ;
					case Hint::Sequential: return (std::uint32_t) MADV_SEQUENTIAL;
					case Hint::Random    : return (std::uint32_t) MADV_RANDOM    ;
				}
			#else//unknown platform
				static_assert(false, "memorymap::detail::translateHint isn't implemented for this platform");
			#endif
			throw std::logic_error("failed to parse memory map hint");
		}
	}

	//@brief: open a memory mapped file
	//@param filename: name of file to open with memory map
	//@param hint: access pattern hint
	//@param write: true to allow write access, false for read only
	//@param size: size to create or resize file to (or 0 to use current file size, ignored if no write access is requested)
	File::File(std::string fileName, Hint hint, const bool write, const uint64_t size) : fileBuffer(NULL), fileBytes(write ? size : 0), fileHandle(NULL), fileMap(NULL), canWrite(write) {
		#if _MMAP_API_TYPE_ == _MMAP_WIN_//windows
			const DWORD attrib = GetFileAttributesA(fileName.c_str());//get info about the file
			const bool fileExists = (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));//check if the file exists
			if(fileExists || (canWrite && fileBytes != 0) ) {//if the file doesn't exist we should have write only access
				fileHandle = (HANDLE) CreateFileA(fileName.c_str(), (canWrite ? GENERIC_WRITE : 0) | GENERIC_READ, 0, NULL, fileExists ? OPEN_EXISTING : CREATE_NEW, (DWORD)detail::translateHint(hint), 0);//open file with access hint
				if(NULL != fileHandle) {//the file was successfully opened
					bool error = false;
					LARGE_INTEGER lInt;//windows 64-bit integer struct (holdover from pair of 32 bit ints)
					if(0 == fileBytes) {//get file size if needed
						if(0 == GetFileSizeEx(fileHandle, &lInt)) error = true;//get file size
						fileBytes = (std::uint64_t)lInt.QuadPart;//convert file size to uint64_t
					} else {//otherwise set file size
						bool resized = false;
						lInt.QuadPart = fileBytes;
						if (0 == SetFilePointerEx(fileHandle, lInt, NULL, FILE_BEGIN)) {//set the end of the file (extend/truncate as needed)
							error = true;
						} else if (0 == SetEndOfFile(fileHandle)) {
							error = true;
						}
					} 
					if(!error) {//did we get/update the file size without issue?
						fileMap = CreateFileMapping(fileHandle, NULL, canWrite ? PAGE_READWRITE : PAGE_READONLY, 0, 0, NULL);//create memory map
						if(NULL != fileMap) fileBuffer = MapViewOfFile(fileMap, canWrite ? FILE_MAP_WRITE : FILE_MAP_READ, 0, 0, 0);//get pointer to raw data if the map was successful
					}
				}
				if(NULL == fileBuffer) {//if the raw pointer is null something went wrong
					if(NULL != fileMap   )	CloseHandle(fileMap   );//close the map if it was created
					if(NULL != fileHandle)	CloseHandle(fileHandle);//close the handle if it was opened
					throw std::runtime_error(fileName + " couldn't be memory mapped: " + detail::getErrorMessage());
				}
			}
		#elif _MMAP_API_TYPE_ == _MMAP_NIX_// *nix
			//unix/mac #defined values
			struct STAT fileStat;
			const bool fileExists = STAT(fileName.c_str(), &fileStat) >= 0;//try to get information about the file without opening it
			if(fileExists || (canWrite && fileBytes != 0) ) {//if the file doesn't exist we should have write only access
				// fileId = open(fileName.c_str(), O_RDWR | O_CREAT, 0777);//open file with appropriate access: exist -> read with possible write, doesn't exist -> write only
				fileId = open(fileName.c_str(), fileExists ? (canWrite ? O_RDWR : O_RDONLY) : O_RDWR | O_CREAT, 0777);//open file with appropriate access: exist -> read with possible write, doesn't exist -> write only
				if(-1 != fileId) {//the file was opened successfully
					bool error = false;
					if(0 == fileBytes) {//get the file size if needed
						fileBytes = fileStat.st_size;//update the file size for read only access
					} else {//otherwise change file size
						if(-1 == ftruncate(fileId, fileBytes)) {//update the file size
							error = true;
						}
					}
					if(!error) {//did we get/update the file size without issue?
						fileBuffer = MMAP(NULL, fileBytes, PROT_READ | (canWrite ? PROT_WRITE : 0), MAP_SHARED, fileId, 0);//create the memory map with appropriate access
						if(MAP_FAILED != fileBuffer) madvise(fileBuffer, fileBytes, (int)hint);//give the os our access hint if the map was successful
					}
				}
			}
			if(NULL == fileBuffer || MAP_FAILED == fileBuffer) {//if the raw pointer is null/bad something went wrong
				if(-1 == fileId) close(fileId);//close the file if it was opened
				throw std::runtime_error(fileName + " couldn't be memory mapped: " + detail::getErrorMessage());
			}
		#endif
	}

	File::~File() {
		#if _MMAP_API_TYPE_ == _MMAP_WIN_//windows
			UnmapViewOfFile(fileBuffer);//close memory map view
			CloseHandle(fileMap);//close map
			CloseHandle(fileHandle);//close file
		#elif _MMAP_API_TYPE_ == _MMAP_NIX_// *nix
			munmap(fileBuffer, fileBytes);//close memory map
			close(fileId);//close file
		#endif
	}
}

#endif//_mmap_h_
