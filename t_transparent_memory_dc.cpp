// t_transparent_memory_dc.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "t_transparent_memory_dc.h"
#include <atlconv.h>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

#define MAX_LOADSTRING 100

#define IMAGE_PATH L"test.emf"
#define ORIG_W 100
#define ORIG_H 100
#define RADIO 1
#define IMAGE_W (int)(ORIG_W * RADIO)
#define IMAGE_H (int)(ORIG_H * RADIO)

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

Gdiplus::GdiplusStartupInput g_gsi;
ULONG_PTR g_token;

// 本函数把一种指定的颜色变成透明色
// hBitmap 要显示的位图
// xStart，xStart 显示的位置
// xadd，yadd 显示的位图的大小变化：放大缩小
// cTransparentColor 变成透明的那种颜色
void TransparentBitmap(HDC hdc, HBITMAP hBitmap, short xStart, short yStart, short xadd, short yadd, COLORREF cTransparentColor)
{
  HDC	hMem, hBack, hObject, hTemp, hSave;

  hBack = CreateCompatibleDC(hdc);
  hObject = CreateCompatibleDC(hdc);
  hMem = CreateCompatibleDC(hdc);
  hSave = CreateCompatibleDC(hdc);
  hTemp = CreateCompatibleDC(hdc);

  SelectObject(hTemp, hBitmap);

  BITMAP m_bm;
  GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&m_bm);

  POINT	ptSize;
  ptSize.x = m_bm.bmWidth;
  ptSize.y = m_bm.bmHeight;

  // 转换为逻辑点值
  DPtoLP(hTemp, &ptSize, 1);

  // 创建临时位图
  HBITMAP	bmBack, bmObject, bmMem, bmSave;

  // 单色位图
  bmBack = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL);
  bmObject = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL);

  // 与设备兼容位图
  bmMem = CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y);
  bmSave = CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y);

  // 将创建的临时位图选入临时 DC 中

  HBITMAP OldbmBack, OldbmObject, OldbmMem, OldbmSave;
  OldbmBack = (HBITMAP)SelectObject(hBack, bmBack);
  OldbmObject = (HBITMAP)SelectObject(hObject, bmObject);
  OldbmMem = (HBITMAP)SelectObject(hMem, bmMem);
  OldbmSave = (HBITMAP)SelectObject(hSave, bmSave);

  // 设置映射模式
  SetMapMode(hTemp, GetMapMode(hdc));

  // 先保留原始位图
  BitBlt(hSave, 0, 0, ptSize.x, ptSize.y, hTemp, 0, 0, SRCCOPY);

  // 将背景颜色设置为需透明的颜色
  COLORREF cColor = SetBkColor(hTemp, cTransparentColor);

  // 创建目标屏蔽码
  BitBlt(hObject, 0, 0, ptSize.x, ptSize.y, hTemp, 0, 0, SRCCOPY);

  // 恢复源 DC 的原始背景色
  SetBkColor(hTemp, cColor);

  // 创建反转的目标屏蔽码
  BitBlt(hBack, 0, 0, ptSize.x, ptSize.y, hObject, 0, 0, NOTSRCCOPY);

  // 拷贝主 DC 的背景到目标 DC
  BitBlt(hMem, 0, 0, ptSize.x, ptSize.y, hdc, xStart, yStart, SRCCOPY);

  // 屏蔽位图的显示区
  BitBlt(hMem, 0, 0, ptSize.x, ptSize.y, hObject, 0, 0, SRCAND);

  // 屏蔽位图中的透明色
  BitBlt(hTemp, 0, 0, ptSize.x, ptSize.y, hBack, 0, 0, SRCAND);

  // 将位图与目标 DC 的背景左异或操作
  BitBlt(hMem, 0, 0, ptSize.x, ptSize.y, hTemp, 0, 0, SRCPAINT);

  // 拷贝目标到屏幕上
  StretchBlt(hdc, xStart, yStart, ptSize.x + xadd, ptSize.y + yadd, hMem, 0, 0, ptSize.x, ptSize.y, SRCCOPY);

  // 恢复原始位图
  BitBlt(hTemp, 0, 0, ptSize.x, ptSize.y, hSave, 0, 0, SRCCOPY);

  // 删除临时内存位图
  DeleteObject(SelectObject(hBack, OldbmBack));
  DeleteObject(SelectObject(hObject, OldbmObject));
  DeleteObject(SelectObject(hMem, OldbmMem));
  DeleteObject(SelectObject(hSave, OldbmSave));

  // 删除临时内存 DC
  DeleteDC(hMem);
  DeleteDC(hBack);
  DeleteDC(hObject);
  DeleteDC(hSave);
  DeleteDC(hTemp);
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
  SelectObject(mem_dc, mem_bmp);

  RECT rc{ 0, 0, IMAGE_W, IMAGE_H };
  HBRUSH hbrush = CreateSolidBrush(RGB(255, 255, 0));
  FillRect(mem_dc, &rc, hbrush);

  HENHMETAFILE hemf = GetEnhMetaFile(IMAGE_PATH);
  if (hemf) {
    PlayEnhMetaFile(mem_dc, hemf, &rc);
    TextOut(mem_dc, 0, 0, L"GDI", 3);
    BitBlt(hdc, 0, IMAGE_H, IMAGE_W, IMAGE_H, mem_dc, 0, 0, SRCCOPY);
    DeleteEnhMetaFile(hemf);
  }

  t_gdi_paint_hbitmap(hdc, mem_bmp);
  t_gdiplus_paint_hbitmap(hdc, mem_bmp);
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
    t_transparent_memory_dc(hdc);
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
