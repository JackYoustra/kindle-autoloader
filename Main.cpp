#include <Windows.h>
#include <Dbt.h>
#include <string>


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){
	
}

bool iequals(const std::string& a, const std::string& b)
{
    unsigned int sz = a.size();
    if (b.size() != sz)
        return false;
    for (unsigned int i = 0; i < sz; ++i)
        if (tolower(a[i]) != tolower(b[i]))
            return false;
    return true;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
	switch(msg){
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	// special ones here
	case WM_DEVICECHANGE:
		if(wParam == DBT_DEVICEARRIVAL){
			PDEV_BROADCAST_HDR anydevice = (PDEV_BROADCAST_HDR)lParam;
			DWORD deviceType = anydevice->dbch_devicetype;
			if(deviceType == DBT_DEVTYP_PORT){
				PDEV_BROADCAST_PORT portDevice = (PDEV_BROADCAST_PORT)anydevice;
				// convert name to string
				char ch[260];
				char DefChar = ' ';
				WideCharToMultiByte(CP_ACP, NULL, portDevice->dbcp_name, -1, ch, 260, &DefChar, NULL);

				std::string deviceName(ch);
				if(iequals(deviceName, "kindle")){
					// we know that this is the device we want

				}
			}
		}
		return true;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}


