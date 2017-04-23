#include "stdafx.h"
#include "renderD3D.h"
#include <d3dx9tex.h>
#include <wchar.h>
#include <stdio.h>
#include <assert.h>
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p)=NULL; } }
#endif
#include <string>
std::basic_string<TCHAR> g_strModuleFileName;
const std::basic_string<TCHAR>& XGetModuleFilename()
{

	if (g_strModuleFileName.empty())
	{
		if(g_strModuleFileName.empty())
		{
			TCHAR filename[MAX_PATH] = { 0 };
			::GetModuleFileName(NULL, filename, _countof(filename));
			g_strModuleFileName = filename;
		}
	}
	return g_strModuleFileName;
}
std::basic_string<TCHAR> GetModulePath()
{
	std::basic_string<TCHAR> strModuleFileName = XGetModuleFilename();
	unsigned int index = strModuleFileName.find_last_of(L"\\");
	if (index != std::string::npos)
	{
		return strModuleFileName.substr(0, index);
	}
	return L"";
}


#define VFYUV420P
typedef IDirect3D9* (WINAPI* LPDIRECT3DCREATE9)( UINT );


typedef HRESULT (WINAPI *DX9CTFromFileInMemory)(LPDIRECT3DDEVICE9 pDevice,LPCVOID pSrcData,UINT  SrcDataSize,LPDIRECT3DTEXTURE9*  ppTexture);
typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);  
#include <core\SkCanvas.h>
#include <core\SkBitmap.h>
#include <core\SkTypeface.h>

DX9CTFromFileInMemory fpDX9CTFromFileInMemory=NULL;
LPFN_ISWOW64PROCESS fnIsWow64Process;  
BOOL IsWow64()  
{  
	BOOL bIsWow64 = FALSE;  

	//IsWow64Process is not available on all supported versions of Windows.  
	//Use GetModuleHandle to get a handle to the DLL that contains the function  
	//and GetProcAddress to get a pointer to the function if available.  

	fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(  
		GetModuleHandle(TEXT("kernel32")),"IsWow64Process");  

	if(NULL != fnIsWow64Process)  
	{  
		if (!fnIsWow64Process(GetCurrentProcess(),&bIsWow64))  
		{  
			//handle error  
		}  
	}  
	return bIsWow64;  
}  
CRenderD3D::CRenderD3D()
    :m_hView(NULL)
    ,m_pD3D(NULL)
    ,m_pDevice(NULL)
	,m_pDxTexture(NULL)
	,Fontexture(NULL)
	,m_pWaitPicTexture(NULL)
	,m_CenterLogoTexture(NULL)
	,m_pLeftPicTexture(NULL)
	,m_pYUVAVTexture(NULL)
	,m_pBackBuffer(NULL)
	,m_bShowErrPic(FALSE)
	,m_ErrTexture(NULL)
	,m_ErrBufImg(NULL)
    ,m_ErrBufImgLen(0)
	,m_lastpicw(0)
	,m_lastpich(0)
{

}

CRenderD3D::~CRenderD3D()
{
	
	ResetTexture();
    SAFE_RELEASE(m_pDevice);
    SAFE_RELEASE(m_pD3D);
}
void CRenderD3D::DrawFontInfo()
{
	
}

LPCSTR GetUTF8String(LPCWSTR str)
{
	int iLen = ::WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);

	if (iLen > 1)
	{
		char *pBuf = new char[iLen];
		::WideCharToMultiByte(CP_UTF8, 0, str, -1, pBuf, iLen, NULL, NULL);
		return pBuf;
	}

	return NULL;
}
void DrawFontSkia()
{
	
	
}
D3DPRESENT_PARAMETERS GetPresentParams(HWND hView)
{
    D3DPRESENT_PARAMETERS PresentParams;
    ZeroMemory(&PresentParams, sizeof(PresentParams));
    PresentParams.BackBufferFormat =D3DFMT_UNKNOWN;// D3DFMT_X8R8G8B8;
    PresentParams.BackBufferCount=2;
    PresentParams.MultiSampleType = D3DMULTISAMPLE_NONE;
    PresentParams.MultiSampleQuality = 0;
    PresentParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
    PresentParams.hDeviceWindow = hView;
    PresentParams.Windowed = TRUE;
    PresentParams.EnableAutoDepthStencil = TRUE;
    PresentParams.AutoDepthStencilFormat = D3DFMT_D24X8;
    PresentParams.Flags = D3DPRESENTFLAG_VIDEO;
    PresentParams.FullScreen_RefreshRateInHz = 0;
    PresentParams.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    return PresentParams; 
}


