#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#include "framework.h"
#include "HEX.h"
#include "Addons.h"
#include "commctrl.h"
#include "fileapi.h"
#include "winbase.h"
//#include "resource.h"

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
HINSTANCE       hInst                           = NULL;     //
WCHAR           szTitle[MAX_LOADSTRING]         = { 0 };    //
WCHAR           szWindowClass[MAX_LOADSTRING]   = { 0 };    //
CHAR            FileName[MAX_PATH]              = { 0 };    //путь и название выбранного файла 
//WCHAR szChildClass[] = L"Checker3_Child" ;
//ATOM                MyRegisterClass(HINSTANCE hInstance);
//ATOM                MyRegisterChildClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, INT);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    ChildWndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

WNDPROC fnOldList;

/// <summary>
/// Считывание файла
/// </summary>
/// <param name="FileName">Путь и название файла</param>
/// <param name="Buf">Буфер для текста файла</param>
/// <param name="BufSize">Размер буфера</param>
/// <returns>TRUE в случае успеха и ...</returns>
BOOL LoadData(  _In_    LPCSTR FileName, 
                _Out_ LPCSTR* Buf,
                _Out_ LONGLONG* BufSize) 
{
    //Открытие файла
    HANDLE FileToLoad = CreateFileA(FileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (FileToLoad == INVALID_HANDLE_VALUE) 
    {
        CloseHandle(FileToLoad);
        MessageBoxW(hWnd, L"File couldn't open!", L"ERROR", MB_OK);
        return FALSE;
    }

    //Создание мапы файла
    HANDLE hfilemap = CreateFileMapping(FileToLoad, NULL, PAGE_WRITECOPY, 0, 0, NULL);
    if (hfilemap == NULL) 
    {
        CloseHandle(FileToLoad);
        MessageBoxW(hWnd, L"File couldn't open!", L"ERROR", MB_OK);
        return FALSE;
    }
    //Создание проекции, заполнение буфера, замер размера файла
    PBYTE pbFile = (PBYTE)MapViewOfFile(hfilemap, FILE_MAP_COPY, 0, 0, 0); //проверка на NULL ина извлечение файла
    *Buf = (LPCSTR)pbFile;
    LARGE_INTEGER FileSize;
    GetFileSizeEx(FileToLoad, &FileSize);
    *BufSize = FileSize.QuadPart;

    //Недостаточно прав, GetLastError, GetFormatMassage, файл больше чем буфер, Файл пустой, вывод 

    CloseHandle(hfilemap);
    CloseHandle(FileToLoad);

    return TRUE;
}

VOID SetOpenFileParams(HWND hWnd) {

    ZeroMemory(&File, sizeof(File));

    File.lStructSize        = sizeof(File);
    File.hwndOwner          = hWnd;
    File.lpstrFile          = FileName;
    File.nMaxFile           = sizeof(FileName);
    File.lpstrFilter        = "*.txt";
    File.lpstrFileTitle     = NULL;
    File.nMaxFileTitle      = 0;
    File.lpstrInitialDir    = "C:/Users/vladk/source/repos/HEX";
    File.Flags              = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
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
    _In_reads_bytes_(size) const                VOID*   dp,
    _In_                                        size_t  size,
    _Outptr_opt_result_bytebuffer_(*str_len)    PCHAR*  str,
    _Out_                                       size_t* str_len)
{
    const char hex[]    = "0123456789ABCDEF";           //Заполнение массива???
    unsigned char* b    = (unsigned char*)dp;           //Указатель на буфер
    char* bp            = (char*)malloc(size * 3 + 1);  //Выделение памяти
    char* p             = bp;                           //Указатель на переменую с выделенной для неё памятью

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
    _In_        LPCSTR      Text,
    _In_        LONGLONG    SymCount,
    _In_        LONGLONG    HexCount,
    _In_        size_t      len,
    _In_        PCHAR       NewBuf,
    _Outptr_    PCHAR*      FinStr, 
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

void Scroll(int NumLines, int *iVscrollMax, int *iVscrollPos, int cyClient, int cyChar) 
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

    wcex.cbSize         = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_HEX));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_HEX);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

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
    HACCEL      hAccelTable      = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_HEX));
    MSG         msg;
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
    static LPCSTR       Buf;                //Текстовый буфер
    static LONGLONG     BufSize;            //Размер текстового буфера
    static INT          NumLines = 0,       //Количество строк
                        cxChar,             //Ширина символа
                        cyChar,             //Высота символа
                        cyClient,           //?
                        iVscrollPos,        //Позиция скрола
                        iVscrollMax;        //Максимум скролла

    static HWND         hWndList,           //Описатель TreeView
                        hWndText,           //Описатель строки пути
                        hWndTree;
    static CHAR         szBuffer[MAXPATH+1]; //Путь папки

    /*static BOOL bValidFile;
    static OFSTRUCT ofs;
    static char sReadBuffer[MAXREAD], szFile[MAXPATH];*/

    /*TV_INSERTSTRUCT TV_InsertStruct = { 0 };
    HIMAGELIST himl;
    HBITMAP hBmp;
    int idxBooks;*/

    INT             i           = 0,    //Инкремент
                    y,                  //Координата отрисовки строк
                    iPaintBeg,          //Начальная точка отрисовки
                    iPaintEnd,          //Конечная точка отрисовки
                    iVscrollInc,        //Управление скролом
                    PosX,               //Координаты TreeView и строки пути
                    PosY;               //
    LONGLONG        SymCount    = 0,    //Счётчик символов
                    HexCount    = 0;    //Счётчик хекс символов
    RECT            Window;             //
    HDC             hdc;                //
    LARGE_INTEGER   FileSize;           //

    SCROLLINFO skif;

    switch (message)
    {
    case WM_COMMAND:
    {
        //Вывод пути в строке пути (одно нажатие)
        /*if (LOWORD(wParam) == 1 && HIWORD(wParam) == LBN_SELCHANGE)
        {
            i = SendMessage(hWndList, LB_GETCURSEL, 0, 0);
            i = SendMessageA(hWndList, LB_GETTEXT, i,(LPARAM)szBuffer);
            char* r = getenv(szBuffer);
            strcpy(szBuffer + i + 1, r);
            *(szBuffer + i) = '=';
            SetWindowTextA(hWndText, (LPCSTR)szBuffer);
        }*/

        //Двойное нажатие
        //if (LOWORD(wParam) == 1 && HIWORD(wParam) == LBN_DBLCLK)
        //{
        //    if (LB_ERR == (i = SendMessage(hWndList, LB_GETCURSEL, 0, 0L)))
        //        break;
        //    SendMessageA(hWndList, LB_GETTEXT, i, (LPARAM)szBuffer);
        //    if (-1 != OpenFile(szBuffer, &ofs, OF_EXIST | OF_READ))
        //    {
        //        bValidFile = TRUE;
        //        strcpy(szFile, szBuffer);
        //        getcwd(szBuffer, MAXPATH);
        //        if (szBuffer[strlen(szBuffer) - 1] != '\\')
        //            strcat(szBuffer, "\\");
        //        SetWindowTextA(hWndText, strcat(szBuffer, szFile));
        //    }
        //    else
        //    {
        //        bValidFile = FALSE;
        //        szBuffer[strlen(szBuffer)] = '\0';
        //        //char l = _chdir("C:\\Windows");
        //        chdir(szBuffer + 1);
        //        getcwd(szBuffer, MAXPATH);
        //        SetWindowTextA(hWndText, szBuffer);
        //        SendMessage(hWndList, LB_RESETCONTENT, 0, 0L);
        //        SendMessage(hWndList, LB_DIR, 0x37 | DDL_DIRECTORY, (LONG)"*.*");
        //    }
        //    InvalidateRect(hWnd, NULL, TRUE);
        //}

        INT wmId = LOWORD(wParam);
        switch (wmId)
        {
            //Открытие файла
        case ID_OPEN:
            if (GetOpenFileNameA(&File)) {
                LoadData(FileName, &Buf, &BufSize);
                
                NumLines = BufSize + NUMBER_OF_SYMBOLS - 1; //
                NumLines /= NUMBER_OF_SYMBOLS;              //Подсчёт количества строк
                iVscrollPos = 0;
                //Scroll(NumLines, &iVscrollMax, &iVscrollPos, cyClient, cyChar);
                iVscrollMax = max(0, NumLines + 2 - cyClient / cyChar); //Вычисление позиций для скрола
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
        cxChar  = tm.tmAveCharWidth;                                //Ширина символа 
        cyChar  = tm.tmHeight + tm.tmExternalLeading;               //Высота символа
        //cxCaps  = (tm.tmPitchAndFamily & 1 ? 3 : 2) * cxChar / 2; 

        ReleaseDC(hWnd, hdc);

        hWndTree = CreateWindowEx(0, WC_TREEVIEW, L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER |
            TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT,
            0, 0, Window.right - Window.left, Window.bottom - Window.top,
            hWnd, (HMENU)1, hInst, NULL);

        SetCurrentDirectory(L"C:\\");
        /*WCHAR l[] = L"D:\\";
        HANDLE Attempt = FindFirstVolumeW((LPWSTR)l, sizeof(l)/sizeof(WCHAR));
        if (Attempt == INVALID_HANDLE_VALUE) break;*/
        WIN32_FIND_DATA fwd = {0};
        HANDLE attempt = FindFirstFileW(L"*", &fwd);
        if (attempt == INVALID_HANDLE_VALUE)
            break;
        WCHAR* a = fwd.cFileName;
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
        int nLevel = 1;
        do
        {
            TVINSERTSTRUCT TVS = { 0 };
            TVITEM TVI;
            static HTREEITEM hPrev = (HTREEITEM)TVI_FIRST;
            static HTREEITEM hPrevRootItem = NULL;
            static HTREEITEM hPrevLev2Item = NULL;
            HTREEITEM hti;

            TVI.mask = TVIF_TEXT | TVIF_IMAGE
                | TVIF_SELECTEDIMAGE | TVIF_PARAM;

            TVI.pszText = fwd.cFileName;
            //(LPWSTR)L"aboba";
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
            else if (nLevel == 2)
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
        }         while (FindNextFile(attempt, &fwd));

        //for (int nLevel = 1; nLevel < 3; nLevel++)
        //{
        //    TVINSERTSTRUCT TVS = { 0 };
        //    TVITEM TVI;
        //    static HTREEITEM hPrev = (HTREEITEM)TVI_FIRST;
        //    static HTREEITEM hPrevRootItem = NULL;
        //    static HTREEITEM hPrevLev2Item = NULL;
        //    HTREEITEM hti;

        //    TVI.mask = TVIF_TEXT | TVIF_IMAGE
        //        | TVIF_SELECTEDIMAGE | TVIF_PARAM;

        //    TVI.pszText = fwd.cFileName;
        //        //(LPWSTR)L"aboba";
        //    TVI.cchTextMax = sizeof(TVI.pszText) / sizeof(TVI.pszText[0]);

        //    TVI.iImage = document;
        //    TVI.iSelectedImage = document;

        //    TVI.lParam = (LPARAM)nLevel;
        //    TVS.item = TVI;
        //    TVS.hInsertAfter = hPrev;

        //    if (nLevel == 1)
        //        TVS.hParent = TVI_ROOT;
        //    else if (nLevel == 2)
        //        TVS.hParent = hPrevRootItem;
        //    else
        //        TVS.hParent = hPrevLev2Item;

        //    hPrev = (HTREEITEM)SendMessage(hWndTree, TVM_INSERTITEM,
        //        0, (LPARAM)(LPTVINSERTSTRUCT)&TVS);

        //    if (hPrev == NULL)
        //        return NULL;

        //    if (nLevel == 1)
        //        hPrevRootItem = hPrev;
        //    else if (nLevel == 2)
        //        hPrevLev2Item = hPrev;

        //    if (nLevel > 1)
        //    {
        //        hti = TreeView_GetParent(hWndTree, hPrev);
        //        TVI.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        //        TVI.hItem = hti;
        //        TVI.iImage = closed;
        //        TVI.iSelectedImage = closed;
        //        TreeView_SetItem(hWndTree, &TVI);
        //    }
        //}

        //TreeView_InsertItem(hWndTree, TV_InsertStruct);
        //TV_InsertStruct.hParent = NULL;
        //TV_InsertStruct.hInsertAfter = TVI_ROOT;
        //TV_InsertStruct.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        //TV_InsertStruct.item.pszText = (LPWSTR)L"Родительский";
        //TV_InsertStruct.item.iImage = 0; // не нажата pic
        //TV_InsertStruct.item.iSelectedImage = 1; // нажатое изображение
        //Parent = (HTREEITEM)SendMessage(hWndTree,
        //    TVM_INSERTITEM, 0, (LPARAM)&TV_InsertStruct);

        /*hWndList = CreateWindow(L"listbox", NULL, WS_CHILD | WS_VISIBLE | LBS_STANDARD,
            tm.tmAveCharWidth, tm.tmHeight * 3,
            tm.tmAveCharWidth * 16 +
            GetSystemMetrics(SM_CXVSCROLL),
            tm.tmHeight * 5,
            hWnd, (HMENU)1,
            (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE),
            NULL, );

        char* n = getcwd(szBuffer, MAXPATH);

        hWndText = CreateWindow(L"static", NULL,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            tm.tmAveCharWidth, tm.tmHeight,
            tm.tmAveCharWidth * MAXPATH, tm.tmHeight,
            hWnd, (HMENU)2,
            (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE),
            NULL);


        //for (int i = 0; environ[i]; i++)
        //{
        //    if (strlen(environ[i]) > MAXPATH)
        //        continue;
        //    //strcpy(szBuffer, environ[i]);
        //    *strchr(strcpy(szBuffer, environ[i]), '=') = '\0';
        //    SendMessageA(hWndList, LB_ADDSTRING, 0, (LPARAM)szBuffer);
        //}
        
        //fnOldList = (WNDPROC)SetWindowLong(hWndList, GWL_WNDPROC,(LPARAM)ListProc);

        //SendMessage(hWndList, LB_DIR, 0x37 | DDL_DRIVES, (LPARAM)"*.*");
        SendMessage(hWndList, LB_DIR,DDL_READWRITE | DDL_READONLY | DDL_HIDDEN | DDL_SYSTEM | DDL_DIRECTORY | DDL_DRIVES | DDL_ARCHIVE, (LPARAM)"*.*");*/
        
        /*if (!InitTreeViewImageLists(hWndTree) ||
            !InitTreeViewItems(hWndTree))
        {
            DestroyWindow(hWndTree);
            return FALSE;
        }*/
        FindClose(attempt);
        SetOpenFileParams(hWnd);
        break;
    }
    case WM_SETFOCUS:
    {
        SetFocus(hWndList);
        return 0;
    }
    case WM_SIZE:
    {
        cyClient    = HIWORD(lParam);
        GetClientRect(hWnd, &Window);

        PosX    = Window.right / 4 * 3; //Координаты для TreeView
        PosY    = 50;                   //
        //MoveWindow(hWndList, PosX, PosY, Window.right / 4, Window.bottom, TRUE);
        //MoveWindow(hWndText, PosX, 16, Window.right / 4, 16, TRUE);
        MoveWindow(hWndTree, PosX, PosY, Window.right / 4, Window.bottom, TRUE);

        NumLines    = BufSize + NUMBER_OF_SYMBOLS - 1; //
        NumLines    /= NUMBER_OF_SYMBOLS;              //Подсчёт количества строк
        iVscrollPos = 0;
        //Scroll(NumLines, &iVscrollMax, &iVscrollPos, cyClient, cyChar);
        iVscrollMax = max(0, NumLines + 2 - cyClient / cyChar); //Вычисление позиций для скрола
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
        LPCSTR      Text;                                        //Текстовый буфер
        HDC         hdc                                 = BeginPaint(hWnd, &ps);                    //
        CHAR        Offset[NUMBER_OF_SYMBOLS_OFFSET+1]  = { 0 };                                    //Смещение в 16ой
        INT         Stroka                              = 0,                              //Делит буфер на строки
                    NumStr                              = 0;                                        //Кол-во строк, умещающихся на экране
        static INT  Number                              = 0;                                        //Смещение в 10ой
        size_t      len;                                                                            //Длина хекс буфера
        PCHAR       NewBuf,                                                                         //хекс буфер
                    FinStr;                                                                         //Выводимая на экран строка

        SymCount = iVscrollPos * NUMBER_OF_SYMBOLS;
        HexCount = iVscrollPos * NUMBER_OF_SYMBOLS * 3;
        GetClientRect(hWnd, &Window);

        SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));

        iPaintBeg = max(-1, iVscrollPos + Window.top / cyChar - 1);
        iPaintEnd = min(NumLines, iVscrollPos + Window.bottom / cyChar);

        NumStr = Window.bottom / cyChar * NUMBER_OF_SYMBOLS;
        Text = (LPCSTR)malloc(NumStr + 1);
        LPCSTR s = Text;
        s = Buf + (iVscrollPos * NUMBER_OF_SYMBOLS);

        
        //Text[NewStr] = Buf + (iVscrollPos * NUMBER_OF_SYMBOLS);
        //считывание буфера размером с экран
        if (Buf != NULL)
        {
            Out(s, NumStr, &NewBuf, &len);
            len = BufSize * 3;
            for (int g = iPaintBeg; g < iPaintEnd - 1; g++) {
                //выбор файла должен быть через тривью treeview должны отображаться файлы и их содержимое и катологи 
                //библиотека осикс для считывания пути
                y = cyChar * (1 - iVscrollPos + g);
                Number = (g + 1) * NUMBER_OF_SYMBOLS;
                sprintf(Offset, "%08X", Number);

                PrintText(Offset, Stroka, s, SymCount, HexCount, len, NewBuf, &FinStr, BufSize);
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