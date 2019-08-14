#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <windows.h>
#include <scrnsave.h>

#define POINT_BUFFER_SIZE 10
#define MAXLEV 4

#define MINVEL  1                 // minimum redraw speed value     
#define MAXVEL  10                // maximum redraw speed value    
#define DEFVEL  5                 // default redraw speed value    
 
LONG    lSpeed = DEFVEL;          // redraw speed variable         
  
extern HINSTANCE hMainInstance;   // screen saver instance handle  
 
typedef struct tagRECT_DBL
{ 
    double left;
    double top;
    double right;
    double bottom;
} RECT_DBL;

typedef struct tagPIXEL
{ 
    LONG x;
    LONG y;
    COLORREF c;
} PIXEL, *PPIXEL;

//CHAR   szAppName[80];             // .ini section name             
CHAR   szTemp[20];                // temporary array of characters  
CHAR   szRedrawSpeed[ ] = "Redraw Speed";   // .ini speed entry 
CHAR   szIniFile[MAXFILELEN];     // .ini or registry file name  

HDC scr;

static double f[2][3][MAXLEV];  /* three non-homogeneous transforms */
static int max_fractals;
static int max_iterations;
static int max_levels;
static int max_points;
static int cur_level;
static int snum;
static int anum;
static int num_pixels;
static int total_points;
static int cur_color;
static int num_colors;
static COLORREF *colors;
static PIXEL pixels [POINT_BUFFER_SIZE];
SIZE screen;
RECT_DBL bounds;
static int test_bounds;
COLORREF cccc;

static int delay2;
static int width, height;

int opt_points = 50000;
int opt_iterations = 25;
int opt_fractals = 5;
int opt_ncolors = 64;
int opt_delay = 2000;
COLORREF background, opt_background;

static short halfrandom (int mv)
{
  static short lasthalf = 0;
  unsigned long r;

  if (lasthalf)
    {
      r = lasthalf;
      lasthalf = 0;
    }
  else
    {
      r = rand ();
      lasthalf = r >> 16;
    }
  return (r % mv);
}

static void DrawPixelArray( PPIXEL points, int num_pixels)
{
  int i;
  for ( i = 0; i < num_pixels; i++ )
    SetPixel( scr, pixels[i].x, pixels[i].y, pixels[i].c );
}

static void init_flame ( )
{
  max_iterations = opt_iterations;
  max_fractals = opt_fractals;
  if (max_iterations <= 0) max_iterations = 100;

  max_points = opt_points;
  if (max_points <= 0) max_points = 10000;

  delay2 = opt_delay;


  int saturation = 255;
  int value = 255;
  if (opt_ncolors <= 0) opt_ncolors = 128;
  colors = (COLORREF *) malloc ((opt_ncolors+1) * sizeof (*colors));

  background = opt_background;
}

