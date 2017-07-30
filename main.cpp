#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>

#define ISDOT(a) (!lstrcmp(a,".") || !lstrcmp(a,".."))
#define CONFIGFILE "config.dat"

extern "C" { void WinMainCRTStartup(void); };
/*  Declare Windows procedure  */

LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

HWND MainWindowHandle;
HHOOK Hook;

/*  Make the class name into a global variable  */
char szClassName[ ] = "File Search/Scanner";
char BufTextStatus[1024];
long ScanFile(char *sFileName, char *sString, int len, BOOL Sensitive);
long ScanPath(char *sPath, char *sMask, char *sString);
long InitListView();
long AddFileToListView(WIN32_FIND_DATA *FindData, char *Path);
long AddTextToListView(char *Text);
long ResizeControls(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
long JointPathFile(char *sPath, char *sFile, char *New);
long OrderTabulation();
long LoadConfig(struct ConfigData *ptrData);
long SaveConfig(struct ConfigData *ptrData);
long Start();
long CutPath( char *s );
long mostra(char *s);

void *(memset)(void *s, int c, size_t n);

HHOOK InstallHook(HWND hwnd);
long UninstallHook();
LRESULT CALLBACK ProcHook(int code, WPARAM wParam, LPARAM lParam);


struct ConfigData {
    char Files[10][128];
    char Masks[10][128];
    char Strings[10][128];
    BOOL Case;
    BOOL Recurse;
    BOOL ShowDirs;
};
struct ConfigData Config;

HWND hCboPath,       //file name
        hCboString,  //string to seek inside files
        hCboMask,    //file mask to find
        hChkCase,    //case sensitive option
        hChkRecurse, //recursive search inside subfolders
        hChkShowDirs,//show directories in listview 
        hButStart,   //start scanning
        hButCancel,  //cancel seek/scannin
        hLvsResults, //listview with search results
        hStatusBar;  //seven headed sea monster

int WINAPI WinMain (HINSTANCE hThisInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR lpszArgument,
                    int nFunsterStil)

{
    InitCommonControls();    /* Prepare the ground for the fucking ListView/StatusBar */
    HWND hwnd;               /* This is the handle for our window */
    MSG messages;            /* Here messages to the application are saved */
    WNDCLASSEX wincl;        /* Data structure for the windowclass */

    /* The Window structure */
    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc = WindowProcedure;      /* This function is called by windows */
    wincl.style = CS_DBLCLKS;                 /* Catch double-clicks */
    wincl.cbSize = sizeof (WNDCLASSEX);

    /* Use default icon and mouse-pointer */
    wincl.hIcon = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
    wincl.lpszMenuName = NULL;                 /* No menu */
    wincl.cbClsExtra = 0;                      /* No extra bytes after the window class */
    wincl.cbWndExtra = 0;                      /* structure or the window instance */
    /* Use Windows's default color as the background of the window */
    wincl.hbrBackground = (HBRUSH) COLOR_BACKGROUND;

    /* Register the window class, and if it fails quit the program */
    if (!RegisterClassEx (&wincl))
        return 0;

    /* The class is registered, let's create the program*/
    hwnd = CreateWindowEx (
           0,                   /* Extended possibilites for variation */
           szClassName,         /* Classname */
           "File Search",       /* Title Text */
           WS_OVERLAPPEDWINDOW, /* default window */
           CW_USEDEFAULT,       /* Windows decides the position */
           CW_USEDEFAULT,       /* where the window ends up on the screen */
           600,                 /* The programs width */
           417,                 /* and height in pixels */
           HWND_DESKTOP,        /* The window is a child-window to desktop */
           NULL,                /* No menu */
           hThisInstance,       /* Program Instance handler */
           NULL                 /* No Window Creation data */
           );
    MainWindowHandle = hwnd;
    hCboPath = CreateWindowEx(WS_EX_CLIENTEDGE,
            "COMBOBOX", "C:\\Windows",
            WS_VISIBLE | WS_CHILD | WS_TABSTOP | CBS_DROPDOWN | WS_VSCROLL,
            10, 10, 140, 150, 
            hwnd, NULL, hThisInstance, NULL);

    hCboMask = CreateWindowEx(WS_EX_CLIENTEDGE,
            "COMBOBOX", "*.dll",
            WS_VISIBLE | WS_CHILD | WS_TABSTOP | CBS_DROPDOWN | WS_VSCROLL,
            10, 35, 300, 150, 
            hwnd, NULL, hThisInstance, NULL);

    hCboString = CreateWindowEx(WS_EX_CLIENTEDGE,
            "COMBOBOX", "hihihihi",
            WS_VISIBLE | WS_CHILD | WS_TABSTOP | CBS_DROPDOWN | WS_VSCROLL,
            10, 60, 300, 150, 
            hwnd, NULL, hThisInstance, NULL);

    hChkCase = CreateWindowEx(0,
            "BUTTON", "Case Sensitive",
            WS_CHILD | WS_VISIBLE| BS_AUTOCHECKBOX | WS_TABSTOP, 
            10, 87, 180, 15,
            hwnd, NULL, hThisInstance, NULL);

    hChkRecurse = CreateWindowEx(0,
            "BUTTON", "Recurse subfolders",
            WS_CHILD | WS_VISIBLE| BS_AUTOCHECKBOX | WS_TABSTOP,
            10, 105, 180, 15,
            hwnd, NULL, hThisInstance, NULL);

    hChkShowDirs = CreateWindowEx(0,
            "BUTTON", "Show folders in list",
            WS_CHILD | WS_VISIBLE| BS_AUTOCHECKBOX | WS_TABSTOP,
            10, 123, 180, 15,
            hwnd, NULL, hThisInstance, NULL);

    hButStart = CreateWindowEx(WS_EX_CLIENTEDGE,
            "BUTTON", "Start",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            260, 88, 50, 25,
            hwnd, NULL, hThisInstance, NULL);

    hButCancel = CreateWindowEx(WS_EX_CLIENTEDGE,
            "BUTTON", "Cancel",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            260, 115, 50, 25,
            hwnd, NULL, hThisInstance, NULL);

    hLvsResults = CreateWindowEx(WS_EX_CLIENTEDGE,
            WC_LISTVIEW, NULL, /*syslistview32*/
            WS_BORDER | WS_CHILD | WS_VISIBLE | LVS_REPORT | WS_TABSTOP,
            8, 145, 300, 240,
            hwnd, NULL, hThisInstance, NULL);

    hStatusBar = CreateWindow(STATUSCLASSNAME, /*"msctls_statusbar32"*/
            "Ready.", WS_CHILD | WS_VISIBLE,
            0, 0, 0, 0, 
            hwnd,NULL, hThisInstance, NULL);

    InitListView();

    HFONT Fonte = CreateFont(
            -12, 0, 0, 0, 400, 0, 0, 0,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,DEFAULT_PITCH + FF_DONTCARE,
            "Courier New");

    SendMessage( hCboPath,     WM_SETFONT, (int)Fonte, (int)hCboPath);
    SendMessage( hCboString,   WM_SETFONT, (int)Fonte, (int)hCboString);
    SendMessage( hCboMask,     WM_SETFONT, (int)Fonte, (int)hCboMask);
    SendMessage( hChkCase,     WM_SETFONT, (int)Fonte, (int)hChkCase);
    SendMessage( hChkRecurse,  WM_SETFONT, (int)Fonte, (int)hChkRecurse);
    SendMessage( hChkShowDirs, WM_SETFONT, (int)Fonte, (int)hChkShowDirs);
    SendMessage( hButStart,    WM_SETFONT, (int)Fonte, (int)hButStart);
    SendMessage( hButCancel,   WM_SETFONT, (int)Fonte, (int)hButCancel);
    SendMessage( hLvsResults,  WM_SETFONT, (int)Fonte, (int)hLvsResults);

    //SendMessage( hCboPath, CB_ADDSTRING, 0, (LPARAM)"33333");
    //SendMessage( hCboPath, CB_ADDSTRING, 0, (LPARAM)"22222");
    //SendMessage( hCboPath, CB_ADDSTRING, 0, (LPARAM)"11111");
    
    LoadConfig(&Config);

    InstallHook(hwnd);


    /* Make the window visible on the screen */
    ShowWindow (hwnd, nFunsterStil);

    SetFocus(hCboString); //first two is to deselect text in a lame way.
    SetFocus(hCboMask);
    SetFocus(hCboPath);

    //isdialog necessario para tabular entre controles.
    while(GetMessage(&messages, NULL, 0, 0) > 0){ //This is our message loop, which loops until 
                                                  //we recieve a quit message
        if (!IsDialogMessage(hwnd, &messages)){ //This makes Tabbing and Alt+UnderlinedLetter work, 
                                                //so the window behaves as a dialog
            TranslateMessage(&messages); //First we translate the recieved message
            DispatchMessage(&messages); //And then we dispatch it to our message handler
        }
    }

    /* The program return-value is 0 - The value that PostQuitMessage() gave */
    return messages.wParam;
}


/*  This function is called by the Windows function DispatchMessage()  */
LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)                  /* handle the messages */
    {
        case WM_DESTROY:
            UninstallHook();
            SaveConfig(&Config);
            PostQuitMessage (0);       /* send a WM_QUIT to the message queue */
            break;
        case WM_COMMAND:
            if ( HIWORD(wParam) == BN_CLICKED &&
                    (HWND)lParam == hButStart )
            {
                Start();
            }
            break;
        case WM_SIZE:
            ResizeControls( hwnd, message, wParam, lParam );
            break;
        default:                      /* for messages that we don't deal with */
            return DefWindowProc( hwnd, message, wParam, lParam );
    }

    return 0;
}

