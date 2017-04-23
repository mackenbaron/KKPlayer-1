#ifndef RENDER_GDI_H
#define RENDER_GDI_H

#include "../KKPlayerCore/render/render.h"
#include "RendLock.h"
#include <core\SkCanvas.h>
#include <core\SkBitmap.h>
#include <core\SkTypeface.h>
#include <core\SkImageDecoder.h>
#include <core\SkStream.h>
class CRenderGDI : public IkkRender
{
public:
    CRenderGDI();
    ~CRenderGDI();
    virtual bool init(HWND hView);
    virtual void destroy();
    virtual void resize(unsigned int w, unsigned int h);
	virtual void LoadCenterLogo(unsigned char* buf,int len);
	void render(char* buf,int width,int height,int Imgwidth);
	virtual void renderBk(unsigned char* buf,int len);
	void SetWaitPic(unsigned char* buf,int len);
	void SetBkImagePic(unsigned char* buf,int len);
    void WinSize(unsigned int w, unsigned int h);

	virtual void SetErrPic(unsigned char* buf,int len);
	virtual void ShowErrPic(bool show);
	virtual void FillRect(kkBitmap img,kkRect rt,unsigned int color);
	virtual void SetLeftPicStr(const char *str);
protected:
    void createBitmap(unsigned int w, unsigned int h);
    void DrawSkVideo(SkCanvas& canvas,char *buf,int w,int h);
    void skiaSal(char *buf,int w,int h);
private:

	bool m_bShowErrPic;
	SkPaint  m_Paint; 
	HWND m_hView;
	HBITMAP m_hBitmap;
	HDC m_hDC;

	//显示区域宽度
	unsigned int m_width;
	//显示区域高度
	unsigned int m_height;

	//图像区域宽度
	unsigned int m_Picwidth;
	//图像区域高度
	unsigned int m_Picheight;

	char* m_BkBuffer;
	int m_BkLen;

	char *m_WaitBuffer;
	int m_WaitLen;

	void* m_pixels;
	unsigned char* m_CenterLogoBuf;
	int m_CenterLogoBufLen;

	SkBitmap *m_pErrbitmap;
};

#endif