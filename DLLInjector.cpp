#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <commdlg.h>
#include <string>
#include <sstream>

// Function to select a DLL file
std::string SelectDLLFile() {
    OPENFILENAME ofn;
    char szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "DLL Files\0*.dll\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn) == TRUE) {
        return std::string(ofn.lpstrFile);
    }
    return "";
}

// Function to get the ID of the target process
DWORD GetProcessID(const std::string& processName) {
    DWORD processID = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(hSnapshot, &pe)) {
            do {
                if (processName == pe.szExeFile) {
                    processID = pe.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnapshot, &pe));
        }
        CloseHandle(hSnapshot);
    }
    return processID;
}

// Function to inject the DLL
void InjectDLL(HANDLE hProcess, const std::string& dllPath) {
    void* pDllPath = VirtualAllocEx(hProcess, 0, dllPath.size() + 1, MEM_COMMIT, PAGE_READWRITE);
    WriteProcessMemory(hProcess, pDllPath, dllPath.c_str(), dllPath.size() + 1, 0);
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    HANDLE hThread = CreateRemoteThread(hProcess, 0, 0, (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryA"), pDllPath, 0, 0);
    WaitForSingleObject(hThread, INFINITE);
    VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
    CloseHandle(hThread);
}

// Main function
int main() {
    std::string targetProcessName;
    std::cout << "Enter the process name to inject into (e.g., notepad.exe): ";
    std::getline(std::cin, targetProcessName);

    DWORD processID = GetProcessID(targetProcessName);
    if (processID == 0) {
        std::cerr << "Unable to find the target process." << std::endl;
        return 1;
    }

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    if (!hProcess) {
        std::cerr << "Unable to open the target process." << std::endl;
        return 1;
    }

    std::string dllPath = SelectDLLFile();
    if (dllPath.empty()) {
        std::cerr << "No DLL file selected." << std::endl;
        CloseHandle(hProcess);
        return 1;
    }

    // Inject the DLL
    InjectDLL(hProcess, dllPath);
    std::cout << "DLL injected successfully!" << std::endl;

    CloseHandle(hProcess);
    return 0;
}
