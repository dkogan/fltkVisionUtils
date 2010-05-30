#ifndef __IIDC_FEATURES_WIDGET_HH__
#define __IIDC_FEATURES_WIDGET_HH__

#include <dc1394/dc1394.h>
#include <FL/Fl.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Value_Slider.H>
#include <vector>
#include <map>

class IIDC_featuresWidget : public Fl_Scroll
{
    dc1394camera_t *camera;

    enum modeSelection_t { OFF, AUTO, AUTO_SINGLE, MAN_RELATIVE, MAN_ABSOLUTE };
    struct featureUI_t
    {
        dc1394feature_t                id;
        Fl_Choice*                     modes;
        Fl_Value_Slider*               setting;
        std::vector<modeSelection_t>   modeChoices;
        std::map<modeSelection_t, int> choiceIndices;
    };
    std::vector<featureUI_t>         featureUIs;

public:
    IIDC_featuresWidget(dc1394camera_t *_camera,
                        int X,int Y,int W,int H,const char*l=0);

    void syncControls(void);
};

#endif
