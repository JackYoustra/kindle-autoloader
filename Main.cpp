#include <Windows.h>
#include <Dbt.h>
#include <string>
#include <sstream>

const wchar_t windowsClassName[] = L"KindleClass";

// return drive letter
char FirstDriveFromMask( ULONG unitmask ){
  char i;

  for (i = 0; i < 26; ++i)
   {
    if (unitmask & 0x1)
      break;
    unitmask = unitmask >> 1;
   }

  return( i + 'A' );
}

bool stringEqualsIgnoreCase(std::wstring a, std::wstring b){
    unsigned int sz = a.size();
    if (b.size() != sz)
        return false;
    for (unsigned int i = 0; i < sz; ++i)
        if (tolower(a[i]) != tolower(b[i]))
            return false;
    return true;
}

// returns null on sucess, error message on failure
std::wstring writeToFile(LPCWSTR path, LPCVOID toWrite, DWORD byteLength){
	HANDLE destinationFileHandle = CreateFile(path, 
		GENERIC_WRITE, // wants to write
		NULL, // no sharing
		NULL, // no inheiritance to child processess
		CREATE_NEW, // create new, will fail if it doesn't work
		FILE_ATTRIBUTE_NORMAL, //normal file
		NULL); // no attribute template

	if(destinationFileHandle == INVALID_HANDLE_VALUE) return std::wstring(L"Error! Couldn't open file");
	// guarunteed to work
	bool success = WriteFile(destinationFileHandle,
		toWrite, // thing to write
		byteLength, // amount of bytes to write
		NULL, // don't care about bytes written
		false); // file wasn't oppened with overlapped
	if(!success) return std::wstring(L"Error! Couldn't write to file");
	CloseHandle(destinationFileHandle);
	return NULL;
}


LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
	switch(msg){
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	// special ones here
	case WM_DEVICECHANGE:
		if(wParam == DBT_DEVICEARRIVAL){
			PDEV_BROADCAST_HDR anydevice = (PDEV_BROADCAST_HDR)lParam;
			DWORD deviceType = anydevice->dbch_devicetype;
			if(deviceType == DBT_DEVTYP_VOLUME){
				PDEV_BROADCAST_VOLUME volumeDevice = (PDEV_BROADCAST_VOLUME)anydevice;
				// convert name to string

				char driveChar = FirstDriveFromMask(volumeDevice->dbcv_unitmask);

				std::wstringstream mutableDeviceLocation;
				mutableDeviceLocation << driveChar << ":\\";
				const std::wstring deviceID = mutableDeviceLocation.str();

				TCHAR volumeName[MAX_PATH + 1] = { 0 };
				GetVolumeInformation(
					deviceID.c_str(),
					volumeName,
					ARRAYSIZE(volumeName),
					NULL,
					NULL,
					NULL,
					NULL,
					NULL
					);

				if(stringEqualsIgnoreCase(volumeName, L"Kindle")){
					// we know that this is the device we want
					MessageBox(hwnd,
						L"We found the kindle!",
						L"Success!",
						MB_ICONEXCLAMATION | MB_OK);
				}
			}
		}
		return true;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){
	// the handle for the window, filled by a function
    HWND hWnd;
    // this struct holds information for the window class
    WNDCLASSEX wc;

    // clear out the window class for use
    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    // fill in the struct with the needed information
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = windowsClassName;

    // register the window class
    RegisterClassEx(&wc);

    // create the window and use the result as the handle
    hWnd = CreateWindowEx(NULL,
                          windowsClassName,    // name of the window class
                          L"Our First Windowed Program",   // title of the window
                          WS_OVERLAPPEDWINDOW,    // window style
                          300,    // x-position of the window
                          300,    // y-position of the window
                          500,    // width of the window
                          400,    // height of the window
                          NULL,    // we have no parent window, NULL
                          NULL,    // we aren't using menus, NULL
                          hInstance,    // application handle
                          NULL);    // used with multiple windows, NULL

    // display the window on the screen
    ShowWindow(hWnd, nCmdShow);

    // enter the main loop:

    // this struct holds Windows event messages
    MSG msg;

    // wait for the next message in the queue, store the result in 'msg'
    while(GetMessage(&msg, NULL, 0, 0))
    {
        // translate keystroke messages into the right format
        TranslateMessage(&msg);

        // send the message to the WindowProc function
        DispatchMessage(&msg);
    }

    // return this part of the WM_QUIT message to Windows
    return msg.wParam;
}