bool CRenderD3D::init(HWND hView)
{
	m_hView = hView;
	HMODULE hModD3D9 = LoadLibraryA("d3d9.dll");
	if(!hModD3D9)
	{
		std::wstring path=GetModulePath();
		if(IsWow64())
			path+=L"\\win7\\d3d9.dll";
		else
		{
			path+=L"\\xp\\d3d9.dll";
		}
		hModD3D9 = LoadLibrary(path.c_str());
	}
	if (hModD3D9)
	{

		HMODULE hD3dx9_43 = LoadLibraryA("D3dx9_43.dll");
		if(hD3dx9_43)
		{
			fpDX9CTFromFileInMemory= (DX9CTFromFileInMemory)GetProcAddress(hD3dx9_43, "D3DXCreateTextureFromFileInMemory");
		}
		
		

		LPDIRECT3DCREATE9 pfnDirect3DCreate9 = (LPDIRECT3DCREATE9)GetProcAddress(hModD3D9, "Direct3DCreate9");
		if(pfnDirect3DCreate9==NULL)
		{
			//::MessageBox(hView,L"未能加载Direct3DCreate9函数",L"提示",MB_ICONHAND);
			return false;
		}
		m_pD3D=pfnDirect3DCreate9(D3D_SDK_VERSION);//Direct3DCreate9(D3D_SDK_VERSION);//

		//DWORD BehaviorFlags =D3DCREATE_SOFTWARE_VERTEXPROCESSING;
		
		DWORD BehaviorFlags =D3DCREATE_FPU_PRESERVE | D3DCREATE_PUREDEVICE | 
			D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_NOWINDOWCHANGES;
		D3DPRESENT_PARAMETERS PresentParams = GetPresentParams(hView);

		
		HRESULT hr = m_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,GetParent(hView), BehaviorFlags, &PresentParams, &m_pDevice);
	    HRESULT hr2= D3DERR_WRONGTEXTUREFORMAT;
		hr2=D3DERR_UNSUPPORTEDCOLOROPERATION;
		hr2= D3DERR_UNSUPPORTEDCOLORARG;
		hr2= D3DERR_UNSUPPORTEDALPHAOPERATION ;
		hr2= D3DERR_UNSUPPORTEDALPHAARG;
		hr2= D3DERR_TOOMANYOPERATIONS;
		hr2= D3DERR_CONFLICTINGTEXTUREFILTER;
		hr2= D3DERR_UNSUPPORTEDFACTORVALUE;
		hr2= D3DERR_CONFLICTINGRENDERSTATE;
		hr2= D3DERR_UNSUPPORTEDTEXTUREFILTER;
		hr2= D3DERR_CONFLICTINGTEXTUREPALETTE;
		hr2=D3DERR_DRIVERINTERNALERROR;

		hr2= D3DERR_NOTFOUND;
		hr2= D3DERR_MOREDATA;
		hr2= D3DERR_DEVICELOST;
		hr2= D3DERR_DEVICENOTRESET;
		hr2= D3DERR_NOTAVAILABLE;
		hr2= D3DERR_OUTOFVIDEOMEMORY;
		hr2= D3DERR_INVALIDDEVICE;
		hr2= D3DERR_INVALIDCALL;
		hr2= D3DERR_DRIVERINVALIDCALL;


		if (FAILED(hr) && hr != D3DERR_DEVICELOST)
		{
			BehaviorFlags = D3DCREATE_FPU_PRESERVE | D3DCREATE_PUREDEVICE | D3DCREATE_SOFTWARE_VERTEXPROCESSING;
			hr = m_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,hView, BehaviorFlags, &PresentParams, &m_pDevice);
			if (FAILED(hr) && hr != D3DERR_DEVICELOST)
			{
				return false;
			}
			//::MessageBox(hView,L"d3d创建错误",L"提示",MB_ICONHAND);
			return false;
		}
		
		return true;
	}

	//::MessageBox(hView,L"未能加载d3d9.dll，请安装d3d9",L"提示",MB_ICONHAND);
	return false;
}

void CRenderD3D::destroy()
{
    delete this;
}