long ScanFile(char *sFileName, char *sString, int len, BOOL Sensitive)
{
register unsigned int i;
HANDLE f;
char buf[0xFFFF];
char CmpString[1024];
DWORD NumBytesRead;
long Position = 0;
BOOL Match = FALSE;
long FileSize;

    f = CreateFile((LPCSTR)sFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if(f == INVALID_HANDLE_VALUE) return(-1);

    FileSize = GetFileSize(f, NULL);

    lstrcpy( CmpString, sString );
    if( !Sensitive ) CharLower( CmpString );
    do
    {
        ReadFile(f,&buf,sizeof(buf),&NumBytesRead,NULL);
        i = 0;
        if( !Sensitive ) CharLowerBuff( buf, NumBytesRead );
        {
            while ( memcmp(CmpString,&buf[i++],len) && i <= NumBytesRead );
            Position += i;
            wsprintf((LPSTR)BufTextStatus, "%3d% done: %s", Position * 100 / FileSize, sFileName);
            SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)BufTextStatus);

            if ( i < NumBytesRead )
            { 
                CloseHandle(f);
                return( Position ); 
            }
        }
    } while(NumBytesRead == sizeof(buf));

    CloseHandle(f);
    return( 0 );

}

long ScanPath(char *sPath, char *sMask, char *sString)
{
DWORD dwError;
WIN32_FIND_DATA FindFileData;
HANDLE hFind;
char PathMask[1024];
char String[1024];
char buf[1024];
char TempPath[1024];
BOOL CaseSensitive;
BOOL ShowDirs;
BOOL HaveString;
BOOL Recursive;
int LenStr;

    lstrcpy (PathMask, sPath);
    PathAddBackslash(PathMask);
    lstrcat (PathMask, sMask);
    lstrcpy (String, sString);
    
    CaseSensitive = SendMessage(hChkCase, BM_GETCHECK, 0, 0);
    ShowDirs = SendMessage(hChkShowDirs, BM_GETCHECK, 0, 0);
    Recursive = SendMessage(hChkRecurse, BM_GETCHECK, 0, 0);


//        wsprintf ((LPSTR)buf,"%s", PathMask);
//        MessageBox(NULL,buf,"erro. mas senhor ERRO",MB_OK);


    LenStr = lstrlen(sString);
    HaveString = LenStr;

    hFind = FindFirstFile(PathMask, &FindFileData);

    if (hFind == INVALID_HANDLE_VALUE) 
    {
        FindClose(hFind);
        return (-1);
    } 

    if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        if( HaveString )
        {
            JointPathFile(sPath, FindFileData.cFileName, TempPath);
            if( ScanFile(TempPath, sString, LenStr, CaseSensitive) )
                    AddFileToListView(&FindFileData, TempPath);
        } else { AddFileToListView(&FindFileData, TempPath); }
    }
    while (FindNextFile(hFind, &FindFileData) != 0) 
    {
        if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            if( HaveString )
            {
                JointPathFile(sPath, FindFileData.cFileName, TempPath);
                if( ScanFile(TempPath, sString, LenStr, CaseSensitive) )
                        AddFileToListView(&FindFileData, TempPath);
            } else { AddFileToListView(&FindFileData, TempPath); }
        }
    }

    FindClose(hFind);

    if( !Recursive ) return(0); //RECURSIVE!!! RECURSIVE RECURSIVE RECURSIVE!!!!

    ZeroMemory(&FindFileData, sizeof(FindFileData));
    JointPathFile(sPath, "*.*", TempPath);
    hFind = FindFirstFile(TempPath, &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE) 
    {
        wsprintf ((LPSTR)buf,"Invalid file handle. Error is %u\n", GetLastError());
        MessageBox(NULL,buf,"erro. mas senhor ERRO",MB_OK);
        return (-1);
    } 
    if ( !ISDOT(FindFileData.cFileName) )
    {
        if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            JointPathFile(sPath, FindFileData.cFileName, TempPath);
            ScanPath(TempPath, sMask, sString);
            if( ShowDirs ) { AddFileToListView(&FindFileData, TempPath); }
        } 
    }
    while (FindNextFile(hFind, &FindFileData) != 0) 
    {
        if ( !ISDOT(FindFileData.cFileName) )
        {
            if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                JointPathFile(sPath, FindFileData.cFileName, TempPath);
                ScanPath(TempPath, sMask, sString);
                if( ShowDirs ) { AddFileToListView(&FindFileData, TempPath); }
            }
        }
    }

    FindClose(hFind);
}

