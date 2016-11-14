#ifndef SVG_CTRL_
#define SVG_CTRL_

#include "agg_path_storage.h"
#include "agg_conv_transform.h"
#include "agg_conv_stroke.h"
#include "agg_conv_contour.h"
#include "agg_conv_curve.h"
#include "agg_color_rgba.h"
#include "agg_renderer_scanline.h"
#include "agg_bounding_rect.h"
#include "agg_rasterizer_scanline_aa.h"
#include "agg_svg_path_tokenizer.h"

#include "agg_svg_path_renderer.h"
#include "agg_svg_parser.h"

#include <string>
#include <vector>
class SvgCtrl
{
public:
    typedef agg::pixfmt_bgra32 pixfmt;
    typedef agg::renderer_base<pixfmt> renderer_base;
    typedef agg::renderer_scanline_aa_solid<renderer_base> renderer_solid;

    SvgCtrl(const char* fname)
    {
        agg::svg::parser p(m_path);
        p.parse(fname);
        m_path.arrange_orientations();
        m_path.bounding_rect(&m_min_x, &m_min_y, &m_max_x, &m_max_y);

        on_size(m_max_x - m_min_x, m_max_y - m_min_y);
    }
    ~SvgCtrl()
    {
    }
    
    void on_size(int x, int y)
    {
        int width = x;
        int height = y;
        int stride = width/4*4;
        data.resize(stride * height * pixfmt::pix_width);
        ren_buf.attach(&data[0], width, height, stride);
        pixf.attach(ren_buf);
        renderer_base rb(pixf);
        //renderer_base.clear(fill_color);???
    }

    virtual agg::pixfmt_bgra32& buf()
    {
        render();
        return pixf;
    }

    virtual bool in_rect(double x, double y) const
    {
        return false;
    }

    //------------------------------------------------------------------------
    virtual bool on_mouse_button_down(double x, double y)
    {
        return false;
    }
    //------------------------------------------------------------------------
    virtual bool on_mouse_move(double x, double y, bool button_flag)
    {
        return false;
    }

    //------------------------------------------------------------------------
    virtual bool on_mouse_button_up(double, double)
    {
        return false;
    }
private:

    void render()
    {
        renderer_base rb(pixf);
        renderer_solid ren(rb);

        agg::rasterizer_scanline_aa<> ras;
        agg::scanline_p8 sl;
        agg::trans_affine mtx;

        //�Ƚ�����ԭ���ƶ���SVGͼ������Ȼ����ת,���ƻ�ȥ,�ƻ�ȥ�ľ��뿼��scale����
        double scale1 = (double)pixf.width()  / (m_max_x - m_min_x);
        double scale2 = (double)pixf.height() / (m_max_y - m_min_y);
        double scale = scale2 < scale1?scale2:scale1;
        mtx *= agg::trans_affine_translation((m_min_x + m_max_x) * -0.5, (m_min_y + m_max_y) * -0.5);
        mtx *= agg::trans_affine_scaling(scale);
        int x1 = (m_min_x + m_max_x) * 0.5 * scale;
        int y1 = (m_min_y + m_max_y) * 0.5 * scale;
        mtx *= agg::trans_affine_translation(x1, y1);
        
        agg::rect_i rc;
        rc.x1 = rc.y1 = 0;
        rc.x2 = pixf.width();
        rc.y2 = pixf.height();
        m_path.render(ras, sl, ren, mtx, rc, opacity);
    }
public:
    //svg��Χ
    double m_min_x;
    double m_min_y;
    double m_max_x;
    double m_max_y;

    agg::svg::path_renderer m_path;
    agg::pixfmt_bgra32 pixf;
    int x1,y1;
    double opacity;
    std::string Name;

    std::vector<unsigned char> data;
    agg::rendering_buffer ren_buf;
};

class CtrlContainer : public SvgCtrl
{
    struct CtrlInner  
    {
        SvgCtrl* ctrl;
        int x,y;
    };
public:
    CtrlContainer(const char* path):SvgCtrl(path)
    {
        pod_ctrl.capacity(10);
    }
    virtual void AddCtrl(SvgCtrl* c)
    {
        CtrlInner inner;
        inner.ctrl = c;
        inner.x = 0;
        inner.y = 0;
        pod_ctrl.push_back(inner);
    }
    agg::pixfmt_bgra32& buf()
    {
        for (int i = 0; i < pod_ctrl.size(); i++)
        {
            agg::pixfmt_bgra32& subBuf = pod_ctrl[i].ctrl->buf();
            int x1 = pod_ctrl[i].ctrl->x1;
            int y1 = pod_ctrl[i].ctrl->y1;
            int subLen = subBuf.width();

            //��������buf������x1,y1
            pixf.copy_from(subBuf, x1, y1, 0, 0, subLen);
        }

        return pixf;
    }
    
