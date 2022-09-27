VOID FileInfo(HANDLE hFileToLoad, BufferParams* BufData);

/// <summary>
/// ���������� �����
/// </summary>
/// <param name="FileName">���� � �������� �����</param>
/// <param name="BufData">����� ��� ������ ����� � ������ ������</param>
/// <returns>TRUE � ������ ������ � ...</returns>
BOOL LoadData(  _In_    LPCSTR FileName,
                _Out_   BufferParams* BufData)
{
    //�������� �����
    HANDLE hFileToLoad = CreateFileA(FileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFileToLoad == INVALID_HANDLE_VALUE)
    {
        MessageBoxW(hWnd, L"File couldn't open!", L"ERROR", MB_OK);
        return FALSE;
    }

    //�������� ���� �����
    BufData->hFileMap = CreateFileMapping(hFileToLoad, NULL, PAGE_WRITECOPY, 0, 0, NULL);

    if (BufData->hFileMap == NULL)
    {
        CloseHandle(hFileToLoad);
        MessageBoxW(hWnd, L"File couldn't open!", L"ERROR", MB_OK);
        return FALSE;
    }

    //�������� ��������, ���������� ������, ����� ������� �����
    FileInfo(hFileToLoad, BufData);
    CloseHandle(hFileToLoad);
    return TRUE;
}

/// <summary>
/// ��������� ������ ��������� �����
/// </summary>
/// <param name="hFileToLoad">���������� �����</param>
/// <param name="BufData">��������� ��� ������</param>
VOID FileInfo(  _In_    HANDLE hFileToLoad, 
                _Out_   BufferParams* BufData)
{
    SYSTEM_INFO SysInf;                 //��������� ��������� ������
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
/// ��������� ��� �������� ������
/// </summary>
/// <param name="hWnd">���������� ����</param>
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
/// ����������� �� �����
/// </summary>
/// <param name="FileBuf">��������� � �������</param>
/// <param name="ullFileOffSet">������������ � �����</param>
/// <param name="iVscrollPos">��������� �������</param>
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
/// ������������� ����� � HEX ������
/// </summary>
/// <param name="dp">�����</param>
/// <param name="size">������ ������</param>
/// <param name="str">��������������� �����</param>
/// <param name="str_len">����� ������ ������</param>
/// <returns>1 � ������ ������</returns>
INT Out(
    _In_reads_bytes_(size) const                VOID* dp,
    _In_                                        size_t  size,
    _Outptr_opt_result_bytebuffer_(*str_len)    PCHAR* str,
    _Out_                                       size_t* str_len)
{
    const char      hex[]   = "0123456789ABCDEF";           //���������� �������???
    unsigned char*  b       = (unsigned char*)dp;           //��������� �� �����
    char*           bp      = (char*)malloc(size * 3 + 1);  //��������� ������
    char*           p       = bp;                           //��������� �� ��������� � ���������� ��� �� �������

    if (!p || (!b && !size))
        return 0;

    //���������� ������� �� ������� � ������� ���� ��� �������������� � ����
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
/// ��������� ������ � ����� � �� ����� 
/// </summary>
/// <param name="Offset">��������</param>
/// <param name="Stroka">������ ��� ��������� ������ �� ������</param>
/// <param name="Text">���������� �����</param>
/// <param name="SymCount">������� �������� ��� ������</param>
/// <param name="HexCount">������� ����-�������� ��� ������</param>
/// <param name="len">����� ���� ������</param>
/// <param name="NewBuf">����� � ������</param>
/// <param name="FinStr">�������� ������</param>
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
    INT     Symbol = 0;                 //����� ������� ��� ���������� ����������� �� ������ �����
    PCHAR   Str = (PCHAR)malloc(SUM);   //��������� ������ ��� ������
    PCHAR   Vivod = Str;                //��������� �� ���������� ������
    INT     i;                          //���������

    //���������� ������ ���������
    for (i = 0; i < 8; i++)
    {
        *Vivod++ = Offset[i];
    }
    *Vivod++ = ' ';

    //���������� ������ ������
    for (i = 0; i < NUMBER_OF_SYMBOLS * NUMBER_OF_SYMBOLS_HEX; i++)
    {
        *Vivod++ = NewBuf[Stroka * 48 + Symbol];
        Symbol++;
        HexCount++;

        //���������� ������ ���� ������ ���������
        if (HexCount == len)
        {
            for (i; i < NUMBER_OF_SYMBOLS * NUMBER_OF_SYMBOLS_HEX; i++) *Vivod++ = ' ';
            break;
        }
    }
    *Vivod++ = ' ';

    //���������� ������ ������� �����
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

        //��������� ������ ���� ������ ���������
        if (SymCount == BufSize)
        {
            for (i; i < NUMBER_OF_SYMBOLS * 2; i++) *Vivod++ = ' ';
            break;
        }
    }
    *Vivod = '\0';
    *FinStr = Str;
}

//��������
VOID Scroll(INT NumLines, INT* iVscrollMax, INT* iVscrollPos, INT cyClient, INT cyChar)
{
    //������ ����� ���������� ����� ����������
    *iVscrollMax = max(0, NumLines + 2 - cyClient / cyChar);
    *iVscrollPos = min(*iVscrollPos, *iVscrollMax);
    SetScrollRange(hWnd, SB_VERT, 0, *iVscrollMax, FALSE);
    SetScrollPos(hWnd, SB_VERT, *iVscrollPos, TRUE);
}

/// <summary>
/// ������� �������
/// </summary>
/// <param name="hWnd">����������� ����</param>
/// <param name="cxChar">������ �������</param>
/// <param name="cyChar">������ �������</param>
VOID Metric(_In_    HWND hWnd, 
            _Out_   INT* cxChar, 
            _Out_   INT* cyChar)
{
    HDC         hdc = GetDC(hWnd);
    TEXTMETRIC  tm;
    GetTextMetrics(hdc, &tm);
    *cxChar = tm.tmAveCharWidth;                                //������ ������� 
    *cyChar = tm.tmHeight + tm.tmExternalLeading;               //������ �������
    ReleaseDC(hWnd, hdc);
}