long InitListView()
{
LV_COLUMN LvCol;

    ZeroMemory( &LvCol,sizeof(LvCol) );
    LvCol.mask = LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
    LvCol.cx = 230;
    LvCol.pszText = "File name";
    SendMessage( hLvsResults,LVM_INSERTCOLUMN,0,(LPARAM)&LvCol );

    LvCol.pszText = "Directory";
    SendMessage( hLvsResults,LVM_INSERTCOLUMN,1,(LPARAM)&LvCol );
	return(0);
}

long AddFileToListView(WIN32_FIND_DATA *FindData, char *Path)
{
LV_ITEM LvItem;
char buf[1024];
    ZeroMemory(&LvItem,sizeof(LvItem));
    
    LvItem.mask=LVIF_TEXT;
    LvItem.cchTextMax = 256;
    //LvItem.iItem=0;          // choose item  
    //LvItem.iSubItem=0;       // Put in first coluom
    LvItem.pszText=FindData->cFileName;
    SendMessage(hLvsResults,LVM_INSERTITEM,0,(LPARAM)&LvItem);
    //LvItem.iItem=0;
    LvItem.iSubItem=1;
    lstrcpy(buf,Path);
    //wsprintf((LPSTR)buf,"buf eh: %s",Path);
    //MessageBox(NULL,buf,NULL,MB_OK);
    CutPath(buf);
    LvItem.pszText=buf;
    SendMessage(hLvsResults,LVM_SETITEM,0,(LPARAM)&LvItem);

	return(0);
}