static void generate_colors ( )
{
  int i;
  double c1[3], c2[3], c3[3];
  int i1[3], i2[3], l;
  double k, n, a;
  k = 6; n = 100; l = 512;
//  double pi = 3.14159265359;

  // ��६�蠥� ����������
  i1[0] = (rand() % 3);
  for (i1[1]=i1[0];i1[1]==i1[0];i1[1] = (rand() % 3));
  i1[2] = 3 - (i1[0] + i1[1]);

  // ���⮢� 梥�
  for (i=0;i<3;i++)
  {
    c1[i] = ((double)rand() / RAND_MAX);
  }
  a = (sqrt(1.0 + 4.0 / k) - 1) / 2.0;
  c1[i1[0]] = ((1.0 / ((c1[i1[0]] + a) * k)) - a) * 255; // ���� ��������� - ��⥬���
  c1[i1[1]] = (1 - ((1.0 / ((c1[i1[1]] + a) * k)) - a)) * 255; // ��㣠� - �����
  c1[i1[2]] = c1[i1[2]] * 255;  // ����� - ��� ��������

  // �� ࠧ ��६�蠥� ����������
  i1[0] = (rand() % 3);
  for (i1[1]=i1[0];i1[1]==i1[0];i1[1] = (rand() % 3));
  i1[2] = 3 - (i1[0] + i1[1]);

  // ᬥ饭�� �� ��������⠬
  n = 200;
  for (i=0;i<3;i++)
  {
    if( i < 2 )
      c2[i1[i]] = (((double)rand() / RAND_MAX) * 2.0 - 1.0) * n;
    else
      c2[i1[i]] = rand() & 1 ? n : -n;

    if( c1[i1[i]] + c2[i1[i]] < 0 || c1[i1[i]] + c2[i1[i]] > 255)
      c2[i1[i]] = -c2[i1[i]];

    if( c1[i1[i]] + c2[i1[i]] < 0 || c1[i1[i]] + c2[i1[i]] > 255)
      c2[i1[i]] = c1[i1[i]] > 128 ? -c1[i1[i]] : 255 - c1[i1[i]];

    n = n - abs(c2[i1[i]]);
    c2[i1[i]] = c2[i1[i]] / opt_ncolors * 2;
  }

  for (i=0;i<3;i++) c3[i] = c1[i];

  for (num_colors = opt_ncolors / 2; num_colors < opt_ncolors; num_colors++)
  {
    for (i=0;i<3;i++) i2[i] = (int)c3[i];
    while( i2[0] + i2[1] + i2[2] < l && i2[0] < 255 && i2[1] < 255 && i2[2] < 255)
      for (i=0;i<3;i++) i2[i]++;
    colors [num_colors] = RGB(i2[0], i2[1], i2[2]);
    for (i=0;i<3;i++) c3[i] += c2[i];
  }

  // �� ࠧ ��६�蠥� ����������
  i1[0] = (rand() % 3);
  for (i1[1]=i1[0];i1[1]==i1[0];i1[1] = (rand() % 3));
  i1[2] = 3 - (i1[0] + i1[1]);

  // ᬥ饭�� �� ��������⠬
  n = 200;
  for (i=0;i<3;i++)
  {
    if( i < 2 )
      c2[i1[i]] = (((double)rand() / RAND_MAX) * 2.0 - 1.0) * n;
    else
      c2[i1[i]] = rand() & 1 ? n : -n;

    if( c1[i1[i]] + c2[i1[i]] < 0 || c1[i1[i]] + c2[i1[i]] > 255)
      c2[i1[i]] = -c2[i1[i]];

    if( c1[i1[i]] + c2[i1[i]] < 0 || c1[i1[i]] + c2[i1[i]] > 255)
      c2[i1[i]] = c1[i1[i]] > 128 ? -c1[i1[i]] : 255 - c1[i1[i]];

    n = n - abs(c2[i1[i]]);
    c2[i1[i]] = c2[i1[i]] / opt_ncolors * 2;
  }

  for (i=0;i<3;i++) c3[i] = c1[i];

  for (num_colors = opt_ncolors / 2; num_colors >= 0; num_colors--)
  {
    for (i=0;i<3;i++) i2[i] = (int)c3[i];
    while( i2[0] + i2[1] + i2[2] < l && i2[0] < 255 && i2[1] < 255 && i2[2] < 255)
      for (i=0;i<3;i++) i2[i]++;
    colors [num_colors] = RGB(i2[0], i2[1], i2[2]);
    for (i=0;i<3;i++) c3[i] += c2[i];
  }
/*  
  {
    HBRUSH hbr; RECT rc; int width;

    width = (screen.cx * 2) / opt_ncolors;
    rc.top = 0;
    rc.bottom = 20;
    rc.left = 0;
    rc.right = rc.left + width;
    for (num_colors = 0; num_colors < opt_ncolors; num_colors++)
    {
      hbr = CreateSolidBrush( colors [num_colors] );
      FillRect( scr, &rc, hbr );
      rc.left = rc.right;
      rc.right += width;
      DeleteObject( hbr );
    }
  }
*/
  cur_color = opt_ncolors / 4 * 3; //halfrandom (num_colors);
}

