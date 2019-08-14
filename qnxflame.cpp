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
 
//CHAR   szAppName[80];             // .ini section name             
CHAR   szTemp[20];                // temporary array of characters  
CHAR   szRedrawSpeed[ ] = "Redraw Speed";   // .ini speed entry 
CHAR   szIniFile[MAXFILELEN];     // .ini or registry file name  

HDC scr;

static double f[2][3][MAXLEV];  /* three non-homogeneous transforms */
static int max_total;
static int max_levels;
static int max_points;
static int cur_level;
static int snum;
static int anum;
static int num_points;
static int total_points;
static int pixcol;
static int npixels;
static COLORREF *pixels;
static POINT points [POINT_BUFFER_SIZE];
SIZE screen;

static int delay2 = 2000;
static int width, height;

int opt_points = 10000;
int opt_iterations = 25;
int opt_ncolors = 128;
COLORREF foreground, background, opt_foreground, opt_background;

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

static COLORREF HSV(int byHue, int bySaturation, int byValue) 
      {
        // HSV contains values scaled as in the color wheel:
        // that is, all from 0 to 255. 

        // for ( this code to work, HSV.Hue needs
        // to be scaled from 0 to 360 (it//s the angle of the selected
        // point within the circle). HSV.Saturation and HSV.value must be 
        // scaled to be between 0 and 1.

        double h;
        double s;
        double v;

        double r = 0;
        double g = 0;
        double b = 0;

        // Scale Hue to be between 0 and 360. Saturation
        // and value scale to be between 0 and 1.
        h = ((double) byHue / 255 * 360);
        while (h>=360){h-=360;}
        s = (double) bySaturation / 255;
        v = (double) byValue / 255;

        if ( s == 0 ) 
        {
          // If s is 0, all colors are the same.
          // This is some flavor of gray.
          r = v;
          g = v;
          b = v;
        } 
        else 
        {
          double p;
          double q;
          double t;

          double fractionalSector;
          int sectorNumber;
          double sectorPos;

          // The color wheel consists of 6 sectors.
          // Figure out which sector you//re in.
          sectorPos = h / 60;
          sectorNumber = (int)(floor(sectorPos));

          // get the fractional part of the sector.
          // That is, how many degrees into the sector
          // are you?
          fractionalSector = sectorPos - sectorNumber;

          // Calculate values for the three axes
          // of the color. 
          p = v * (1 - s);
          q = v * (1 - (s * fractionalSector));
          t = v * (1 - (s * (1 - fractionalSector)));

          // Assign the fractional colors to r, g, and b
          // based on the sector the angle is in.
          switch (sectorNumber) 
          {
            case 0:
              r = v;
              g = t;
              b = p;
              break;

            case 1:
              r = q;
              g = v;
              b = p;
              break;

            case 2:
              r = p;
              g = v;
              b = t;
              break;

            case 3:
              r = p;
              g = q;
              b = v;
              break;

            case 4:
              r = t;
              g = p;
              b = v;
              break;

            case 5:
              r = v;
              g = p;
              b = q;
              break;
          }
        }
        // return an RGB structure, with values scaled
        // to be between 0 and 255.
        return RGB((int)(r * 255), (int)(g * 255), (int)(b * 255));
      }

static void DrawPixelArray( PPOINT points, int num_points)
{
  int i;
  for ( i = 0; i < num_points; i++ )
    SetPixel( scr, points[i].x, points[i].y, pixels [pixcol] );
}

static void init_flame ( )
{
  max_points = opt_iterations;
  if (max_points <= 0) max_points = 100;

  max_levels = max_points;

  max_total = opt_points;
  if (max_total <= 0) max_total = 10000;

  if (delay2 < 0) delay2 = 0;

    {
      int i = opt_ncolors;
      int saturation = 255;
      int value = 255;
      COLORREF color;
      if (i <= 0) i = 128;

      pixels = (COLORREF *) malloc ((i+1) * sizeof (*pixels));
      for (npixels = 0; npixels < i; npixels++)
  {
   color =  HSV((0xffff*npixels)/i, saturation, value );
    pixels [npixels] = color;
  }
    }

  foreground = opt_foreground;
  background = opt_background;

    {
      pixcol = halfrandom (npixels);
      foreground = (pixels [pixcol]);
    }
}

static int recurse ( register double x,register double  y, register int l )
{
  int xp, yp, i;
  double nx, ny;

  if (l == max_levels)
    {
      total_points++;
      if (total_points > max_total) /* how long each fractal runs */
  return 0;

      if (x > -1.0 && x < 1.0 && y > -1.0 && y < 1.0)
  {
    xp = points[num_points].x = (short) ((width / 2) * (x + 1.0));
    yp = points[num_points].y = (short) ((height / 2) * (y + 1.0));
    num_points++;
    if (num_points >= POINT_BUFFER_SIZE)
      {
        DrawPixelArray (points, num_points);
        num_points = 0;
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
    if (!recurse (nx, ny, l + 1))
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

  if (!(cur_level++ % max_levels))
    {
      if (delay2) Sleep (delay2);
      rc.left = 0; rc.top = 0; rc.right = width; rc.bottom = height;
      FillRect (scr, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH)); 
      alt = !alt;
    }
  else
    {
      if (npixels > 2)
  {
    if (--pixcol < 0)
      pixcol = npixels - 1;
  }
    }

  /* number of functions */
  snum = 2 + (cur_level % (MAXLEV - 1));

  /* how many of them are of alternate form */
  if (alt)
    anum = 0;
  else
    anum = halfrandom (snum) + 2;

  /* 6 coefs per function */
  for (k = 0; k < snum; k++)
    {
      for (i = 0; i < 2; i++)
  for (j = 0; j < 3; j++)
    f[i][j][k] = ((double) (rand() & 1023) / 512.0 - 1.0);
    }
  num_points = 0;
  total_points = 0;
  (void) recurse (0.0, 0.0, 0);
  DrawPixelArray( points, num_points);
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
            lSpeed = 10;
 
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
              }
              else
              {
                uTimer = SetTimer(hwnd, 1, lSpeed * 10, NULL);
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

