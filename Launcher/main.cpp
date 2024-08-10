/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
#include <Windows.h>
#include <string>
#include "deps/Detours/src/detours.h"

int main()
{
    std::wstring glquake_exe_file_name  = L"glquake.exe";
    std::wstring glquake_exe_file_path;
    std::string  replayer_dll_file_name =  "Replayer.dll";

    /* Identify where glquake.exe is located. */
    if (::GetFileAttributesW(glquake_exe_file_name.c_str() ) == INVALID_FILE_ATTRIBUTES)
    {
        /* Not in the working directory. Ask the user to point to the file. */
        OPENFILENAMEW open_file_name_descriptor           = {};
        wchar_t       result_file_name         [MAX_PATH] = {};

        open_file_name_descriptor.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
        open_file_name_descriptor.hwndOwner   = HWND_DESKTOP;
        open_file_name_descriptor.lStructSize = sizeof(open_file_name_descriptor);
        open_file_name_descriptor.lpstrFilter  = L"GLQuake Executable (glquake.exe)\0glquake.exe\0";
        open_file_name_descriptor.lpstrFile    = result_file_name;
        open_file_name_descriptor.nMaxFile     = sizeof(result_file_name);

        if (::GetOpenFileNameW(&open_file_name_descriptor) == 0)
        {
            ::MessageBox(HWND_DESKTOP,
                         "You must provide a path to glquake.exe in order to run the tool.",
                         "Error",
                         MB_OK | MB_ICONERROR);

            goto end;
        }

        glquake_exe_file_name = std::wstring(result_file_name);
        glquake_exe_file_path = std::wstring(result_file_name,
                                             0,
                                             open_file_name_descriptor.nFileOffset);
    }

    /* Append args required for the tool to work as expected with glquake. */
    glquake_exe_file_name += std::wstring(L" -window -fullsbar");

    /* Now look for replayer.dll .. */
    {
        char working_dir[MAX_PATH] = {};

        if (::GetCurrentDirectory(sizeof(working_dir),
                                  working_dir) == 0)
        {
            ::MessageBox(HWND_DESKTOP,
                         "Failed to identify working directory.",
                         "Error",
                         MB_OK | MB_ICONERROR);

            goto end;
        }

        replayer_dll_file_name = std::string(working_dir) + "\\" + replayer_dll_file_name;

        if (::GetFileAttributes(replayer_dll_file_name.c_str() ) == INVALID_FILE_ATTRIBUTES)
        {
            ::MessageBox(HWND_DESKTOP,
                         "Replayer.dll not found in the working directory.",
                         "Error",
                         MB_OK | MB_ICONERROR);

            goto end;
        }
    }

    // Launch GLQuake with APIInterceptor attached.
    {
        PROCESS_INFORMATION process_info;
        STARTUPINFOW        startup_info;

        memset(&process_info,
               0,
               sizeof(process_info) );
        memset(&startup_info,
               0,
               sizeof(startup_info) );

        startup_info.cb = sizeof(startup_info);

        if (::DetourCreateProcessWithDllW(nullptr,
                                          const_cast<LPWSTR>(glquake_exe_file_name.c_str() ),
                                          nullptr,                          /* lpProcessAttributes */
                                          nullptr,                          /* lpThreadAttributes  */
                                          FALSE,                            /* bInheritHandles     */
                                          CREATE_DEFAULT_ERROR_MODE,
                                          nullptr,                          /* lpEnvironment       */
                                          glquake_exe_file_path.c_str(),    /* lpCurrentDirectory  */
                                         &startup_info,
                                         &process_info,
                                          replayer_dll_file_name.c_str(),
                                          nullptr) != TRUE)                 /* pfCreateProcessA    */
        {
            std::string error = "Could not launch GLQuake with APIInterceptor attached.";

            ::MessageBox(HWND_DESKTOP,
                         error.c_str(),
                         "Error",
                         MB_OK | MB_ICONERROR);
        }

        #if defined(_DEBUG)
        {
            ::WaitForSingleObject(process_info.hProcess,
                                  INFINITE);
        }
        #endif
    }

end:
    return EXIT_SUCCESS;
}