void CRenderD3D::resize(unsigned int w, unsigned int h)
{
    m_w=w;
	m_h=h;
   {	  
		m_LeftPicVertex[0].x = -0.5f;
	    m_LeftPicVertex[0].y = -0.5f;
	    m_LeftPicVertex[0].z = 0.f;
	    m_LeftPicVertex[0].w = 1.f;
	    m_LeftPicVertex[0].u = 0.f;
	    m_LeftPicVertex[0].v = 0.f;

	    m_LeftPicVertex[1].x = 200 - 0.5f;
	    m_LeftPicVertex[1].y = -0.5f;
	    m_LeftPicVertex[1].z = 0.f;
	    m_LeftPicVertex[1].w = 1.f;
	    m_LeftPicVertex[1].u = 1.f;
	    m_LeftPicVertex[1].v = 0.f;

	    m_LeftPicVertex[2].x = -0.5f;
	    m_LeftPicVertex[2].y = 200 - 0.5f;
	    m_LeftPicVertex[2].z = 0.f;
	    m_LeftPicVertex[2].w = 1.f;
	    m_LeftPicVertex[2].u = 0.f;
	    m_LeftPicVertex[2].v = 1.f;

	    m_LeftPicVertex[3].x = 200 - 0.5f;
	    m_LeftPicVertex[3].y = 200 - 0.5f;
	    m_LeftPicVertex[3].z = 0.f;
	    m_LeftPicVertex[3].w = 1.f;
	    m_LeftPicVertex[3].u = 1.f;
	    m_LeftPicVertex[3].v = 1.f;
   }
//没有定义 YUV420p
#ifndef VFYUV420P
   {
		m_VideoVertex[0].x = -0.5f;
		m_VideoVertex[0].y = -0.5f;
		m_VideoVertex[0].z = 0.f;
		m_VideoVertex[0].w = 1.f;
		m_VideoVertex[0].u = 0.f;
		m_VideoVertex[0].v = 0.f;

		m_VideoVertex[1].x = w - 0.5f;
		m_VideoVertex[1].y = -0.5f;
		m_VideoVertex[1].z = 0.f;
		m_VideoVertex[1].w = 1.f;
		m_VideoVertex[1].u = 1.f;
		m_VideoVertex[1].v = 0.f;

		m_VideoVertex[2].x = -0.5f;
		m_VideoVertex[2].y = h - 0.5f;
		m_VideoVertex[2].z = 0.f;
		m_VideoVertex[2].w = 1.f;
		m_VideoVertex[2].u = 0.f;
		m_VideoVertex[2].v = 1.f;

		m_VideoVertex[3].x = w - 0.5f;
		m_VideoVertex[3].y = h - 0.5f;
		m_VideoVertex[3].z = 0.f;
		m_VideoVertex[3].w = 1.f;
		m_VideoVertex[3].u = 1.f;
		m_VideoVertex[3].v = 1.f;
    }
#endif
    {
		w=200;
		h=100;
		int yx=m_w/2-w/2;
		int yy=m_h/2-h/2;
		m_FontVertex[0].x =yx -0.5f;  //A     A(leftTop)->B(rightTop)->C(leftBu)->D->(rightBu)
		m_FontVertex[0].y =yy -0.5f;
		m_FontVertex[0].z = 0.0f;
		m_FontVertex[0].w = 1.f;
		m_FontVertex[0].u = 0.f;
		m_FontVertex[0].v = 0.f;

		m_FontVertex[1].x = yx+w - 0.5f;  //B
		m_FontVertex[1].y = yy-0.5f;
		m_FontVertex[1].z = 0.0f;
		m_FontVertex[1].w = 1.f;
		m_FontVertex[1].u = 1.f;
		m_FontVertex[1].v = 0.f;

		m_FontVertex[2].x =yx -0.5f;   //C
		m_FontVertex[2].y =yy+h - 0.5f;
		m_FontVertex[2].z = 0.0f;
		m_FontVertex[2].w = 1.f;
		m_FontVertex[2].u = 0.f;
		m_FontVertex[2].v = 1.f;

		m_FontVertex[3].x =yx+ w - 0.5f; //D
		m_FontVertex[3].y =yy+ h - 0.5f;
		m_FontVertex[3].z = 0.f;
		m_FontVertex[3].w = 1.f;
		m_FontVertex[3].u = 1.f;
		m_FontVertex[3].v = 1.f;
	}

	{
		w=44;
		h=46;
		int yx=m_w/2-w/2;
		int yy=m_h/2-h/2;
		 m_WaitVertex[0].x =yx -0.5f;  //A     A(leftTop)->B(rightTop)->C(leftBu)->D->(rightBu)
		 m_WaitVertex[0].y =yy -0.5f;
		 m_WaitVertex[0].z = 0.0f;
		 m_WaitVertex[0].w = 1.f;
		 m_WaitVertex[0].u = 0.f;
		 m_WaitVertex[0].v = 0.f;

		 m_WaitVertex[1].x = yx+w - 0.5f;  //B
		 m_WaitVertex[1].y = yy-0.5f;
		 m_WaitVertex[1].z = 0.0f;
		 m_WaitVertex[1].w = 1.f;
		 m_WaitVertex[1].u = 1.f;
		 m_WaitVertex[1].v = 0.f;

		 m_WaitVertex[2].x =yx -0.5f;   //C
		 m_WaitVertex[2].y =yy+h - 0.5f;
		 m_WaitVertex[2].z = 0.0f;
		 m_WaitVertex[2].w = 1.f;
		 m_WaitVertex[2].u = 0.f;
		 m_WaitVertex[2].v = 1.f;

		 m_WaitVertex[3].x =yx+ w - 0.5f; //D
		 m_WaitVertex[3].y =yy+ h - 0.5f;
		 m_WaitVertex[3].z = 0.f;
		 m_WaitVertex[3].w = 1.f;
		 m_WaitVertex[3].u = 1.f;
		 m_WaitVertex[3].v = 1.f;
	}
	{
		
		w=97;
		h=72;
		int yx=m_w/2-w/2;
		int yy=m_h/2-h/2;
		m_CenterLogVertex[0].x =yx -0.5f;  //A     A(leftTop)->B(rightTop)->C(leftBu)->D->(rightBu)
		m_CenterLogVertex[0].y =yy -0.5f;
		m_CenterLogVertex[0].z = 0.0f;
		m_CenterLogVertex[0].w = 1.f;
		m_CenterLogVertex[0].u = 0.f;
		m_CenterLogVertex[0].v = 0.f;

		m_CenterLogVertex[1].x = yx+w - 0.5f;  //B
		m_CenterLogVertex[1].y = yy-0.5f;
		m_CenterLogVertex[1].z = 0.0f;
		m_CenterLogVertex[1].w = 1.f;
		m_CenterLogVertex[1].u = 1.f;
		m_CenterLogVertex[1].v = 0.f;

		m_CenterLogVertex[2].x =yx -0.5f;   //C
		m_CenterLogVertex[2].y =yy+h - 0.5f;
		m_CenterLogVertex[2].z = 0.0f;
		m_CenterLogVertex[2].w = 1.f;
		m_CenterLogVertex[2].u = 0.f;
		m_CenterLogVertex[2].v = 1.f;

		m_CenterLogVertex[3].x =yx+ w - 0.5f; //D
		m_CenterLogVertex[3].y =yy+ h - 0.5f;
		m_CenterLogVertex[3].z = 0.f;
		m_CenterLogVertex[3].w = 1.f;
		m_CenterLogVertex[3].u = 1.f;
		m_CenterLogVertex[3].v = 1.f;
	}
	
	WinSize(w,h);
}
void CRenderD3D::ShowErrPic(bool show)
{
   m_bShowErrPic=show;
}
void CRenderD3D::WinSize(unsigned int w, unsigned int h)
{

	m_lock.Lock();
	ResetTexture();
	D3DPRESENT_PARAMETERS PresentParams = GetPresentParams(m_hView);
	m_pDevice->Reset(&PresentParams);
	m_lock.Unlock();
}
void CRenderD3D::SetWaitPic(unsigned char* buf,int len)
{
	if (!LostDeviceRestore())
		return;
	// 将刚才构建好的bmp数据，转成IDirect3DTexture9*  的纹理  
	SAFE_RELEASE(m_pWaitPicTexture);
	if(fpDX9CTFromFileInMemory!=NULL)
	{
	    fpDX9CTFromFileInMemory(this->m_pDevice,buf, len, &m_pWaitPicTexture);
	}
	//if ( FAILED( D3DXCreateTextureFromFileInMemory( this->m_pDevice,buf, len, &m_pWaitPicTexture)))
	//{
	//	//assert(0);
	//	return;// S_FALSE;
	//}
}

