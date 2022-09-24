#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#include "framework.h"
#include "HEX.h"
#include "Addons.h"
#include "commctrl.h"
#include "fileapi.h"
#include "winbase.h"
//#include "resource.h"

struct BufferParams
{
    UINT64  SizeBuffer      = 0;
    UINT64  NumLines        = 0;

    UINT64  BottomOffset    = 0;
    UINT64  TopOffset       = 0;
    UINT64  CurrentOffset   = 0;
    
    UINT    Granularity     = 0;
    
    HANDLE  hFileMap        = NULL;
    PBYTE   pbBuffer        = NULL;
    UINT64  Increment       = 0;
};

#define MAX_LOADSTRING              100     //
#define NUMBER_OF_SYMBOLS           16      //количество символов выводимых в строку
#define NUMBER_OF_SYMBOLS_HEX       3       //количество символов для преобразования в хекс
#define NUMBER_OF_SYMBOLS_OFFSET    8       //количество символов под смещение
//#define MAXENV                      4096    //длина пути
#define MAXPATH 256 
#define MAXREAD 8192 

//длина строки
#define SUM     NUMBER_OF_SYMBOLS_OFFSET + 1 + \
                NUMBER_OF_SYMBOLS * NUMBER_OF_SYMBOLS_HEX + \
                NUMBER_OF_SYMBOLS * 2    //длина строки

OPENFILENAMEA   File;                                       //
HINSTANCE       hInst = NULL;     //
WCHAR           szTitle[MAX_LOADSTRING] = { 0 };    //
WCHAR           szWindowClass[MAX_LOADSTRING] = { 0 };    //
CHAR            FileName[MAX_PATH] = { 0 };    //путь и название выбранного файла 
//WCHAR szChildClass[] = L"Checker3_Child" ;
//ATOM                MyRegisterClass(HINSTANCE hInstance);
//ATOM                MyRegisterChildClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, INT);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    ChildWndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
VOID FileInfo(HANDLE hFileToLoad, BufferParams* BufData);

WNDPROC fnOldList;

/// <summary>
/// Считывание файла
/// </summary>
/// <param name="FileName">Путь и название файла</param>
/// <param name="Buf">Буфер для текста файла</param>
/// <param name="BufSize">Размер буфера</param>
/// <returns>TRUE в случае успеха и ...</returns>
BOOL LoadData(_In_    LPCSTR FileName,
    _Out_ BufferParams* BufData)
{
    //Открытие файла
    HANDLE hFileToLoad = CreateFileA(FileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFileToLoad == INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFileToLoad);
        MessageBoxW(hWnd, L"File couldn't open!", L"ERROR", MB_OK);
        return FALSE;
    }

    //Создание мапы файла
    BufData->hFileMap = CreateFileMapping(hFileToLoad, NULL, PAGE_WRITECOPY, 0, 0, NULL);
    if (BufData->hFileMap == NULL)
    {
        CloseHandle(hFileToLoad);
        MessageBoxW(hWnd, L"File couldn't open!", L"ERROR", MB_OK);
        return FALSE;
    }
    //Создание проекции, заполнение буфера, замер размера файла
    
    FileInfo(hFileToLoad, BufData);
    //Недостаточно прав, GetLastError, GetFormatMassage, файл больше чем буфер, Файл пустой, вывод 

    CloseHandle(hFileToLoad);

    return TRUE;
}