long AddTextToListView(char *Text)
{
LV_ITEM LvItem;

    ZeroMemory(&LvItem,sizeof(LvItem));
    LvItem.mask=LVIF_TEXT;
    LvItem.cchTextMax = 256;
    LvItem.pszText=Text;
    SendMessage(hLvsResults,LVM_INSERTITEM,0,(LPARAM)&LvItem);
	return(0);
}


long ResizeControls(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
RECT MainRect, TmpRect;
const int MaxTxtWidth = 300;
const int MaxLeftStart = 260;

    GetWindowRect(hwnd, &MainRect);
    GetWindowRect(hLvsResults, &TmpRect);
    SetWindowPos(hLvsResults, 0, 0, 0, MainRect.right- 10 - TmpRect.left, 
            MainRect.bottom-25-TmpRect.top, SWP_NOMOVE);
    //SendMessage(hLvsResults, LVM_SETCOLUMNWIDTH, 0, MainRect.right - TmpRect.left - 30);
    SendMessage(hStatusBar, message, wParam, lParam);
    if( MainRect.right - MainRect.left < 325 )
    {
        SetWindowPos(hButStart, 0, MainRect.right-MainRect.left - 62, 88, 0, 0, SWP_NOSIZE);
        SetWindowPos(hButCancel, 0, MainRect.right-MainRect.left - 62, 115, 0, 0, SWP_NOSIZE);
        SetWindowPos(hCboPath, 0, 0, 0, 
                MainRect.right - 10 - TmpRect.left, 20, SWP_NOMOVE);
        SetWindowPos(hCboMask, 0, 0, 0, 
                MainRect.right - 10 - TmpRect.left, 20, SWP_NOMOVE);
        SetWindowPos(hCboString, 0, 0, 0, 
                MainRect.right - 10 - TmpRect.left, 20, SWP_NOMOVE);
    }
    else
    {
        SetWindowPos(hButStart,    0, 260, 88, 0, 0, SWP_NOSIZE);
        SetWindowPos(hButCancel,    0, 260, 115, 0, 0, SWP_NOSIZE);
        SetWindowPos(hCboPath,     0, 0, 0, 300, 20, SWP_NOMOVE);
        SetWindowPos(hCboMask, 0, 0, 0, 300, 20, SWP_NOMOVE);
        SetWindowPos(hCboString,   0, 0, 0, 300, 20, SWP_NOMOVE);
    }
    OrderTabulation();
    //DEBUGGIN STUFF!
//    char buf[111];
//    GetWindowRect(hCboPath, &TmpRect);
//    wsprintf ((LPSTR)buf,"%d %d", MainRect.right, TmpRect.right);
//    MessageBox(NULL,buf,"main.cx:%d txt.cx:%d",MB_OK);
    //END OF DEBUGGIN STUFF!

	return(0);
}


