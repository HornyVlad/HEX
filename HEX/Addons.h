#pragma once
unsigned num;

HWND			hStaticControl;
HWND			hWnd;
HWND			hNumberControl;
OPENFILENAMEA   File;                                       //
HINSTANCE       hInst							= NULL;     //
WCHAR           szTitle[MAX_LOADSTRING]			= { 0 };    //
WCHAR           szWindowClass[MAX_LOADSTRING]	= { 0 };    //
CHAR            FileName[MAX_PATH]				= { 0 };    //путь и название выбранного файла 
WCHAR           szChildClass[]					= L"Checker3_Child";

BOOL                InitInstance(HINSTANCE, INT);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    ChildWndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

//LRESULT CALLBACK 
//void SaveData(LPCSTR path);
//void LoadData(LPCSTR path);