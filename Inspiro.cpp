#pragma warning(disable : 4996)

#define NAME "Inspiro"
#define TITLE "Inspiro"
#define ShowDelayIntro 2000

//#define DEBUG

#define WIN32_LEAN_AND_MEAN

#include "Inspiro.h"				// Haupt-Includedatei

LPDIRECTDRAW		lpDD;			// DirectDraw Object
LPDIRECTDRAWSURFACE lpDDSPrimary;	// DirectDraw primary surface
LPDIRECTDRAWSURFACE lpDDSOffScreen;	// DirectDraw offscreen surface
LPDIRECTDRAWSURFACE lpDDSIntro;		// DirectDraw offscreen surface
LPDIRECTDRAWSURFACE lpDDSMenu;		// DirectDraw offscreen surface

const int TIMER_ID				= 1;
const int TIMER_RATE			= 100;
unsigned int Time				= 0;
const unsigned int UPDATE_TIME	= 10;	// für Skelett und Stacheln

HWND hWnd;

LPSTR PlayerDirection;					// DOWN, UP, RIGHT, LEFT
int PlayerPosition			= 0;		// aktuelle Position des Spielers
int CursorPosition			= 0;
int PlayerAlive				= TRUE;   
int Feld[300];							// Enthält das komplette Level
int EditorFeld[300];					// Enthält ein Level im Editor-Modus
int FeldStachelnState[300];				// Speichert den Zustand aller mechanischen Stacheln
int Lives					= 0;		// Anzahl Leben
int Gold					= 0;		// Anzahl Gold
int Keys					= 0;		// Anzahl Schlüssel
BOOL Editor					= FALSE;	// Editormodus?
int SelectedObject;						// Ausgewähltes Objekt im Editormodus
BOOL bActive				= FALSE;
BOOL Spielmodus				= FALSE;
BOOL Skelett				= FALSE;
BOOL Menu					= TRUE;

LPSTR lpScenarioFile = NULL;
LPSTR lpScenarioFile_1  = "res\\Level1.szn";
LPSTR lpScenarioFile_2  = "res\\Level2.szn";
LPSTR lpScenarioFile_3  = "res\\Level3.szn";
LPSTR lpScenarioFile_4	= "res\\Level4.szn";
LPSTR lpNewScenarioFile = "res\\NewLevel.szn";	// Neues Level wird hier gespeichert

RECT Src;								// Quellrechteck
RECT Targ;								// Zielrechteck


//=====================================================================
// LoadBitmap
//
// Lädt eine Bitmap Datei in die Oberfläche
//=====================================================================

BOOL LoadBitmap( LPDIRECTDRAWSURFACE lpDDS, LPSTR szImage )
{
    HBITMAP       hbm;
    HDC           hdcImage = NULL;
    HDC           hdcSurf  = NULL;
    BOOL          bReturn = FALSE;
    DDSURFACEDESC ddsd;
    
    ZeroMemory( &ddsd, sizeof( ddsd ) );
    ddsd.dwSize = sizeof( ddsd );
    
    if( FAILED( lpDDS->GetSurfaceDesc( &ddsd ) ) )
    {
        goto Exit;
    }

    // If the pixel format isn't some flavor of RGB, we can't handle 
    // it.
    if ( ( ddsd.ddpfPixelFormat.dwFlags != DDPF_RGB ) ||
         ( ddsd.ddpfPixelFormat.dwRGBBitCount < 16 ) )
    {
        OutputDebugString( "Non-palettized RGB mode required.\n" );
        goto Exit;
    }
 
    // Try loading the image
    hbm = ( HBITMAP )LoadImage( NULL, szImage,
            IMAGE_BITMAP, ddsd.dwWidth,
            ddsd.dwHeight, LR_LOADFROMFILE | LR_CREATEDIBSECTION );

    if ( hbm == NULL )
    {
        OutputDebugString( "Couldn't find the resource.\n" );
        goto Exit;
    }

    // Create a DC and select the image into it
    hdcImage = CreateCompatibleDC( NULL );
    SelectObject( hdcImage, hbm );

    // Get a DC for the surface
    if ( FAILED( lpDDS->GetDC( &hdcSurf ) ) )
    {
        OutputDebugString( "Couldn't get a DC.\n" );
        goto Exit;
    }

    // The BitBlt will perform format conversion as necessary
    if ( BitBlt( hdcSurf, 0, 0, ddsd.dwWidth, ddsd.dwHeight,
         hdcImage, 0, 0,SRCCOPY ) == FALSE )
    {
        OutputDebugString( "Blt failed.\n" );
        goto Exit;
    }

    // Success
    bReturn = TRUE;

Exit:
    // Clean up everything
    if ( hdcSurf )
       lpDDS->ReleaseDC( hdcSurf );
    if ( hdcImage )
       DeleteDC( hdcImage );
    if ( hbm )
       DeleteObject( hbm );

    return bReturn;

} // LoadBitmap   

//=====================================================================
// ReleaseDXObjects
//
// Gibt den Speicher der DirectX Objekte wieder frei
//=====================================================================

static void ReleaseDXObjects( void )
{
    if (  lpDD != NULL )
    {
        if ( lpDDSPrimary != NULL )
        {
            if ( lpDDSOffScreen != NULL )
            {
                lpDDSOffScreen->Release();
                lpDDSOffScreen = NULL;
            }

			if ( lpDDSIntro != NULL )
			{
				lpDDSIntro->Release();
				lpDDSIntro = NULL;
			}

			if ( lpDDSMenu != NULL )
			{
				lpDDSMenu->Release();
				lpDDSMenu = NULL;
			}

            lpDDSPrimary->Release();
            lpDDSPrimary = NULL;
         }
         lpDD->Release();
         lpDD = NULL;
     }
} // ReleaseDXObjects

//=====================================================================
// Fail
//
// Prozedur zur Ausgabe von Fehlern
//=====================================================================

BOOL Fail( char *szMsg )
{
    ReleaseDXObjects();
    OutputDebugString( szMsg );
    MessageBox( hWnd, szMsg, "Error" , MB_OK );
    DestroyWindow( hWnd );

    return FALSE;

}  // Fail

//=====================================================================
// WindowProc
//
// Funktion zur Auswertung der Nachrichten
//=====================================================================

