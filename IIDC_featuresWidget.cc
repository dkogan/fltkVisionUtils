#include "IIDC_featuresWidget.hh"

#include <stdio.h>
#include <math.h>
#include <map>
using namespace std;

#define FEATURE_HEIGHT 20
#define PACK_SPACING   10
#define LABEL_SPACE    120
#define MODE_BOX_WIDTH 180
#define SETTING_WIDTH  200
#define UNITS_WIDTH    70

static void settingChanged(Fl_Widget* widget, void* cookie)
{
    ((IIDC_featuresWidget*)cookie)->settingsChanged(widget);
}

static void modeChanged(Fl_Widget* widget, void* cookie)
{
    ((IIDC_featuresWidget*)cookie)->modeChanged(widget);
}

void IIDC_featuresWidget::addModeUI(Fl_Choice* modes,
                                    map<modeSelection_t, const char*>& modeStrings,
                                    modeSelection_t modeChoice)
{
    modes->add(modeStrings[ modeChoice ]);
    featureUIs.back()->choiceIndices[modeChoice] = featureUIs.back()->modeChoices.size();
    featureUIs.back()->modeChoices.push_back(modeChoice);
}

void IIDC_featuresWidget::initMappings(map<modeSelection_t, const char*>&          modeStrings,
                                       map<dc1394feature_mode_t, modeSelection_t>& modeMapping,
                                       map<dc1394feature_t, const char*>&          featureNames,
                                       map<dc1394feature_t, const char*>&          absUnits)
{
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

    // These are from the IIDC spec. Tables B.1 and B.2 on TA document 2003017
    absUnits[DC1394_FEATURE_BRIGHTNESS]          = "%";
    absUnits[DC1394_FEATURE_EXPOSURE]            = "EV";
    absUnits[DC1394_FEATURE_WHITE_BALANCE]       = "K";
    absUnits[DC1394_FEATURE_HUE]                 = "deg";
    absUnits[DC1394_FEATURE_SATURATION]          = "%";
    absUnits[DC1394_FEATURE_SHUTTER]             = "sec";
    absUnits[DC1394_FEATURE_GAIN]                = "dB";
    absUnits[DC1394_FEATURE_IRIS]                = "F";
    absUnits[DC1394_FEATURE_FOCUS]               = "m";
    absUnits[DC1394_FEATURE_TRIGGER]             = "times";
    absUnits[DC1394_FEATURE_TRIGGER_DELAY]       = "sec";
    absUnits[DC1394_FEATURE_FRAME_RATE]          = "fps";
    absUnits[DC1394_FEATURE_ZOOM]                = "power";
    absUnits[DC1394_FEATURE_PAN]                 = "deg";
    absUnits[DC1394_FEATURE_TILT]                = "deg";

    modeMapping[DC1394_FEATURE_MODE_MANUAL]        = MAN_RELATIVE;
    modeMapping[DC1394_FEATURE_MODE_AUTO]          = AUTO;
    modeMapping[DC1394_FEATURE_MODE_ONE_PUSH_AUTO] = AUTO_SINGLE;

    modeStrings[OFF]          = "Off";
    modeStrings[AUTO]         = "Auto";
    modeStrings[AUTO_SINGLE]  = "One-push auto";
    modeStrings[MAN_RELATIVE] = "Manual in relative coordinates";
    modeStrings[MAN_ABSOLUTE] = "Manual in absolute coordinates";
}

