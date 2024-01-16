// t_transparent_memory_dc.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "t_transparent_memory_dc.h"
#include <atlconv.h>
#include <gdiplus.h>
#include <vector>

#pragma comment(lib, "gdiplus.lib")

#define MAX_LOADSTRING 100

#define IMAGE_PATH L"test.wmf"
#define ORIG_W 175
#define ORIG_H 188
#define RADIO 1
#define IMAGE_W (int)(ORIG_W * RADIO)
#define IMAGE_H (int)(ORIG_H * RADIO)

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

Gdiplus::GdiplusStartupInput g_gsi;
ULONG_PTR g_token;

#pragma pack(1)
typedef struct tagWIN16RECT {
  WORD left;
  WORD top;
  WORD right;
  WORD bottom;
} WIN16RECT;

typedef struct tagPLACEABLEMETAHEADER {
  DWORD key;
  WORD hmf;
  WIN16RECT bbox;
  WORD inch;
  DWORD reserved;
  WORD checksum;
} PLACEABLEMETAHEADER;
#pragma pack()

HENHMETAFILE WINAPI convert_wmf_to_emf(IN LPCWSTR meta_file) {
  HANDLE hfile = CreateFileW(meta_file, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (!hfile || INVALID_HANDLE_VALUE == hfile) {
    return NULL;
  }

  DWORD dwsize = GetFileSize(hfile, NULL);
  std::vector<BYTE> data(dwsize);

  DWORD dwread = -1;
  BOOL bret = ReadFile(hfile, &data[0], dwsize, &dwread, NULL);
  CloseHandle(hfile);

  HENHMETAFILE hemf = NULL;

  if (bret) {
    PLACEABLEMETAHEADER* pmh = (PLACEABLEMETAHEADER*)&data[0];

    int offset = -1;
    if (pmh->key != 0x9AC6CDD7)
      offset = 0;
    else
      offset = sizeof(PLACEABLEMETAHEADER);

    hemf = SetWinMetaFileBits((UINT)data.size(), &data[offset], NULL, NULL);
  }

  return hemf;
}

void DrawTransparentBitmap(HDC hdc, HBITMAP hBitmap, short xStart, short yStart, COLORREF cTransparentColor)
{
  BITMAP bm;
  COLORREF cColor;
  HBITMAP bmAndBack, bmAndObject, bmAndMem, bmSave;
  HBITMAP bmBackOld, bmObjectOld, bmMemOld, bmSaveOld;
  HDC hdcMem, hdcBack, hdcObject, hdcTemp, hdcSave;
  POINT ptSize;

  hdcTemp = CreateCompatibleDC(hdc);
  SelectObject(hdcTemp, hBitmap); // Select the bitmap

  GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
  ptSize.x = bm.bmWidth; // Get width of bitmap
  ptSize.y = bm.bmHeight; // Get height of bitmap
  DPtoLP(hdcTemp, &ptSize, 1); // Convert from device

  // Create some DCs to hold temporary data.
  hdcBack = CreateCompatibleDC(hdc);
  hdcObject = CreateCompatibleDC(hdc);
  hdcMem = CreateCompatibleDC(hdc);
  hdcSave = CreateCompatibleDC(hdc);

  // Create a bitmap for each DC. DCs are required for a number of
  // GDI functions.

  // Monochrome DC
  bmAndBack = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL);

  // Monochrome DC
  bmAndObject = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL);

  bmAndMem = CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y);
  bmSave = CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y);

  // Each DC must select a bitmap object to store pixel data.
  bmBackOld = (HBITMAP)SelectObject(hdcBack, bmAndBack);
  bmObjectOld = (HBITMAP)SelectObject(hdcObject, bmAndObject);
  bmMemOld = (HBITMAP)SelectObject(hdcMem, bmAndMem);
  bmSaveOld = (HBITMAP)SelectObject(hdcSave, bmSave);

  // Set proper mapping mode.
  SetMapMode(hdcTemp, GetMapMode(hdc));

  // Save the bitmap sent here, because it will be overwritten.
  BitBlt(hdcSave, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCCOPY);

  // Set the background color of the source DC to the color.
  // contained in the parts of the bitmap that should be transparent
  cColor = SetBkColor(hdcTemp, cTransparentColor);

  // Create the object mask for the bitmap by performing a BitBlt
  // from the source bitmap to a monochrome bitmap.
  BitBlt(hdcObject, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCCOPY);
  BitBlt(hdc, ptSize.x * 1, ptSize.y * 1, ptSize.x, ptSize.y, hdcObject, 0, 0, SRCCOPY);

  // Set the background color of the source DC back to the original color.
  SetBkColor(hdcTemp, cColor);

  // Create the inverse of the object mask.
  BitBlt(hdcBack, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0, NOTSRCCOPY);
  BitBlt(hdc, ptSize.x * 2, ptSize.y * 1, ptSize.x, ptSize.y, hdcBack, 0, 0, SRCCOPY);

  // Copy the background of the main DC to the destination.
  BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdc, xStart, yStart, SRCCOPY);
  BitBlt(hdc, ptSize.x * 3, ptSize.y * 1, ptSize.x, ptSize.y, hdcMem, 0, 0, SRCCOPY);

  // Mask out the places where the bitmap will be placed.
  BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0, SRCAND);
  BitBlt(hdc, ptSize.x * 4, ptSize.y * 1, ptSize.x, ptSize.y, hdcMem, 0, 0, SRCCOPY);

  // Mask out the transparent colored pixels on the bitmap.
  BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcBack, 0, 0, SRCAND);
  BitBlt(hdc, ptSize.x * 5, ptSize.y * 1, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCCOPY);

  // XOR the bitmap with the background on the destination DC.
  BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCPAINT);
  BitBlt(hdc, ptSize.x * 6, ptSize.y * 1, ptSize.x, ptSize.y, hdcMem, 0, 0, SRCCOPY);

  // Copy the destination to the screen.
  BitBlt(hdc, xStart, yStart, ptSize.x, ptSize.y, hdcMem, 0, 0, SRCCOPY);

  // Place the original bitmap back into the bitmap sent here.
  BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcSave, 0, 0, SRCCOPY);

  // Delete the memory bitmaps.
  DeleteObject(SelectObject(hdcBack, bmBackOld));
  DeleteObject(SelectObject(hdcObject, bmObjectOld));
  DeleteObject(SelectObject(hdcMem, bmMemOld));
  DeleteObject(SelectObject(hdcSave, bmSaveOld));

  // Delete the memory DCs.
  DeleteDC(hdcMem);
  DeleteDC(hdcBack);
  DeleteDC(hdcObject);
  DeleteDC(hdcSave);
  DeleteDC(hdcTemp);
}