long FAR PASCAL WindowProc( HWND hWnd, UINT message,
                            WPARAM wParam, LPARAM lParam )
{
	switch ( message )
	{
		case WM_SETCURSOR: 
			SetCursor( NULL );  // Turn off the mouse cursor
			return TRUE;

		case WM_ACTIVATEAPP:
			bActive = wParam;
			break;

		case WM_KEYDOWN:
			switch (wParam)
			{
			case VK_ESCAPE:
				if (Menu)
				{
					PostMessage(hWnd, WM_CLOSE, 0, 0);
					break;
				}

				if (Editor)
				{
					Save();
					Editor = FALSE;
				}

				Spielmodus = FALSE;
				ShowMenu();

				break;

			case VK_NUMPAD1:
				lpScenarioFile = lpScenarioFile_1;
				ClearScreen(0, 0, 640, 480);
				LoadLevel();
				ShowInventar();
				Lives = 5;
				Gold = 0;
				Keys = 0;
				PlayerAlive = TRUE;
				Editor = FALSE;
				UpdateInventar(Lives, Gold, Keys);
				Spielmodus = TRUE;
				Menu = FALSE;
				break;

			case VK_NUMPAD2:
				lpScenarioFile = lpScenarioFile_2;
				ClearScreen(0, 0, 640, 480);
				LoadLevel();
				ShowInventar();
				Lives = 5;
				Gold = 0;
				Keys = 0;
				PlayerAlive = TRUE;
				Editor = FALSE;
				UpdateInventar(Lives, Gold, Keys);
				Spielmodus = TRUE;
				Menu = FALSE;
				break;

			case VK_NUMPAD3:
				lpScenarioFile = lpScenarioFile_3;
				ClearScreen(0, 0, 640, 480);
				LoadLevel();
				ShowInventar();
				Lives = 5;
				Gold = 0;
				Keys = 0;
				PlayerAlive = TRUE;
				Editor = FALSE;
				UpdateInventar(Lives, Gold, Keys);
				Spielmodus = TRUE;
				Menu = FALSE;
				break;

			case VK_NUMPAD4:
				lpScenarioFile = lpScenarioFile_4;
				ClearScreen(0, 0, 640, 480);
				LoadLevel();
				ShowInventar();
				Lives = 5;
				Gold = 0;
				Keys = 0;
				PlayerAlive = TRUE;
				Editor = FALSE;
				UpdateInventar(Lives, Gold, Keys);
				Spielmodus = TRUE;
				Menu = FALSE;
				break;

			case VK_F1:
				if (!Editor)
					RunEditor();
				break;

			case VK_F2:
				if (Editor)
				{
					// Speichern, bevor es losgeht
					Save();
		
					lpScenarioFile = lpNewScenarioFile;
					ClearScreen(0, 0, 640, 480);
					LoadLevel();
					ShowInventar();
					Lives = 5;
					Gold = 0;
					Keys = 0;
					PlayerAlive = TRUE;
					Editor = FALSE;
					UpdateInventar(Lives, Gold, Keys);
					Spielmodus = TRUE;
					Menu = FALSE;
				}
				break;

				case VK_UP:
					if ( PlayerAlive == TRUE && Menu != TRUE  )
					MoveUp();				
					break;

				case VK_DOWN:
					if ( PlayerAlive == TRUE && Menu != TRUE ) 
					MoveDown();
					break;
				   
				case VK_RIGHT:
					if ( PlayerAlive == TRUE && Menu != TRUE ) 
					MoveRight();
					break;

				case VK_LEFT:
					if ( PlayerAlive == TRUE && Menu != TRUE ) 
					MoveLeft();
					break;

				case VK_RETURN:
					PlaceObject( CursorPosition, SelectedObject );
					break;

				case VK_PRIOR:  // Bildtaste nach oben
					if ( Editor == TRUE && SelectedObject > 1 )
					{
						SelectedObject--;
						DrawObject(GetObjectNameById(SelectedObject)[0], GetObjectNameById(SelectedObject)[1], 600, 60);
					}
					break;

				case VK_NEXT:  // Bildtaste nach unten
					if ( Editor == TRUE && SelectedObject < 18 )
					{
						SelectedObject++;
						DrawObject(GetObjectNameById(SelectedObject)[0], GetObjectNameById(SelectedObject)[1], 600, 60);
					}
					break;
			}
			break;

		case WM_DESTROY:
			ReleaseDXObjects();
			PostQuitMessage( 0 );
			break; 
		
		case WM_TIMER:

			if ( bActive && Spielmodus && Skelett )
			{
				if ( Time == UPDATE_TIME )
				{
					UpdateSkelett();
					UpdateStacheln();
					Time = 0;
				}
				Time++;
			}		
				
			break;
	}

	return DefWindowProc( hWnd, message, wParam, lParam );

}  // WindowProc 

//=====================================================================
// doInit
// 
// Führt alle Initializierungen des Fensters durch
//=====================================================================

static BOOL doInit( HINSTANCE hInstance, int nCmdShow )
{
    WNDCLASS wc;
    
    // Set up and register window class
    wc.style = CS_HREDRAW | CS_VREDRAW;  
    wc.lpfnWndProc = WindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon( hInstance, "IDI_ICON1" );
    wc.hCursor = LoadCursor( NULL, "IDC_CURSOR" );
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NAME;
    wc.lpszClassName = NAME;
    RegisterClass( &wc );
	
    // Create a fullscreen window
    hWnd = CreateWindowEx(
#ifdef DEBUG
        0,
#else
		WS_EX_TOPMOST,
#endif
        NAME,
        TITLE,
#ifdef DEBUG
		WS_OVERLAPPEDWINDOW,
#else
        WS_POPUP,
#endif
        0, 0,
#ifdef DEBUG
		640,
		480,
#else
        GetSystemMetrics( SM_CXSCREEN ),
        GetSystemMetrics( SM_CYSCREEN ),
#endif
        NULL,
        NULL,
        hInstance,
        NULL );
    
    if ( !hWnd )
    {
        return FALSE;
    }

    ShowWindow( hWnd, nCmdShow );
    UpdateWindow( hWnd );
	
	if ( !doInitDirectDraw( hWnd ) )
	{
		return Fail( "Fehler beim Initialisieren von DDraw.\n" );
	}

	ShowIntro();
	Sleep( ShowDelayIntro );

	if ( !ShowMenu() )
	{
		return Fail( "Fehler beim Anzeigen des Menüs.\n" );
	}

    return ( bActive = TRUE );

}  // doInit

//=====================================================================
// doInitDirectDraw
//
// Initialization von DirectDraw
//=====================================================================

static BOOL doInitDirectDraw( HWND hWnd )
{
	DDSURFACEDESC ddsd;
	
    // Create the DirectDraw object -- we just need an IDirectDraw
    // interface so we won't bother to query an IDirectDraw2
    if ( FAILED( DirectDrawCreate( NULL, &lpDD, NULL ) ) )
    {
        return Fail( "Couldnt create DirectDraw object.\n" );
    }
 
    // Get exclusive mode
#ifdef DEBUG
	if (FAILED(lpDD->SetCooperativeLevel(hWnd, DDSCL_NORMAL ) ) )
#else
	if (FAILED(lpDD->SetCooperativeLevel(hWnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN ) ) )
#endif
    {
        return Fail( "Couldn't set cooperative level.\n" );
    }

#ifndef DEBUG
    // RGB-Videomodi von 32 bis 16 Bit der Reihe nach durchprobieren
    if ( FAILED( lpDD->SetDisplayMode( 640, 480, 32 ) ) )
    {
        // 32 Bit nicht verfügbar. Unterstützt das System 24 Bit?
        if ( FAILED( lpDD->SetDisplayMode( 640, 480, 24 ) ) )
        {  
            //Wieder nichts. Geht es wenigstens mit 16 Bit?
            if ( FAILED( lpDD->SetDisplayMode( 640, 480, 16 ) ) )
            {
                return Fail( "Fehler beim Setzen des Videomodus.\n" );
			}
        }
    }
#endif

    // Erstelle die primäre Oberfläche
    ddsd.dwSize = sizeof( ddsd );
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    if ( FAILED( lpDD->CreateSurface( &ddsd, &lpDDSPrimary, NULL ) ) )
    {
        return Fail( "Couldn't create primary surface.\n" );
    }

	// Erstelle Oberfläche außerhalb des Bildschirms
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT |
                   DDSD_WIDTH;

    // Oberfläche ist außerhalb des Bildschirms
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwHeight = 90;
    ddsd.dwWidth = 420;

    // ...und die Oberfläche anlegen
    if ( FAILED( lpDD->CreateSurface( &ddsd, &lpDDSOffScreen, NULL ) ) )
    {
        return Fail( "Fehler beim Anlegen der Oberfläche außerhalb des Bildschirms.\n" );
    }

	// Intro-Oberfläche anlegen
	if ( FAILED( lpDD->CreateSurface( &ddsd, &lpDDSIntro, NULL ) ) )
	{
		return Fail( "Fehler beim Anlegen der Oberfläche außerhalb des Bildschirms.\n" );
	}

    // Bilddatei 'tileset.bmp' laden
    if ( !LoadBitmap( lpDDSOffScreen, "res\\tileset.bmp" ) )
    {
        return Fail( "Konnte 'tileset.bmp' nicht laden.\n" );
    }

	// Bilddatei 'intro.bmp' laden
	if ( !LoadBitmap( lpDDSIntro, "res\\intro.bmp" ) )
	{
		return Fail( "Konnte 'intro.bmp' nicht laden.\n" );
	}
	
	// Erstelle Menü-Oberfläche außerhalb des Bildschirms
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT |
                   DDSD_WIDTH;

    // Menü-Oberfläche ist außerhalb des Bildschirms
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwHeight = 422;
    ddsd.dwWidth = 625;

    // ...und die Oberfläche anlegen
    if ( FAILED( lpDD->CreateSurface( &ddsd, &lpDDSMenu, NULL ) ) )
    {
        return Fail( "Fehler beim Anlegen der Menü-Oberfläche außerhalb des Bildschirms.\n" );
    }

	// Bilddatei 'menu.bmp' laden
    if ( !LoadBitmap( lpDDSMenu, "res\\menu.bmp" ) )
    {
        return Fail( "Konnte 'menu.bmp' nicht laden.\n" );
    }

	//  Create a timer 
	if( SetTimer( hWnd, TIMER_ID, TIMER_RATE, NULL ) )
	{
		return TRUE;
	}

    return TRUE;

}  // InitDirectDraw