static int recurse ( register double x, register double y, register int l, register int c )
{
  int i;
  double nx, ny;

  if (l == max_iterations)
  {
    total_points++;
    if (max_points > 0 && total_points > max_points) /* how long each fractal runs */
      return 0;

    if (test_bounds >= 0)
    {
      if (x < bounds.left) bounds.left = x;
      if (x > bounds.right) bounds.right = x;
      if (y < bounds.top) bounds.top = y;
      if (y > bounds.bottom) bounds.bottom = y;
    }
    else
    {
      if (x > bounds.left && x < bounds.right && y > bounds.top && y < bounds.bottom)
      {
        pixels[num_pixels].x = (short) ((x - bounds.left) / (bounds.right - bounds.left) * width);
        pixels[num_pixels].y = (short) ((y - bounds.top) / (bounds.bottom - bounds.top) * height);
        if (cur_color + c < 0)
          pixels[num_pixels].c = colors[0];
        else if (cur_color + c >= opt_ncolors)
          pixels[num_pixels].c = colors[opt_ncolors-1];
        else
          pixels[num_pixels].c = colors[cur_color + c];

//pixels[num_pixels].c = cccc;

        num_pixels++;
        if (num_pixels >= POINT_BUFFER_SIZE)
        {
          DrawPixelArray (pixels, num_pixels);
          num_pixels = 0;
        }
      }
    }
  }
  else
  {
    for (i = 0; i < snum; i++)
    {
      nx = f[0][0][i] * x + f[0][1][i] * y + f[0][2][i];
      ny = f[1][0][i] * x + f[1][1][i] * y + f[1][2][i];
      if (i < anum)
      {
        nx = sin(nx);
        ny = sin(ny);
      }
      if (!recurse (nx, ny, l + 1, c + (i-1)*(l<max_iterations/3?1:l<max_iterations/3*2?2:l+6-max_iterations<3?3:l+6-max_iterations)))
        return 0;
    }
  }
  return 1;
}


static void flame ()
{
  int i, j, k;
  static int alt = 0;
  static RECT   rc;

  if (!(cur_level % max_fractals))
  {
      rc.left = 0; rc.top = 0; rc.right = width; rc.bottom = height;
      FillRect (scr, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH)); 
  }

  if (num_colors > 2)
  {
//    if ((--cur_color) < 0)
    if ((cur_color-=7) < 0)
      cur_color = num_colors - 1;
  }

  /* number of functions */
  snum = 2 + (cur_level++ % (MAXLEV - 1));

  /* how many of them are of alternate form */
  alt = !alt;
  if (alt)
    anum = 0;
  else
    anum = halfrandom (snum);

  /* 6 coefs per function */
  for (k = 0; k < snum; k++)
    for (i = 0; i < 2; i++)
      for (j = 0; j < 3; j++)
        f[i][j][k] = ((double) (rand() & 1023) / 512.0 - 1.0);

  /* ����稬 �ਬ��� �࠭��� �ࠪ⠫� */
  bounds.left = 0; bounds.top = 0; bounds.right = 0, bounds.bottom = 0;
  test_bounds = 1;
  max_iterations = 4;
  num_pixels = 0;
  total_points = 0;
  (void) recurse (0.0, 0.0, 0, 0);
  test_bounds = -1;
  bounds.left = bounds.left - (bounds.right - bounds.left) * 0.2;
  bounds.right = bounds.right + (bounds.right - bounds.left) * 0.2;
  bounds.top = bounds.top - (bounds.bottom - bounds.top) * 0.2;
  bounds.bottom = bounds.bottom + (bounds.bottom - bounds.top) * 0.2;
  max_iterations = opt_iterations;
  
  num_pixels = 0;
  total_points = 0;
  generate_colors();
//  cccc = RGB(100,100,100);
  (void) recurse (0.0, 0.0, 0, 0);
  DrawPixelArray( pixels, num_pixels);
