#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#include "framework.h"
#include "HEX.h"
#include "Addons.h"
#include "string"
#include <stdio.h>

#define MAX_LOADSTRING 100
#define NUMBER_OF_SYMBOLS 16
#define NUMBER_OF_SYMBOLS_HEX 3
#define NUMBER_OF_SYMBOLS_OFFSET 8
#define SUM NUMBER_OF_SYMBOLS_OFFSET + 1 + NUMBER_OF_SYMBOLS * NUMBER_OF_SYMBOLS_HEX + NUMBER_OF_SYMBOLS * 2

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];
//WCHAR szChildClass[] = L"Checker3_Child" ;
//ATOM                MyRegisterClass(HINSTANCE hInstance);
//ATOM                MyRegisterChildClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    ChildWndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

OPENFILENAMEA File;
char FileName[MAX_PATH];
LPCSTR Buf = 0;
LONGLONG BufSize = 0;

bool LoadData(LPCSTR FileName) {
    HANDLE FileToLoad = CreateFileA(FileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (FileToLoad == INVALID_HANDLE_VALUE) {
        CloseHandle(FileToLoad);
        MessageBoxW(hWnd, L"File couldn't open!", L"ERROR", MB_OK);
        return false;
    }
    HANDLE hfilemap = CreateFileMapping(FileToLoad, NULL, PAGE_WRITECOPY, 0, 0, NULL);
    if (hfilemap == NULL) {
        CloseHandle(FileToLoad);
        MessageBoxW(hWnd, L"File couldn't open!", L"ERROR", MB_OK);
        return false;
    }
    PBYTE pbFile = (PBYTE)MapViewOfFile(hfilemap, FILE_MAP_COPY, 0, 0, 0); //проверка на NULL ина извлечение файла

    Buf = (LPCSTR)pbFile;

    LARGE_INTEGER FileSize;
    GetFileSizeEx(FileToLoad, &FileSize);//проверка
    BufSize = FileSize.QuadPart;

    //getsystemmetrics размер блоков для мапы 
    //GetSystemInfo
    //гранулярность почитать
    //CloseHandle(FileToLoad);
    //Недостаточно прав, GetLastError, GetFormatMassage, файл больше чем буфер, Файл пустой, вывод 

    CloseHandle(hfilemap);
    CloseHandle(FileToLoad);
}

void SetOpenFileParams(HWND hWnd) {

    if (Buf != NULL) {
        UnmapViewOfFile(Buf);
    }

    ZeroMemory(&File, sizeof(File));
    File.lStructSize = sizeof(File);
    File.hwndOwner = hWnd;
    File.lpstrFile = FileName;
    File.nMaxFile = sizeof(FileName);
    File.lpstrFilter = "*.txt";
    File.lpstrFileTitle = NULL;
    File.nMaxFileTitle = 0;
    File.lpstrInitialDir = "C:/Users/vladk/source/repos/HEX";
    File.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
}

/// <summary>
/// Преобразовать буфер в HEX строку
/// </summary>
/// <param name="dp"></param>
/// <param name="size"></param>
/// <param name="str"></param>
/// <param name="str_len"></param>
/// <returns>1 в случае успеха</returns>
int out(
    _In_reads_bytes_(size) const void* dp,
    _In_ size_t size,
    _Outptr_opt_result_bytebuffer_(*str_len) char** str,
    _Out_ size_t* str_len)
{
    const char hex[] = "0123456789ABCDEF";
    unsigned char* b = (unsigned char*)dp;
    char* bp = (char*)malloc(size * 3 + 1);
    char* p = bp;
    if (!p || (!b && !size))
        return 0;
    
    for (size_t i = 0; i < size; ++i)
    {
        *p++ = hex[*b >> 4];
        *p++ = hex[*b & 0xF];
        *p++ = ' ';
        b++;
    }
    *p = '\0';

    *str = bp;
    *str_len = p - bp;
    return 1;
}

/// <summary>
/// 
/// </summary>
/// <param name="Offset">Смещение</param>
/// <param name="Stroka">Служит для разбиения буфера на строки</param>
/// <param name="Text">Символьный буфер</param>
/// <param name="SymCount">Счётчик символов для брейка</param>
/// <param name="HexCount">Счётчик хекс-символов для брейка</param>
/// <param name="len">Длина хекс буфера</param>
/// <param name="NewBuf">Буфер с хексом</param>
/// <param name="FinStr">Итоговая строка</param>
void PrintText(char* Offset, int Stroka, LPCSTR Text, long long SymCount, long long HexCount, size_t len, char* NewBuf, char** FinStr) {
    int Symbol = 0;
    char* Str = (char*)malloc(SUM);
    char* vivod = Str;
    int i;
    for (i = 0; i < 8; i++) {
        *vivod++ = Offset[i];
    }
    *vivod++ = ' ';

    for (i = 0; i < NUMBER_OF_SYMBOLS * NUMBER_OF_SYMBOLS_HEX; i++) {
        *vivod++ = NewBuf[Stroka * 48 + Symbol];
        Symbol++;
        HexCount++;
        if (HexCount == len) {
            for (i; i < NUMBER_OF_SYMBOLS * NUMBER_OF_SYMBOLS_HEX; i++) *vivod++ = ' ';
            break;
        }
    }
    *vivod++ = ' ';

    for (i = 0; i < NUMBER_OF_SYMBOLS * 2; i += 2) {
        if (iswprint(*Text) == 0) {
            *vivod++ = '.';
        }
        else {
            *vivod++ = *Text;
        }
        *vivod++ = ' ';
        Text++;
        SymCount++;
        if (SymCount == BufSize) {
            for (i; i < NUMBER_OF_SYMBOLS * 2; i++) *vivod++ = ' ';
            break;
        }
    }
    *vivod = '\0';
    *FinStr = Str;
    //return Text;
}

void Scroll(int NumLines, int *iVscrollMax, int *iVscrollPos, int cyClient, int cyChar) {//понять какие переменные будут изменяться
    *iVscrollMax = max(0, NumLines + 2 - cyClient / cyChar);
    *iVscrollPos = min(*iVscrollPos, *iVscrollMax);
    SetScrollRange(hWnd, SB_VERT, 0, *iVscrollMax, FALSE);
    SetScrollPos(hWnd, SB_VERT, *iVscrollPos, TRUE);
}






int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_HEX, szWindowClass, MAX_LOADSTRING);
    
    /*size_t len;
    char* str;

    out(wWinMain, 32, &str, &len);*/

    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_HEX));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_HEX);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    RegisterClassExW(&wcex);

    /*wcex.lpfnWndProc = ChildWndProc;
    wcex.cbWndExtra = sizeof(WORD);
    wcex.hIcon = NULL;
    wcex.lpszClassName = szChildClass;
    wcex.hIconSm = NULL;

    RegisterClassExW(&wcex);*/

    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_HEX));
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int) msg.wParam;
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance;

   HWND hWnd = CreateWindowExW(0, szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_VSCROLL,
       CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) 