//=====================================================================
// LoadLevel
//
// Lädt ein Level
//=====================================================================

void LoadLevel()
{
    FILE *myFile;
	char Objekt[2];
    int x = 0;
    int y = 0;
	short ret;
	int i;

    if ( ( myFile = fopen(lpScenarioFile, "r" ) ) == NULL )
    {
        Fail( "Fehler beim Öffnen der Datei.\n" );            
    }

	for (i = 0; i <= 299; i++)
	{
		ret = fscanf(myFile, "%s", &Objekt);  // Lese Objekt

		char strObj[3];
		strObj[0] = Objekt[0];
		strObj[1] = Objekt[1];
		strObj[2] = '\0';

		Feld[i] = GetObjectIdByName((const char*)strObj);
		
		switch (Feld[i])
		{
			case 6: PlayerPosition = i; break;
			case 7: PlayerPosition = i; break;
			case 8: PlayerPosition = i; break;
			case 9: PlayerPosition = i; break;

			case 13: FeldStachelnState[i] = 1; break;

			case 15: Skelett = TRUE; break;
			case 16: Skelett = TRUE; break;
			case 17: Skelett = TRUE; break;
			case 18: Skelett = TRUE; break;
		}

		ret = fscanf(myFile, "%d", &x);	// Lese x-Position
		ret = fscanf(myFile, "%d", &y);  // Lese y-Position

		DrawObject(Objekt[0], Objekt[1], x, y);

		if (ret == EOF) break;
	}
  
	fclose( myFile );    

}  // LoadLevel

//===================================================================== 
// DrawObject
//
// Plaziert Objekt an angegebener Position
// Achtung!!! Diese Funktion ist nur für die Grafik
// verantwortlich aber nicht für Initialisierungen
// des Objekts
//=====================================================================

void DrawObject( char ch1 , char ch2, short x, short y )
{
	RECT rcRect;
    RECT rcObjekt;
	char Objekt[2];

	Objekt[0] = ch1;
	Objekt[1] = ch2;

	rcRect.left = x;
	rcRect.top = y;
	rcRect.right = x + 30;
	rcRect.bottom = y + 30;

	// Mauer
	if ( Objekt[0] == 'W' && Objekt[1] == 'L' )
    {     
        rcObjekt.left = 0;
        rcObjekt.top = 0;
        rcObjekt.right = 30; 
        rcObjekt.bottom = 30;
	}
 
	// Geld
	if ( Objekt[0] == 'G' && Objekt[1] == 'O' )
	{
        rcObjekt.left = 30;
        rcObjekt.top = 0;
        rcObjekt.right = 60; 
        rcObjekt.bottom = 30;
	}
  
	// Schlüssel
	if ( Objekt[0] == 'K' && Objekt[1] == 'Y' )
	{	        
        rcObjekt.left = 60;
        rcObjekt.top = 0;
        rcObjekt.right = 90; 
        rcObjekt.bottom = 30;
	}

	// Grabstein
	if ( Objekt[0] == 'R' && Objekt[1] == 'P' )
	{	        
        rcObjekt.left = 90;
        rcObjekt.top = 0;
        rcObjekt.right = 120; 
        rcObjekt.bottom = 30;
	}

	// Tür
	if ( Objekt[0] == 'D' && Objekt[1] == 'R' )
	{
        rcObjekt.left = 120;
        rcObjekt.top = 0;
        rcObjekt.right = 150; 
        rcObjekt.bottom = 30;
	}

	// Spieler von vorne
	if ( Objekt[0] == 'P' && Objekt[1] == 'V' )
	{
        rcObjekt.left = 150;
        rcObjekt.top = 0;
        rcObjekt.right = 180; 
        rcObjekt.bottom = 30;
                   
        PlayerDirection = "DOWN";
	}

	// Spieler von hinten
	if ( Objekt[0] == 'P' && Objekt[1] == 'H' )
	{
        rcObjekt.left = 180;
        rcObjekt.top = 0;
        rcObjekt.right = 210; 
        rcObjekt.bottom = 30;
                   
		PlayerDirection = "UP";
	}

	// Spieler von rechts
	if ( Objekt[0] == 'P' && Objekt[1] == 'R' )
	{
         rcObjekt.left = 210;
        rcObjekt.top = 0;
        rcObjekt.right = 240; 
        rcObjekt.bottom = 30;
                   
		PlayerDirection = "RIGHT";
	}

	// Spieler von links
    if ( Objekt[0] == 'P' && Objekt[1] == 'L' )
	{
        rcObjekt.left = 240;
        rcObjekt.top = 0;
        rcObjekt.right = 270; 
        rcObjekt.bottom = 30;
                   
		PlayerDirection = "LEFT";
	}

	// Löschfeld
    if ( Objekt[0] == 'C' && Objekt[1] == 'R' )
	{
        rcObjekt.left = 270;
        rcObjekt.top = 0;
        rcObjekt.right = 300; 
        rcObjekt.bottom = 30;
	}

	// Herz
	if ( Objekt[0] == 'H' && Objekt[1] == 'Z' )
	{
        rcObjekt.left = 300;
        rcObjekt.top = 0;
        rcObjekt.right = 330; 
        rcObjekt.bottom = 30;
	}

	// Stacheln
	if ( Objekt[0] == 'S' && Objekt[1] == 'T' )
	{
        rcObjekt.left = 330;
        rcObjekt.top = 0;
        rcObjekt.right = 360; 
        rcObjekt.bottom = 30;
	}

	// Pfeil für Editor
	if ( Objekt[0] == 'E' && Objekt[1] == 'D' )
	{
        rcObjekt.left = 360;
        rcObjekt.top = 0;
        rcObjekt.right = 390; 
        rcObjekt.bottom = 30;
	}

	// Grün - Hintergrund vom Editor
	if ( Objekt[0] == 'G' && Objekt[1] == 'R' )
	{
        rcObjekt.left = 390;
        rcObjekt.top = 0;
        rcObjekt.right = 420; 
        rcObjekt.bottom = 30;
	}

	// verschiebare Mauer
	if ( Objekt[0] == 'W' && Objekt[1] == 'V' )
	{
        rcObjekt.left = 0;
        rcObjekt.top = 0;
        rcObjekt.right = 30; 
        rcObjekt.bottom = 30;
	}

	// Skelett von vorne
	if ( Objekt[0] == 'S' && Objekt[1] == 'V' )
	{
        rcObjekt.left = 0;
        rcObjekt.top = 30;
        rcObjekt.right = 30; 
        rcObjekt.bottom = 60;
	}

	// Skelett von hinten
	if ( Objekt[0] == 'S' && Objekt[1] == 'H' )
	{
        rcObjekt.left = 30;
        rcObjekt.top = 30;
        rcObjekt.right = 60; 
        rcObjekt.bottom = 60;
	}

	// Skelett von rechts
	if ( Objekt[0] == 'S' && Objekt[1] == 'R' )
	{
        rcObjekt.left = 60;
        rcObjekt.top = 30;
        rcObjekt.right = 90; 
        rcObjekt.bottom = 60;
	}

	// Skelett von links
	if ( Objekt[0] == 'S' && Objekt[1] == 'L' )
	{
        rcObjekt.left = 90;
        rcObjekt.top = 30;
        rcObjekt.right = 120; 
        rcObjekt.bottom = 60;
	}

	// Pfeil nach oben - Objektauswahl im Editor
	if ( Objekt[0] == 'A' && Objekt[1] == 'O' )
	{
        rcObjekt.left = 120;
        rcObjekt.top = 30;
        rcObjekt.right = 150; 
        rcObjekt.bottom = 60;
	}

	// Pfeil nach unten - Objektauswahl im Editor
	if ( Objekt[0] == 'A' && Objekt[1] == 'U' )
	{
        rcObjekt.left = 150;
        rcObjekt.top = 30;
        rcObjekt.right = 180; 
        rcObjekt.bottom = 60;
	}

	// Stacheln aus dem Boden
	if ( Objekt[0] == 'S' && Objekt[1] == 'B' )
	{
        rcObjekt.left = 300;
        rcObjekt.top = 30;
        rcObjekt.right = 330; 
        rcObjekt.bottom = 60;
	}

	lpDDSPrimary->Blt( &rcRect, lpDDSOffScreen, &rcObjekt,
      				        DDBLT_WAIT, NULL );     		
}  // DrawObject

