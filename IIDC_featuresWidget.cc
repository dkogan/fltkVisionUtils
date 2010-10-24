#include "IIDC_featuresWidget.hh"

#include <math.h>
#include <map>
#include <iostream>
using namespace std;

#define FEATURE_HEIGHT     20
#define PACK_SPACING       10
#define LABEL_SPACE        120
#define MODE_BOX_WIDTH     180
#define SETTING_WIDTH      200
#define UNITS_WIDTH        70

#define SYNC_CONTROLS_PERIOD_US 100000

#define FOREACH(itrtype, itrname, container) \
    for(itrtype itrname = container.begin(); \
        itrname != container.end();          \
        itrname++)

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

static void* syncControlsThread(void *pArg)
{
    IIDC_featuresWidget* widget = (IIDC_featuresWidget*)pArg;

    while(1)
    {
        struct timespec delay;
        delay.tv_sec  = SYNC_CONTROLS_PERIOD_US / 1000000;
        delay.tv_nsec = (SYNC_CONTROLS_PERIOD_US - delay.tv_sec*1000000) * 1000;
        nanosleep(&delay, NULL);

        Fl::lock();
        widget->syncControls();
        Fl::unlock();
    }

    return NULL;
}

void IIDC_featuresWidget::cleanupThreads(void)
{
    if(syncControlsThread_id != 0)
    {
        pthread_cancel(syncControlsThread_id);
        pthread_join(syncControlsThread_id, NULL);
        syncControlsThread_id = 0;
    }
}

