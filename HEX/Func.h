VOID FileInfo(HANDLE hFileToLoad, BufferParams* BufData);

/// <summary>
/// Считывание файла
/// </summary>
/// <param name="FileName">Путь и название файла</param>
/// <param name="BufData">Буфер для текста файла и размер буфера</param>
/// <returns>TRUE в случае успеха и ...</returns>
BOOL LoadData(  _In_    LPCSTR FileName,
                _Out_   BufferParams* BufData)
{
    //Открытие файла
    HANDLE hFileToLoad = CreateFileA(FileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFileToLoad == INVALID_HANDLE_VALUE)
    {
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
    CloseHandle(hFileToLoad);
    return TRUE;
}

/// <summary>
/// Получение данных открытого файла
/// </summary>
/// <param name="hFileToLoad">Дискриптор файла</param>
/// <param name="BufData">Структура для данных</param>
VOID FileInfo(  _In_    HANDLE hFileToLoad, 
                _Out_   BufferParams* BufData)
{
    SYSTEM_INFO SysInf;                 //Получение системных данных
    DWORD       dwFileSizeHigh  = 0;    //
    INT64       qwFileOffset    = 0;    //
    ULONG       BytesInBlock    = 0;    //

    ZeroMemory(&SysInf, sizeof(SYSTEM_INFO));

    GetSystemInfo(&SysInf);

    BufData->Granularity = SysInf.dwAllocationGranularity;
    BytesInBlock = BufData->Granularity * 2;

    BufData->BottomOffset = BufData->Granularity;

    BufData->SizeBuffer = GetFileSize(hFileToLoad, &dwFileSizeHigh);
    BufData->SizeBuffer += (((UINT64)dwFileSizeHigh) << 32);

    BufData->NumLines = BufData->SizeBuffer + NUMBER_OF_SYMBOLS - 1;
    BufData->NumLines /= NUMBER_OF_SYMBOLS;

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
}

/// <summary>
/// Параметры для открытия файлов
/// </summary>
/// <param name="hWnd">Дискриптор окна</param>
VOID SetOpenFileParams(_In_ HWND hWnd) {

    ZeroMemory(&File, sizeof(File));

    File.lStructSize    = sizeof(File);
    File.hwndOwner      = hWnd;
    File.lpstrFile      = FileName;
    File.nMaxFile       = sizeof(FileName);
    File.lpstrFilter    = "*.txt";
    File.lpstrFileTitle = NULL;
    File.nMaxFileTitle  = 0;
    File.lpstrInitialDir = "C:/Users/vladk/source/repos/HEX";
    File.Flags          = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
}

/// <summary>
/// Перемещение по файлу
/// </summary>
/// <param name="FileBuf">Структура с данными</param>
/// <param name="ullFileOffSet">Расположение в файле</param>
/// <param name="iVscrollPos">Положение скролла</param>
VOID MoveToMap( _Out_   BufferParams* FileBuf, 
                _In_    ULONGLONG ullFileOffSet, 
                _In_    INT iVscrollPos)
{
    ULONG ullBytesInBlock = FileBuf->Granularity * 2;

    if (ullFileOffSet % FileBuf->Granularity != 0)
    {
        ullFileOffSet -= ullFileOffSet % FileBuf->Granularity;
    }

    FileBuf->CurrentOffset = ullFileOffSet;

    if (iVscrollPos > FileBuf->BottomOffset / NUMBER_OF_SYMBOLS || iVscrollPos < FileBuf->TopOffset / NUMBER_OF_SYMBOLS)
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
    const char      hex[]   = "0123456789ABCDEF";           //Заполнение массива???
    unsigned char*  b       = (unsigned char*)dp;           //Указатель на буфер
    char*           bp      = (char*)malloc(size * 3 + 1);  //Выделение памяти
    char*           p       = bp;                           //Указатель на переменую с выделенной для неё памятью

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

//доделать
VOID Scroll(INT NumLines, INT* iVscrollMax, INT* iVscrollPos, INT cyClient, INT cyChar)
{
    //понять какие переменные будут изменяться
    *iVscrollMax = max(0, NumLines + 2 - cyClient / cyChar);
    *iVscrollPos = min(*iVscrollPos, *iVscrollMax);
    SetScrollRange(hWnd, SB_VERT, 0, *iVscrollMax, FALSE);
    SetScrollPos(hWnd, SB_VERT, *iVscrollPos, TRUE);
}

/// <summary>
/// Просчёт метрики
/// </summary>
/// <param name="hWnd">Дискрикптор окна</param>
/// <param name="cxChar">Ширина символа</param>
/// <param name="cyChar">Высота символа</param>
VOID Metric(_In_    HWND hWnd, 
            _Out_   INT* cxChar, 
            _Out_   INT* cyChar)
{
    HDC         hdc = GetDC(hWnd);
    TEXTMETRIC  tm;
    GetTextMetrics(hdc, &tm);
    *cxChar = tm.tmAveCharWidth;                                //Ширина символа 
    *cyChar = tm.tmHeight + tm.tmExternalLeading;               //Высота символа
    ReleaseDC(hWnd, hdc);
}