void CRenderD3D::SetErrPic(unsigned char* buf,int len)
{
	m_ErrBufImg=buf;
	m_ErrBufImgLen=len;
	if (!LostDeviceRestore())
		return;
	
	SAFE_RELEASE(m_ErrTexture);
	//if ( FAILED( D3DXCreateTextureFromFileInMemory( this->m_pDevice,buf, len, &m_ErrTexture)))
	//{
	//	//assert(0);
	//	return;// S_FALSE;
	//}
	if ( FAILED(fpDX9CTFromFileInMemory(this->m_pDevice,buf, len, &m_ErrTexture))){
	   return;
	}
	D3DSURFACE_DESC rc;

	m_ErrTexture->GetLevelDesc(0,&rc);
	int w=rc.Width;
	int h=rc.Height;
	
	AdJustErrPos(w,h);
}
void CRenderD3D::AdJustErrPos(int picw,int pich)
{
	int w=picw;
	int h=pich;
	int yx=m_w/2-w/2;
	int yy=m_h/2-h/2;
	m_ErrPicVertex[0].x =yx -0.5f;  //A     A(leftTop)->B(rightTop)->C(leftBu)->D->(rightBu)
	m_ErrPicVertex[0].y =yy -0.5f;
	m_ErrPicVertex[0].z = 0.0f;
	m_ErrPicVertex[0].w = 1.f;
	m_ErrPicVertex[0].u = 0.f;
	m_ErrPicVertex[0].v = 0.f;

	m_ErrPicVertex[1].x = yx+w - 0.5f;  //B
	m_ErrPicVertex[1].y = yy-0.5f;
	m_ErrPicVertex[1].z = 0.0f;
	m_ErrPicVertex[1].w = 1.f;
	m_ErrPicVertex[1].u = 1.f;
	m_ErrPicVertex[1].v = 0.f;

	m_ErrPicVertex[2].x =yx -0.5f;   //C
	m_ErrPicVertex[2].y =yy+h - 0.5f;
	m_ErrPicVertex[2].z = 0.0f;
	m_ErrPicVertex[2].w = 1.f;
	m_ErrPicVertex[2].u = 0.f;
	m_ErrPicVertex[2].v = 1.f;

	m_ErrPicVertex[3].x =yx+ w - 0.5f; //D
	m_ErrPicVertex[3].y =yy+ h - 0.5f;
	m_ErrPicVertex[3].z = 0.f;
	m_ErrPicVertex[3].w = 1.f;
	m_ErrPicVertex[3].u = 1.f;
	m_ErrPicVertex[3].v = 1.f;

}
void CRenderD3D::render(char *pBuf,int width,int height,int Imgwidth)
{
  m_lock.Lock();
  if (!LostDeviceRestore())
  {
	  m_lock.Unlock();
        return; /* */
  }
 
	if(m_pDevice==NULL)
	{
		m_lock.Unlock();
		return;
	}
	    m_pDevice->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
		if( SUCCEEDED(m_pDevice->BeginScene()) )
		{
			if(pBuf!=NULL&&m_bShowErrPic==false)
			{
                UpdateTexture(pBuf,width,height,Imgwidth);	
#ifdef VFYUV420P
			
				if(m_pYUVAVTexture!=NULL)
				{
					if(m_pBackBuffer == NULL)
					{
						GetClientRect(m_hView,&m_rtViewport);  
						int dw=m_rtViewport.right-m_rtViewport.left;
						int dh=m_rtViewport.bottom-m_rtViewport.top;
						int h=dh,w=dw;
						//if(dw>width)
						{
                            dh=dw*height/Imgwidth;
						}
						if(dh>h)
						{
							dh=h;
							dw=Imgwidth*dh/height;
						}
                        if(dw<w)
						{
							m_rtViewport.left+=(w-dw)/2;
							m_rtViewport.right=m_rtViewport.left+dw;
						}
						if(dh<h)
						{
							m_rtViewport.top+=(h-dh)/2;
							m_rtViewport.bottom=m_rtViewport.top+dh;
						}
						m_pDevice->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&m_pBackBuffer);  
					}
					
					m_pDevice->StretchRect(m_pYUVAVTexture,NULL,m_pBackBuffer,&m_rtViewport,D3DTEXF_LINEAR);  
					
				}/**/
#else


				        int dw=m_w;
						int dh=m_h;
						int h=dh,w=dw;
					    int cx=0;
						int cy=0;
						int cw=m_w;
						int ch=m_h;
						//if(dw>width)
						{
                            dh=dw*height/Imgwidth;
						}
						if(dh>h)
						{
							dh=h;
							dw=Imgwidth*dh/height;
						}
                        if(dw<w)
						{
							cx+=(w-dw)/2;
							cw=cx+dw;
						}
						if(dh<h)
						{
							cy+=(h-dh)/2;
							ch=cy+dh;
						}

						m_VideoVertex[0].x = cx;
						m_VideoVertex[0].y = cy;
						m_VideoVertex[0].z = 0.f;
						m_VideoVertex[0].w = 1.f;
						m_VideoVertex[0].u = 0.f;
						m_VideoVertex[0].v = 0.f;

						m_VideoVertex[1].x = cw - 0.5f;
						m_VideoVertex[1].y = cy-0.5f;
						m_VideoVertex[1].z = 0.f;
						m_VideoVertex[1].w = 1.f;
						m_VideoVertex[1].u = 1.f;
						m_VideoVertex[1].v = 0.f;

						m_VideoVertex[2].x = cx;//左下角
						m_VideoVertex[2].y = ch;
						m_VideoVertex[2].z = 0.f;
						m_VideoVertex[2].w = 1.f;
						m_VideoVertex[2].u = 0.f;
						m_VideoVertex[2].v = 1.f;

						m_VideoVertex[3].x = cw;  //右下角
						m_VideoVertex[3].y = ch;
						m_VideoVertex[3].z = 0.f;
						m_VideoVertex[3].w = 1.f;
						m_VideoVertex[3].u = 1.f;
						m_VideoVertex[3].v = 1.f;

				m_pDevice->SetTexture(0, m_pDxTexture);
				m_pDevice->SetFVF(Vertex::FVF);
				m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
				m_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
				m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, m_VideoVertex, sizeof(Vertex));
#endif
				if(m_pLeftPicTexture!=NULL)
				{
					m_pDevice->SetTexture(0, m_pLeftPicTexture);
					m_pDevice->SetFVF(Vertex::FVF);
					m_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
					m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
					m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
					m_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
					m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2,  m_LeftPicVertex, sizeof(Vertex));/**/
				}
				

			}else if(pBuf==NULL&&m_pWaitPicTexture!=NULL&&m_bShowErrPic==false)
			{
				m_pDevice->SetTexture(0,  m_pWaitPicTexture);
				m_pDevice->SetFVF(Vertex::FVF);
				m_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
				m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
				m_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
				m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2,  m_WaitVertex, sizeof(Vertex));/**/
	
			}
			
			if(m_bShowErrPic==true)
			{
				if(m_ErrTexture==NULL&&m_ErrBufImg!=NULL)
				{
					SetErrPic(m_ErrBufImg,m_ErrBufImgLen);
				}
				if(m_ErrTexture!=NULL)
				{
		                m_pDevice->SetTexture(0, m_ErrTexture);
						m_pDevice->SetFVF(Vertex::FVF);
						m_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
						m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
						m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
						m_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
						m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, m_ErrPicVertex, sizeof(Vertex));/**/
				}
				
			}
			
			m_pDevice->EndScene();
			m_pDevice->Present(NULL, NULL, NULL, NULL);
		}
		 m_lock.Unlock();
}


