#include "Pch.h"

class Application
{
	HINSTANCE m_hInstance;
	HWND m_hWnd;
	UINT m_width, m_height;

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		auto& app = *(reinterpret_cast<Application*>(GetWindowLongPtr(hWnd, GWLP_USERDATA)));
		switch (msg)
		{
		case WM_DESTROY:PostQuitMessage(0); break;
		case WM_KEYDOWN:app.Handle_WM_KEYDOWN(wParam, lParam); break;
		case WM_LBUTTONDOWN:app.Handle_WM_LBUTTONDOWN(wParam, lParam); break;
		case WM_LBUTTONUP:app.Handle_WM_LBUTTONUP(wParam, lParam); break;
		case WM_MOUSEMOVE:app.Handle_WM_MOUSEMOVE(wParam, lParam); break;
		case WM_CLIPBOARDUPDATE:app.Handle_WM_CLIPBOARDUPDATE(wParam, lParam); break;
		default:return DefWindowProc(hWnd, msg, wParam, lParam);
		}
		return 0;
	}

	void Handle_WM_KEYDOWN(WPARAM wParam, LPARAM lParam)
	{
		if (wParam == VK_ESCAPE)
		{
			ShowWindow(m_hWnd, SW_HIDE);
		}
		if (wParam == VK_F1)
		{
			DestroyWindow(m_hWnd);
		}
		if (wParam == VK_RETURN)
		{
			WICRect clipRc;
			clipRc.X = (int)m_selectedRegion.left;
			clipRc.Y = (int)m_selectedRegion.top;
			clipRc.Width = (int)(m_selectedRegion.right - m_selectedRegion.left)+1;
			clipRc.Height = (int)(m_selectedRegion.bottom - m_selectedRegion.top)+1;


			ComPtr<IWICBitmapClipper> clipper;
			m_factory->CreateBitmapClipper(&clipper);
			clipper->Initialize(m_wicBmp.Get(), &clipRc);

			SYSTEMTIME systm;
			GetSystemTime(&systm);
			TCHAR buf[MAX_PATH];
			wsprintf(buf, L"E:\\Capture\\%d-%d-%d-%d-%d-%d.bmp",
				systm.wYear, systm.wMonth, systm.wDay, systm.wHour, systm.wMinute, systm.wSecond);

			Helper::Save(m_factory.Get(), buf, clipper.Get());
		}
	}

	bool flag = false;
	D2D1_POINT_2F pt1;
	void Handle_WM_LBUTTONDOWN(WPARAM wParam, LPARAM lParam)
	{
		flag = true;
		pt1.x = LOWORD(lParam);
		pt1.y = HIWORD(lParam);
		SetCapture(m_hWnd);
		RECT rc;
		GetWindowRect(m_hWnd, &rc);
		ClipCursor(&rc);
	}
	void Handle_WM_LBUTTONUP(WPARAM wParam, LPARAM lParam)
	{
		flag = false;
		ReleaseCapture();
		ClipCursor(nullptr);
	}
	void Handle_WM_MOUSEMOVE(WPARAM wParam, LPARAM lParam)
	{
			D2D1_POINT_2F pt2;
			pt2.x = LOWORD(lParam);
			pt2.y = HIWORD(lParam);

		if (flag)
		{
			m_selectedRegion.left = min(pt1.x, pt2.x);
			m_selectedRegion.right = max(pt1.x, pt2.x);
			m_selectedRegion.top = min(pt1.y, pt2.y);
			m_selectedRegion.bottom = max(pt1.y, pt2.y);

			UpdateFr(m_selectedRegion);
		}
		UpdateFocus(pt2.x, pt2.y);
		m_device->Commit();
	}
	void Handle_WM_CLIPBOARDUPDATE(WPARAM wParam, LPARAM lParam)
	{
		if (OpenClipboard(m_hWnd))
		{
			HBITMAP hBitmap = (HBITMAP)GetClipboardData(CF_BITMAP);
			if (hBitmap)
			{
				Helper::LoadPictureFromHBitmap(hBitmap, m_factory.Get(), m_dc.Get(), &m_bmp,&m_wicBmp);
				AdjustWindowPos();
				UpdateBg();
				UpdateFr({});
				m_device->Commit();

				ShowWindow(m_hWnd, SW_NORMAL);
			}
		}
		CloseClipboard();
	}

	ComPtr<IDCompositionDesktopDevice>						m_device;
	ComPtr<IDCompositionTarget>								m_bgTarget, m_frTarget;
	ComPtr<IDCompositionVisual2>							m_bgVisual, m_frVisual, m_focusVisual;
	ComPtr<IDCompositionVirtualSurface>						m_bgContent, m_frContent,m_focusContent;

	D2D1_SIZE_U	m_focusSize;

	ComPtr<ID2D1DeviceContext>								m_dc;
	ComPtr<ID2D1Bitmap1>									m_bmp;
	ComPtr<ID2D1SolidColorBrush>							m_brush;
	ComPtr<IWICImagingFactory2>								m_factory;			//WIC¹¤³§
	ComPtr<IWICBitmap>										m_wicBmp;

	D2D1_RECT_F												m_selectedRegion;

	void CreateResources()
	{
		D3D_FEATURE_LEVEL featureLevelSupported;
		ComPtr<ID3D11Device> d3dDevice;
		D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION, d3dDevice.GetAddressOf(), &featureLevelSupported, nullptr);
		ComPtr<IDXGIDevice> dxgiDevice;
		d3dDevice.As(&dxgiDevice);
		ComPtr<ID2D1Device> d2dDevice;
		D2D1CreateDevice(dxgiDevice.Get(), D2D1::CreationProperties(D2D1_THREADING_MODE_SINGLE_THREADED, D2D1_DEBUG_LEVEL_NONE, D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS), &d2dDevice);
		d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_dc);
		m_dc->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::DodgerBlue), &m_brush);
		DCompositionCreateDevice3(d2dDevice.Get(), IID_PPV_ARGS(&m_device));
		CoInitialize(nullptr);
		CoCreateInstance(CLSID_WICImagingFactory2, nullptr,CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_factory));

		m_device->CreateTargetForHwnd(m_hWnd, FALSE, &m_bgTarget);
		m_device->CreateTargetForHwnd(m_hWnd, TRUE, &m_frTarget);

		m_device->CreateVisual(&m_bgVisual);
		m_device->CreateVisual(&m_frVisual);
		m_device->CreateVisual(&m_focusVisual);

		m_device->CreateVirtualSurface(m_width, m_height, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_ALPHA_MODE_PREMULTIPLIED, &m_bgContent);
		m_device->CreateVirtualSurface(m_width, m_height, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_ALPHA_MODE_PREMULTIPLIED, &m_frContent);
		m_device->CreateVirtualSurface(m_focusSize.width, m_focusSize.height, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_ALPHA_MODE_PREMULTIPLIED, &m_focusContent);

		m_bgTarget->SetRoot(m_bgVisual.Get()); m_bgVisual->SetContent(m_bgContent.Get());
		m_frTarget->SetRoot(m_frVisual.Get()); m_frVisual->SetContent(m_frContent.Get());
		m_frVisual->AddVisual(m_focusVisual.Get(), true, nullptr); m_focusVisual->SetContent(m_focusContent.Get());

		Helper::LoadPictureFromResource(MAKEINTRESOURCE(IDR_IMAGE_PNG), L"IMAGE", m_factory.Get(), m_dc.Get(), &m_bmp, &m_wicBmp);
		AdjustWindowPos();


		UpdateBg();
		UpdateFr(m_selectedRegion);
		m_device->Commit();
		return;
	}

	void UpdateBg()
	{
		POINT offset;
		ComPtr<ID2D1DeviceContext> dc;
		m_bgContent->BeginDraw(nullptr, IID_PPV_ARGS(&dc), &offset);
		if (dc)
		{
			dc->DrawBitmap(m_bmp.Get(),nullptr);
		}
		m_bgContent->EndDraw();
	}

	void UpdateFr(D2D1_RECT_F rc)
	{
		POINT offset;
		ComPtr<ID2D1DeviceContext> dc;
		m_frContent->BeginDraw(nullptr, IID_PPV_ARGS(&dc), &offset);
		if (dc)
		{
			dc->Clear({ 0.f,0.f,0.f,0.3f });
			dc->DrawBitmap(m_bmp.Get(), rc, 1.f, D2D1_INTERPOLATION_MODE_LINEAR, &rc);
			dc->DrawRectangle(rc, m_brush.Get(), 2.f);
		}
		m_frContent->EndDraw();
	}

	void UpdateFocus(float x,float y)
	{
		POINT offset;
		ComPtr<ID2D1DeviceContext> dc;
		m_focusContent->BeginDraw(nullptr, IID_PPV_ARGS(&dc), &offset);
		if (dc)
		{
			dc->SetTransform(D2D1::Matrix3x2F::Translation(offset.x, offset.y));
			dc->Clear(0);

			float delt = 16.f;
			auto w = m_focusSize.width / delt; auto h = m_focusSize.height / delt;

			D2D1_RECT_F src = {x-w/2,y-h/2,x + w / 2,y + h / 2 };

			D2D1_RECT_F drc = { 0,0,(float)m_focusSize.width ,(float)m_focusSize.height };

			dc->DrawBitmap(m_bmp.Get(), drc, 1.f, D2D1_INTERPOLATION_MODE_LINEAR, src);
		}
		m_focusContent->EndDraw();
	}

	void AdjustWindowPos()
	{
		auto size = m_bmp->GetPixelSize();
		m_width = size.width;
		m_height = size.height;

		m_selectedRegion = { 0,0,(float)m_width,(float)m_height };

		auto cx = GetSystemMetrics(SM_CXSCREEN);
		auto cy = GetSystemMetrics(SM_CYSCREEN);

		SetWindowPos(m_hWnd, nullptr, (cx - m_width) / 2, (cy - m_height) / 2, m_width, m_height, SWP_NOZORDER);

		m_bgContent->Resize(m_width, m_height);
		m_frContent->Resize(m_width, m_height);

		m_focusVisual->SetOffsetY((float)m_height - m_focusSize.height);

		m_device->Commit();
	}

