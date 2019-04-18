#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <d2d1_3.h>
#include <d3d11.h>
#include <dcomp.h>
#include <wincodec.h>
#include <wrl.h>
using namespace Microsoft::WRL;

#pragma comment(lib, "d2d1")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "dcomp")
#pragma comment(lib, "windowscodecs")

#include "resource.h"

struct Helper
{
private:
	static void CreateBitmapFromDecoder(
		IWICImagingFactory2* factory, 
		IWICBitmapDecoder* decoder, 
		ID2D1DeviceContext* dc, 
		ID2D1Bitmap1** d2Bmp,
		IWICBitmap** bmp
	)
	{
		ComPtr<IWICBitmapFrameDecode>		frame;			//一帧图像数据
		ComPtr<IWICFormatConverter>			format;			//格式转换器

		decoder->GetFrame(0, &frame);		//从解码器中获取一帧图像

		factory->CreateFormatConverter(&format);
		format->Initialize(
			frame.Get(),
			GUID_WICPixelFormat32bppPBGRA,	//转换图像格式为RGBA格式
			WICBitmapDitherTypeNone,
			nullptr,
			1.0,
			WICBitmapPaletteTypeCustom);

		factory->CreateBitmapFromSource(	//利用转换完成的图像数据创建位图
			format.Get(),
			WICBitmapCacheOnDemand,
			bmp);

		auto hr = dc->CreateBitmapFromWicBitmap(*bmp, d2Bmp);
	}

public:
	static void LoadPictureFromResource(
		LPCWSTR name, 
		LPCWSTR type,
		IWICImagingFactory2* factory, 
		ID2D1DeviceContext* dc, 
		ID2D1Bitmap1** d2Bmp,
		IWICBitmap** wicBmp)
	{
		//1、锁定资源的位置和大小
		auto imageResHandle = FindResource(nullptr, name, type);
		auto imageResDataHandle = LoadResource(nullptr, imageResHandle);
		auto pImageFile = LockResource(imageResDataHandle);
		auto imageFileSize = SizeofResource(nullptr, imageResHandle);

		//2、创建流
		ComPtr<IWICStream> stream;
		factory->CreateStream(&stream);
		stream->InitializeFromMemory(reinterpret_cast<WICInProcPointer>(pImageFile), imageFileSize);

		//3、创建解码器
		ComPtr<IWICBitmapDecoder> decoder;		//解码器
		factory->CreateDecoderFromStream(
			stream.Get(),
			nullptr,
			WICDecodeMetadataCacheOnDemand,
			&decoder);

		CreateBitmapFromDecoder(factory, decoder.Get(), dc, d2Bmp, wicBmp);
	}

	static void LoadPictureFromHBitmap(
		HBITMAP hBitmap,
		IWICImagingFactory2* factory,
		ID2D1DeviceContext* dc,
		ID2D1Bitmap1** d2Bmp,
		IWICBitmap** wicBmp)
	{
		factory->CreateBitmapFromHBITMAP(hBitmap, nullptr, WICBitmapUsePremultipliedAlpha, wicBmp);
		dc->CreateBitmapFromWicBitmap(*wicBmp, d2Bmp);
	}

	
	static HRESULT Save(IWICImagingFactory2* factory,LPCTSTR filename,IWICBitmapSource* bmp)
	{
		HRESULT hr = S_OK;
		UINT width, height;
		hr = bmp->GetSize(&width, &height);

		ComPtr<IWICStream> stream;
		hr = factory->CreateStream(&stream);
		hr = stream->InitializeFromFilename(filename, GENERIC_WRITE);

		ComPtr<IWICBitmapEncoder> encoder;
		hr = factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder);
		hr = encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache);
		CoInitialize(nullptr);
		ComPtr<IWICBitmapFrameEncode> frame;
		hr = encoder->CreateNewFrame(&frame, nullptr);
		hr = frame->Initialize(nullptr);
		auto format = GUID_ContainerFormatBmp;
		hr = frame->SetPixelFormat(&format);

		hr = frame->SetSize(width, height);
		hr = frame->WriteSource(bmp, nullptr);
		hr = frame->Commit();
		hr = encoder->Commit();
		stream->Commit(STGC_DEFAULT);
		return hr;
	}
};