VOID FileInfo(HANDLE hFileToLoad, BufferParams* BufData) 
{
    SYSTEM_INFO synf;
    DWORD dwFileSizeHigh = 0;
    INT64 qwFileOffset = 0;
    ULONG BytesInBlock = 0;
    
    ZeroMemory(&synf, sizeof(SYSTEM_INFO));

    GetSystemInfo(&synf);
    
    BufData->Granularity    =   synf.dwAllocationGranularity;
    BytesInBlock = BufData->Granularity * 2;

    BufData->BottomOffset   =   BufData->Granularity;

    BufData->SizeBuffer     =   GetFileSize(hFileToLoad, &dwFileSizeHigh);
    BufData->SizeBuffer     +=  (((UINT64)dwFileSizeHigh) << 32);

    BufData->NumLines       =   BufData->SizeBuffer + NUMBER_OF_SYMBOLS - 1;
    BufData->NumLines       /=  NUMBER_OF_SYMBOLS;

    if (BufData->SizeBuffer < BytesInBlock) 
    {
        BytesInBlock = BufData->SizeBuffer;
    }

    BufData->pbBuffer = (PBYTE)MapViewOfFile(
        BufData->hFileMap, 
        FILE_MAP_READ, 
        (DWORD)(qwFileOffset >> 32), 
        (DWORD)(qwFileOffset & 0xFFFFFFFF), 
        BytesInBlock
    ); 
    //*Buf = (LPCSTR)pbFile;
}

