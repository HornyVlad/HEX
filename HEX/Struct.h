#pragma once
struct BufferParams
{
    UINT64  SizeBuffer  = 0;
    UINT64  NumLines    = 0;

    UINT64  BottomOffset    = 0;
    UINT64  TopOffset       = 0;
    UINT64  CurrentOffset   = 0;

    UINT    Granularity = 0;

    HANDLE  hFileMap    = NULL;
    PBYTE   pbBuffer    = NULL;
    UINT64  Increment   = 0;
};