/*
  max_iterations = 4;
  num_pixels = 0;
  total_points = 0;
  cccc = RGB(255,0,0);
  (void) recurse (0.0, 0.0, 0, 0);
  DrawPixelArray( pixels, num_pixels);
  max_iterations = opt_iterations;
*/

  if (!(cur_level % max_fractals))
  {
      if (delay2)
      {
        Sleep (delay2);
        MSG msg;
        while (PeekMessage( &msg, NULL, WM_TIMER, WM_TIMER, PM_REMOVE ));
      }
  }
}

LRESULT WINAPI ScreenSaverProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{ 
    static RECT         rc;       // RECT structure  
    static UINT         uTimer;   // timer identifier  
 
    switch(message) 
    { 
        case WM_CREATE: 
 
            // Retrieve the application name from the .rc file. 
            //LoadString(hMainInstance, idsAppName, szAppName, 80 * sizeof(TCHAR)); 
 
            // Retrieve the .ini (or registry) file name. 
            //LoadString(hMainInstance, idsIniFile, szIniFile, MAXFILELEN * sizeof(TCHAR)); 
 
            // TODO: Add error checking to verify LoadString success
            //       for both calls.
      
            srand( (unsigned)time( NULL ) );

            GetClientRect (hwnd, &rc); 
            width = rc.right - rc.left+1;
            height = rc.bottom - rc.top+1;
  
            screen.cx = width/2;
            screen.cy = height/2;
            init_flame(); 
            
            scr = GetDC(hwnd); 
            SelectObject(scr,GetStockObject(DC_BRUSH));
            ReleaseDC(hwnd,scr); 

            // Retrieve any redraw speed data from the registry.  
            //lSpeed = GetPrivateProfileInt(szAppName, szRedrawSpeed, 
            //                              DEFVEL, szIniFile); 
            lSpeed = 50;
 
            // Set a timer for the screen saver window using the 
            // redraw rate stored in Regedit.ini. 
            uTimer = SetTimer(hwnd, 1, lSpeed * 10, NULL); 
            break; 
 
        case WM_ERASEBKGND: 
 
            // The WM_ERASEBKGND message is issued before the 
            // WM_TIMER message, allowing the screen saver to 
            // paint the background as appropriate. 

            scr = GetDC(hwnd); 
            GetClientRect (hwnd, &rc); 
            FillRect (scr, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH)); 
            ReleaseDC(hwnd,scr); 
            break; 
 
        case WM_TIMER: 
 
            // The WM_TIMER message is issued at (lSpeed * 1000) 
            // intervals, where lSpeed == .001 seconds. This 
            // code repaints the entire desktop with a white, 
            // light gray, dark gray, or black brush each 
            // time a WM_TIMER message is issued. 

            scr = GetDC(hwnd);
            flame();
            GdiFlush(); 
            ReleaseDC(hwnd,scr); 
            break; 
 
        case WM_DESTROY: 
 
            // When the WM_DESTROY message is issued, the screen saver 
            // must destroy any of the timers that were set at WM_CREATE 
            // time. 

            if (uTimer) 
                KillTimer(hwnd, uTimer); 
            break; 

        case WM_KEYDOWN: 
            if (wParam == VK_SPACE)
            {
              if (uTimer)
              {
                KillTimer(hwnd, uTimer);
                uTimer = 0;
                MSG msg;
                while (PeekMessage( &msg, NULL, WM_TIMER, WM_TIMER, PM_REMOVE ));
              }
              else
              {
                uTimer = SetTimer(hwnd, 1, lSpeed * 10, NULL);
                max_fractals = opt_fractals;
                max_points = opt_points;
                delay2 = opt_delay;
              }
              return 0; 
            }

            if (wParam == VK_RIGHT)
            {
              if (uTimer)
              {
                KillTimer(hwnd, uTimer);
                uTimer = 0;
                MSG msg;
                while (PeekMessage( &msg, NULL, WM_TIMER, WM_TIMER, PM_REMOVE ));
              }
              else
              {
                max_fractals = 1;
                max_points = 250000;
                delay2 = 0;
                PostMessage(hwnd, WM_TIMER, 0, 0);
              }
              return 0; 
            }
            break;
    } 
 
    // DefScreenSaverProc processes any messages ignored by ScreenSaverProc. 
    return DefScreenSaverProc(hwnd, message, wParam, lParam); 
}