//=====================================================================
// ShowMenu
//
// Zeigt das Hauptmenü an
//=====================================================================

static BOOL ShowMenu()
{
	if ( FAILED( lpDDSPrimary->Blt( NULL, lpDDSMenu, NULL,
      				           DDBLT_WAIT, NULL ) ) )
	{
		return Fail( "Fehler beim Blt des Menüs.\n" );
	}

	Spielmodus = FALSE;

    return (Menu = TRUE);

}  // ShowMenu

//=====================================================================
// ClearScreen
//
// Löscht angegebenen Bildschirmbereich
//=====================================================================

void ClearScreen( int x, int y, int breite, int hoehe )
{
	RECT rcRect;
    RECT rcObjekt;

	rcRect.left = x;
    rcRect.top = y; 
    rcRect.right = x + breite;
    rcRect.bottom = y + hoehe;

    rcObjekt.left = 270;
    rcObjekt.top = 0;
    rcObjekt.right = 300; 
    rcObjekt.bottom = 30;
                   
    lpDDSPrimary->Blt( &rcRect, lpDDSOffScreen, &rcObjekt,
      				   DDBLT_WAIT, NULL );

}  // ClearScreen

//=====================================================================
// ShowInventar
//
// Zeigt das Inventar an
//=====================================================================

void ShowInventar()
{
	DrawObject( 'H', 'Z', 160, 0 );
	DrawObject( 'G', 'O', 390, 0 );
	DrawObject( 'K', 'Y', 450, 0 );

}  // ShowInventar

//=====================================================================
// UpdateInventar
//
// Aktualisiert das Inventar
//=====================================================================

static BOOL UpdateInventar(int lives, int gold, int keys )
{
	HDC hDC;
	char buf[5];

	// Gerätekontext abholen
	if FAILED( lpDDSPrimary->GetDC( &hDC ) )
	{
		return Fail( "Kein DC verfügbar.\n" );
	}

	SetBkColor( hDC, RGB( 0, 0, 0 ) );
	SetTextColor( hDC, RGB( 200, 200, 200 ) );
	SetTextAlign( hDC, TA_CENTER );

	sprintf ( buf, "%d", lives );
	TextOut( hDC, 140, 10, buf, lstrlen( buf ) );

	sprintf ( buf, "%d", gold );
	TextOut( hDC, 370, 10, buf, lstrlen( buf ) );
	
	sprintf ( buf, "%d", keys );
	TextOut( hDC, 430, 10, buf, lstrlen( buf ) );

	lpDDSPrimary->ReleaseDC( hDC );

	return TRUE;

}  // UpdateInventar

//=====================================================================
// MoveUp
//
// Bewegt Spieler um ein Feld nach vorne
//=====================================================================

void MoveUp()
{
	if ( Editor && Spielmodus != TRUE && GetY( CursorPosition ) > 30 )
	{
		RestoreFeld(CursorPosition);
		CursorPosition = CursorPosition - 20;
		DrawObject( 'E', 'D', GetX( CursorPosition ), GetY( CursorPosition ) );

		return;
	}

	if (GetY(PlayerPosition) == 30)
	{
		PlaceObject(PlayerPosition, GetObjectIdByName("PH"));

		return;
	}

    // Tür
	if ( Editor == FALSE && ( Keys != 0 ) &&  Feld[PlayerPosition - 20] == 5 )
	{
		Keys -= 1;
		UpdateInventar( Lives, Gold, Keys );
		DeleteObject( PlayerPosition );
		PlayerPosition = PlayerPosition - 20;  // Neue Position
		PlaceObject(PlayerPosition, GetObjectIdByName("PH"));

		return;
	}
 
	// Schlüssel
    if ( Editor == FALSE && Feld[PlayerPosition - 20] == 3 )
	{
		Keys += 1;
		UpdateInventar( Lives, Gold, Keys );
		DeleteObject( PlayerPosition );
		PlayerPosition = PlayerPosition - 20;  // Neue Position
		PlaceObject(PlayerPosition, GetObjectIdByName("PH"));

		return;
	}
		
	// Geld
	if ( Editor == FALSE && Feld[PlayerPosition - 20] == 2 )
	{
		Gold += 1;
		UpdateInventar( Lives, Gold, Keys );
		DeleteObject( PlayerPosition );
		PlayerPosition = PlayerPosition - 20;  // Neue Position
		PlaceObject(PlayerPosition, GetObjectIdByName("PH"));

		return;
	}
	    
	// Herz
	if ( Editor == FALSE && Feld[PlayerPosition - 20] == 11 )
	{
		Lives += 1;
		UpdateInventar( Lives, Gold, Keys );
		DeleteObject( PlayerPosition );
		PlayerPosition = PlayerPosition - 20;  // Neue Position
		PlaceObject(PlayerPosition, GetObjectIdByName("PH"));

		return;
	}
        
	// Nichts
	if ( Editor == FALSE && Feld[PlayerPosition - 20] == 10 )
	{
		DeleteObject( PlayerPosition );
		PlayerPosition = PlayerPosition - 20;  // Neue Position
		PlaceObject(PlayerPosition, GetObjectIdByName("PH"));

		return;
	}

	// Stacheln
	if ( Editor == FALSE && Feld[PlayerPosition - 20] == 12 )  // Ist eine Mauer im Weg? 
	{
		DeleteObject( PlayerPosition );
		PlayerPosition = PlayerPosition - 20;  // Neue Position
		PlaceObject(PlayerPosition, GetObjectIdByName("RP"));
		PlayerAlive = FALSE;  // Spieler tot

		return;
	}

	// mechanische Stacheln
	if (Editor == FALSE && Feld[PlayerPosition - 20] == 13)
	{
		DeleteObject(PlayerPosition);
		PlayerPosition = PlayerPosition - 20;  // Neue Position

		if (FeldStachelnState[PlayerPosition])
		{
			PlaceObject(PlayerPosition, GetObjectIdByName("RP"));
			PlayerAlive = FALSE;  // Spieler tot
		}
		else
		{
			DrawObject('P', 'H', GetX(PlayerPosition), GetY(PlayerPosition));
		}

		return;
	}

	// verschiebbare Mauer
	if ( Editor == FALSE && Feld[PlayerPosition - 20] == 14 && Feld[PlayerPosition - 40] == 10 && GetY( PlayerPosition ) != 60  )  
	{
		DeleteObject( PlayerPosition );
		PlaceObject(PlayerPosition - 20, GetObjectIdByName("PH"));
		PlaceObject(PlayerPosition - 40, GetObjectIdByName("WV"));
		PlayerPosition = PlayerPosition - 20;  // Neue Position	   

		return;
	}

	// Mauer
	if (Editor == FALSE && Feld[PlayerPosition - 20] == 1 )
	{
		DeleteObject(PlayerPosition);
		PlaceObject(PlayerPosition, GetObjectIdByName("PH"));

		return;
	}
}  // MoveUp

