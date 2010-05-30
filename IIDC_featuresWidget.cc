#include "IIDC_featuresWidget.hh"

#include <stdio.h>
#include <math.h>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Box.H>
#include <map>
using namespace std;

#define FEATURE_HEIGHT 20
#define PACK_SPACING   10
#define LABEL_SPACE    120
#define MODE_BOX_WIDTH 180
#define SETTING_WIDTH  200

IIDC_featuresWidget::IIDC_featuresWidget(dc1394camera_t *_camera,
                                         int X,int Y,int W,int H,const char*l)
    : Fl_Scroll(X, Y, W, H, l), camera(_camera)
{
    map<modeSelection_t, const char*> modeStrings;
    modeStrings[OFF]          = "Off";
    modeStrings[AUTO]         = "Auto";
    modeStrings[AUTO_SINGLE]  = "One-push auto";
    modeStrings[MAN_RELATIVE] = "Manual in relative coordinates";
    modeStrings[MAN_ABSOLUTE] = "Manual in absolute coordinates";

    map<dc1394feature_mode_t, modeSelection_t> modeMapping;
    modeMapping[DC1394_FEATURE_MODE_MANUAL]        = MAN_RELATIVE;
    modeMapping[DC1394_FEATURE_MODE_AUTO]          = AUTO;
    modeMapping[DC1394_FEATURE_MODE_ONE_PUSH_AUTO] = AUTO_SINGLE;

    map<dc1394feature_t, const char*> featureNames;
    featureNames[DC1394_FEATURE_BRIGHTNESS]      = "Brightness";
    featureNames[DC1394_FEATURE_EXPOSURE]        = "Exposure";
    featureNames[DC1394_FEATURE_SHARPNESS]       = "Sharpness";
    featureNames[DC1394_FEATURE_WHITE_BALANCE]   = "White balance";
    featureNames[DC1394_FEATURE_HUE]             = "Hue";
    featureNames[DC1394_FEATURE_SATURATION]      = "Saturation";
    featureNames[DC1394_FEATURE_GAMMA]           = "Gamma";
    featureNames[DC1394_FEATURE_SHUTTER]         = "Shutter";
    featureNames[DC1394_FEATURE_GAIN]            = "Gain";
    featureNames[DC1394_FEATURE_IRIS]            = "Iris";
    featureNames[DC1394_FEATURE_FOCUS]           = "Focus";
    featureNames[DC1394_FEATURE_TEMPERATURE]     = "Temperature";
    featureNames[DC1394_FEATURE_TRIGGER]         = "Trigger";
    featureNames[DC1394_FEATURE_TRIGGER_DELAY]   = "Trigger delay";
    featureNames[DC1394_FEATURE_WHITE_SHADING]   = "White shading";
    featureNames[DC1394_FEATURE_FRAME_RATE]      = "Frame rate";
    featureNames[DC1394_FEATURE_ZOOM]            = "Zoom";
    featureNames[DC1394_FEATURE_PAN]             = "Pan";
    featureNames[DC1394_FEATURE_TILT]            = "Tilt";
    featureNames[DC1394_FEATURE_OPTICAL_FILTER]  = "Optical filter";
    featureNames[DC1394_FEATURE_CAPTURE_SIZE]    = "Capture size";
    featureNames[DC1394_FEATURE_CAPTURE_QUALITY] = "Capture quality";

    box(FL_FLAT_BOX);

    Fl_Pack* pack = new Fl_Pack(X, Y, W, H);
    pack->spacing(PACK_SPACING);
    {
        dc1394featureset_t featureSet;
        dc1394_feature_get_all(camera, &featureSet);

        int Xmode   = X + LABEL_SPACE;
        int Xslider = X + LABEL_SPACE + MODE_BOX_WIDTH;
        for(int i=0; i<DC1394_FEATURE_NUM; i++)
        {
            if(!featureSet.feature[i].available)
                continue;

            Fl_Group* featureGroup = new Fl_Group(X, Y, W, FEATURE_HEIGHT);
            {
                featureUI_t featureUI;

                Fl_Box* label = new Fl_Box(X, Y, LABEL_SPACE, FEATURE_HEIGHT, featureNames[ featureSet.feature[i].id ]);
                label->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);

                Fl_Choice* modes = new Fl_Choice(Xmode, Y, MODE_BOX_WIDTH, FEATURE_HEIGHT);

                for(unsigned int m=0; m<featureSet.feature[i].modes.num; m++)
                {
                    modeSelection_t modeChoice = modeMapping[ featureSet.feature[i].modes.modes[m] ];
                    modes->add(modeStrings[ modeChoice ]);
                    featureUI.choiceIndices[modeChoice] = featureUI.modeChoices.size();
                    featureUI.modeChoices.push_back(modeChoice);
                }

                if(featureSet.feature[i].on_off_capable == DC1394_TRUE)
                {
                    modeSelection_t modeChoice = OFF;
                    modes->add(modeStrings[ modeChoice ]);
                    featureUI.choiceIndices[modeChoice] = featureUI.modeChoices.size();
                    featureUI.modeChoices.push_back(modeChoice);
                }

                if(featureSet.feature[i].absolute_capable == DC1394_TRUE)
                {
                    modeSelection_t modeChoice = MAN_ABSOLUTE;
                    modes->add(modeStrings[ modeChoice ]);
                    featureUI.choiceIndices[modeChoice] = featureUI.modeChoices.size();
                    featureUI.modeChoices.push_back(modeChoice);
                }

                Fl_Value_Slider* setting = new Fl_Value_Slider(Xslider, Y, SETTING_WIDTH, FEATURE_HEIGHT);
                setting->type(FL_HOR_SLIDER);
                setting->deactivate();

                featureUI.id      = featureSet.feature[i].id;
                featureUI.modes   = modes;
                featureUI.setting = setting;
                featureUIs.push_back(featureUI);
            }
            featureGroup->end();
        }
    }
    pack->end();

    end();

    syncControls();
}