void t_gdi_paint_hbitmap(HDC hdc, HBITMAP hbitmap) {
  HDC mem_dc = CreateCompatibleDC(hdc);
  HGDIOBJ old = SelectObject(mem_dc, hbitmap);
  if (!old)
    MessageBox(0, L"SelectObject return NULL", 0, 0);

  BITMAP bmpinfo;
  GetObject(hbitmap, sizeof(bmpinfo), &bmpinfo);
  BitBlt(hdc, 0, 0, bmpinfo.bmWidth, bmpinfo.bmHeight, mem_dc, 0, 0, SRCCOPY);
}

void t_gdiplus_paint_hbitmap(HDC hdc, HBITMAP hbitmap) {
  BITMAP bmpinfo;
  GetObject(hbitmap, sizeof(bmpinfo), &bmpinfo);
  Gdiplus::Bitmap bitmap(hbitmap, NULL);
  Gdiplus::Graphics g(hdc);
  g.DrawImage(&bitmap, bitmap.GetWidth(), 0, bitmap.GetWidth(), bitmap.GetHeight());
}

void t_transparent_memory_dc(HDC hdc) {
  HBITMAP mem_bmp = NULL;

  BITMAPINFOHEADER bih = { 0 };
  bih.biSize = sizeof(BITMAPINFOHEADER);
  bih.biWidth = IMAGE_W;
  bih.biHeight = -IMAGE_H;
  bih.biPlanes = 1;
  bih.biBitCount = 32;
  bih.biCompression = BI_RGB;
  bih.biSizeImage = 0;
  bih.biXPelsPerMeter = 0;
  bih.biYPelsPerMeter = 0;
  bih.biClrUsed = 0;
  bih.biClrImportant = 0;

  BITMAPINFO bi = { 0 };
  bi.bmiHeader = bih;

  int len = IMAGE_W * IMAGE_H;
  PBYTE pbits = new BYTE[len * 4];
  memset(pbits, 0, len * 4);

  HDC dc = CreateDC(L"DISPLAY", NULL, NULL, NULL);
  mem_bmp = CreateDIBitmap(dc, &bih, CBM_INIT, pbits, &bi, DIB_RGB_COLORS);
  DeleteDC(dc);

  HDC mem_dc = CreateCompatibleDC(NULL);
  HGDIOBJ old_gdi = SelectObject(mem_dc, mem_bmp);

  RECT rc{ 0, 0, IMAGE_W, IMAGE_H };
  HBRUSH hbrush = CreateSolidBrush(RGB(255, 255, 255));
  FillRect(mem_dc, &rc, hbrush);

  HENHMETAFILE hemf = convert_wmf_to_emf(IMAGE_PATH);
  if (hemf) {
    PlayEnhMetaFile(hdc, hemf, &rc);
    PlayEnhMetaFile(mem_dc, hemf, &rc);
    // BitBlt(hdc, 0, 0, IMAGE_W, IMAGE_H, mem_dc, 0, 0, SRCCOPY);
    DeleteEnhMetaFile(hemf);
  }

  SelectObject(mem_dc, old_gdi);

  // t_gdi_paint_hbitmap(hdc, mem_bmp);
  // t_gdiplus_paint_hbitmap(hdc, mem_bmp);
  DrawTransparentBitmap(hdc, mem_bmp, 0, IMAGE_H, RGB(255, 255, 255));

  DeleteObject(mem_bmp);
  DeleteDC(mem_dc);
}