public:
	Application(UINT width, UINT height, HINSTANCE hInstance) :
		m_width(width),
		m_height(height),
		m_hInstance(hInstance),
		m_focusSize{200u,200u}
	{}

	int Run(int nCmdShow)
	{
		WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
		wc.style = CS_VREDRAW | CS_HREDRAW;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = m_hInstance;
		wc.hIcon = LoadIcon(m_hInstance, IDI_APPLICATION);
		wc.hCursor = LoadCursor(nullptr, IDC_CROSS);
		wc.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
		wc.lpszClassName = L"ScreenCapture";
		wc.hIconSm = nullptr;

		RegisterClassEx(&wc);

		DWORD style = WS_OVERLAPPEDWINDOW ^ WS_SIZEBOX ^ WS_SYSMENU ^ WS_CAPTION ^ WS_DLGFRAME;
		DWORD styleEx = 0;
		
		m_hWnd = CreateWindowEx(styleEx, wc.lpszClassName, L"ScreenCapture - 2019", style,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, m_hInstance, nullptr);

		SetWindowLong(m_hWnd, GWL_STYLE, 0);
		//SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		SetWindowLongPtr(m_hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

		CreateResources();
		AddClipboardFormatListener(m_hWnd);

		ShowWindow(m_hWnd, nCmdShow);

		MSG msg = {};
		while (GetMessage(&msg, nullptr, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		return static_cast<int>(msg.wParam);
	}
};

int __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdline, int nCmdShow)
{
	return Application(1280, 720, hInstance).Run(nCmdShow);
}