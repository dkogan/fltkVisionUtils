#ifndef __IIDC_FEATURES_WIDGET_HH__
#define __IIDC_FEATURES_WIDGET_HH__

#include <dc1394/dc1394.h>
#include <FL/Fl.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Pack.H>
#include <vector>
#include <map>
using namespace std;

class IIDC_featuresWidget : public Fl_Pack
{
    dc1394camera_t* camera;
    int             widestFeatureLabel;
    int             widestUnitLabel;

    enum modeSelection_t { OFF, AUTO, AUTO_SINGLE, MAN_RELATIVE, MAN_ABSOLUTE };
    struct featureUI_t
    {
        dc1394feature_t           id;
        Fl_Box*                   featureLabel;
        Fl_Choice*                modes;
        Fl_Value_Slider*          setting;
        Fl_Box*                   unitsWidget;
        vector<modeSelection_t>   modeChoices;
        map<modeSelection_t, int> choiceIndices;
    };
    vector<featureUI_t*>          featureUIs;

    void addModeUI(Fl_Choice* modes,
                   map<modeSelection_t, const char*>& modeStrings,
                   modeSelection_t modeChoice);
    void initMappings(map<modeSelection_t, const char*>&          modeStrings,
                      map<dc1394feature_mode_t, modeSelection_t>& modeMapping,
                      map<dc1394feature_t, const char*>&          featureNames,
                      map<dc1394feature_t, const char*>&          absUnits);

public:
    IIDC_featuresWidget(dc1394camera_t *_camera,
                        int X,int Y,int W,int H,const char*l=0);
    ~IIDC_featuresWidget();

    void syncControls(void);

    // These are widget callbacks. They are called from globals and thus must be public
    void settingsChanged(Fl_Widget* widget);
    void modeChanged    (Fl_Widget* widget);
};

#endif