//{
//    static HWND hwndChild;
//    int cxBlock, cyBlock, x, y;
//    
//    switch (message)
//    {
//
//    case WM_CREATE://WS_EX_COMPOSITED
//        hwndChild = CreateWindowExW(0, szChildClass, NULL, WS_CHILDWINDOW | WS_VISIBLE | WS_VSCROLL | WS_BORDER, 0, 0, 0, 0, hWnd, (HMENU)1, (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE), NULL);
//
//        return 0;
//    case WM_SIZE:
//        cxBlock = LOWORD(lParam);
//        cyBlock = HIWORD(lParam);
//        MoveWindow(hwndChild, 0, 0, cxBlock, cyBlock, TRUE);
//        return 0;
//    case WM_DESTROY:
//        PostQuitMessage(0);
//        return 0;
//    }
//    return DefWindowProc(hWnd, message, wParam, lParam);
//}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    //static LPCSTR Buf;
    //static LONGLONG BufSize;
    static int NumLines = 0;
    static int cxChar, cyChar, cxCaps, cyClient, iVscrollPos, iMaxWidth, iVscrollMax;
    int i = 0, x, y, iPaintBeg, iPaintEnd, iVscrollInc;
    long long SymCount = 0, HexCount = 0;
    RECT Window;
    HDC hdc;
    LARGE_INTEGER FileSize;
    switch (message)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);

        switch (wmId)
        {
        case ID_OPEN:
            if (GetOpenFileNameA(&File)) {
                LoadData(FileName);
                
                NumLines = BufSize + NUMBER_OF_SYMBOLS - 1;
                NumLines /= NUMBER_OF_SYMBOLS;
                iVscrollPos = 0;
                //Scroll(NumLines, &iVscrollMax, &iVscrollPos, cyClient, cyChar);
                iVscrollMax = max(0, NumLines + 2 - cyClient / cyChar);
                iVscrollPos = min(iVscrollPos, iVscrollMax);
                SetScrollRange(hWnd, SB_VERT, 0, iVscrollMax, FALSE);
                SetScrollPos(hWnd, SB_VERT, iVscrollPos, TRUE);

                GetClientRect(hWnd, &Window);
                InvalidateRect(hWnd, &Window, false);
            }
            break;
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    }
    case WM_CREATE:
    {
        hdc = GetDC(hWnd);

        TEXTMETRIC tm;
        GetTextMetrics(hdc, &tm);
        cxChar = tm.tmAveCharWidth;
        cyChar = tm.tmHeight + tm.tmExternalLeading;
        cxCaps = (tm.tmPitchAndFamily & 1 ? 3 : 2) * cxChar / 2;

        ReleaseDC(hWnd, hdc);

        SetOpenFileParams(hWnd);
        break;
    }
    case WM_SIZE:
    {
        cyClient = HIWORD(lParam);

        NumLines = BufSize + NUMBER_OF_SYMBOLS - 1;
        NumLines /= NUMBER_OF_SYMBOLS;
        //Scroll(NumLines, &iVscrollMax, &iVscrollPos, cyClient, cyChar);
        iVscrollMax = max(0, NumLines + 1 - cyClient / cyChar);
        iVscrollPos = min(iVscrollPos, iVscrollMax);
        SetScrollRange(hWnd, SB_VERT, 0, iVscrollMax, FALSE);
        SetScrollPos(hWnd, SB_VERT, iVscrollPos, TRUE);
        break;
    }
    case WM_VSCROLL:
    {
        switch (LOWORD(wParam))
        {
        case SB_TOP:
            iVscrollInc = -iVscrollPos;
            break;
        case SB_BOTTOM:
            iVscrollInc = iVscrollMax - iVscrollPos;
            break;
        case SB_LINEUP:
            iVscrollInc = -1;
            break;
        case SB_LINEDOWN:
            iVscrollInc = 1;
            break;
        case SB_PAGEUP:
            iVscrollInc = min(-1, -cyClient / cyChar);
            break;
        case SB_PAGEDOWN:
            iVscrollInc = max(1, cyClient / cyChar);
            break;
        case SB_THUMBTRACK:
            iVscrollInc = HIWORD(wParam) - iVscrollPos;
            break;
        default:
            iVscrollInc = 0;
        }
        iVscrollInc = max(-iVscrollPos, min(iVscrollInc, iVscrollMax - iVscrollPos)
        );
        if (iVscrollInc != 0)
        {
            iVscrollPos += iVscrollInc;
            ScrollWindow(hWnd, 0, -cyChar * iVscrollInc, NULL, NULL);
            SetScrollPos(hWnd, SB_VERT, iVscrollPos, TRUE);
            UpdateWindow(hWnd);
        }
        return 0;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        LPCSTR Text = Buf + (iVscrollPos * NUMBER_OF_SYMBOLS);
        HDC hdc = BeginPaint(hWnd, &ps);
        char Hex[NUMBER_OF_SYMBOLS_HEX] = { 0 };
        char Offset[NUMBER_OF_SYMBOLS_OFFSET+1] = { 0 };
        static int Number = 0;
        int Stroka = iVscrollPos;
        SymCount = iVscrollPos * NUMBER_OF_SYMBOLS;
        HexCount = iVscrollPos * NUMBER_OF_SYMBOLS * 3;
        GetClientRect(hWnd, &Window);
        SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT/*OEM_FIXED_FONT*/));
        iPaintBeg = max(-1, iVscrollPos + Window.top / cyChar - 1);
        iPaintEnd = min(NumLines, iVscrollPos + Window.bottom / cyChar);

        size_t len;
        char* NewBuf, *FinStr;
        out(Buf, BufSize, &NewBuf, &len);

        if (Buf != NULL)
        {
            for (int g = iPaintBeg; g < iPaintEnd; g++) {
                //выбор файла должен быть через тривью treeview должны отображаться файлы и их содержимое и катологи 
                //библиотека осикс для считывания пути
                y = cyChar * (1 - iVscrollPos + g);
                Number = (g + 1) * NUMBER_OF_SYMBOLS;
                sprintf(Offset, "%08X", Number);

                //PrintText(Offset, Stroka, Text, SymCount, HexCount, len, NewBuf, &FinStr);

                int Symbol = 0;
                char* FinStr = (char*)malloc(SUM);
                char* vivod = FinStr;
                int i;
                for (i = 0; i < 8; i++) {
                    *vivod++ = Offset[i];
                }
                *vivod++ = ' ';

                for (i = 0; i < NUMBER_OF_SYMBOLS * NUMBER_OF_SYMBOLS_HEX; i++) {
                    *vivod++ = NewBuf[Stroka * 48 + Symbol];
                    Symbol++;
                    HexCount++;
                    if (HexCount == len) {
                        for (i; i < NUMBER_OF_SYMBOLS * NUMBER_OF_SYMBOLS_HEX - 1; i++) *vivod++ = ' ';
                        break;
                    }
                }
                *vivod++ = ' ';

                for (i = 0; i < NUMBER_OF_SYMBOLS * 2; i += 2) {
                    if (iswprint(*Text) == 0) {
                        *vivod++ = '.';
                    }
                    else {
                        *vivod++ = *Text;
                    }
                    *vivod++ = ' ';
                    Text++;
                    SymCount++;
                    if (SymCount == BufSize) {
                        for (i; i < NUMBER_OF_SYMBOLS * 2; i++) *vivod++ = ' ';
                        break;
                    }
                }
                *vivod = '\0';
                TextOutA(hdc, 0, y, FinStr, SUM);

                Stroka++;
                if (SymCount == BufSize) {
                    break;
                }
            }
        }
        
        EndPaint(hWnd, &ps);
        break;
    }
    case WM_DESTROY:
        UnmapViewOfFile(Buf);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Обработчик сообщений для окна "О программе".
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}