void desc(HDC hdc) {
  SetBkMode(hdc, TRANSPARENT);
  char info[300] = { 0 };

  SetTextColor(hdc, RGB(0, 0, 255));
  strcpy_s(info, "1, PlayEnhMetaFile");
  TextOutA(hdc, 0, 0, info, (int)strlen(info));

  strcpy_s(info, "2, DrawTransparentBitmap");
  TextOutA(hdc, 0, IMAGE_H, info, (int)strlen(info));

  strcpy_s(info, "2.1, 目标掩码");
  TextOutA(hdc, IMAGE_W * 1, IMAGE_H, info, (int)strlen(info));

  strcpy_s(info, "2.2, 目标掩码取反");
  TextOutA(hdc, IMAGE_W * 2, IMAGE_H, info, (int)strlen(info));

  strcpy_s(info, "2.3, 背景");
  TextOutA(hdc, IMAGE_W * 3, IMAGE_H, info, (int)strlen(info));

  strcpy_s(info, "2.4, 屏蔽位图所在位置");
  TextOutA(hdc, IMAGE_W * 4, IMAGE_H, info, (int)strlen(info));

  strcpy_s(info, "2.5, 屏蔽位图彩色像素");
  TextOutA(hdc, IMAGE_W * 5, IMAGE_H, info, (int)strlen(info));

  strcpy_s(info, "2.6, 最终效果，即最左侧图形");
  TextOutA(hdc, IMAGE_W * 6, IMAGE_H, info, (int)strlen(info));
}

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
  _In_opt_ HINSTANCE hPrevInstance,
  _In_ LPWSTR    lpCmdLine,
  _In_ int       nCmdShow)
{
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

  // TODO: Place code here.
  Gdiplus::GdiplusStartup(&g_token, &g_gsi, NULL);

  // Initialize global strings
  LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
  LoadStringW(hInstance, IDC_TTRANSPARENTMEMORYDC, szWindowClass, MAX_LOADSTRING);
  MyRegisterClass(hInstance);

  // Perform application initialization:
  if (!InitInstance(hInstance, nCmdShow))
  {
    return FALSE;
  }

  HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TTRANSPARENTMEMORYDC));

  MSG msg;

  // Main message loop:
  while (GetMessage(&msg, nullptr, 0, 0))
  {
    if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  Gdiplus::GdiplusShutdown(g_token);
  return (int)msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
  WNDCLASSEXW wcex;

  wcex.cbSize = sizeof(WNDCLASSEX);

  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = WndProc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = hInstance;
  wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TTRANSPARENTMEMORYDC));
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_TTRANSPARENTMEMORYDC);
  wcex.lpszClassName = szWindowClass;
  wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

  return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
  hInst = hInstance; // Store instance handle in our global variable

  HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

  if (!hWnd)
  {
    return FALSE;
  }

  ShowWindow(hWnd, nCmdShow);
  UpdateWindow(hWnd);

  return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_COMMAND:
  {
    int wmId = LOWORD(wParam);
    // Parse the menu selections:
    switch (wmId)
    {
    case IDM_ABOUT:
      DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
      break;
    case IDM_EXIT:
      DestroyWindow(hWnd);
      break;
    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
    }
  }
  break;
  case WM_PAINT:
  {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    // TODO: Add any drawing code that uses hdc here...
    RECT rc;
    GetClientRect(hWnd, &rc);
    FillRect(hdc, &rc, (HBRUSH)GetStockObject(LTGRAY_BRUSH));

    char bkstr[] = "ABCDEFGHIJKLMNOPQRS";
    SetTextColor(hdc, RGB(255, 0, 0));
    SetBkMode(hdc, TRANSPARENT);
    TextOutA(hdc, 0, IMAGE_H * 3 / 2, bkstr, (int)strlen(bkstr));

    t_transparent_memory_dc(hdc);
    desc(hdc);

    EndPaint(hWnd, &ps);
  }
  break;
  case WM_DESTROY:
    PostQuitMessage(0);
    break;
  default:
    return DefWindowProc(hWnd, message, wParam, lParam);
  }
  return 0;
}

// Message handler for about box.
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
