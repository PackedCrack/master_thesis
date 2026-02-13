#include "C:\\Program Files\\University of Arizona\\Tigress C Source Code Obfuscator\\Tigress\\tigress.h"
#include "T1005.h"

#include "../misc/common.h"
#include "../misc/function_pointers.h"
#include "../misc/vector.h"
#include "../misc/wstr.h"
#include "../runtime_linking.h"

#include <assert.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ShlObj_core.h>

// Kernel32.dll
#define FIND_FIRST_FILE_W 0
#define FIND_NEXT_FILE_W 1
#define CLOSE_HANDLE 2
#define FIND_CLOSE 3
static const char* t1005_kernel32Procedures[4] = { "FindFirstFileW", "FindNextFileW", "CloseHandle", "FindClose" };
// Shell32.dll
#define SH_GET_KNOWN_FOLDER_PATH 0
static const char* t1005_shell32Procedures[1] = { "SHGetKnownFolderPath" };
// Ole32.dll
#define CO_TASK_MEM_FREE 0
static const char* t1005_ole32Procedures[1] = { "CoTaskMemFree" };


static LPCWSTR s_extensions[] = {
	// Images
	L".jpg", L".jpeg", L".jpe", L".png", L".bmp", L".gif", L".tif", L".tiff", L".webp", L".heic", L".heif", L".ico", L".svg",

	// Source code
	L".c", L".h", L".cpp", L".hpp", L".cc", L".cxx", L".hh", L".hxx",
	L".cs", L".java", L".kt", L".swift",
	L".m", L".mm",
	L".py", L".pyw", L".ipynb",
	L".js", L".mjs", L".cjs", L".ts", L".tsx",
	L".go", L".rs", L".lua", L".rb", L".php",
	L".sh", L".bash", L".zsh", L".ps1", L".bat", L".cmd",
	L".sql",
	L".html", L".htm", L".css", L".scss", L".xml", L".json", L".yaml", L".yml", L".toml",
	L".make", L".mk", L".cmake", L".gradle",

	// Word
	L".doc", L".docx", L".dot", L".dotx",
	L".odt", L".ott", L".rtf", L".txt", L".pdf",

	// Excel
	L".xls", L".xlsx", L".xlsm", L".xlt", L".xltx",
	L".ods", L".ots", L".csv", L".tsv",

	// PowerPoint
	L".ppt", L".pptx", L".pptm", L".pot", L".potx",
	L".odp", L".otp",

	// Video
	L".mp4", L".m4v", L".mkv", L".webm", L".avi", L".mov", L".qt",
	L".wmv", L".flv", L".f4v", L".mpeg", L".mpg", L".m2v",
	L".3gp", L".3g2", L".ts", L".m2ts"
};


