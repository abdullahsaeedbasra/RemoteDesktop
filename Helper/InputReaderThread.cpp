#include "pch.h"
#include "InputReaderThread.h"
#include "ScreenCaptureThread.h"
#include "Logger.h"

IMPLEMENT_DYNCREATE(InputReaderThread, CWinThread)

InputReaderThread::InputReaderThread() : m_bRunning(FALSE)
{
	m_hStopped = CreateEvent(nullptr, TRUE, FALSE, nullptr);
}

BOOL InputReaderThread::InitInstance()
{
	return TRUE;
}

int InputReaderThread::Run()
{
	Log("Input Reader Thread started");
	SetCapsLockOff();
	OVERLAPPED olForPipe = {};
	olForPipe.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	m_bRunning = TRUE;
	while (m_bRunning)
	{
		MessageHeader hdr{};
		BOOL bRetVal = FALSE;
		DWORD dwBytesRead = 0;
		{
			CSingleLock lock(&m_pNamedPipe->csSynchronizer, TRUE);
			ReadFile(m_pNamedPipe->handle, &hdr, sizeof(hdr), &dwBytesRead, &olForPipe);
		}
		::WaitForSingleObject(olForPipe.hEvent, INFINITE);

		bRetVal = GetOverlappedResult(m_pNamedPipe->handle, &olForPipe, &dwBytesRead, FALSE);

		if (!bRetVal || dwBytesRead != sizeof(hdr))
		{
			DWORD dwError = GetLastError();
			if (dwError == ERROR_BROKEN_PIPE)
				Log("Pipe closed");
			else
				Log("Failed to read message" + to_string(dwError));
		}
		else
		{
			hdr.type = ntohl(hdr.type);
			hdr.size = ntohl(hdr.size);

			switch (hdr.type)
			{
			case MSG_CLIENT_DISCONNECTED:
				Log("Client Disconnected. Closing process");
				m_pScreenThread->SignalStop();
				m_bRunning = FALSE;
				break;

			case MSG_MOUSEMOVE:
			{
				if (hdr.size != sizeof(MouseMoveData))
				{
					Log("hdr.size != sizeof(MouseMoveData)");
					continue;
				}
				bRetVal = FALSE;
				dwBytesRead = 0;
				MouseMoveData data{};
				{
					CSingleLock lock(&m_pNamedPipe->csSynchronizer, TRUE);
					bRetVal = ReadFile(m_pNamedPipe->handle, &data, sizeof(data), &dwBytesRead, &olForPipe);
				}
				::WaitForSingleObject(olForPipe.hEvent, INFINITE);
				bRetVal = GetOverlappedResult(m_pNamedPipe->handle, &olForPipe, &dwBytesRead, FALSE);

				if (!bRetVal || dwBytesRead != sizeof(data))
				{
					DWORD dwError = GetLastError();
					if (dwError == ERROR_BROKEN_PIPE)
						Log("Pipe closed");
					else
						Log("Failed to read message" + to_string(dwError));
				}
				else
				{
					Log("Read Mouse Position Percentage Successfully");
					int percentage_x = ntohl(data.percentage_x);
					int percentage_y = ntohl(data.percentage_y);

					int x = (percentage_x * GetSystemMetrics(SM_CXSCREEN)) / 100;
					int y = (percentage_y * GetSystemMetrics(SM_CYSCREEN)) / 100;
					MoveMouse(x, y);
				}
				break;
			}

			case MSG_LEFT_BUTTON_DOWN:
				if (hdr.size != sizeof(MessageHeader))
				{
					Log("hdr.size != sizeof(MessageHeader) for left button down");
					continue;
				}
				OnLeftButtonDown();
				break;

			case MSG_LEFT_BUTTON_UP:
				if (hdr.size != sizeof(MessageHeader))
				{
					Log("hdr.size != sizeof(MessageHeader) for left button up");
					continue;
				}
				OnLeftButtonUp();
				break;

			case MSG_RIGHT_BUTTON_DOWN:
				if (hdr.size != sizeof(MessageHeader))
				{
					Log("hdr.size != sizeof(MessageHeader) for right button down");
					continue;
				}
				OnRightButtonDown();
				break;

			case MSG_RIGHT_BUTTON_UP:
				if (hdr.size != sizeof(MessageHeader))
				{
					Log("hdr.size != sizeof(MessageHeader) for right button up");
					continue;
				}
				OnRightButtonUp();
				break;

			case MSG_LEFT_BUTTON_DOUBLE_CLICK:
				if (hdr.size != sizeof(MessageHeader))
				{
					Log("hdr.size != sizeof(MessageHeader) for Left Button Double Click");
					continue;
				}
				OnLeftButtonDoubleClick();
				break;

			case MSG_MOUSE_WHEEL_UP:
				if (hdr.size != sizeof(MessageHeader))
				{
					Log("hdr.size != sizeof(MessageHeader) for mouse wheel up");
					continue;
				}
				OnMouseWheelUp();
				break;

			case MSG_MOUSE_WHEEL_DOWN:
				if (hdr.size != sizeof(MessageHeader))
				{
					Log("hdr.size != sizeof(MessageHeader) for mouse wheel down");
					continue;
				}
				OnMouseWheelDown();
				break;

			case MSG_KEYBOARD:
			{
				Log("In Keyboard case");
				if (hdr.size != sizeof(KeyData))
				{
					Log("hdr.size != sizeof(KeyData) for keyboard input");
					continue;
				}

				KeyData keyData{};
				bRetVal = FALSE;
				dwBytesRead = 0;
				{
					CSingleLock lock(&m_pNamedPipe->csSynchronizer, TRUE);
					ReadFile(m_pNamedPipe->handle, &keyData, sizeof(keyData), &dwBytesRead, &olForPipe);
				}
				::WaitForSingleObject(olForPipe.hEvent, INFINITE);

				bRetVal = GetOverlappedResult(m_pNamedPipe->handle, &olForPipe, &dwBytesRead, FALSE);

				if (!bRetVal)
				{
					DWORD dwError = GetLastError();
					if (dwError == ERROR_BROKEN_PIPE)
						Log("Pipe closed");
					else
						Log("Failed to read message KEY INPUT: " + to_string(dwError));
				}
				else
				{
					Log("READ KEY DATA SUCCESSFULLY FROM PIPE");
					keyData.flags = ntohl(keyData.flags);
					keyData.vk = ntohl(keyData.vk);
					SendKeyInput(keyData);
				}
			}
			}
		}
	}
	SetEvent(m_hStopped);
	Log("InputReaderStopped");
	return 0;
}