void CRenderD3D::ResetTexture()
{

	SAFE_RELEASE(m_pDxTexture);
	SAFE_RELEASE(Fontexture);
	SAFE_RELEASE(m_CenterLogoTexture);
   
	SAFE_RELEASE(m_pYUVAVTexture);
	SAFE_RELEASE(m_pWaitPicTexture);
	SAFE_RELEASE(m_pLeftPicTexture);
	SAFE_RELEASE(m_pBackBuffer);
	SAFE_RELEASE(m_ErrTexture);
}
bool CRenderD3D::LostDeviceRestore()
{
    HRESULT hr = m_pDevice->TestCooperativeLevel();
    if (hr == D3DERR_DEVICELOST)
    {
       ResetTexture();
        return false;
    }


    if (hr == D3DERR_DEVICENOTRESET)
    {
        ResetTexture();
        D3DPRESENT_PARAMETERS PresentParams = GetPresentParams(m_hView);
        hr = m_pDevice->Reset(&PresentParams);
       
  

	int i=0;
	if (hr ==  D3DERR_NOTFOUND ){
         i=0;
	}else if (hr ==  D3DERR_MOREDATA){
         i=0;
	}else if (hr ==  D3DERR_DEVICELOST){
         i=0;
	}else if (hr ==  D3DERR_DEVICENOTRESET){
         i=0;
	}else if (hr ==  D3DERR_NOTAVAILABLE){
         i=0;
	}else if (hr ==  D3DERR_OUTOFVIDEOMEMORY){
         i=0;
	}else if (hr ==  D3DERR_INVALIDDEVICE){
         i=0;
	}else if (hr ==  D3DERR_INVALIDCALL){
         i=0;
	}else if (hr ==  D3DERR_DRIVERINVALIDCALL  ){
         i=0;
    }else if (hr ==  D3DERR_WASSTILLDRAWING     ){
         i=0;
    }else if (hr ==  D3DOK_NOAUTOGEN   ){
       i=0;
    }

   if(FAILED(hr))
            return false;/**/
    }

    return true;
}
int GetBmpSize(int w,int h);