BOOL WINAPI ScreenSaverConfigureDialog (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{ 
/*    static HWND hSpeed;   // handle to speed scroll bar 
    static HWND hOK;      // handle to OK push button  
 
    switch(message) 
    { 
        case WM_INITDIALOG: 
 
            // Retrieve the application name from the .rc file.  
            LoadString(hMainInstance, idsAppName, szAppName, 
                       80 * sizeof(TCHAR)); 
 
            // Retrieve the .ini (or registry) file name. 
            LoadString(hMainInstance, idsIniFile, szIniFile, 
                       MAXFILELEN * sizeof(TCHAR)); 
 
            // TODO: Add error checking to verify LoadString success
            //       for both calls.
      
            // Retrieve any redraw speed data from the registry. 
            lSpeed = GetPrivateProfileInt(szAppName, szRedrawSpeed, 
                                          DEFVEL, szIniFile); 
 
            // If the initialization file does not contain an entry 
            // for this screen saver, use the default value. 
            if(lSpeed > MAXVEL || lSpeed < MINVEL) 
                lSpeed = DEFVEL; 
 
            // Initialize the redraw speed scroll bar control.
            hSpeed = GetDlgItem(hDlg, ID_SPEED); 
            SetScrollRange(hSpeed, SB_CTL, MINVEL, MAXVEL, FALSE); 
            SetScrollPos(hSpeed, SB_CTL, lSpeed, TRUE); 
 
            // Retrieve a handle to the OK push button control.  
            hOK = GetDlgItem(hDlg, ID_OK); 
 
            return TRUE; 
 
        case WM_HSCROLL: 

            // Process scroll bar input, adjusting the lSpeed 
            // value as appropriate. 
            switch (LOWORD(wParam)) 
                { 
                    case SB_PAGEUP: 
                        --lSpeed; 
                    break; 
 
                    case SB_LINEUP: 
                        --lSpeed; 
                    break; 
 
                    case SB_PAGEDOWN: 
                        ++lSpeed; 
                    break; 
 
                    case SB_LINEDOWN: 
                        ++lSpeed; 
                    break; 
 
                    case SB_THUMBPOSITION: 
                        lSpeed = HIWORD(wParam); 
                    break; 
 
                    case SB_BOTTOM: 
                        lSpeed = MINVEL; 
                    break; 
 
                    case SB_TOP: 
                        lSpeed = MAXVEL; 
                    break; 
 
                    case SB_THUMBTRACK: 
                    case SB_ENDSCROLL: 
                        return TRUE; 
                    break; 
                } 

                if ((int) lSpeed <= MINVEL) 
                    lSpeed = MINVEL; 
                if ((int) lSpeed >= MAXVEL) 
                    lSpeed = MAXVEL; 
 
                SetScrollPos((HWND) lParam, SB_CTL, lSpeed, TRUE); 
            break; 
 
        case WM_COMMAND: 
            switch(LOWORD(wParam)) 
            { 
                case ID_OK: 
 
                    // Write the current redraw speed variable to
                    // the .ini file. 
                    hr = StringCchPrintf(szTemp, 20, "%ld", lSpeed);
                    if (SUCCEEDED(hr))
                        WritePrivateProfileString(szAppName, szRedrawSpeed, 
                                                  szTemp, szIniFile); 
 
                case ID_CANCEL: 
                    EndDialog(hDlg, LOWORD(wParam) == ID_OK); 

                return TRUE; 
            } 
    } 
*/
    return FALSE; 
}

BOOL WINAPI RegisterDialogClasses (HANDLE hInst)
{ 
    return TRUE; 
}