IIDC_featuresWidget::IIDC_featuresWidget(dc1394camera_t *_camera,
                                         int X,int Y,int W,int H,const char*l,
                                         bool doResizeNatural)
    : Fl_Pack(X, Y, W, H, l), camera(_camera),
      widestFeatureLabel(0), widestUnitLabel(0),
      syncControlsThread_id(0)
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


            // currently I have general support for multiple-slider features, but white balance is
            // the only one that actually has multiple sliders
            int numSliders = 1;
            if(featureSet.feature[i].id == DC1394_FEATURE_WHITE_BALANCE)
                numSliders = 2;

            for(int j=0; j<numSliders; j++)
            {
                Fl_Value_Slider* setting = new Fl_Value_Slider(Xslider + j*(SETTING_WIDTH / numSliders), Y,
                                                               SETTING_WIDTH / numSliders, FEATURE_HEIGHT);
                setting->type(FL_HOR_SLIDER);
                setting->callback(&::settingChanged, this);
                featureUIs.back()->setting.push_back( setting );
            }

            Fl_Box* unitsWidget = new Fl_Box(Xslider, Y, UNITS_WIDTH, FEATURE_HEIGHT,
                                             absUnits[ featureSet.feature[i].id ]);
            unitsWidget->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
            unitsWidget->measure_label(ww, hh);
            if(ww > widestUnitLabel) widestUnitLabel = ww;

            featureUIs.back()->id           = featureSet.feature[i].id;
            featureUIs.back()->featureLabel = featureLabel;
            featureUIs.back()->modes        = modes;
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
    FOREACH(vector<featureUI_t*>::iterator, itr, featureUIs)
    {
        Fl_Box* featureLabel = (*itr)->featureLabel;
        featureLabel->size(widestFeatureLabel, FEATURE_HEIGHT);

        Fl_Choice* modes = (*itr)->modes;
        modes->position(X + widestFeatureLabel, Y);

        int numSliders = (*itr)->setting.size();
        int sliderIndex = 0;

        FOREACH(vector<Fl_Value_Slider*>::iterator, itrslider, (*itr)->setting)
        {
            (*itrslider)->position(X + widestFeatureLabel + MODE_BOX_WIDTH +
                                   sliderIndex * (SETTING_WIDTH/numSliders),
                                   Y);
            sliderIndex++;
        }

        // I hide the units widget until I know that I need it
        Fl_Box* unitsWidget = (*itr)->unitsWidget;
        unitsWidget->resize(X + widestFeatureLabel + MODE_BOX_WIDTH + SETTING_WIDTH - widestUnitLabel, Y,
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

    if(pthread_create(&syncControlsThread_id, NULL, &syncControlsThread, this) != 0)
    {
        fprintf(stderr, "IIDC_featuresWidget couldn't spawn syncControls thread\n");
        syncControlsThread_id = 0;
    }
}

void IIDC_featuresWidget::getNaturalSize(int* ww, int* hh)
{
    *ww = widestFeatureLabel + MODE_BOX_WIDTH + SETTING_WIDTH;
    *hh = (children() - 1) * (FEATURE_HEIGHT + PACK_SPACING) + FEATURE_HEIGHT;
}

IIDC_featuresWidget::~IIDC_featuresWidget()
{
    cleanupThreads();

    FOREACH(vector<featureUI_t*>::iterator, itr, featureUIs)
    {
        delete *itr;
    }
}

static inline void activateSettings(vector<Fl_Value_Slider*>& sliders)
{
    FOREACH(vector<Fl_Value_Slider*>::iterator, itrslider, sliders)
        (*itrslider)->activate();
}

static inline void deactivateSettings(vector<Fl_Value_Slider*>& sliders)
{
    FOREACH(vector<Fl_Value_Slider*>::iterator, itrslider, sliders)
        (*itrslider)->deactivate();
}

void IIDC_featuresWidget::syncControls(void)
{
    FOREACH(vector<featureUI_t*>::iterator, itr, featureUIs)
    {
        int numSliders = (*itr)->setting.size();

        dc1394feature_info_t feature;
        feature.id = (*itr)->id;
        dc1394_feature_get(camera, &feature);

        if(feature.on_off_capable && feature.is_on == DC1394_OFF)
        {
            (*itr)->modes->value( (*itr)->choiceIndices[OFF] );
            deactivateSettings((*itr)->setting);
        }
        else if(feature.current_mode == DC1394_FEATURE_MODE_AUTO)
        {
            (*itr)->modes->value( (*itr)->choiceIndices[AUTO] );
            deactivateSettings((*itr)->setting);
        }
        else if(feature.current_mode == DC1394_FEATURE_MODE_ONE_PUSH_AUTO)
        {
            (*itr)->modes->value( (*itr)->choiceIndices[AUTO_SINGLE] );
            deactivateSettings((*itr)->setting);
        }
        else if(feature.current_mode == DC1394_FEATURE_MODE_MANUAL)
        {
            activateSettings((*itr)->setting);

            if(feature.absolute_capable == DC1394_TRUE && feature.abs_control == DC1394_ON)
                (*itr)->modes->value( (*itr)->choiceIndices[MAN_ABSOLUTE] );
            else
                (*itr)->modes->value( (*itr)->choiceIndices[MAN_RELATIVE] );
        }
        else
        {
            cerr << "IIDC_featuresWidget can't syncControls():" << endl;
            dc1394_feature_print(&feature, stderr);
            continue;
        }

        if(feature.absolute_capable == DC1394_TRUE && feature.abs_control == DC1394_ON)
        {
            double range = feature.abs_max - feature.abs_min;
            double minstep = range / 10000.0;
            FOREACH(vector<Fl_Value_Slider*>::iterator, itrslider, (*itr)->setting)
            {
                (*itrslider)->bounds(feature.abs_min, feature.abs_max);
                (*itrslider)->value(feature.abs_value);
                (*itrslider)->precision(ceil(-log(minstep) / log(10)));
            }

            // show the units
            (*itr)->setting[numSliders - 1]->size(SETTING_WIDTH/numSliders - widestUnitLabel, FEATURE_HEIGHT);
            (*itr)->unitsWidget->show();
        }
        else
        {
            FOREACH(vector<Fl_Value_Slider*>::iterator, itrslider, (*itr)->setting)
            {
                (*itrslider)->bounds(feature.min, feature.max);
                (*itrslider)->value(feature.value);
                (*itrslider)->precision(0); // integers
            }

            FOREACH(vector<Fl_Value_Slider*>::iterator, itrslider, (*itr)->setting)
            {
                (*itrslider)->size(SETTING_WIDTH / numSliders, FEATURE_HEIGHT);
            }
            (*itr)->unitsWidget->hide();
        }
    }
}

void IIDC_featuresWidget::settingsChanged(Fl_Widget* widget)
{
    featureUI_t* feature = (featureUI_t*)widget->parent()->user_data();
    modeSelection_t mode = feature->modeChoices[ feature->modes->value() ];

    // Only the white balance has more than one controllable slider, so I hardcode the [0] for
    // everything else. I don't have an IIDC camera with manually-controllable white balance, but I
    // should call dc1394_feature_whitebalance_set_value() to set its value
    switch(mode)
    {
    case MAN_RELATIVE:
        if(feature->id == DC1394_FEATURE_TEMPERATURE)
            dc1394_feature_temperature_set_value(camera, (uint32_t)feature->setting[0]->value());
        else
            dc1394_feature_set_value(camera, feature->id, (uint32_t)feature->setting[0]->value());
        return;

    case MAN_ABSOLUTE:
        dc1394switch_t absPwr;
        dc1394_feature_get_absolute_control(camera, feature->id, &absPwr);
        if(absPwr != DC1394_ON)
        {
            cerr << "IIDC_featuresWidget: setting changed while not in absolute mode" << endl;
            return;
        }
        dc1394_feature_set_absolute_value(camera, feature->id, (float)feature->setting[0]->value());
        return;

    default: ;
    }

    cerr << "IIDC_featuresWidget::settingsChanged(): not in manual mode..." << endl;
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
        cerr << "IIDC_featuresWidget::modeChanged(): unhandled switch..." << endl;
        break;
    }
}