void IIDC_featuresWidget::syncControls(void)
{
    for(vector<featureUI_t>::iterator itr = featureUIs.begin();
        itr != featureUIs.end();
        itr++)
    {
        dc1394feature_info_t feature;
        feature.id = (*itr).id;
        dc1394_feature_get(camera, &feature);

        if(feature.on_off_capable && feature.is_on == DC1394_OFF)
        {
            (*itr).modes->value( (*itr).choiceIndices[OFF] );
            (*itr).setting->deactivate();
        }
        else if(feature.current_mode == DC1394_FEATURE_MODE_AUTO)
        {
            (*itr).modes->value( (*itr).choiceIndices[AUTO] );
            (*itr).setting->deactivate();
        }
        else if(feature.current_mode == DC1394_FEATURE_MODE_ONE_PUSH_AUTO)
        {
            (*itr).modes->value( (*itr).choiceIndices[AUTO_SINGLE] );
            (*itr).setting->deactivate();
        }
        else if(feature.current_mode == DC1394_FEATURE_MODE_MANUAL)
        {
            (*itr).setting->activate();

            if(feature.absolute_capable == DC1394_TRUE && feature.abs_control == DC1394_ON)
            {
                (*itr).modes->value( (*itr).choiceIndices[MAN_ABSOLUTE] );
                (*itr).setting->bounds(feature.abs_min, feature.abs_max);
                (*itr).setting->value(feature.abs_value);

                double range = feature.abs_max - feature.abs_min;
                double minstep = range / 10000.0;
                (*itr).setting->precision(floor(log(10) / log(minstep)));
            }
            else
            {
                (*itr).modes->value( (*itr).choiceIndices[MAN_RELATIVE] );
                (*itr).setting->bounds(feature.min, feature.max);
                (*itr).setting->value(feature.value);
                (*itr).setting->precision(0); // integers
            }
        }
        else
        {
            fprintf(stderr, "IIDC_featuresWidget can't syncControls():\n");
            dc1394_feature_print(&feature, stderr);
        }
    }
}
