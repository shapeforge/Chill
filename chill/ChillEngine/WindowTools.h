#ifndef WINDOWTOOLS_H
#define WINDOWTOOLS_H

#ifdef WIN32

#define NOMINMAX // disable windows min() max() macros

#include "windows.h"

typedef HWND                WindowHandle;
typedef PROCESS_INFORMATION ProcessInformation;

#elif UNIX

#include <X11/Xlib.h>
#include <X11/Xatom.h>

typedef Window WindowHandle;
typedef struct ProcessInformation {
  __pid_t       hProcess = -1;
  pthread_t     hThread  =  0;
  unsigned long dwProcessId;
  unsigned long dwThreadId;
} ProcessInformation;

#endif

#include <vector>
#include <stack>
#include <algorithm>

#include <iostream>
#include <fstream>
#include <filesystem>

#include <cstdint>
#include <string>



#ifdef WIN32

static void setIcon() {
  HWND window = NULL;
  HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(16001));
  SendMessage(window, WM_SETICON, ICON_BIG  , (LPARAM)hIcon);
  SendMessage(window, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
}

BOOL CALLBACK EnumWindowsFromPid(HWND hwnd, LPARAM lParam)
{
  DWORD pID;
  GetWindowThreadProcessId(hwnd, &pID);
  if (pID == lParam)
  {
    NodeEditor::Instance()->m_icesl_hwnd = hwnd;
    return FALSE;
  }
  return TRUE;
}

BOOL CALLBACK TerminateAppEnum(HWND hwnd, LPARAM lParam)
{
  DWORD pID;
  GetWindowThreadProcessId(hwnd, &pID);

  if (pID == (DWORD)lParam)
  {
    PostMessage(hwnd, WM_CLOSE, 0, 0);
  }

  return TRUE;
}
#endif //WIN32



#ifdef UNIX

static void setIcon() {

}

/* Based on https://stackoverflow.com/questions/151407/how-to-get-an-x11-window-from-a-process-id */

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <iostream>
#include <list>

class WindowsMatchingPid
{
  public:
    WindowsMatchingPid(Display *display, Window wRoot, unsigned long pid)
      : _display(display)
      , _pid(pid)
    {
      // Get the PID property atom.
      _atomPID = XInternAtom(display, "_NET_WM_PID", True);
      if(_atomPID == None)
      {
        std::cout << "No such atom" << std::endl;
        return;
      }

      search(wRoot);
    }

    const std::list<Window> &result() const { return _result; }

  private:
    unsigned long  _pid;
    Atom           _atomPID;
    Display       *_display;
    std::list<Window>   _result;

    void search(Window w)
    {
      // Get the PID for the current Window.
      Atom           type;
      int            format;
      unsigned long  nItems;
      unsigned long  bytesAfter;
      unsigned char *propPID = 0;
      if(Success == XGetWindowProperty(_display, w, _atomPID, 0, 1, False, XA_CARDINAL,
                                       &type, &format, &nItems, &bytesAfter, &propPID))
      {
        if(propPID != 0)
        {
          // If the PID matches, add this window to the result set.
          if(_pid == *((unsigned long *)propPID))
            _result.push_back(w);

          XFree(propPID);
        }
      }

      // Recurse into child windows.
      Window    wRoot;
      Window    wParent;
      Window   *wChild;
      unsigned  nChildren;
      if(0 != XQueryTree(_display, w, &wRoot, &wParent, &wChild, &nChildren))
      {
        for(unsigned i = 0; i < nChildren; i++)
          search(wChild[i]);
      }
    }
};

#endif //UNIX



static void setChillIcon() {
  static bool icon_changed = false;
  if (!icon_changed) {
    setIcon();
    icon_changed = true;
  }
}

static WindowHandle getWindowHandle() {
  return 0;
}


static bool modalWindow(const char* _modalTitle = "",  const char* _modalText = "") {
#ifdef WIN32
      uint modalFlags = MB_OKCANCEL | MB_DEFBUTTON1 | MB_SYSTEMMODAL | MB_ICONINFORMATION;

      int modal = MessageBox(m_chill_hwnd, modalText, modalTitle, modalFlags);

      if (modal != 1) return false;
#else
      std::system( ("xmessage \""+ std::string(_modalText) + "\" -title \"" + std::string(_modalTitle) + "\"").c_str() );
#endif
      return true;
}


#endif //WINDOWTOOLS_H