//=====================================================================
// MoveDown
//
// Bewegt Spieler um ein Feld nach unten
//=====================================================================

void MoveDown()
{
	if ( Editor && Spielmodus != TRUE && GetY( CursorPosition ) < 450 )
	{
		RestoreFeld(CursorPosition);
		CursorPosition = CursorPosition + 20;		
		DrawObject( 'E', 'D', GetX( CursorPosition ), GetY( CursorPosition ) );

		return;
	}

	if (GetY(PlayerPosition) == 450)
	{
		PlaceObject(PlayerPosition, GetObjectIdByName("PV"));
		return;
	}
	
	// Tür
    if ( Editor == FALSE && ( Keys != 0 ) && Feld[PlayerPosition + 20] == 5 )
	{
		Keys -= 1;
		UpdateInventar( Lives, Gold, Keys );
		DeleteObject( PlayerPosition );
		PlayerPosition = PlayerPosition + 20;  // Neue Position
		PlaceObject(PlayerPosition, GetObjectIdByName("PV"));
	
		return;
	}

	// Schlüssel
	if ( Editor == FALSE && Feld[PlayerPosition + 20] == 3 )
	{
		Keys += 1;
		UpdateInventar( Lives, Gold, Keys );
		DeleteObject( PlayerPosition );
		PlayerPosition = PlayerPosition + 20;  // Neue Position
		PlaceObject(PlayerPosition, GetObjectIdByName("PV"));
	
		return;
	}
	    
	// Geld
	if ( Editor == FALSE && Feld[PlayerPosition + 20] == 2 )
	{

		Gold += 1;
		UpdateInventar( Lives, Gold, Keys );
		DeleteObject( PlayerPosition );
		PlayerPosition = PlayerPosition + 20;  // Neue Position
		PlaceObject(PlayerPosition, GetObjectIdByName("PV"));
	
		return;
	}
		
	// Herz
	if ( Editor == FALSE && Feld[PlayerPosition + 20] == 11 )
	{
		Lives += 1;
		UpdateInventar( Lives, Gold, Keys );
		DeleteObject( PlayerPosition );
		PlayerPosition = PlayerPosition + 20;  // Neue Position
		PlaceObject(PlayerPosition, GetObjectIdByName("PV"));

		return;
	}

	// Nichts
	if ( Editor == FALSE && Feld[PlayerPosition + 20] == 10 )  
	{
		DeleteObject( PlayerPosition );
		PlayerPosition = PlayerPosition + 20;  // Neue Position  
		PlaceObject(PlayerPosition, GetObjectIdByName("PV"));

		return;
	}

	// Stacheln
	if ( Editor == FALSE && Feld[PlayerPosition + 20] == 12 ) 
	{
		DeleteObject( PlayerPosition );
		PlayerPosition = PlayerPosition + 20;  // Neue Position
		PlaceObject(PlayerPosition, GetObjectIdByName("RP"));
		PlayerAlive = FALSE;  // Spieler tot

		return;
	}

	// mechanische Stacheln
	if (Editor == FALSE && Feld[PlayerPosition + 20] == 13)
	{
		DeleteObject(PlayerPosition);
		PlayerPosition = PlayerPosition + 20;  // Neue Position

		if (FeldStachelnState[PlayerPosition])
		{
			PlaceObject(PlayerPosition, GetObjectIdByName("RP"));
			PlayerAlive = FALSE;  // Spieler tot
		}
		else
		{
			DrawObject('P', 'V', GetX(PlayerPosition), GetY(PlayerPosition));
		}

		return;
	}

	// verschiebbare Mauer
	if ( Editor == FALSE && Feld[PlayerPosition + 20] == 14 && Feld[PlayerPosition + 40] == 10 && GetY( PlayerPosition ) != 420  )  
	{
		DeleteObject(PlayerPosition + 20);  // Mauer löschen
		PlaceObject(PlayerPosition + 40, GetObjectIdByName("WV"));
		DeleteObject(PlayerPosition);  // Spieler löschen
		PlayerPosition = PlayerPosition + 20;  // Neue Position
		PlaceObject(PlayerPosition, GetObjectIdByName("PV"));

		return;
	}

	// Mauer
	if ( Editor == FALSE && Feld[PlayerPosition + 20] == 1 )
	{
		DeleteObject( PlayerPosition );		   
		PlaceObject(PlayerPosition, GetObjectIdByName("PV"));

		return;
	}

}  // MoveDown

//=====================================================================
// MoveRight
//
// Bewegt Spieler um ein Feld nach rechts
//=====================================================================

void MoveRight()
{
	if ( Editor && Spielmodus != TRUE &&  GetX( CursorPosition ) < 570 )
	{
		RestoreFeld(CursorPosition);
		CursorPosition = CursorPosition + 1;	
		DrawObject( 'E', 'D', GetX( CursorPosition ), GetY( CursorPosition ) );

		return;
	}

	if (GetX(PlayerPosition) == 570)
	{
		PlaceObject(PlayerPosition, GetObjectIdByName("PR"));

		return;
	}

	// Tür
	if ( Editor == FALSE && ( Keys != 0 ) && Feld[PlayerPosition + 1] == 5 )
	{
		Keys -= 1;
		UpdateInventar( Lives, Gold, Keys );
		DeleteObject( PlayerPosition );
		PlayerPosition = PlayerPosition + 1;  // Neue Position
		PlaceObject(PlayerPosition, GetObjectIdByName("PR"));

		return;
	}
	
	// Schlüssel
	if ( Editor == FALSE && Feld[PlayerPosition + 1] == 3 )
	{
		Keys += 1;
		UpdateInventar( Lives, Gold, Keys );
		DeleteObject( PlayerPosition );
		PlayerPosition = PlayerPosition + 1;  // Neue Position
		PlaceObject(PlayerPosition, GetObjectIdByName("PR"));

		return;
	}

	// Geld
	if ( Editor == FALSE && Feld[PlayerPosition + 1] == 2 )
	{
		Gold += 1;
		UpdateInventar( Lives, Gold, Keys );
		DeleteObject( PlayerPosition );
		PlayerPosition = PlayerPosition + 1;  // Neue Position
		PlaceObject(PlayerPosition, GetObjectIdByName("PR"));

		return;
	}
	
	// Herz
	if ( Editor == FALSE && Feld[PlayerPosition + 1] == 11 )
	{
		Lives += 1;
		UpdateInventar( Lives, Gold, Keys );
		DeleteObject( PlayerPosition );
		PlayerPosition = PlayerPosition + 1;  // Neue Position
		PlaceObject(PlayerPosition, GetObjectIdByName("PR"));

		return;
	}

	// Ist da nichts?
	if ( Editor == FALSE && Feld[PlayerPosition + 1] == 10 )  
	{
		DeleteObject(PlayerPosition);
		PlayerPosition = PlayerPosition + 1;  // Neue Position
		PlaceObject(PlayerPosition, GetObjectIdByName("PR"));

		return;
	}

	// Stacheln
	if ( Editor == FALSE && Feld[PlayerPosition + 1] == 12 )  
	{		
		DeleteObject( PlayerPosition );
		PlayerPosition = PlayerPosition + 1;  // Neue Position
		PlaceObject(PlayerPosition, GetObjectIdByName("RP"));
		PlayerAlive = FALSE;  // Spieler tot
		return;
	}

	// mechanische Stacheln
	if (Editor == FALSE && Feld[PlayerPosition + 1] == 13)
	{
		DeleteObject(PlayerPosition);
		PlayerPosition = PlayerPosition + 1;  // Neue Position

		if (FeldStachelnState[PlayerPosition])
		{
			PlaceObject(PlayerPosition, GetObjectIdByName("RP"));
			PlayerAlive = FALSE;  // Spieler tot
		}
		else
		{
			DrawObject('P', 'R', GetX(PlayerPosition), GetY(PlayerPosition));
		}

		return;
	}

	// verschiebare Mauer
	if ( Editor == FALSE && Feld[PlayerPosition + 1] == 14 && Feld[PlayerPosition + 2] == 10 && GetX( PlayerPosition ) < 540  )  
	{
		DeleteObject(PlayerPosition);
		PlaceObject(PlayerPosition+1, GetObjectIdByName("PR"));
		PlaceObject(PlayerPosition+2, GetObjectIdByName("WV"));
		PlayerPosition = PlayerPosition + 1;  // Neue Position
			  
		return;
	}

	// Mauer
	if ( Editor == FALSE && Feld[PlayerPosition + 1] == 1 ) 
	{
		DeleteObject(PlayerPosition);
		PlaceObject(PlayerPosition, GetObjectIdByName("PR"));
		   
		return;
	}		
}  // MoveRight