    SvgCtrl* findCtrl(std::string Name)
    {
    }

    virtual bool in_rect(double x, double y) const
    {
        return false;
    }

    //------------------------------------------------------------------------
    virtual bool on_mouse_button_down(double x, double y)
    {
        for (int i = 0; i < pod_ctrl.size(); i++)
        {
            pod_ctrl[i].ctrl->on_mouse_button_down(x,y);
        }
        return false;
    }
    //------------------------------------------------------------------------
    virtual bool on_mouse_move(double x, double y, bool button_flag)
    {
        for (int i = 0; i < pod_ctrl.size(); i++)
        {
            pod_ctrl[i].ctrl->on_mouse_move(x,y,button_flag);
        }
        return false;
    }

    //------------------------------------------------------------------------
    virtual bool on_mouse_button_up(double x, double y)
    {
        for (int i = 0; i < pod_ctrl.size(); i++)
        {
            pod_ctrl[i].ctrl->on_mouse_button_up(x,y);
        }
        return false;
    }
protected:
    agg::pod_vector<CtrlInner> pod_ctrl;
};


/*
<vlayout alian=bottom width=200 height=200 gap=10>
<slider name="movieProgress"></slider>
<hlayout >
<button name="play"></button>
<button name="stop"></button>
<button name="pause"></button>
<button name="next"></button>
</hlayout>
</vlayout>

button* pPlayerBtn = getCtrl();
pPlayerBtn->onClick(onMyClick);

slider* pSlider = getCtrl();
pSlider->onChange(onSliderChange);

while(1)
{
    pSlider->newPos();
}

*/


class Layout : public CtrlContainer
{
public:
    Layout(const char* path):CtrlContainer(path){}

    virtual void setSize(int width, int height)
    {
        CalcuLayout();
    }
    virtual void AddCtrl(SvgCtrl* c)
    {
        CtrlContainer::AddCtrl(c);
        CalcuLayout();
    }
    virtual void CalcuLayout() = 0;
protected:
private:
};

class HorizontalLayout : public CtrlContainer
{
public:
    HorizontalLayout(const char* path):CtrlContainer(path){gap = 3;}
    virtual void CalcuLayout()
    {
        int EachCtrlSize = ((m_max_x - m_min_x) - pod_ctrl.size()*gap)/pod_ctrl.size();
        for (int i = 0; i < pod_ctrl.size(); i++)
        {
            agg::rect_i rc;
            rc.x1 = EachCtrlSize*i;
            rc.x2 = rc.x1 + EachCtrlSize;
            rc.y1 = m_min_y;
            rc.y2 = m_max_y;

            pod_ctrl[i].ctrl->on_size(rc.x2-rc.x1, rc.y2-rc.x1);
            pod_ctrl[i].x = rc.x1;
            pod_ctrl[i].y = rc.y1;
        }
    }
private:
    int gap;
};

class VerticalLayout : public CtrlContainer
{
public:
    VerticalLayout(const char* path):CtrlContainer(path){gap = 3;}
    virtual void CalcuLayout()
    {
        int EachCtrlSize = ((m_max_x - m_min_x) - pod_ctrl.size()*gap)/pod_ctrl.size();
        for (int i = 0; i < pod_ctrl.size(); i++)
        {
            agg::rect_i rc;
            rc.x1 = m_min_x;
            rc.x2 = m_min_y;
            rc.y1 = EachCtrlSize*i;
            rc.y2 = rc.y1 + EachCtrlSize;

            pod_ctrl[i].ctrl->on_size(rc.x2-rc.x1, rc.y2-rc.x1);
            pod_ctrl[i].x = rc.x1;
            pod_ctrl[i].y = rc.y1;
        }
    }
private:
    int gap;
};

class Button : public SvgCtrl
{
public:
    Button(const char* path):SvgCtrl(path){}
    virtual void on_click()
    {

    }
protected:
private:
};

#endif