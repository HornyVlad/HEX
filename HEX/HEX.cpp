#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#include "framework.h"

INT APIENTRY wWinMain(
    _In_        HINSTANCE hInstance,
    _In_opt_    HINSTANCE hPrevInstance,
    _In_        LPWSTR    lpCmdLine,
    _In_        INT       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_HEX, szWindowClass, MAX_LOADSTRING);
    WNDCLASSEXW wcex; //класс окна
    /*WCHAR bif[100];
    SHORT count = GetLogicalDriveStringsW(100, bif);*/

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

    wcex.lpfnWndProc    = ChildWndProc;
    wcex.cbWndExtra     = sizeof(WORD);
    wcex.hIcon          = NULL;
    wcex.lpszClassName  = szChildClass;
    wcex.hIconSm        = NULL;

    RegisterClassExW(&wcex);

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
    return (INT)msg.wParam;
}

BOOL InitInstance(HINSTANCE hInstance, INT nCmdShow)
{
    hInst = hInstance;

    HWND hWnd = CreateWindowExW(0, szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

LRESULT CALLBACK WndProc(
    _In_    HWND hWnd,
    _In_    UINT message,
    _In_    WPARAM wParam,
    _In_    LPARAM lParam)
{
    //static LPCSTR       Buf;                //Текстовый буфер
    //static LONGLONG     BufSize;            //Размер текстового буфера
    static INT          iVscrollPos,    //Позиция скрола
                        iVscrollMax;    //Максимум скролла

    static HWND         hWndChild,      //
                        hWndTree;
    static BufferParams BufData;

    INT                 i = 0,          //Инкремент
                        y = 0;          //Координата отрисовки строк
    LONGLONG            SymCount = 0,   //Счётчик символов
                        HexCount = 0;   //Счётчик хекс символов
    RECT                Window;         //

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

                SendMessage(hWndChild, WM_COMMAND, wParam, lParam);
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
    case WM_NOTIFY:
    {
        LPNMHDR lpnmh = (LPNMHDR)lParam;
        if (lpnmh->idFrom == 1)
            switch (lpnmh->code)
            {
            case NM_CLICK:
                TVITEM tvItem;

                //tvItem.mask = TVIF_HANDLE | TVIF_STATE;
                //tvItem.hItem = (HTREEITEM)hti;
                //tvItem.stateMask = TVIS_STATEIMAGEMASK;

                //// Request the information.
                //TreeView_GetItem(hWndTree, &tvItem);
                //printf("");
            }
        break;
    }
    case WM_CREATE:
    {
        GetClientRect(hWnd, &Window);

        INT cxChar, cyChar;

        Metric(hWnd, &cxChar, &cyChar);

        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG)&BufData);

        hWndChild = CreateWindowEx(WS_EX_COMPOSITED, szChildClass, NULL, WS_CHILDWINDOW | WS_BORDER | WS_VISIBLE, 0, 0, 0, 0, hWnd, (HMENU)0, hInst, NULL);

        hWndTree = CreateWindowEx(0, WC_TREEVIEW, L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER |
            TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT,
            0, 0, 0, 0,
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
        do {
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
                static HTREEITEM hPrevLev3Item = NULL;
                HTREEITEM hti;

                TVS.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
                TVS.item.pszText = Parent.cFileName;
                TVS.item.cchTextMax = sizeof(TVS.item.pszText) / sizeof(TVS.item.pszText[0]);
                TVS.item.lParam = (LPARAM)nLevel;
                TVS.hInsertAfter = hPrev;

                if (nLevel == 1)
                    TVS.hParent = TVI_ROOT;
                else if (nLevel == 2)
                    TVS.hParent = hPrevRootItem;
                else if(nLevel == 3)
                    TVS.hParent = hPrevLev2Item;
                else
                    TVS.hParent = hPrevLev3Item;

                hPrev = (HTREEITEM)SendMessage(hWndTree, TVM_INSERTITEM,
                    0, (LPARAM)(LPTVINSERTSTRUCT)&TVS);

                if (hPrev == NULL)
                    return NULL;
                if (nLevel == 1)
                    hPrevRootItem = hPrev;
                else if (nLevel == 2)
                    hPrevLev2Item = hPrev;
                else 
                    hPrevLev3Item = hPrev;

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
                    wcscat_s(NewPath, 260, L"\\");
                    wcscat_s(NewPath, 260, Parent.cFileName);// Parent = C:\\file
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
    }
    case WM_SIZE:
    {
        USHORT cyClient = HIWORD(lParam);
        USHORT cxClient = LOWORD(lParam);
        INT cxChar, cyChar;

        Metric(hWnd, &cxChar, &cyChar);

        MoveWindow(hWndChild, 0, 0, cxClient * 3 / 4, cyClient, TRUE);
        MoveWindow(hWndTree, cxClient / 4 * 3, 0, cxClient / 4, cyClient, TRUE);
        return 0;
    }
    case WM_DESTROY:
        UnmapViewOfFile(BufData.hFileMap);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}


LRESULT CALLBACK ChildWndProc(
    _In_    HWND hWnd,
    _In_    UINT message,
    _In_    WPARAM wParam,
    _In_    LPARAM lParam)
{
    static INT  iVscrollPos,        //Позиция скрола
        iVscrollMax;        //Максимум скролла
    LONGLONG    SymCount = 0,       //Счётчик символов
        HexCount = 0;       //Счётчик хекс символов

    BufferParams* BufData = (BufferParams*)GetWindowLongPtr(GetParent(hWnd), GWLP_USERDATA);
    switch (message)
    {
    case WM_CREATE:
    {
        return 0;
    }
    case WM_SIZE:
    {
        USHORT cyClient = HIWORD(lParam);
        USHORT cxClient = LOWORD(lParam);
        INT cxChar, cyChar;

        Metric(hWnd, &cxChar, &cyChar);
        iVscrollMax = max(0, BufData->NumLines - cyClient / cyChar); //Вычисление позиций для скрола
        iVscrollPos = min(iVscrollPos, iVscrollMax);            //Вычисление позиции скрола
        SetScrollRange(hWnd, SB_VERT, 0, iVscrollMax, FALSE);   //Вычисление позиций для скрола 
        SetScrollPos(hWnd, SB_VERT, iVscrollPos, TRUE);         //?
        return 0;
    }
    case WM_COMMAND:
    {
        RECT Win;
        GetClientRect(hWnd, &Win);

        iVscrollPos = 0;
        iVscrollMax = max(0, BufData->NumLines - Win.bottom / 16); //Вычисление позиций для скрола
        iVscrollPos = min(iVscrollPos, iVscrollMax);            //Вычисление позиции скрола
        SetScrollRange(hWnd, SB_VERT, 0, iVscrollMax, FALSE);   //Вычисление позиций для скрола 
        SetScrollPos(hWnd, SB_VERT, iVscrollPos, TRUE);         //?
        //InvalidateRect(hWnd, &Win, false);
        return 0;
    }
    case WM_VSCROLL:
    {
        RECT Win;
        GetClientRect(hWnd, &Win);
        INT cyClient = Win.bottom,
            iVscrollInc, cyChar = 16;

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
            SCROLLINFO skif;
            ZeroMemory(&skif, sizeof(SCROLLINFO));
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
            BufData->Increment = (UINT64)iVscrollPos * NUMBER_OF_SYMBOLS;
            ScrollWindow(hWnd, 0, -cyChar * iVscrollInc, NULL, NULL);
            SetScrollPos(hWnd, SB_VERT, iVscrollPos, TRUE);
        }
        if (BufData->SizeBuffer > (UINT64)BufData->Granularity * 2)
        {
            MoveToMap(BufData, BufData->Increment, iVscrollPos);
        }
        UpdateWindow(hWnd);
        return 0;
    }
    case WM_PAINT:
    {
        RECT        Window;
        PAINTSTRUCT ps;
        PBYTE       Text;                                        //Текстовый буфер
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

        INT cxChar, cyChar;
        Metric(hWnd, &cxChar, &cyChar);

        SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));

        INT iPaintBeg = max(-1, iVscrollPos + Window.top / cyChar - 1);
        INT iPaintEnd = min(BufData->NumLines, iVscrollPos + Window.bottom / cyChar);

        NumStr = Window.bottom / cyChar * NUMBER_OF_SYMBOLS;
        Text = (PBYTE)malloc(NumStr + 1);
        PBYTE s = Text;
        s = BufData->pbBuffer + (iVscrollPos * NUMBER_OF_SYMBOLS);

        //считывание буфера размером с экран
        if (BufData->pbBuffer != NULL)
        {
            Out(s, NumStr, &NewBuf, &len);
            len = BufData->SizeBuffer * 3;
            for (INT g = iPaintBeg; g < iPaintEnd - 1; g++)
            {
                INT y = cyChar * (1 - iVscrollPos + g);
                Number = (g + 1) * NUMBER_OF_SYMBOLS;
                sprintf(Offset, "%08X", Number);

                PrintText(Offset, Stroka, s, SymCount, HexCount, len, NewBuf, &FinStr, BufData->SizeBuffer);
                HexCount += NUMBER_OF_SYMBOLS * 3;
                SymCount += NUMBER_OF_SYMBOLS;
                TextOutA(hdc, 0, y, FinStr, SUM);
                s += NUMBER_OF_SYMBOLS;
                Stroka++;
                if (SymCount == BufData->SizeBuffer)
                {
                    break;
                }
            }
        }
        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
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