//=====================================================================
// MoveLeft
//
// Bewegt Spieler um ein Feld nach links
//=====================================================================

void MoveLeft()
{
	if ( Editor && Spielmodus != TRUE && GetX( CursorPosition ) > 0 )
	{
		RestoreFeld(CursorPosition);
		CursorPosition = CursorPosition - 1;
		DrawObject( 'E', 'D', GetX( CursorPosition ), GetY( CursorPosition ) );
			
		return;
	}

	if (GetX(PlayerPosition) == 0)
	{
		PlaceObject(PlayerPosition, GetObjectIdByName("PL"));

		return;
	}

	// Tür
	if ( Editor == FALSE && ( Keys != 0 ) && Feld[PlayerPosition - 1] == 5 )
	{
		Keys -= 1;
		UpdateInventar( Lives, Gold, Keys );
		DeleteObject( PlayerPosition );
		PlayerPosition = PlayerPosition - 1;  // Neue Position
		PlaceObject(PlayerPosition, GetObjectIdByName("PL"));

		return;
	}
	
	// Schlüssel
	if ( Editor == FALSE && Feld[PlayerPosition - 1] == 3 )
	{
		Keys += 1;
		UpdateInventar( Lives, Gold, Keys );
		DeleteObject( PlayerPosition );
		PlayerPosition = PlayerPosition - 1;  // Neue Position
		PlaceObject(PlayerPosition, GetObjectIdByName("PL"));

		return;
	}
		
	// Geld
	if ( Editor == FALSE && Feld[PlayerPosition - 1] == 2 )
	{
		Gold += 1;
		UpdateInventar( Lives, Gold, Keys );
		DeleteObject( PlayerPosition );
		PlayerPosition = PlayerPosition - 1;  // Neue Position
		PlaceObject(PlayerPosition, GetObjectIdByName("PL"));

		return;
	}
	
	// Herz
	if ( Editor == FALSE && Feld[PlayerPosition - 1] == 11 )
	{
		Lives += 1;
		UpdateInventar( Lives, Gold, Keys );
		DeleteObject( PlayerPosition );
		PlayerPosition = PlayerPosition - 1;  // Neue Position
		PlaceObject(PlayerPosition, GetObjectIdByName("PL"));

		return;
	}

	// Nichts
	if ( Editor == FALSE && Feld[PlayerPosition - 1] == 10 )  
	{
		DeleteObject( PlayerPosition );
		PlayerPosition = PlayerPosition - 1;  // Neue Position
		PlaceObject(PlayerPosition, GetObjectIdByName("PL"));

		return;
	}

	// Stacheln
	if ( Editor == FALSE && Feld[PlayerPosition - 1] == 12 )  
	{
		DeleteObject( PlayerPosition );
		PlayerPosition = PlayerPosition - 1;  // Neue Position
		PlaceObject(PlayerPosition, GetObjectIdByName("RP"));
		PlayerAlive = FALSE;  // Spieler tot

		return;
	}

	// mechanische Stacheln
	if (Editor == FALSE && Feld[PlayerPosition - 1] == 13)
	{
		DeleteObject(PlayerPosition);
		PlayerPosition = PlayerPosition - 1;  // Neue Position

		if (FeldStachelnState[PlayerPosition])
		{
			PlaceObject(PlayerPosition, GetObjectIdByName("RP"));
			PlayerAlive = FALSE;  // Spieler tot
		}
		else
		{
			DrawObject('P', 'L', GetX(PlayerPosition), GetY(PlayerPosition));
		}

		return;
	}

	// verschiebbare Mauer
	if ( Editor == FALSE && Feld[PlayerPosition - 1] == 14 && Feld[PlayerPosition - 2] == 10 && GetX( PlayerPosition ) > 30  )  
	{  		
		DeleteObject(PlayerPosition);
		PlaceObject(PlayerPosition - 1, GetObjectIdByName("PL"));
		PlaceObject(PlayerPosition - 2, GetObjectIdByName("WV"));
		PlayerPosition = PlayerPosition - 1;  // Neue Position

		return;
	}
		
	// Mauer? 
	if ( Editor == FALSE && Feld[PlayerPosition - 1] == 1 )  
	{
		DeleteObject(PlayerPosition);
		PlaceObject(PlayerPosition, GetObjectIdByName("PL"));
		   
		return;
	}

}  // MoveLeft

//=====================================================================
// DeleteObject
//
// Löscht das entsprechende Feld
//=====================================================================

void DeleteObject( short FeldIndex, bool bForce )
{
	if (FeldIndex >= 0 && FeldIndex <= 299)
	{
		// Ausnahme: mechanische Stacheln nicht löschen
		if (!bForce && Feld[FeldIndex] == 13 && FeldStachelnState[FeldIndex] == 0)
			DrawObject('C', 'R', GetX(FeldIndex), GetY(FeldIndex));
		else
		{
			Feld[FeldIndex] = 10;
			ClearScreen(GetX(FeldIndex), GetY(FeldIndex), 30, 30);
		}
	}

}  // DeleteObject

//=====================================================================
// RunEditor
//
// Ruft den Editor auf
//=====================================================================