IIDC_featuresWidget::IIDC_featuresWidget(dc1394camera_t *_camera,
                                         int X,int Y,int W,int H,const char*l,
                                         bool doResizeNatural)
    : Fl_Pack(X, Y, W, H, l), camera(_camera),
      widestFeatureLabel(0), widestUnitLabel(0)
{
    int ww, hh;

    map<modeSelection_t, const char*>          modeStrings;
    map<dc1394feature_mode_t, modeSelection_t> modeMapping;
    map<dc1394feature_t, const char*>          featureNames;
    map<dc1394feature_t, const char*>          absUnits;

    initMappings(modeStrings, modeMapping, featureNames, absUnits);

    spacing(PACK_SPACING);
    resizable(NULL);

    dc1394featureset_t featureSet;
    dc1394_feature_get_all(camera, &featureSet);

    int Xmode   = X + LABEL_SPACE;
    int Xslider = X + LABEL_SPACE + MODE_BOX_WIDTH;
    for(int i=0; i<DC1394_FEATURE_NUM; i++)
    {
        if(!featureSet.feature[i].available)
            continue;

        Fl_Group* featureGroup = new Fl_Group(X, Y, W, FEATURE_HEIGHT);
        featureGroup->resizable(NULL);
        {
            featureUIs.push_back(new featureUI_t);

            Fl_Box* featureLabel = new Fl_Box(X, Y, LABEL_SPACE, FEATURE_HEIGHT,
                                              featureNames[ featureSet.feature[i].id ]);
            featureLabel->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
            featureLabel->measure_label(ww, hh);
            if(ww > widestFeatureLabel) widestFeatureLabel = ww;

            Fl_Choice* modes = new Fl_Choice(Xmode, Y, MODE_BOX_WIDTH, FEATURE_HEIGHT);
            modes->callback(&::modeChanged, this);

            for(unsigned int m=0; m<featureSet.feature[i].modes.num; m++)
                addModeUI(modes, modeStrings, modeMapping[ featureSet.feature[i].modes.modes[m] ]);

            if(featureSet.feature[i].on_off_capable == DC1394_TRUE)
                addModeUI(modes, modeStrings, OFF);

            if(featureSet.feature[i].absolute_capable == DC1394_TRUE)
                addModeUI(modes, modeStrings, MAN_ABSOLUTE);

            Fl_Value_Slider* setting = new Fl_Value_Slider(Xslider, Y, SETTING_WIDTH, FEATURE_HEIGHT);
            setting->type(FL_HOR_SLIDER);
            setting->callback(&::settingChanged, this);

            Fl_Box* unitsWidget = new Fl_Box(Xslider, Y, UNITS_WIDTH, FEATURE_HEIGHT,
                                             absUnits[ featureSet.feature[i].id ]);
            unitsWidget->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
            unitsWidget->measure_label(ww, hh);
            if(ww > widestUnitLabel) widestUnitLabel = ww;

            featureUIs.back()->id           = featureSet.feature[i].id;
            featureUIs.back()->featureLabel = featureLabel;
            featureUIs.back()->modes        = modes;
            featureUIs.back()->setting      = setting;
            featureUIs.back()->unitsWidget  = unitsWidget;

            // I pass "this" to the widget callbacks, but I would like to have another "user
            // data" area for the featureUI structure. I store it in the user data area of the
            // parent of my widgets. This has an additional advantage in that both the slider
            // and the selector have the same parent, so they should both reference this same
            // memory
            modes->parent()->user_data((void*)featureUIs.back());
        }
        featureGroup->end();
    }

    end();

    // Now that I added all of the widgets, I know how wide the widest labels need to be, and can
    // resize everything accordingly
    for(vector<featureUI_t*>::iterator itr = featureUIs.begin();
        itr != featureUIs.end();
        itr++)
    {
        Fl_Box* featureLabel = (*itr)->featureLabel;
        featureLabel->size(widestFeatureLabel, FEATURE_HEIGHT);

        Fl_Choice* modes = (*itr)->modes;
        modes->position(widestFeatureLabel, Y);

        Fl_Value_Slider* setting = (*itr)->setting;
        setting->position(widestFeatureLabel + MODE_BOX_WIDTH, Y);

        // I hide the units widget until I know that I need it
        Fl_Box* unitsWidget = (*itr)->unitsWidget;
        unitsWidget->resize(widestFeatureLabel + MODE_BOX_WIDTH + SETTING_WIDTH - widestUnitLabel, Y,
                            widestUnitLabel, FEATURE_HEIGHT);
        unitsWidget->hide();
    }
    for(int i=0; i<children(); i++)
        child(i)->size(widestFeatureLabel + MODE_BOX_WIDTH + SETTING_WIDTH, FEATURE_HEIGHT);

    if(doResizeNatural)
    {
        int ww, hh;
        getNaturalSize(&ww, &hh);
        size(ww, hh);
    }
    parent()->init_sizes();


    syncControls();
}

void IIDC_featuresWidget::getNaturalSize(int* ww, int* hh)
{
    *ww = widestFeatureLabel + MODE_BOX_WIDTH + SETTING_WIDTH;
    *hh = (children() - 1) * (FEATURE_HEIGHT + PACK_SPACING) + FEATURE_HEIGHT;
}

IIDC_featuresWidget::~IIDC_featuresWidget()
{
    for(vector<featureUI_t*>::iterator itr = featureUIs.begin();
        itr != featureUIs.end();
        itr++)
    {
        delete *itr;
    }
}

