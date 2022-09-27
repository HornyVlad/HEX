//
//LRESULT CALLBACK ChildWndProc(
//    _In_    HWND hWnd,
//    _In_    UINT message,
//    _In_    WPARAM wParam,
//    _In_    LPARAM lParam)
//{
//    static INT  iVscrollPos,        //Позиция скрола
//        iVscrollMax;        //Максимум скролла
//    LONGLONG    SymCount = 0,       //Счётчик символов
//        HexCount = 0;       //Счётчик хекс символов
//
//    BufferParams* BufData = (BufferParams*)GetWindowLongPtr(GetParent(hWnd), GWLP_USERDATA);
//    switch (message)
//    {
//    case WM_CREATE:
//    {
//        return 0;
//    }
//    case WM_SIZE:
//    {
//        USHORT cyClient = HIWORD(lParam);
//        USHORT cxClient = LOWORD(lParam);
//        INT cxChar, cyChar;
//
//        Metric(hWnd, &cxChar, &cyChar);
//        iVscrollMax = max(0, BufData->NumLines - cyClient / cyChar); //Вычисление позиций для скрола
//        iVscrollPos = min(iVscrollPos, iVscrollMax);            //Вычисление позиции скрола
//        SetScrollRange(hWnd, SB_VERT, 0, iVscrollMax, FALSE);   //Вычисление позиций для скрола 
//        SetScrollPos(hWnd, SB_VERT, iVscrollPos, TRUE);         //?
//        return 0;
//    }
//    case WM_COMMAND:
//    {
//        RECT Win;
//        GetClientRect(hWnd, &Win);
//
//        iVscrollPos = 0;
//        iVscrollMax = max(0, BufData->NumLines - Win.bottom / 16); //Вычисление позиций для скрола
//        iVscrollPos = min(iVscrollPos, iVscrollMax);            //Вычисление позиции скрола
//        SetScrollRange(hWnd, SB_VERT, 0, iVscrollMax, FALSE);   //Вычисление позиций для скрола 
//        SetScrollPos(hWnd, SB_VERT, iVscrollPos, TRUE);         //?
//        InvalidateRect(hWnd, &Win, false);
//        return 0;
//    }
//    case WM_VSCROLL:
//    {
//        RECT Win;
//        GetClientRect(hWnd, &Win);
//        INT cyClient = Win.bottom,
//            iVscrollInc, cyChar = 16;
//
//        switch (LOWORD(wParam))
//        {
//        case SB_TOP:
//            iVscrollInc = -iVscrollPos;
//            break;
//        case SB_BOTTOM:
//            iVscrollInc = iVscrollMax - iVscrollPos;
//            break;
//        case SB_LINEUP:
//            iVscrollInc = -1;
//            break;
//        case SB_LINEDOWN:
//            iVscrollInc = 1;
//            break;
//        case SB_PAGEUP:
//            iVscrollInc = min(-1, -cyClient / cyChar);
//            break;
//        case SB_PAGEDOWN:
//            iVscrollInc = max(1, cyClient / cyChar);
//            break;
//        case SB_THUMBTRACK:
//            SCROLLINFO skif;
//            ZeroMemory(&skif, sizeof(SCROLLINFO));
//            skif.cbSize = sizeof(SCROLLINFO);
//            skif.fMask = SIF_TRACKPOS;
//            GetScrollInfo(hWnd, SB_VERT, &skif);
//            iVscrollInc = skif.nTrackPos - iVscrollPos;
//
//            break;
//        default:
//            iVscrollInc = 0;
//        }
//        iVscrollInc = max(-iVscrollPos, min(iVscrollInc, iVscrollMax - iVscrollPos));
//        if (iVscrollInc != 0)
//        {
//            iVscrollPos += iVscrollInc;
//            BufData->Increment = (UINT64)iVscrollPos * NUMBER_OF_SYMBOLS;
//            ScrollWindow(hWnd, 0, -cyChar * iVscrollInc, NULL, NULL);
//            SetScrollPos(hWnd, SB_VERT, iVscrollPos, TRUE);
//        }
//        if (BufData->SizeBuffer > (UINT64)BufData->Granularity * 2)
//        {
//            MoveToMap(BufData, BufData->Increment, iVscrollPos);
//        }
//        UpdateWindow(hWnd);
//        return 0;
//    }
//    case WM_PAINT:
//    {
//        RECT Window;
//        PAINTSTRUCT ps;
//        PBYTE       Text;                                        //Текстовый буфер
//        HDC         hdc = BeginPaint(hWnd, &ps);                    //
//        CHAR        Offset[NUMBER_OF_SYMBOLS_OFFSET + 1] = { 0 };                                    //Смещение в 16ой
//        INT         Stroka = 0,                              //Делит буфер на строки
//            NumStr = 0;                                        //Кол-во строк, умещающихся на экране
//        static INT  Number = 0;                                        //Смещение в 10ой
//        size_t      len;                                                                            //Длина хекс буфера
//        PCHAR       NewBuf,                                                                         //хекс буфер
//            FinStr;                                                                         //Выводимая на экран строка
//
//        SymCount = iVscrollPos * NUMBER_OF_SYMBOLS;
//        HexCount = iVscrollPos * NUMBER_OF_SYMBOLS * 3;
//
//        GetClientRect(hWnd, &Window);
//
//        INT cxChar, cyChar;
//        Metric(hWnd, &cxChar, &cyChar);
//
//        SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));
//
//        INT iPaintBeg = max(-1, iVscrollPos + Window.top / cyChar - 1);
//        INT iPaintEnd = min(BufData->NumLines, iVscrollPos + Window.bottom / cyChar);
//
//        NumStr = Window.bottom / cyChar * NUMBER_OF_SYMBOLS;
//        Text = (PBYTE)malloc(NumStr + 1);
//        PBYTE s = Text;
//        s = BufData->pbBuffer + (iVscrollPos * NUMBER_OF_SYMBOLS);
//
//        //считывание буфера размером с экран
//        if (BufData->pbBuffer != NULL)
//        {
//            Out(s, NumStr, &NewBuf, &len);
//            len = BufData->SizeBuffer * 3;
//            for (INT g = iPaintBeg; g < iPaintEnd - 1; g++)
//            {
//                INT y = cyChar * (1 - iVscrollPos + g);
//                Number = (g + 1) * NUMBER_OF_SYMBOLS;
//                sprintf(Offset, "%08X", Number);
//
//                PrintText(Offset, Stroka, s, SymCount, HexCount, len, NewBuf, &FinStr, BufData->SizeBuffer);
//                HexCount += NUMBER_OF_SYMBOLS * 3;
//                SymCount += NUMBER_OF_SYMBOLS;
//                TextOutA(hdc, 0, y, FinStr, SUM);
//                s += NUMBER_OF_SYMBOLS;
//                Stroka++;
//                if (SymCount == BufData->SizeBuffer)
//                {
//                    break;
//                }
//            }
//        }
//        EndPaint(hWnd, &ps);
//        return 0;
//    }
//    case WM_DESTROY:
//        PostQuitMessage(0);
//        return 0;
//    default:
//        return DefWindowProc(hWnd, message, wParam, lParam);
//    }
//}