void RunEditor ()
{
	RECT rcRect;
    RECT rcObjekt;

	SelectedObject = 1;
	CursorPosition = 0;
	Editor = TRUE;
	Menu = FALSE;
	Spielmodus = FALSE;

	// Bildschrim löschen
	ClearScreen(0, 0, 640, 480);

	// Erzeuge grüne Editor-Fläche
	rcRect.left = 0;
	rcRect.top = 30;
	rcRect.right = 600;
	rcRect.bottom = 480;

	rcObjekt.left = 390;
	rcObjekt.top = 0;
	rcObjekt.right = 420;
	rcObjekt.bottom = 30;

	lpDDSPrimary->Blt(&rcRect, lpDDSOffScreen, &rcObjekt,
		DDBLT_WAIT, NULL);

	// Lade Szenario in den Editor
	FILE *myFile;
	char Objekt[2];
	int x = 0;
	int y = 0;
	short ret;
	int i;

	if ((myFile = fopen(lpNewScenarioFile, "r")) == NULL)
	{
		Fail("Fehler beim Öffnen der Datei.\n");
	}

	for (i = 0; i <= 299; i++)
	{
		ret = fscanf(myFile, "%s", &Objekt);  // Lese Objekt

		if (Objekt[0] == 'W' && Objekt[1] == 'L') EditorFeld[i] = 1;	// Mauer
		if (Objekt[0] == 'G' && Objekt[1] == 'O') EditorFeld[i] = 2;	// Geld
		if (Objekt[0] == 'K' && Objekt[1] == 'Y') EditorFeld[i] = 3;	// Schlüssel
		if (Objekt[0] == 'R' && Objekt[1] == 'P') EditorFeld[i] = 4;	// Grabstein
		if (Objekt[0] == 'D' && Objekt[1] == 'R') EditorFeld[i] = 5;	// Tür

		if (Objekt[0] == 'P' && Objekt[1] == 'V')					// Spieler von vorne
		{
			EditorFeld[i] = 6;
			PlayerPosition = i;
		}
		if (Objekt[0] == 'P' && Objekt[1] == 'H')					// Spieler von hinten
		{
			EditorFeld[i] = 7;
			PlayerPosition = i;
		}
		if (Objekt[0] == 'P' && Objekt[1] == 'R')					// Spieler von rechts
		{
			EditorFeld[i] = 8;
			PlayerPosition = i;
		}

		if (Objekt[0] == 'P' && Objekt[1] == 'L')					// Spieler von links
		{
			EditorFeld[i] = 9;
			PlayerPosition = i;
		}

		if (Objekt[0] == 'C' && Objekt[1] == 'R') EditorFeld[i] = 10;	// Löschfeld
		if (Objekt[0] == 'H' && Objekt[1] == 'Z') EditorFeld[i] = 11;	// Herz
		if (Objekt[0] == 'S' && Objekt[1] == 'T') EditorFeld[i] = 12;	// Stacheln
		if (Objekt[0] == 'S' && Objekt[1] == 'B') { EditorFeld[i] = 13; FeldStachelnState[i] = 1; }	// mechanische Stacheln
		if (Objekt[0] == 'W' && Objekt[1] == 'V') EditorFeld[i] = 14;	// verschiebbare Mauer

		if (Objekt[0] == 'S' && Objekt[1] == 'V')					// Skelett von vorne
		{
			EditorFeld[i] = 15;
			Skelett = TRUE;
		}

		if (Objekt[0] == 'S' && Objekt[1] == 'H')					// Skelett von hinten
		{
			EditorFeld[i] = 16;
			Skelett = TRUE;
		}

		if (Objekt[0] == 'S' && Objekt[1] == 'R')					// Skelett von rechts
		{
			EditorFeld[i] = 17;
			Skelett = TRUE;
		}

		if (Objekt[0] == 'S' && Objekt[1] == 'L')					// Skelett von links
		{
			EditorFeld[i] = 18;
			Skelett = TRUE;
		}

		ret = fscanf(myFile, "%d", &x);	// Lese x-Position
		ret = fscanf(myFile, "%d", &y);  // Lese y-Position

		DrawObject(Objekt[0], Objekt[1], x, y);

		if (ret == EOF) break;
	}

	fclose(myFile);

	DrawObject( 'E', 'D', 0, 30 );  // Kreuz
    // Pfeil nach oben - Objektauswahl
	DrawObject( 'A', 'O', 600,30 );
	// Aktuell ausgewähltes Objekt hier: Mauer 
	DrawObject('W', 'L', 600, 60);
	// Pfeil nach unten - Objektauswahl
	DrawObject( 'A', 'U', 600, 90 );
    
}  // RunEditor

//=====================================================================
// GetObjectIdByName
//
// Liefert den Namen des Objekts
//=====================================================================

int GetObjectIdByName(const char* Object)
{
	if (strcmp(Object, "WL") == 0) return 1;
	if (strcmp(Object, "GO") == 0) return 2;
	if (strcmp(Object, "KY") == 0) return 3;
	if (strcmp(Object, "RP") == 0) return 4;
	if (strcmp(Object, "DR") == 0) return 5;
	if (strcmp(Object, "PV") == 0) return 6;
	if (strcmp(Object, "PH") == 0) return 7;
	if (strcmp(Object, "PR") == 0) return 8;
	if (strcmp(Object, "PL") == 0) return 9;
	if (strcmp(Object, "CR") == 0) return 10;
	if (strcmp(Object, "HZ") == 0) return 11;
	if (strcmp(Object, "ST") == 0) return 12;
	if (strcmp(Object, "SB") == 0) return 13;
	if (strcmp(Object, "WV") == 0) return 14;
	if (strcmp(Object, "SV") == 0) return 15;
	if (strcmp(Object, "SH") == 0) return 16;
	if (strcmp(Object, "SL") == 0) return 17;
	if (strcmp(Object, "SR") == 0) return 18;
	if (strcmp(Object, "GR") == 0) return 26;

	return -1;
}

//=====================================================================
// GetObjectNameById
//
// Liefert den Namen des Objekts
//=====================================================================

const char* GetObjectNameById(int Object)
{
	switch (Object)
	{
		// Mauer
		case 1: return "WL";

		// Geld
		case 2: return "GO";

		// Schlüssel
		case 3: return "KY";
			
		// Grabstein
		case 4: return "RP";

		// Tür
		case 5: return "DR";

		// Spieler von vorne
		case 6: return "PV";

		// Spieler von hinten
		case 7: return "PH";
	
		// Spieler von rechts
		case 8: return "PR";
			
		// Spieler von links
		case 9: return "PL";
	
		// Löschfeld
		case 10: return "CR";

		// Herz
		case 11: return "HZ";
	
		// Stacheln
		case 12: return "ST";

		// Stacheln aus dem Boden
		case 13: return "SB";
	
		// verschiebare Mauer
		case 14: return "WV";
			
		// Skelett von vorne
		case 15: return "SV";

		// Skelett von hinten
		case 16: return "SH";

		// Skelett von links
		case 17: return "SL";

		// Skelett von rechts
		case 18: return "SR";

		// Grün - Hintergrund vom Editor
		case 26: return "GR";
	}

	return "";
}

//=====================================================================
// PlaceObject
//
// Plaziert Objekt an angegebener Position - im Editormodus
//=====================================================================

void PlaceObject( int FieldIndex, int Object )
{
	if (FieldIndex < 0 || FieldIndex > 299)
		return;

	if (Editor)
		EditorFeld[FieldIndex] = Object;
	else
		Feld[FieldIndex] = Object;

	DrawObject(GetObjectNameById(Object)[0], GetObjectNameById(Object)[1], GetX(FieldIndex), GetY(FieldIndex));
	
}  // PlaceObject

//=====================================================================
// RestoreFeld
//
// Stellt Objekte wieder her, wenn das Auswahlkreuz ein
// Objekt visuell löscht
//=====================================================================

void RestoreFeld( int Feld )
{
	DrawObject(GetObjectNameById(EditorFeld[Feld])[0], GetObjectNameById(EditorFeld[Feld])[1], GetX(Feld), GetY(Feld));

}  // RestoreFeld

//=====================================================================
// Save
//
// Speichert das Level ab
//=====================================================================

void Save()
{
	FILE *file;
    int i;

    file = fopen(lpNewScenarioFile, "w" );

	for ( i = 0; i <= 299; i++ )
	{
		// Leere Editor-Felder durch "nichts" ersetzen
		if (EditorFeld[i] == 26)
			EditorFeld[i] = 10;

		fprintf(file, "%c%c %d %d\n", GetObjectNameById(EditorFeld[i])[0], GetObjectNameById(EditorFeld[i])[1], GetX(i), GetY(i));
	}

    fclose( file );

}  // Speichern