void IIDC_featuresWidget::syncControls(void)
{
    for(vector<featureUI_t*>::iterator itr = featureUIs.begin();
        itr != featureUIs.end();
        itr++)
    {
        dc1394feature_info_t feature;
        feature.id = (*itr)->id;
        dc1394_feature_get(camera, &feature);

        if(feature.on_off_capable && feature.is_on == DC1394_OFF)
        {
            (*itr)->modes->value( (*itr)->choiceIndices[OFF] );
            (*itr)->setting->deactivate();
        }
        else if(feature.current_mode == DC1394_FEATURE_MODE_AUTO)
        {
            (*itr)->modes->value( (*itr)->choiceIndices[AUTO] );
            (*itr)->setting->deactivate();
        }
        else if(feature.current_mode == DC1394_FEATURE_MODE_ONE_PUSH_AUTO)
        {
            (*itr)->modes->value( (*itr)->choiceIndices[AUTO_SINGLE] );
            (*itr)->setting->deactivate();
        }
        else if(feature.current_mode == DC1394_FEATURE_MODE_MANUAL)
        {
            (*itr)->setting->activate();

            if(feature.absolute_capable == DC1394_TRUE && feature.abs_control == DC1394_ON)
            {
                (*itr)->modes->value( (*itr)->choiceIndices[MAN_ABSOLUTE] );
                (*itr)->setting->bounds(feature.abs_min, feature.abs_max);
                (*itr)->setting->value(feature.abs_value);

                double range = feature.abs_max - feature.abs_min;
                double minstep = range / 10000.0;
                (*itr)->setting->precision(ceil(-log(minstep) / log(10)));

                // show the units
                (*itr)->setting->size(SETTING_WIDTH - widestUnitLabel, FEATURE_HEIGHT);
                (*itr)->unitsWidget->show();
                continue;
            }
            else
            {
                (*itr)->modes->value( (*itr)->choiceIndices[MAN_RELATIVE] );
                (*itr)->setting->bounds(feature.min, feature.max);
                (*itr)->setting->value(feature.value);
                (*itr)->setting->precision(0); // integers
            }
        }
        else
        {
            fprintf(stderr, "IIDC_featuresWidget can't syncControls():\n");
            dc1394_feature_print(&feature, stderr);
            continue;
        }

        // we get here only if we do not need to show the absolute units. Hide that widget
        (*itr)->setting->size(SETTING_WIDTH, FEATURE_HEIGHT);
        (*itr)->unitsWidget->hide();
    }
}

void IIDC_featuresWidget::settingsChanged(Fl_Widget* widget)
{
    featureUI_t* feature = (featureUI_t*)widget->parent()->user_data();
    modeSelection_t mode = feature->modeChoices[ feature->modes->value() ];

    switch(mode)
    {
    case MAN_RELATIVE:
        dc1394_feature_set_value(camera, feature->id, (uint32_t)feature->setting->value());
        return;

    case MAN_ABSOLUTE:
        dc1394switch_t absPwr;
        dc1394_feature_get_absolute_control(camera, feature->id, &absPwr);
        if(absPwr != DC1394_ON)
        {
            fprintf(stderr, "IIDC_featuresWidget: setting changed while not in absolute mode\n");
            return;
        }
        dc1394_feature_set_absolute_value(camera, feature->id, (float)feature->setting->value());
        return;

    default: ;
    }

    fprintf(stderr, "IIDC_featuresWidget::settingsChanged(): not in manual mode...\n");
}

void IIDC_featuresWidget::modeChanged(Fl_Widget* widget)
{
    featureUI_t* feature = (featureUI_t*)widget->parent()->user_data();
    modeSelection_t mode = feature->modeChoices[ feature->modes->value() ];

    switch(mode)
    {
    case OFF:
        dc1394_feature_set_power(camera, feature->id, DC1394_OFF);
        break;

    case AUTO:
        dc1394_feature_set_power(camera, feature->id, DC1394_ON);
        dc1394_feature_set_mode(camera, feature->id, DC1394_FEATURE_MODE_AUTO);
        break;

    case AUTO_SINGLE:
        dc1394_feature_set_power(camera, feature->id, DC1394_ON);
        dc1394_feature_set_mode(camera, feature->id, DC1394_FEATURE_MODE_ONE_PUSH_AUTO);
        break;

    case MAN_RELATIVE:
        dc1394_feature_set_power(camera, feature->id, DC1394_ON);
        dc1394_feature_set_mode(camera, feature->id, DC1394_FEATURE_MODE_MANUAL);
        dc1394_feature_set_absolute_control(camera, feature->id, DC1394_OFF);
        break;

    case MAN_ABSOLUTE:
        dc1394_feature_set_power(camera, feature->id, DC1394_ON);
        dc1394_feature_set_mode(camera, feature->id, DC1394_FEATURE_MODE_MANUAL);
        dc1394_feature_set_absolute_control(camera, feature->id, DC1394_ON);
        break;

    default:
        fprintf(stderr, "IIDC_featuresWidget::modeChanged(): unhandled switch...\n");
        break;
    }

    syncControls();
}