VOID SetOpenFileParams(HWND hWnd) {

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

VOID MoveToMap(BufferParams* FileBuf, ULONGLONG ullFileOffSet, INT iVscrollPos)
{
    ULONG ullBytesInBlock = FileBuf->Granularity * 2;

    if (ullFileOffSet % FileBuf->Granularity != 0)
    {
        ullFileOffSet -= ullFileOffSet % FileBuf->Granularity;
    }

    FileBuf->CurrentOffset = ullFileOffSet;

    if (iVscrollPos > FileBuf->BottomOffset / NUMBER_OF_SYMBOLS ||  iVscrollPos < FileBuf->TopOffset / NUMBER_OF_SYMBOLS)
    {
        if (FileBuf->pbBuffer != NULL)
        {
            UnmapViewOfFile(FileBuf->pbBuffer);
            FileBuf->pbBuffer = NULL;
        }

        if (ullFileOffSet + ullBytesInBlock > FileBuf->SizeBuffer)
        {
            ullBytesInBlock = FileBuf->SizeBuffer - ullFileOffSet;
        }

        FileBuf->pbBuffer = (PBYTE)MapViewOfFile(
            FileBuf->hFileMap,
            FILE_MAP_READ,
            (DWORD)(ullFileOffSet >> 32),
            (DWORD)(ullFileOffSet & 0xFFFFFFFF),
            ullBytesInBlock
        );

        FileBuf->TopOffset = FileBuf->CurrentOffset;

        FileBuf->BottomOffset = FileBuf->CurrentOffset + FileBuf->Granularity;
    }

    if (FileBuf->pbBuffer == NULL)
    {
        CloseHandle(FileBuf->hFileMap);
        FileBuf->hFileMap = NULL;
    }
}


/// <summary>
/// Преобразовать буфер в HEX строку
/// </summary>
/// <param name="dp">Буфер</param>
/// <param name="size">Размер буфера</param>
/// <param name="str">Преобразованный буфер</param>
/// <param name="str_len">Длина нового буфера</param>
/// <returns>1 в случае успеха</returns>
INT Out(
    _In_reads_bytes_(size) const                VOID* dp,
    _In_                                        size_t  size,
    _Outptr_opt_result_bytebuffer_(*str_len)    PCHAR* str,
    _Out_                                       size_t* str_len)
{
    const char hex[] = "0123456789ABCDEF";           //Заполнение массива???
    unsigned char* b = (unsigned char*)dp;           //Указатель на буфер
    char* bp = (char*)malloc(size * 3 + 1);  //Выделение памяти
    char* p = bp;                           //Указатель на переменую с выделенной для неё памятью

    if (!p || (!b && !size))
        return 0;
    //Разделение символа на старший и младший биты для преобразования в хекс
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
/// Заполение строки и вывод её на экран 
/// </summary>
/// <param name="Offset">Смещение</param>
/// <param name="Stroka">Служит для разбиения буфера на строки</param>
/// <param name="Text">Символьный буфер</param>
/// <param name="SymCount">Счётчик символов для брейка</param>
/// <param name="HexCount">Счётчик хекс-символов для брейка</param>
/// <param name="len">Длина хекс буфера</param>
/// <param name="NewBuf">Буфер с хексом</param>
/// <param name="FinStr">Итоговая строка</param>
VOID PrintText(
    _In_        PCHAR       Offset,
    _In_        INT         Stroka,
    _In_        PBYTE       Text,
    _In_        LONGLONG    SymCount,
    _In_        LONGLONG    HexCount,
    _In_        size_t      len,
    _In_        PCHAR       NewBuf,
    _Outptr_    PCHAR* FinStr,
    _In_        LONGLONG    BufSize)
{
    INT     Symbol = 0;                 //Номер символа для пошагового перемещения по буферу хекса
    PCHAR   Str = (PCHAR)malloc(SUM);   //Выделение памяти для строки
    PCHAR   Vivod = Str;                //Указатель на выделенную память
    INT     i;                          //Инкремент

    //Заполнение строки смещением
    for (i = 0; i < 8; i++)
    {
        *Vivod++ = Offset[i];
    }
    *Vivod++ = ' ';

    //Заполнение строки хексом
    for (i = 0; i < NUMBER_OF_SYMBOLS * NUMBER_OF_SYMBOLS_HEX; i++)
    {
        *Vivod++ = NewBuf[Stroka * 48 + Symbol];
        Symbol++;
        HexCount++;

        //Заполнение пустых мест строки пробелами
        if (HexCount == len)
        {
            for (i; i < NUMBER_OF_SYMBOLS * NUMBER_OF_SYMBOLS_HEX; i++) *Vivod++ = ' ';
            break;
        }
    }
    *Vivod++ = ' ';

    //Заполнение строки текстом файла
    for (i = 0; i < NUMBER_OF_SYMBOLS * 2; i += 2)
    {
        if (iswprint(*Text) == 0)
        {
            *Vivod++ = '.';
        }
        else
        {
            *Vivod++ = *Text;
        }
        *Vivod++ = ' ';
        Text++;
        SymCount++;

        //Заполение пустых мест строки пробелами
        if (SymCount == BufSize)
        {
            for (i; i < NUMBER_OF_SYMBOLS * 2; i++) *Vivod++ = ' ';
            break;
        }
    }
    *Vivod = '\0';
    *FinStr = Str;
}

void Scroll(int NumLines, int* iVscrollMax, int* iVscrollPos, int cyClient, int cyChar)
{
    //понять какие переменные будут изменяться
    *iVscrollMax = max(0, NumLines + 2 - cyClient / cyChar);
    *iVscrollPos = min(*iVscrollPos, *iVscrollMax);
    SetScrollRange(hWnd, SB_VERT, 0, *iVscrollMax, FALSE);
    SetScrollPos(hWnd, SB_VERT, *iVscrollPos, TRUE);
}




int APIENTRY wWinMain(
    _In_        HINSTANCE hInstance,
    _In_opt_    HINSTANCE hPrevInstance,
    _In_        LPWSTR    lpCmdLine,
    _In_        INT       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_HEX, szWindowClass, MAX_LOADSTRING);
    //InitCommonControls();
    WNDCLASSEXW wcex; //класс окна

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

    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }
    HACCEL      hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_HEX));
    MSG         msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int)msg.wParam;
}