void InputReaderThread::SetPipe(NamedPipe* pipe)
{
	m_pNamedPipe = pipe;
}

void InputReaderThread::MoveMouse(int x, int y) {
	INPUT input = { 0 };
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;

	input.mi.dx = (x * 65536) / GetSystemMetrics(SM_CXSCREEN);
	input.mi.dy = (y * 65536) / GetSystemMetrics(SM_CYSCREEN);

	SendInput(1, &input, sizeof(INPUT));
}

void InputReaderThread::OnLeftButtonDown()
{
	INPUT input{};
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

	SendInput(1, &input, sizeof(INPUT));
}

void InputReaderThread::OnLeftButtonUp()
{
	INPUT input{};
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = MOUSEEVENTF_LEFTUP;

	SendInput(1, &input, sizeof(INPUT));
}

void InputReaderThread::OnRightButtonDown()
{
	INPUT input{};
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;

	SendInput(1, &input, sizeof(INPUT));
}

void InputReaderThread::OnRightButtonUp()
{
	INPUT input{};
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;

	SendInput(1, &input, sizeof(INPUT));
}

void InputReaderThread::OnLeftButtonDoubleClick()
{
	OnLeftButtonDown();
	OnLeftButtonUp();
	OnLeftButtonDown();
	OnLeftButtonUp();
}

void InputReaderThread::OnMouseWheelUp()
{
	INPUT input{};
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = MOUSEEVENTF_WHEEL;
	input.mi.mouseData = WHEEL_DELTA;

	SendInput(1, &input, sizeof(INPUT));
}

void InputReaderThread::OnMouseWheelDown()
{
	INPUT input{};
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = MOUSEEVENTF_WHEEL;
	input.mi.mouseData = -WHEEL_DELTA;

	SendInput(1, &input, sizeof(INPUT));
}

void InputReaderThread::SendKeyInput(const KeyData& keyData)
{
	Log("---------INSIDE SENDKEYINPUT()---------");
	INPUT input{};
	input.type = INPUT_KEYBOARD;
	input.ki.wScan = (WORD)MapVirtualKey(keyData.vk, MAPVK_VK_TO_VSC);
	input.ki.wVk = (WORD)keyData.vk;
	input.ki.dwFlags = keyData.flags;

	bool isNavigationKey = (keyData.vk >= VK_PRIOR && keyData.vk <= VK_DOWN) ||
		(keyData.vk == VK_INSERT || keyData.vk == VK_DELETE);

	if (isNavigationKey)
	{
		input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
	}

	SendInput(1, &input, sizeof(INPUT));
	Log("---------SENDKEYINPUT() CALLED---------");
}

void InputReaderThread::SetCapsLockOff()
{
	BYTE keyState[256];
	GetKeyboardState(keyState);
	if (keyState[VK_CAPITAL] & 1) 
	{
		keybd_event(VK_CAPITAL, 0x45, KEYEVENTF_EXTENDEDKEY, 0);
		keybd_event(VK_CAPITAL, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
	}
}

InputReaderThread::~InputReaderThread()
{
	m_bRunning = FALSE;
	if (m_hStopped != nullptr)
		CloseHandle(m_hStopped);
}