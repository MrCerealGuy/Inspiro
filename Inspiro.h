// Header-Dateien der C-Laufzeit
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <windows.h>
#include <windowsx.h>
#include <ddraw.h>
#include <stdio.h>
#include <stdlib.h>

void LoadLevel(void);
static BOOL doInitDirectDraw(HWND);
void DrawObject(char, char, short, short);
void PlaceObject(int, int);
void DeleteObject(short FeldIndex, bool bForce = false);
const char* GetObjectNameById(int);
int GetObjectIdByName(const char*);
void RestoreFeld(int);
static BOOL ShowMenu(void);
void ClearScreen(int, int, int, int);
void ShowInventar(void);
static BOOL UpdateInventar(int, int, int);
void MoveUp(void);	
void MoveDown(void);	
void MoveRight(void);
void MoveLeft(void);
void RunEditor(void);
void Save(void);
void UpdateSkelett(void);
void UpdateStacheln(void);
void ShowIntro(void);
int GetX(int);
int GetY(int);