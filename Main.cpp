#include <Windows.h>
#include <windowsx.h>
#include <Dbt.h>
#include <string>
#include <sstream>
#include <UserEnv.h>
#include <vector>
#include <CommCtrl.h>
#include "tinyxml2\tinyxml2.h"

const wchar_t windowsClassName[] = L"KindleClass";

std::wstring utf8toUtf16(const std::string & str){
   if (str.empty())
      return std::wstring();

   size_t charsNeeded = ::MultiByteToWideChar(CP_UTF8, 0, 
      str.data(), (int)str.size(), NULL, 0);
   if (charsNeeded == 0)
      throw std::runtime_error("Failed converting UTF-8 string to UTF-16");

   std::vector<wchar_t> buffer(charsNeeded);
   int charsConverted = ::MultiByteToWideChar(CP_UTF8, 0, 
      str.data(), (int)str.size(), &buffer[0], buffer.size());
   if (charsConverted == 0)
      throw std::runtime_error("Failed converting UTF-8 string to UTF-16");

   return std::wstring(&buffer[0], charsConverted);
}

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
		NULL); // file wasn't oppened with overlapped
	if(!success) return std::wstring(L"Error! Couldn't write to file");
	CloseHandle(destinationFileHandle);
	return NULL;
}

std::wstring getEbookFolderPath(){
	HANDLE hToken;
	bool success = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);
	if(!success){
		CloseHandle(hToken);
		return NULL;
	}
	DWORD profileDirLength = 512;
	wchar_t profileDir[512];
	success = GetUserProfileDirectory(hToken, profileDir, &profileDirLength);
	if(!success){
		unsigned long a = GetLastError();
		wchar_t buffer[256];
		wsprintfW(buffer, L"%u", a);

		OutputDebugStringW(buffer);
		CloseHandle(hToken);
		return NULL;
	}

	CloseHandle(hToken);

	// got documents folder path; moving on...
	std::wstringstream filePath;
	filePath << profileDir << "\\Documents\\ebooks\\";
	return filePath.str();
}

std::wstring getManifestXMLPath(){
	std::wstringstream filePath;
	filePath << getEbookFolderPath() << "thingToRead.xml";
	return filePath.str();
}

class Book{
private:
	std::string title;
	std::string author;
	std::string filelocation;
public:
	Book(const tinyxml2::XMLElement* readElement){
		this->title = readElement->FirstChildElement("Title")->GetText();
		this->author = readElement->FirstChildElement("Author")->GetText();
		this->filelocation = readElement->FirstChildElement("Filename")->GetText();
	}

	void writeBookDataToPath(std::wstring path){
		CopyFile(std::wstring(filelocation.begin(), filelocation.end()).c_str(), path.c_str(), FALSE);
	}

	std::string getTitle(){
		const std::string temp = title;
		return temp;
	}

	std::string getAuthor(){
		const std::string temp = author;
		return temp;
	}

	std::string getFileLocation(){
		const std::string temp = filelocation;
		return temp;
	}

};

class Library{
private:
	std::vector<Book*> bookList;
public:
	Library(std::wstring XMLPath){

		std::string pathUTF = std::string(XMLPath.begin(), XMLPath.end());
		tinyxml2::XMLDocument xmlManifest;
		if(xmlManifest.LoadFile(pathUTF.c_str()) == NULL){ // it works
			tinyxml2::XMLElement* rootLib = xmlManifest.FirstChildElement("Library");
			for(tinyxml2::XMLElement* currentBook = rootLib->FirstChildElement("Book"); currentBook != NULL; currentBook = currentBook->NextSiblingElement("Book")){
				// goes through each book in xml library
				bookList.push_back(new Book(currentBook));
			}
		}
	}
	~Library(){
		OutputDebugString(L"Problem!");
		// cleanup vector
		for(std::vector<Book*>::iterator bookIterator = bookList.begin(); bookIterator != bookList.end(); bookIterator++){
			delete *bookIterator;
		}
	}

	std::vector<Book*> const &getBookList() const {
		return bookList;
	}
};

NOTIFYICONDATA icondata = {};

enum TrayIcon{
	ID = 13,
	CALLBACKID = WM_APP+1
};

// Open, options, and exit
enum MenuItems{
	OPEN = WM_APP+3,
	OPTIONS = WM_APP+4,
	EXIT = WM_APP + 5
};