long JointPathFile(char *sPath, char *sFile, char *New)
{
    lstrcpy(New, sPath);
    PathAddBackslash(New);
    lstrcat(New,sFile);
	return(0);
}

long OrderTabulation()
{
    SetWindowPos(hLvsResults,  HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    SetWindowPos(hButCancel,   HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    SetWindowPos(hButStart,    HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    SetWindowPos(hChkShowDirs, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    SetWindowPos(hChkRecurse,  HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    SetWindowPos(hChkCase,     HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    SetWindowPos(hCboString,   HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    SetWindowPos(hCboMask,     HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    SetWindowPos(hCboPath,     HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	return(0);
}

long LoadConfig(struct ConfigData *ptrData)
{
HANDLE fData;
DWORD NumBytesRead;
int i;
char buf[1024];

    ZeroMemory(ptrData, sizeof(ConfigData));
    fData = CreateFile(CONFIGFILE, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if(fData == INVALID_HANDLE_VALUE)
    {
        return(-1);
    }
    ReadFile(fData,ptrData,sizeof(ConfigData),&NumBytesRead,NULL);
    CloseHandle(fData);
    
    for(i = 0; i < 10; i++)
    {
        if( lstrcmp(ptrData->Files[i], "") || !i ) 
                SendMessage( hCboPath, CB_ADDSTRING, 0, (LPARAM) ptrData->Files[i] );
        if( lstrcmp(ptrData->Masks[i], "") || !i) 
                SendMessage( hCboMask, CB_ADDSTRING, 0, (LPARAM) ptrData->Masks[i] );
        if( lstrcmp(ptrData->Strings[i], "") || !i) 
                SendMessage( hCboString, CB_ADDSTRING, 0, (LPARAM) ptrData->Strings[i] );
    }
    SendMessage( hCboPath, CB_SETCURSEL, 0, 0 );
    SendMessage( hCboMask, CB_SETCURSEL, 0, 0 );
    SendMessage( hCboString, CB_SETCURSEL, 0, 0 );

}

long SaveConfig(struct ConfigData *ptrData)
{
HANDLE fData;
DWORD NumBytesRead;
int i, TotalItems;
char buf[128];

    ZeroMemory(ptrData, sizeof(ConfigData));
    
    TotalItems = SendMessage( hCboPath, CB_GETCOUNT, 0, 0 );
    for( i = 0; i < TotalItems; i++ )
    {
        SendMessage( hCboPath, CB_GETLBTEXT, i, (LPARAM)buf );
        lstrcpy( ptrData->Files[i], buf);
    }
    TotalItems = SendMessage( hCboMask, CB_GETCOUNT, 0, 0 );
    for( i = 0; i < TotalItems; i++ )
    {
        SendMessage( hCboMask, CB_GETLBTEXT, i, (LPARAM)buf );
        lstrcpy( ptrData->Masks[i], buf);
    }
    TotalItems = SendMessage( hCboString, CB_GETCOUNT, 0, 0 );
    for( i = 0; i < TotalItems; i++ )
    {
        SendMessage( hCboString, CB_GETLBTEXT, i, (LPARAM)buf );
        lstrcpy( ptrData->Strings[i], buf);
    }

    //if some combo was empty, lets zero manually the string on ptrconfig - what a mess!
    SendMessage( hCboPath, CB_GETLBTEXT, 0, (LPARAM)buf );
    if( !lstrcmp(buf,"") ) ptrData->Files[0][0] = 0;
    SendMessage( hCboMask, CB_GETLBTEXT, 0, (LPARAM)buf );
    if( !lstrcmp(buf,"") ) ptrData->Masks[0][0] = 0;
    SendMessage( hCboString, CB_GETLBTEXT, 0, (LPARAM)buf );
    if( !lstrcmp(buf,"") ) ptrData->Strings[0][0] = 0;

    fData = CreateFile(CONFIGFILE, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);
    if(fData == INVALID_HANDLE_VALUE)
    {
        return(-1);
    }
    WriteFile(fData,ptrData,sizeof(ConfigData),&NumBytesRead,NULL);
    CloseHandle(fData);
}


HHOOK InstallHook(HWND hwnd)
{
    Hook = SetWindowsHookEx(WH_KEYBOARD, ProcHook, NULL, GetCurrentThreadId());
    return(Hook);
}

long UninstallHook()
{
    UnhookWindowsHookEx(Hook);
	return(0);
}

LRESULT CALLBACK ProcHook(int code, WPARAM wParam, LPARAM lParam)
{
    //enter was released
    if( wParam == VK_RETURN && ((lParam >> 31) & 1) == 1 )
    {
        SendMessage(MainWindowHandle, WM_COMMAND, BN_CLICKED, (LPARAM)hButStart);
    }
    return CallNextHookEx(NULL, code, wParam, lParam); 
}

long Start()
{
long pos;
long lItems;
char tmpFile[1024];
char tmpString[1024];
char tmpMask[1024];
char CboText[1024];
long ResultCboFind;
    
    GetWindowText( hCboPath, (LPTSTR)tmpFile, sizeof(tmpFile) );
    GetWindowText( hCboMask, (LPTSTR)tmpMask, sizeof(tmpMask) );
    GetWindowText( hCboString, (LPTSTR)tmpString, sizeof(tmpString) );
    
    if( !lstrcmp(tmpFile, "") )
    {
        MessageBox(NULL,"Please, select a path to scan.","cant go on",MB_OK);
        return(-1);
    }
    
    if( !lstrcmp(tmpMask, "") )
    {
        SetWindowText( hCboMask, "*.*" );
        lstrcpy( tmpMask, "*.*" );
    }    

    GetWindowText(hCboPath, (LPSTR)CboText, sizeof(CboText));
    ResultCboFind = SendMessage( hCboPath, CB_FINDSTRINGEXACT, 0, (LPARAM)CboText);
    if(ResultCboFind == CB_ERR) SendMessage(hCboPath, CB_DELETESTRING, 9, 0);
    SendMessage(hCboPath, CB_INSERTSTRING, 0, (LPARAM)CboText);
    
    GetWindowText(hCboMask, (LPSTR)CboText, sizeof(CboText));
    ResultCboFind = SendMessage( hCboMask, CB_FINDSTRINGEXACT, 0, (LPARAM)CboText);
    if(ResultCboFind == CB_ERR) SendMessage(hCboMask, CB_DELETESTRING, 9, 0);
    SendMessage(hCboMask, CB_INSERTSTRING, 0, (LPARAM)CboText);

    GetWindowText(hCboString, (LPSTR)CboText, sizeof(CboText));
    ResultCboFind = SendMessage( hCboString, CB_FINDSTRINGEXACT, 0, (LPARAM)CboText);
    if(ResultCboFind == CB_ERR) SendMessage(hCboString, CB_DELETESTRING, 9, 0);
    SendMessage(hCboString, CB_INSERTSTRING, 0, (LPARAM)CboText);

    SendMessage( hLvsResults, LVM_DELETEALLITEMS, 0, 0 );
    AddTextToListView( "scanning in progress, please wait." );
    UpdateWindow( hLvsResults );
    LockWindowUpdate( hLvsResults );
    SendMessage( hLvsResults, LVM_DELETEALLITEMS, 0, 0 );
    ScanPath( tmpFile, tmpMask, tmpString );
    LockWindowUpdate( 0 );
    lItems = SendMessage( hLvsResults, LVM_GETITEMCOUNT, 0, 0 );
    wsprintf( (LPSTR)BufTextStatus,"%ld items found.",lItems );
    SendMessage( hStatusBar, SB_SETTEXT, 0, (LPARAM)BufTextStatus );
}

void *memset(void *s, int c, size_t n)
{
int i;
    for (i = 0; i < n; i++)
        ((UCHAR *) s)[i] = (UCHAR) c;
        return s;
}

extern "C" { void WinMainCRTStartup(void)
{
INT iExitCode;
INT iCmdShow;
STARTUPINFO StartupInformation;
HINSTANCE g_hInst;
LPSTR g_lpCommandLine;

    g_hInst = GetModuleHandle(NULL);
    g_lpCommandLine = GetCommandLine();
    GetStartupInfo(&StartupInformation);
    if( StartupInformation.dwFlags & STARTF_USESHOWWINDOW )
    {
        iCmdShow = StartupInformation.wShowWindow;
    } else 
    {
        iCmdShow = SW_SHOWNORMAL;
    }
    iExitCode = WinMain(g_hInst, NULL, g_lpCommandLine, iCmdShow);
    ExitProcess(iExitCode);
}
};

long CutPath( char *s )
{
int i;
    i = lstrlen(s);
    while(*(s+i--) != '\\' && i );
    *(s+i+2) = 0;
    return( i );
}

long mostra(char *s)
{
char buf[1111];
    wsprintf((LPSTR)buf, "mostra: %s", s);
    MessageBox(NULL, buf, "mostra.", MB_OK);
	return(0);
}