bool Utf8toWchar(wchar_t *pwstr,size_t len,const char *str);
void charTowchar(const char *chr, wchar_t *wchar, int size);
void CRenderD3D::SetLeftPicStr(const char *str)
{

	wchar_t pwstr[1024]=L"";
    charTowchar(str,pwstr,1024);
    m_LeftStr=pwstr;
}

void  CRenderD3D::FillRect(kkBitmap img,kkRect rt,unsigned int colour)
{
    SkBitmap Skbit;
	Skbit.setInfo(SkImageInfo::Make( img.width,img.height,SkColorType::kBGRA_8888_SkColorType,SkAlphaType::kPremul_SkAlphaType));
	Skbit.setPixels(img.pixels);

	SkCanvas canvas(Skbit);

	m_skPaint.setStyle(SkPaint::kFill_Style);
	SkRect srt={rt.left,rt.top,rt.right,rt.bottom};
	m_skPaint.setARGB(255, GetRValue(colour), GetGValue(colour), GetBValue(colour));
	canvas.drawRect(srt,m_skPaint);
}
//提示
bool CRenderD3D::UpdateLeftPicTexture()
{
	if(m_LstLeftStr!=m_LeftStr)
	{
		m_LstLeftStr=m_LeftStr;/**/
		SAFE_RELEASE(m_pLeftPicTexture);
	}
	if(m_LstLeftStr.length()>7)
		return false;
//	return true;
	if (m_pLeftPicTexture == NULL)
	{
		RECT rect2;
		GetClientRect(m_hView, &rect2);
		UINT hei=rect2.bottom - rect2.top;
		UINT Wei=rect2.right - rect2.left;
		

		HRESULT  hr = m_pDevice->CreateTexture(
			200,
			200,
			0,
			D3DUSAGE_DYNAMIC,
			D3DFMT_A8R8G8B8,
			D3DPOOL_DEFAULT,
			&m_pLeftPicTexture,
			NULL);/**/
		 if (FAILED(hr))
		 {
			 m_pLeftPicTexture = NULL;
			return false;
			}
	
			D3DLOCKED_RECT rect;//1009*488
			m_pLeftPicTexture->LockRect(0, &rect, NULL, D3DLOCK_DISCARD);

			
			unsigned char* dst = (unsigned char*)rect.pBits;   
			memset(dst,0,rect.Pitch*200);
			if(1)
			{
				LPCSTR strFont = GetUTF8String(L"宋体");
				//SkTypeface *font = SkTypeface::CreateFromName(strFont, SkTypeface::kNormal);

				SkBitmap Skbit;
				Skbit.setInfo(SkImageInfo::Make(200,200,SkColorType::kBGRA_8888_SkColorType,SkAlphaType::kPremul_SkAlphaType));
				Skbit.allocPixels();
				char* XBuf= (char*)Skbit.getPixels();
				memset(XBuf,0,200*200*4);

				SkCanvas canvas(Skbit);
				SkPaint  paint; 


				//设置文字编码
				paint.setTextEncoding(SkPaint::TextEncoding::kUTF16_TextEncoding);
				paint.setTextAlign(SkPaint::Align::kLeft_Align);

				SkRect r; 
				paint.setARGB(255, 255,255, 255); /**/
				r.set(25, 25, 200, 145); 
				// canvas.drawRect(r, paint); 


				SkRect dst2 = r;
				paint.setTextSize(15);
				canvas.drawText(m_LeftStr.c_str(), m_LeftStr.length()*sizeof(WCHAR), dst2.fLeft, dst2.fTop , paint);

				

				dst = (unsigned char*)rect.pBits; 
				int yxx=0;
				int row=Skbit.rowBytes();
				for(int i = 0; i < 200; ++i) 
				{
					memcpy(dst,XBuf+yxx ,row);
					yxx+=row;
					dst += rect.Pitch;
				}
			}
			m_pLeftPicTexture->UnlockRect(0);	
	}
	return 1;
}
bool CRenderD3D::UpdateTexture(char *pBuf,int w,int h,int imgwidth)
{
	
#ifdef VFYUV420P	
   if (m_pYUVAVTexture == NULL||m_lastpicw!=w|| m_lastpich!=h)
    {
        RECT rect2;
        GetClientRect(m_hView, &rect2);
		UINT hei=rect2.bottom - rect2.top;
		UINT Wei=rect2.right - rect2.left;
		
		SAFE_RELEASE(m_pYUVAVTexture);
			if(m_w!=Wei&&hei!=m_h){
				resize( Wei,hei);
			}
			//DX YV12 就是YUV420
			HRESULT hr= m_pDevice->CreateOffscreenPlainSurface(
				imgwidth, h,
				(D3DFORMAT)MAKEFOURCC('Y', 'V', '1', '2'),
				D3DPOOL_DEFAULT,
				&m_pYUVAVTexture,
				NULL);
			if (FAILED(hr))
				return false;
			 
    }
#else
   {
	   if (m_pDxTexture == NULL)
	   {
		   RECT rect2;
		   GetClientRect(m_hView, &rect2);
		   UINT hei=rect2.bottom - rect2.top;
		   UINT Wei=rect2.right - rect2.left;
		   if(m_w!=Wei&&hei!=m_h){
		     resize( Wei,hei);
		   }

		   HRESULT  hr = m_pDevice->CreateTexture(
			   w,
			   h,
			   1,
			   D3DUSAGE_DYNAMIC,
			   D3DFMT_X8R8G8B8,
			   D3DPOOL_DEFAULT,
			   &m_pDxTexture,
			   NULL);/**/
		   if (FAILED(hr))
			   return false;
	   }
	}
#endif
   
      D3DLOCKED_RECT rect;
		#ifdef VFYUV420P
				 m_pYUVAVTexture->LockRect(&rect,NULL,D3DLOCK_DONOTWAIT);  
				 if(rect.pBits!=NULL)
				 {
						  byte *pSrc = (byte *)pBuf;  
						  byte * pDest = (BYTE *)rect.pBits;  
						  int stride = rect.Pitch;  
						  unsigned long i = 0;  
						  int pixel_h=h;
						  int pixel_w=w;
						  //Copy Data (YUV420P)  
						  //Y
						  for(i = 0;i < pixel_h;i ++)
						  {  
							  memcpy(pDest + i * stride,pSrc + i * pixel_w, imgwidth);  
						  } 
						  //U
						  for(i = 0;i < pixel_h/2;i ++)
						  {  
							  memcpy(pDest + stride * pixel_h + i * stride / 2,pSrc + pixel_w * pixel_h + pixel_w * pixel_h / 4 + i * pixel_w / 2, imgwidth / 2);  
						  }  
						  //V
						  for(i = 0;i < pixel_h/2;i ++)
						  {  
							  memcpy(pDest + stride * pixel_h + stride * pixel_h / 4 + i * stride / 2,pSrc + pixel_w * pixel_h + i * pixel_w / 2, imgwidth / 2);  
						  } /**/
				 }
				 m_pYUVAVTexture->UnlockRect();
#else
			  m_pDxTexture->LockRect(0, &rect, NULL, D3DLOCK_DISCARD);
			  unsigned char* dst = (unsigned char*)rect.pBits; 
			  unsigned char* src = (unsigned char*)pBuf; 
			  memset(dst,255,rect.Pitch*h);
			  if(pBuf!=NULL)
			  {
				  int row=w*4;
			      for(int i = 0; i < h; ++i) 
				  {
				    memcpy(dst, src,row);
				    src += row;
				    dst += rect.Pitch;
				   }
			  }
			m_pDxTexture->UnlockRect(0);		  
#endif
	UpdateLeftPicTexture();
    return true;
}