//=====================================================================
// UpdateSkelett
//
//=====================================================================

void UpdateSkelett()
{
	int SkelettPosition;
	int i;

	// Suche nach Skelett auf Feld
	for ( i = 0; i <= 299; i++ )
	{
		if ( Feld[i] == 15 || Feld[i] == 16 || Feld[i] == 17 || Feld[i] == 18)
		{					
			SkelettPosition = i;
				
			// Skelett befindet sich unterhalb vom Spieler
			if (GetY(PlayerPosition) < GetY(SkelettPosition))
			{
				// Geht es ein Schritt nach oben?
				if (GetY(SkelettPosition) > 30 && Feld[SkelettPosition - 20] == 10)
				{
					// Ja, es geht
					PlaceObject(SkelettPosition, GetObjectIdByName("CR"));
					SkelettPosition = SkelettPosition - 20;
					PlaceObject(SkelettPosition, GetObjectIdByName("SH"));

					return;
				}
				// Auf Feind gestoßen?
				else if (GetY(SkelettPosition) > 30 && Feld[SkelettPosition - 20] == 6 || Feld[SkelettPosition - 20] == 7 || Feld[SkelettPosition - 20] == 8 || Feld[SkelettPosition - 20] == 9)
				{
					// Ja
					PlaceObject(SkelettPosition, GetObjectIdByName("SH"));
					SkelettPosition = SkelettPosition - 20;
					PlaceObject(SkelettPosition, GetObjectIdByName("RP"));
					PlayerAlive = FALSE;  // Spieler ist tot
					
					return;
				}
			}

			// Skelett befindet sich oberhalb vom Spieler
			if (GetY(PlayerPosition) > GetY(SkelettPosition))
			{
				if (GetY(SkelettPosition) < 450 && Feld[SkelettPosition + 20] == 10)
				{
					// Ja, es geht
					PlaceObject(SkelettPosition, GetObjectIdByName("CR"));
					SkelettPosition = SkelettPosition + 20;
					PlaceObject(SkelettPosition, GetObjectIdByName("SV"));

					return;
				}
				// Auf Feind gestoßen?
				else if (GetY(SkelettPosition) < 450 && Feld[SkelettPosition + 20] == 6 || Feld[SkelettPosition - 20] == 7 || Feld[SkelettPosition - 20] == 8 || Feld[SkelettPosition - 20] == 9)
				{
					// Ja
					PlaceObject(SkelettPosition, GetObjectIdByName("SV"));
					SkelettPosition = SkelettPosition + 20;
					PlaceObject(SkelettPosition, GetObjectIdByName("RP"));
					PlayerAlive = FALSE;  // Spieler ist tot
					
					return;
				}
			}

			// Skelett befindet sich links vom Spieler
			if (GetX(PlayerPosition) > GetX(SkelettPosition))
			{
				if (GetX(SkelettPosition) < 570 && Feld[SkelettPosition + 1] == 10)
				{
					// Ja, es geht
					PlaceObject(SkelettPosition, GetObjectIdByName("CR"));
					SkelettPosition = SkelettPosition + 1;
					PlaceObject(SkelettPosition, GetObjectIdByName("SR"));

					return;
				}
				// Auf Feind gestoßen?
				else if (GetX(SkelettPosition) < 570 && Feld[SkelettPosition + 1] == 6 || Feld[SkelettPosition + 1] == 7 || Feld[SkelettPosition + 1] == 8 || Feld[SkelettPosition + 1] == 9)
				{
					// Ja
					PlaceObject(SkelettPosition, GetObjectIdByName("SR"));
					SkelettPosition = SkelettPosition + 1;
					PlaceObject(SkelettPosition, GetObjectIdByName("RP"));
					PlayerAlive = FALSE;  // Spieler ist tot
					
					return;
				}
			}

			// Skelett befindet sich rechts vom Spieler
			if (GetX(PlayerPosition) < GetX(SkelettPosition))
			{
				// Ein Schritt nach links
				if (GetX(SkelettPosition) > 0 && Feld[SkelettPosition - 1] == 10)
				{
					PlaceObject(SkelettPosition, GetObjectIdByName("CR"));
					SkelettPosition = SkelettPosition - 1;
					PlaceObject(SkelettPosition, GetObjectIdByName("SL"));

					return;
				}
				// Auf Feind gestoßen?
				else if (GetX(SkelettPosition) > 0 && Feld[SkelettPosition - 1] == 6 || Feld[SkelettPosition - 1] == 7 || Feld[SkelettPosition - 1] == 8 || Feld[SkelettPosition - 1] == 9)
				{
					// Ja
					PlaceObject(SkelettPosition, GetObjectIdByName("SL"));
					SkelettPosition = SkelettPosition - 1;
					PlaceObject(SkelettPosition, GetObjectIdByName("RP"));
					PlayerAlive = FALSE;  // Spieler ist tot				

					return;
				}
			}	
		}
	}                 
}  // UpdateSkelett

//=====================================================================
// UpdateStacheln
//=====================================================================

void UpdateStacheln()
{
	for ( int i = 0; i <= 299; i++ )
	{
		if ( Feld[i] == 13 )
		{					
			// Wenn Stacheln oben, dann nach unten
			if (FeldStachelnState[i])
			{
				// Spieler in den Stacheln?
				if (PlayerPosition == i)
				{
					PlayerAlive = FALSE;
					PlaceObject(i, GetObjectIdByName("RP"));
				}
				else
				{
					DrawObject('C', 'R', GetX(i), GetY(i));
					FeldStachelnState[i] = 0;
				}
			}
			else  // Wenn Stacheln unten, dann nach oben
			{
				DrawObject('S', 'B', GetX(i), GetY(i));
				FeldStachelnState[i] = 1;
			}
		}
	}
}  // UpdateStacheln

//=====================================================================
// ShowIntro
//
// Zeigt das Intro an 
// 
//=====================================================================

void ShowIntro( void )
{
	RECT Targ;

	Targ.left = 0;
	Targ.top = 202;
	Targ.right = 640;
	Targ.bottom = 278;

	lpDDSPrimary->Blt( &Targ, lpDDSIntro, NULL,
      				           DDBLT_WAIT, NULL );
}

//=====================================================================
// GetX
//
// Liefert die x-Koordinate des zugehörigen 
// FeldIndexes zurück
//=====================================================================

int GetX(int FeldIndex)
{
	int x = -30, i = 0;

	while (i <= FeldIndex)
	{
		x += 30;

		if (x == 600)
			x = 0;

		i++;
	}

	// Nur für Objektauswahl mit Bild+ bzw Bild-
	if (FeldIndex == 301)
		x = 600;

	return x;

} // GetX

//=====================================================================
// GetY
//
// Liefert die y-Koordinate des zugehörigen 
// FeldIndexes zurück
//=====================================================================

int GetY(int FeldIndex)
{
	int x = -30, y = 30, i = 0;

	while (i <= FeldIndex)
	{
		x += 30;

		if (x == 600)
		{
			x = 0;
			y += 30;
		}

		i++;
	}

	// Nur für Objektauswahl mit Bild+ bzw Bild-
	if (FeldIndex == 301)
		y = 60;

	return y;

} // GetY

//=====================================================================
// WinMain
//
// Hauptprogramm
//=====================================================================

int PASCAL WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
          	        LPSTR lpCmdLine, int nCmdShow )
{
    MSG msg;

    if ( !doInit( hInstance, nCmdShow ) )
    {
        return FALSE;
    }

    while ( 1 )
	{
		if ( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) )
		{
			if ( !GetMessage( &msg, NULL, 0, 0 ) )
			{
				return msg.wParam;
			}
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else if ( bActive )
		{
		}
		else
		{
			// Programm läuft nicht: Anhalten bis
			// zur nächsten Meldung
			WaitMessage( );
		}
	}

}  // WinMain