void quit(){
	Shell_NotifyIcon(NIM_DELETE, &icondata);
	PostQuitMessage(0);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
	switch(msg){
	case WM_DESTROY:
		quit();
		break;
	case WM_COMMAND:
		if(HIWORD(wParam) == 0){// menu
			switch(LOWORD(wParam)){ // which menu item
			case MenuItems::OPEN:
				ShowWindow(hwnd, SW_RESTORE);
				break;
			case MenuItems::OPTIONS:
				break;
			case MenuItems::EXIT:
				quit();
				break;
			}
		}
		break;
		// special ones here
	case TrayIcon::CALLBACKID:
		switch (LOWORD(lParam)){
		case WM_RBUTTONDOWN:{
			// get mouse position
			POINT p;
			GetCursorPos(&p);
			const short xpos = p.x;
			const short ypos = p.y;
			// *maybe* parameterize into functions
			// set "Open" menu item info
			std::wstring openString = L"Open";
			MENUITEMINFO openInfo;
			ZeroMemory(&openInfo, sizeof(openInfo));
			openInfo.cbSize = sizeof(openInfo);
			openInfo.fMask = MIIM_ID | MIIM_STATE | MIIM_STRING;
			openInfo.fType = MFT_STRING;
			openInfo.fState = MFS_ENABLED;
			openInfo.dwTypeData = const_cast<LPWSTR>(openString.c_str());
			openInfo.cch = openString.length();
			openInfo.wID = MenuItems::OPEN;

			// set "Options" menu item info
			std::wstring optionsString = L"Options";
			MENUITEMINFO optionsInfo;
			ZeroMemory(&optionsInfo, sizeof(optionsInfo));
			optionsInfo.cbSize = sizeof(optionsInfo);
			optionsInfo.fMask = MIIM_ID | MIIM_STATE | MIIM_STRING;
			optionsInfo.fType = MFT_STRING;
			optionsInfo.fState = MFS_ENABLED;
			optionsInfo.dwTypeData = const_cast<LPWSTR>(optionsString.c_str());
			optionsInfo.cch = optionsString.length();
			optionsInfo.wID = MenuItems::OPTIONS;

			// set "Exit" menu item info
			std::wstring exitString = L"Exit";
			MENUITEMINFO exitInfo;
			ZeroMemory(&exitInfo, sizeof(exitInfo));
			exitInfo.cbSize = sizeof(exitInfo);
			exitInfo.fMask = MIIM_ID | MIIM_STATE | MIIM_STRING;
			exitInfo.fType = MFT_STRING;
			exitInfo.fState = MFS_DEFAULT;
			exitInfo.dwTypeData = const_cast<LPWSTR>(exitString.c_str());
			exitInfo.cch = exitString.length();
			exitInfo.wID = MenuItems::EXIT;


			// actually make popup menu
			HMENU menu = CreatePopupMenu();
			InsertMenuItem(menu, 0, TRUE, &openInfo);
			InsertMenuItem(menu, 1, TRUE, &optionsInfo);
			InsertMenuItem(menu, 2, TRUE, &exitInfo);

			SetForegroundWindow(hwnd);
			TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_LEFTBUTTON, xpos, ypos, NULL, hwnd, NULL);
							}
		default:
			break;
		}
		break;

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

					Library* bookLib = new Library(getManifestXMLPath());
					std::vector<Book*> bookList = bookLib->getBookList();


					SetForegroundWindow(hwnd);
					RECT rect;
					GetWindowRect(hwnd, &rect);

					HWND hListBox = CreateWindowEx(
						WS_EX_CLIENTEDGE,
						L"LISTBOX",
						L"Book Box",
						WS_CHILD | WS_VISIBLE,
						0, //x
						0, //y
						rect.right-rect.left, //width
						rect.bottom-rect.top, //height
						hwnd,
						NULL,
						GetModuleHandle(NULL),
						NULL);

					if(!hListBox){
						MessageBox(hwnd,
							L"Failure creating window",
							L"Fail!",
							MB_ICONERROR | MB_OK);
					}
					else{
						ListView_SetExtendedListViewStyle(hListBox, LVS_EX_CHECKBOXES);
						
						const int bookListSize = bookList.size();
						for(int i = 0; i < bookListSize; i++){
							Book* book = bookList[i];

							std::wstring title = utf8toUtf16(book->getTitle());
							std::wstring author = utf8toUtf16(book->getAuthor());

							std::wstring message = title + std::wstring(L" - ") + author;

							int pos = (int)SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM) message.c_str()); 
							// Set the array index of the player as item data.
							// This enables us to retrieve the item from the array
							// even after the items are sorted by the list box.
							SendMessage(hListBox, LB_SETITEMDATA, pos, (LPARAM) i); 
						}
					}
					ShowWindow(hwnd, SW_RESTORE);
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
	Library localLib(getManifestXMLPath());
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
	ShowWindow(hWnd, SW_MINIMIZE);

	// create taskbar icon
	icondata.cbSize = sizeof(NOTIFYICONDATA);
	icondata.uVersion = NOTIFYICON_VERSION_4;
	icondata.hWnd = hWnd;
	icondata.uID = TrayIcon::ID;
	icondata.uFlags = NIF_ICON | NIF_MESSAGE;
	icondata.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	icondata.uCallbackMessage = TrayIcon::CALLBACKID;
	// add icon
	Shell_NotifyIcon(NIM_ADD, &icondata);



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