void CRenderD3D::renderBk(unsigned char* buf,int len)
{
	 m_lock.Lock();
	 if (!LostDeviceRestore()){
		m_lock.Unlock();
		return;
	 }
	m_pDevice->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0,0), 1.0f, 0);
	if( SUCCEEDED(m_pDevice->BeginScene()) )
	{
		if(m_CenterLogoTexture!=NULL)
		{
				 m_pDevice->SetTexture(0,  m_CenterLogoTexture);
				 m_pDevice->SetFVF(Vertex::FVF);
				 m_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
				 m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				 m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
				 m_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
				 m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2,  m_CenterLogVertex, sizeof(Vertex));/**/
		}

		if(m_bShowErrPic==true&&m_ErrTexture!=NULL)
		{
			m_pDevice->SetTexture(0, m_ErrTexture);
			m_pDevice->SetFVF(Vertex::FVF);
			m_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
			m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			m_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, m_ErrPicVertex, sizeof(Vertex));/**/
		}

		m_pDevice->EndScene();
		m_pDevice->Present(NULL, NULL, NULL, NULL);
	}
	  m_lock.Unlock();
}
void  CRenderD3D::LoadCenterLogo(unsigned char* buf,int len)
{
	if (!LostDeviceRestore())
		return;

	if(m_CenterLogoTexture==NULL)
	{

		if ( FAILED(fpDX9CTFromFileInMemory(this->m_pDevice,buf, len, & m_CenterLogoTexture))){
	   return;
	}
		// 将刚才构建好的bmp数据，转成IDirect3DTexture9*  的纹理  
		//if ( FAILED( D3DXCreateTextureFromFileInMemory( this->m_pDevice,buf, len, & m_CenterLogoTexture))){
		//	//assert(0);
		//	int i=0;
		//	i++;
		//	return;// S_FALSE;
		//}
	}
}