// Quick linked list and queue for use in BFS
typedef struct Node Node;
struct Node
{
	Node* pFlink;
	Node* pBlink;
	WideString directory;
};
static Node* create_node(Node* pBack, LPCWSTR directory)
{
	assert(directory != NULL);

	Node* pNode = (Node*) malloc(sizeof(Node));
	if (pNode == NULL)
	{
		return NULL;
	}

	memset(pNode, 0, sizeof(Node));

	pNode->pBlink = pBack;
	pNode->directory = WSTRING_CREATE_FROM_LPCWSTR(directory);

	return pNode;
}
static void destroy_node(Node* pNode)
{
	assert(pNode != NULL);

	WSTRING_DESTROY(pNode->directory);
	free(pNode);
}
// Queue
typedef struct
{
	Node* pFront;
	Node* pBack;
} Queue;
static Queue make_queue()
{
	Queue q = { 0 };
	return q;
}
static BOOL queue_empty(const Queue* pQueue)
{
	return (pQueue->pFront == NULL);
}
static BOOL queue_push_back(Queue* pQueue, LPCWSTR directory)
{
	Node* pNode = create_node(pQueue->pBack, directory);
	if (pNode == NULL)
	{
		return FALSE;
	}

	if (pQueue->pBack == NULL)
	{
		pQueue->pFront = pNode;
		pQueue->pBack = pNode;

		pNode->pFlink = NULL;
		pNode->pBlink = NULL;
	}
	else
	{
		pQueue->pBack->pFlink = pNode;

		pNode->pBlink = pQueue->pBack;
		pNode->pFlink = NULL;

		pQueue->pBack = pNode;
	}

	return TRUE;
}
static BOOL queue_pop_front(Queue* pQueue)
{
	Node* pNode = pQueue->pFront;
	if (pNode == NULL)
	{
		return FALSE;
	}

	Node* pNext = pNode->pFlink;
	pQueue->pFront = pNext;

	if (pNext)
	{
		pNext->pBlink = NULL;
	}
	else
	{
		pQueue->pBack = NULL;
	}

	pNode->pFlink = NULL;
	pNode->pBlink = NULL;

	destroy_node(pNode);

	return TRUE;
}
static LPCWSTR queue_front(Queue* pQueue)
{
	assert(pQueue != NULL);
	return WSTRING_C_STR(pQueue->pFront->directory);
}
static void queue_destroy(Queue* pQueue)
{
	while (!queue_empty(pQueue))
	{
		queue_pop_front(pQueue);
	}
}
//
//
static BOOL is_current_or_parent(LPCWSTR name)
{
	if (name == NULL)
	{
		return FALSE;
	}

	return ((wcscmp(name, L".") == 0) || (wcscmp(name, L"..") == 0));
}
static BOOL has_any_extension(LPCWSTR filename, Vector* pExtensions)
{
	assert(!VECTOR_EMPTY(*pExtensions));

	LPCWSTR subStr = wcsrchr(filename, L'.');
	if (subStr == NULL)
	{
		return FALSE;
	}

	for (size_t i = 0; i < VECTOR_SIZE(*pExtensions); ++i)
	{
		LPCWSTR extension = WSTRING_C_STR(*VECTOR_AT(*pExtensions, WideString, i));
		assert(extension != NULL);

		if (_wcsicmp(extension, subStr) == 0)
		{
			return TRUE;
		}
	}

	return FALSE;
}
static BOOL has_trailing_backslash(LPCWSTR dir)
{
	size_t len = wcslen(dir);
	if (len > 0)
	{
		WCHAR last = dir[len - 1];
		if (last == L'\\' || last == L'/')
		{
			return TRUE;
		}
	}

	return FALSE;
}
static WideString create_by_merge(LPCWSTR dir, LPCWSTR toAppend)
{
	if (dir == NULL || toAppend == NULL)
	{
		return (WideString) { 0 };
	}
	
	WideString new = WSTRING_CREATE_FROM_LPCWSTR(dir);
	if (!has_trailing_backslash(dir))
	{
		WCHAR backslash = L'\\';
		WSTRING_PUSH_BACK(new, backslash);
	}

	size_t endLen = wcslen(toAppend);
	for (size_t i = 0; i < endLen; ++i)
	{
		WCHAR c = toAppend[i];
		WSTRING_PUSH_BACK(new, c);
	}
	WCHAR terminator = L'\0';
	WSTRING_PUSH_BACK(new, terminator);

	return new;
}
static WideString make_wildcard_pattern(LPCWSTR dir)
{
	return create_by_merge(dir, L"*");
}
static HANDLE find_first(ProcedureList* pKernel32, Queue* pQueue, PWIN32_FIND_DATAW outData)
{
	LPCWSTR directory = queue_front(pQueue);

	HANDLE hFind = INVALID_HANDLE_VALUE;
	WideString pattern = make_wildcard_pattern(directory);
	if (WSTRING_SIZE(pattern) != 0)
	{
		PFN_FindFirstFileW find_first_file_w = *VECTOR_AT(pKernel32->procedures, PFN_FindFirstFileW, FIND_FIRST_FILE_W);
		hFind = find_first_file_w(WSTRING_C_STR(pattern), outData);

		WSTRING_DESTROY(pattern);
	}

	if (hFind == INVALID_HANDLE_VALUE)
	{
		PRINT_WIN32_ERROR(FindFirstFileW);
		queue_pop_front(pQueue);
	}

	return hFind;
}
static BOOL is_sym_link(PWIN32_FIND_DATAW data)
{
	return data->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT;
}
static BOOL is_directory(PWIN32_FIND_DATAW data)
{
	return data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && !is_sym_link(data);
}
static void push_sub_directory_to_queue(Queue* pQueue, PWIN32_FIND_DATAW data)
{
	if (!is_current_or_parent(data->cFileName))
	{
		LPCWSTR directory = queue_front(pQueue);
		WideString subDir = create_by_merge(directory, data->cFileName);
		if (WSTRING_SIZE(subDir) == 0)
		{
			printf("Failed to merge directory and filename\n");
			return;
		}

		if (!queue_push_back(pQueue, WSTRING_C_STR(subDir)))
		{
			WSTRING_DESTROY(subDir);
			printf("Failed to push subdir to queue\n");
			return;
		}

		WSTRING_DESTROY(subDir);
	}
}
static Vector create_extensions()
{
	Vector extensions = VECTOR_CREATE(WideString, 0);
	for (size_t i = 0; i < ARRAYSIZE(s_extensions); ++i)
	{
		WideString ext = WSTRING_CREATE_FROM_LPCWSTR(s_extensions[i]);
		VECTOR_PUSH_BACK(extensions, WideString, ext);
	}

	return extensions;
}
static void destroy_extensions(Vector* pExtensions)
{
	while (!VECTOR_EMPTY(*pExtensions))
	{
		WideString ext = *VECTOR_BACK(*pExtensions, WideString);
		WSTRING_DESTROY(ext);
		VECTOR_POP_BACK(*pExtensions);
	}

	VECTOR_DESTROY(*pExtensions);
}
static void log_file(HANDLE hLog, Queue* pQueue, PWIN32_FIND_DATAW data, Vector* pExtensions)
{
	if (!has_any_extension(data->cFileName, pExtensions))
	{
		return;
	}

	LPCWSTR directory = queue_front(pQueue);
	WideString filepath = create_by_merge(directory, data->cFileName);
	if (WSTRING_SIZE(filepath) != 0)
	{
		WCHAR line[1024] = { 0 };
		swprintf(line, ARRAYSIZE(line), L"Found intresting file: %ls\n", WSTRING_C_STR(filepath));
		write_to_file(hLog, line);
	}

	WSTRING_DESTROY(filepath);
}
static void log_files(ProcedureList* pKernel32,
					  HANDLE hLog,
					  Queue* pQueue)
{
	assert(pQueue != NULL);

	Vector extensions = create_extensions();

	while (!queue_empty(pQueue))
	{
		WIN32_FIND_DATAW findData = { 0 };
		HANDLE hFind = find_first(pKernel32, pQueue, &findData);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			PFN_FindNextFileW find_next_file_w = *VECTOR_AT(pKernel32->procedures, PFN_FindNextFileW, FIND_NEXT_FILE_W);
			do
			{
				if (is_directory(&findData))
				{
					push_sub_directory_to_queue(pQueue, &findData);
				}
				else
				{
					log_file(hLog, pQueue, &findData, &extensions);
				}


			} while (find_next_file_w(hFind, &findData));


			PFN_FindClose find_close = *VECTOR_AT(pKernel32->procedures, PFN_FindClose, FIND_CLOSE);
			find_close(hFind);

			queue_pop_front(pQueue);
		}
	}

	
	destroy_extensions(&extensions);
}
static Queue init_queue(Vector* pRootDirs)
{
	Queue q = make_queue();
	for (size_t i = 0; i < VECTOR_SIZE(*pRootDirs); ++i)
	{
		WideString* pDir = VECTOR_AT(*pRootDirs, WideString, i);
		queue_push_back(&q, WSTRING_C_STR(*pDir));
	}

	return q;
}
WideString resolve_filepath(ProcedureList* pShell32, ProcedureList* pOle32, REFKNOWNFOLDERID knownFolder)
{
	PWSTR pFilepath = NULL;
	PFN_SHGetKnownFolderPath sh_get_known_folder_path = *VECTOR_AT(pShell32->procedures, 
																   PFN_SHGetKnownFolderPath, 
																   SH_GET_KNOWN_FOLDER_PATH);
	HRESULT r = sh_get_known_folder_path(knownFolder,
									     0,
									     NULL,
									     &pFilepath);
	if (!SUCCEEDED(r))
	{
		printf("Failed to resolve filepath. Error: 0x%lX.\n", r);
		return (WideString) { 0 };
	}
	
	WideString s = WSTRING_CREATE_FROM_LPCWSTR(pFilepath);

	PFN_CoTaskMemFree co_task_mem_free = *VECTOR_AT(pOle32->procedures, PFN_CoTaskMemFree, CO_TASK_MEM_FREE);
	co_task_mem_free(pFilepath);

	return s;
}
static Vector create_root_dirs(ProcedureList* pShell32, ProcedureList* pOle32)
{
	Vector roots = VECTOR_CREATE(WideString, 0);

	static REFKNOWNFOLDERID folders[] = { &FOLDERID_Pictures, &FOLDERID_Videos, &FOLDERID_Downloads,
										  &FOLDERID_Documents, &FOLDERID_Desktop, &FOLDERID_AccountPictures };

	for (size_t i = 0; i < ARRAYSIZE(folders); ++i)
	{
		WideString path = resolve_filepath(pShell32, pOle32, folders[i]);
		if (WSTRING_SIZE(path) > 0)
		{
			VECTOR_PUSH_BACK(roots, WideString, path);
		}
	}

	return roots;
}
static void destroy_root_dirs(Vector* pRoots)
{
	while (!VECTOR_EMPTY(*pRoots))
	{
		WideString* pDir = VECTOR_BACK(*pRoots, WideString);
		WSTRING_DESTROY(*pDir);

		VECTOR_POP_BACK(*pRoots);
	}

	VECTOR_DESTROY(*pRoots);
}
//
//
void execute_t1005()
{
	ProcedureList kernel32 = procedure_list_create("kernel32.dll", t1005_kernel32Procedures, ARRAYSIZE(t1005_kernel32Procedures));
	ProcedureList shell32 = procedure_list_create("shell32.dll", t1005_shell32Procedures, ARRAYSIZE(t1005_shell32Procedures));
	ProcedureList ole32 = procedure_list_create("ole32.dll", t1005_ole32Procedures, ARRAYSIZE(t1005_ole32Procedures));
	
	Vector roots = create_root_dirs(&shell32, &ole32);

	Queue q = init_queue(&roots);

	HANDLE hLog = open_log_file(L"LOG_T1005_");
	log_files(&kernel32, hLog, &q);
	FARPROC PFN_CloseHandle = *VECTOR_AT(kernel32.procedures, FARPROC, CLOSE_HANDLE);
	PFN_CloseHandle(hLog);

	queue_destroy(&q);

	destroy_root_dirs(&roots);

	procedure_list_destroy(&ole32);
	procedure_list_destroy(&shell32);
	procedure_list_destroy(&kernel32);
}