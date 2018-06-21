#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

class Window
{
public:
	Window(HINSTANCE instance, LPCTSTR windowName, LPCTSTR windowTitle, int width, int height);
	HWND GetHandle() const { return hwnd; }
	UINT width() const { return m_Width; }
	UINT height()const { return m_Height; }
	float aspectRatio() const;
	void SetTitle(const char * title) const;
private:
	UINT m_Width;
	UINT m_Height;
	HWND hwnd;
};