BOOL InitInstance(HINSTANCE hInstance, INT nCmdShow)
{
    hInst = hInstance;

    HWND hWnd = CreateWindowExW(0, szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_CLIPCHILDREN,
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

LRESULT CALLBACK WndProc(
    _In_    HWND hWnd,
    _In_    UINT message,
    _In_    WPARAM wParam,
    _In_    LPARAM lParam)
{
    //static LPCSTR       Buf;                //Текстовый буфер
    //static LONGLONG     BufSize;            //Размер текстового буфера
    static INT          //NumLines = 0,       //Количество строк
        cxChar,             //Ширина символа
        cyChar,             //Высота символа
        cyClient,           //?
        iVscrollPos,        //Позиция скрола
        iVscrollMax;        //Максимум скролла

    static HWND         hWndList,           //Описатель TreeView
        hWndText,           //Описатель строки пути
        hWndTree;
    static CHAR         szBuffer[MAXPATH + 1]; //Путь папки

    /*static BOOL bValidFile;
    static OFSTRUCT ofs;
    static char sReadBuffer[MAXREAD], szFile[MAXPATH];*/

    /*TV_INSERTSTRUCT TV_InsertStruct = { 0 };
    HIMAGELIST himl;
    HBITMAP hBmp;
    int idxBooks;*/

    INT             i = 0,    //Инкремент
        y,                  //Координата отрисовки строк
        iPaintBeg,          //Начальная точка отрисовки
        iPaintEnd,          //Конечная точка отрисовки
        iVscrollInc,        //Управление скролом
        PosX,               //Координаты TreeView и строки пути
        PosY;               //
    LONGLONG        SymCount = 0,    //Счётчик символов
        HexCount = 0;    //Счётчик хекс символов
    RECT            Window;             //
    HDC             hdc;                //
    LARGE_INTEGER   FileSize;           //
    static BufferParams BufData;
    SCROLLINFO skif;

    switch (message)
    {
    case WM_COMMAND:
    {
        INT wmId = LOWORD(wParam);
        switch (wmId)
        {
            //Открытие файла
        case ID_OPEN:
            if (GetOpenFileNameA(&File)) {
                LoadData(FileName, &BufData);
                
                iVscrollPos = 0;
                //Scroll(NumLines, &iVscrollMax, &iVscrollPos, cyClient, cyChar);
                iVscrollMax = max(0, BufData.NumLines + 2 - cyClient / cyChar); //Вычисление позиций для скрола
                iVscrollPos = min(iVscrollPos, iVscrollMax);            //Вычисление позиции скрола
                SetScrollRange(hWnd, SB_VERT, 0, iVscrollMax, FALSE);   //Вычисление позиций для скрола 
                SetScrollPos(hWnd, SB_VERT, iVscrollPos, TRUE);         //?

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
        GetClientRect(hWnd, &Window);

        TEXTMETRIC  tm;
        GetTextMetrics(hdc, &tm);
        cxChar = tm.tmAveCharWidth;                                //Ширина символа 
        cyChar = tm.tmHeight + tm.tmExternalLeading;               //Высота символа
        //cxCaps  = (tm.tmPitchAndFamily & 1 ? 3 : 2) * cxChar / 2; 

        ReleaseDC(hWnd, hdc);

        hWndTree = CreateWindowEx(0, WC_TREEVIEW, L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER |
            TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT,
            0, 0, Window.right - Window.left, Window.bottom - Window.top,
            hWnd, (HMENU)1, hInst, NULL);

        HIMAGELIST himl;
        HBITMAP hbmp;
        int open, closed, document;
        
        himl = ImageList_Create(16, 16, FALSE, 3, 0);
        hbmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP1));
        open = ImageList_Add(himl, hbmp, NULL);
        DeleteObject(hbmp);

        hbmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP1));
        closed = ImageList_Add(himl, hbmp, NULL);
        DeleteObject(hbmp);

        hbmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP1));
        document = ImageList_Add(himl, hbmp, NULL);
        DeleteObject(hbmp);

        TreeView_SetImageList(hWndTree, himl, TVSIL_NORMAL);



        LPCWSTR path = L"C:\\";
        SetCurrentDirectory(path);
        WIN32_FIND_DATA Parent = { 0 };
        HANDLE attempt = FindFirstFileW(L"*", &Parent);
        if (attempt == INVALID_HANDLE_VALUE)
            break;

        int nLevel = 1, Level = 0;
        WIN32_FIND_DATA parent = { 0 };
        HANDLE Attempt = { 0 };
        do{
            //LPWSTR NewPath = (LPWSTR)calloc(260 + 1, sizeof(WCHAR));
            ////GetCurrentDirectory(260, NewPath);
            //wcscat_s(NewPath, 260, path);// C:
            //wcscat_s(NewPath, 260, L"\\");
            //wcscat_s(NewPath, 260, Parent.cFileName);// Parent = C:\\file
            //if (nLevel == 1)
            //{
            //    if (wcscmp(Parent.cFileName, L".") || wcscmp(Parent.cFileName, L".."))
            //    {
            //        if (wcscmp(Parent.cFileName, L".") && wcscmp(Parent.cFileName, L".."))
            //        {
            //            //Level++;
            //        }   
            //        else
            //        {
            //            FindNextFile(attempt, &Parent);
            //            FindNextFile(attempt, &Parent);
            //        }
            //    }
            //    
            //}
            //else
            //{
            //    SetCurrentDirectory(NewPath);// C:\\file
            //    Attempt = FindFirstFileW(L"*", &parent);// parent = C:\\file\\file
            //    if (wcscmp(parent.cFileName, L".") || wcscmp(parent.cFileName, L".."))
            //    {
            //        if (wcscmp(parent.cFileName, L".") && wcscmp(parent.cFileName, L".."))
            //        {//файл
            //        }
            //        else
            //        {//не файл
            //            FindNextFile(Attempt, &parent);
            //            FindNextFile(Attempt, &parent);
            //        }
            //    }
            //}
            //TVINSERTSTRUCT TVS = { 0 };
            //static HTREEITEM hPrev = (HTREEITEM)TVI_FIRST;
            //static HTREEITEM hPrevRootItem = NULL;
            //static HTREEITEM hPrevLev2Item = NULL;
            //HTREEITEM hti;
            //TVS.item.mask = TVIF_TEXT | TVIF_IMAGE| TVIF_SELECTEDIMAGE | TVIF_PARAM;
            //TVS.item.pszText = Parent.cFileName;
            //TVS.item.cchTextMax = sizeof(TVS.item.pszText) / sizeof(TVS.item.pszText[0]);
            //TVS.item.lParam = (LPARAM)nLevel;
            //TVS.hInsertAfter = hPrev;
            //if (nLevel == 1)
            //    TVS.hParent = TVI_ROOT;
            //else if (nLevel == 2)
            //    TVS.hParent = hPrevRootItem;
            //else
            //    TVS.hParent = hPrevLev2Item;
            //hPrev = (HTREEITEM)SendMessage(hWndTree, TVM_INSERTITEM,
            //    0, (LPARAM)(LPTVINSERTSTRUCT)&TVS);
            //if (hPrev == NULL)
            //    return NULL;
            //if (nLevel == 1)
            //    hPrevRootItem = hPrev;
            //else if (nLevel > 1)
            //    hPrevLev2Item = hPrev;
            //nLevel++;
            LPWSTR Path = (LPWSTR)calloc(260 + 1, sizeof(WCHAR));
            wcscat_s(Path, 260, path);// C:
            wcscat_s(Path, 260, L"\\");
            wcscat_s(Path, 260, Parent.cFileName);// Parent = C:\\file
            SetCurrentDirectoryW(Path);
            for (nLevel = 1; nLevel < 7; nLevel++)
            {
                
                TVINSERTSTRUCT TVS = { 0 };
                static HTREEITEM hPrev = (HTREEITEM)TVI_FIRST;
                static HTREEITEM hPrevRootItem = NULL;
                static HTREEITEM hPrevLev2Item = NULL;
                HTREEITEM hti;

                TVS.item.mask = TVIF_TEXT | TVIF_IMAGE| TVIF_SELECTEDIMAGE | TVIF_PARAM;
                TVS.item.pszText = Parent.cFileName;
                TVS.item.cchTextMax = sizeof(TVS.item.pszText) / sizeof(TVS.item.pszText[0]);
                TVS.item.lParam = (LPARAM)nLevel;
                TVS.hInsertAfter = hPrev;

                if (nLevel == 1)
                    TVS.hParent = TVI_ROOT;
                else if (nLevel == 2)
                    TVS.hParent = hPrevRootItem;
                else
                    TVS.hParent = hPrevLev2Item;

                hPrev = (HTREEITEM)SendMessage(hWndTree, TVM_INSERTITEM,
                    0, (LPARAM)(LPTVINSERTSTRUCT)&TVS);

                if (hPrev == NULL)
                    return NULL;
                if (nLevel == 1)
                    hPrevRootItem = hPrev;
                else if (nLevel > 1)
                    hPrevLev2Item = hPrev;

                LPWSTR NewPath = (LPWSTR)calloc(260 + 1, sizeof(WCHAR));

                if (nLevel == 1) {
                    wcscat_s(NewPath, 260, path);// C:
                    wcscat_s(NewPath, 260, L"\\");
                    wcscat_s(NewPath, 260, Parent.cFileName);// Parent = C:\\file
                    int a = SetCurrentDirectoryW(NewPath);
                    if (a == 0) { break; }
                    Attempt = FindFirstFileW(L"*", &parent);// parent = C:\\file\\file
                    if (Attempt == INVALID_HANDLE_VALUE)
                        break;
                }
                else 
                {
                    GetCurrentDirectory(260, NewPath);
                    //wcscat_s(NewPath, 260, path);// C:
                    wcscat_s(NewPath, 260, L"\\");
                    wcscat_s(NewPath, 260, Parent.cFileName);// Parent = C:\\file
                    LPWSTR V = (LPWSTR)calloc(260 + 1, sizeof(WCHAR));
                    wcscat_s(V, 260, Parent.cFileName);
                    LPWSTR B = (LPWSTR)calloc(260 + 1, sizeof(WCHAR));
                    wcscat_s(B, 260, Parent.cFileName);
                    int a = SetCurrentDirectoryW(NewPath);
                    if (a == 0)
                    {
                        nLevel -= 1;
                        int l = FindNextFile(Attempt, &parent);
                        if (l == 0)
                        { 
                            break;
                        }
                        Parent = parent;
                        continue;
                    }
                    Attempt = FindFirstFileW(L"*", &parent);// parent = C:\\file\\file
                }
                if (wcscmp(parent.cFileName, L".") || wcscmp(parent.cFileName, L".."))
                {
                    if (wcscmp(parent.cFileName, L".") && wcscmp(parent.cFileName, L".."))
                    {
                        if (nLevel > 1)
                            Parent = parent;
                    }
                    else
                    {
                        FindNextFile(Attempt, &parent);
                        FindNextFile(Attempt, &parent);
                        if (!(wcscmp(parent.cFileName, L".") && wcscmp(parent.cFileName, L"..")))
                        {
                            break;
                        }
                            Parent = parent;
                    }
                }
            }
        } while (FindNextFile(attempt, &Parent));

    FindClose(attempt);
    SetOpenFileParams(hWnd);
    break;

        /*do
        {
            LPWSTR NewPath = (LPWSTR)calloc(260 + 1, sizeof(WCHAR));
            if (nLevel == 1) { wcscat_s(NewPath, 260, path); }
            wcscat_s(NewPath, 260, L"\\");
            wcscat_s(NewPath, 260, Parent.cFileName);

            TVINSERTSTRUCT TVS = { 0 };
            TVITEM TVI;
            static HTREEITEM hPrev = (HTREEITEM)TVI_FIRST;
            static HTREEITEM hPrevRootItem = NULL;
            static HTREEITEM hPrevLev2Item = NULL;
            HTREEITEM hti;

            TVI.mask = TVIF_TEXT | TVIF_IMAGE
                | TVIF_SELECTEDIMAGE | TVIF_PARAM;

            TVI.pszText = Parent.cFileName;
            TVI.cchTextMax = sizeof(TVI.pszText) / sizeof(TVI.pszText[0]);

            TVI.iImage = document;
            TVI.iSelectedImage = document;

            TVI.lParam = (LPARAM)nLevel;
            TVS.item = TVI;
            TVS.hInsertAfter = hPrev;

            if (nLevel == 1)
                TVS.hParent = TVI_ROOT;
            else if (nLevel == 2)
                TVS.hParent = hPrevRootItem;
            else
                TVS.hParent = hPrevLev2Item;

            hPrev = (HTREEITEM)SendMessage(hWndTree, TVM_INSERTITEM,
                0, (LPARAM)(LPTVINSERTSTRUCT)&TVS);

            if (hPrev == NULL)
                return NULL;

            if (nLevel == 1)
                hPrevRootItem = hPrev;
            else if (nLevel > 1)
                hPrevLev2Item = hPrev;

            if (nLevel > 1)
            {
                hti = TreeView_GetParent(hWndTree, hPrev);
                TVI.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                TVI.hItem = hti;
                TVI.iImage = closed;
                TVI.iSelectedImage = closed;
                TreeView_SetItem(hWndTree, &TVI);
            }
        } while (FindNextFile(attempt, &Parent));

        FindClose(attempt);
        SetOpenFileParams(hWnd);
        break;*/
    }
    case WM_SETFOCUS:
    {
        SetFocus(hWndList);
        return 0;
    }
    case WM_SIZE:
    {
        cyClient = HIWORD(lParam);
        GetClientRect(hWnd, &Window);

        PosX = Window.right / 4 * 3; //Координаты для TreeView
        PosY = 50;                   //
        //MoveWindow(hWndList, PosX, PosY, Window.right / 4, Window.bottom, TRUE);
        //MoveWindow(hWndText, PosX, 16, Window.right / 4, 16, TRUE);
        MoveWindow(hWndTree, PosX, PosY, Window.right / 4, Window.bottom - 50, TRUE);

        iVscrollPos = 0;
        //Scroll(NumLines, &iVscrollMax, &iVscrollPos, cyClient, cyChar);
        iVscrollMax = max(0, BufData.NumLines + 2 - cyClient / cyChar); //Вычисление позиций для скрола
        iVscrollPos = min(iVscrollPos, iVscrollMax);            //Вычисление позиции скрола
        SetScrollRange(hWnd, SB_VERT, 0, iVscrollMax, FALSE);   //Вычисление позиций для скрола 
        SetScrollPos(hWnd, SB_VERT, iVscrollPos, TRUE);         //?
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

            skif.cbSize = sizeof(SCROLLINFO);
            skif.fMask = SIF_TRACKPOS;
            GetScrollInfo(hWnd, SB_VERT, &skif);
            iVscrollInc = skif.nTrackPos - iVscrollPos;
            
            break;
        default:
            iVscrollInc = 0;
        }

        iVscrollInc = max(-iVscrollPos, min(iVscrollInc, iVscrollMax - iVscrollPos));
        
        if (iVscrollInc != 0)
        {
            iVscrollPos += iVscrollInc;
            BufData.Increment = (UINT64)iVscrollPos * NUMBER_OF_SYMBOLS;
            ScrollWindow(hWnd, 0, -cyChar * iVscrollInc, NULL, NULL);
            SetScrollPos(hWnd, SB_VERT, iVscrollPos, TRUE);
            UpdateWindow(hWnd);
        }
        if (BufData.SizeBuffer > (UINT64)BufData.Granularity * 2)
        {
            MoveToMap(&BufData, BufData.Increment, iVscrollPos);
        }
        return 0;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        PBYTE      Text;                                        //Текстовый буфер
        HDC         hdc = BeginPaint(hWnd, &ps);                    //
        CHAR        Offset[NUMBER_OF_SYMBOLS_OFFSET + 1] = { 0 };                                    //Смещение в 16ой
        INT         Stroka = 0,                              //Делит буфер на строки
            NumStr = 0;                                        //Кол-во строк, умещающихся на экране
        static INT  Number = 0;                                        //Смещение в 10ой
        size_t      len;                                                                            //Длина хекс буфера
        PCHAR       NewBuf,                                                                         //хекс буфер
            FinStr;                                                                         //Выводимая на экран строка

        SymCount = iVscrollPos * NUMBER_OF_SYMBOLS;
        HexCount = iVscrollPos * NUMBER_OF_SYMBOLS * 3;
        GetClientRect(hWnd, &Window);

        SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));

        iPaintBeg = max(-1, iVscrollPos + Window.top / cyChar - 1);
        iPaintEnd = min(BufData.NumLines, iVscrollPos + Window.bottom / cyChar);

        NumStr = Window.bottom / cyChar * NUMBER_OF_SYMBOLS;
        Text = (PBYTE)malloc(NumStr + 1);
        PBYTE s = Text;
        s = BufData.pbBuffer + (iVscrollPos * NUMBER_OF_SYMBOLS);


        //Text[NewStr] = Buf + (iVscrollPos * NUMBER_OF_SYMBOLS);
        //считывание буфера размером с экран
        if (BufData.pbBuffer != NULL)
        {
            Out(s, NumStr, &NewBuf, &len);
            len = BufData.SizeBuffer * 3;
            for (int g = iPaintBeg; g < iPaintEnd - 1; g++) {
                //выбор файла должен быть через тривью treeview должны отображаться файлы и их содержимое и катологи 
                //библиотека осикс для считывания пути
                y = cyChar * (1 - iVscrollPos + g);
                Number = (g + 1) * NUMBER_OF_SYMBOLS;
                sprintf(Offset, "%08X", Number);

                PrintText(Offset, Stroka, s, SymCount, HexCount, len, NewBuf, &FinStr, BufData.SizeBuffer);
                HexCount += NUMBER_OF_SYMBOLS * 3;
                SymCount += NUMBER_OF_SYMBOLS;
                /*int Symbol = 0;
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
                *vivod = '\0';*/
                TextOutA(hdc, 0, y, FinStr, SUM);
                s += NUMBER_OF_SYMBOLS;
                Stroka++;
                if (SymCount == BufData.SizeBuffer) {
                    break;
                }
            }
        }
        
        EndPaint(hWnd, &ps);
        break;
        /*
        PBYTE Text = BufData.pbBuffer;
        iPaintBeg = max(-1, iVscrollPos + Window.top / cyChar - 1);
        iPaintEnd = min(BufData.NumLines, iVscrollPos + Window.bottom / cyChar);
        for (int iteration = paintBegin; i < paintEnd; i++)
        {
            PaintLine(PBYTE Text, UINT64 Increment) - какая-то функция вывода строки
            PaintLine(Text, BufData.Increment);
            // реализация этой функции
                
                for (int i = 0; i < NUMBER_OF_SYMBOLS; i++)
                {
                    if (Increment >= BufData.SizeBuffer)
                    {
                        break;
                    }
                    USHORT iSymbol = (CHAR)Text[Increment - BufData.CurrentOffset]; 
                    INT size = printf(bufer, "%02X", iSymbol);
                    TextOutA(hdc,x,y,bufer,size); // 16ричка
                    TextOutA(hdc,x,y,iSymbol,1); //вывод символа
                    Increment++;
                }
            // реалищация закончилась
            BufData.Increment += NUMBER_OF_SYMBOLS;
        }


        */
    }
    case WM_DESTROY:
        UnmapViewOfFile(BufData.pbBuffer);
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