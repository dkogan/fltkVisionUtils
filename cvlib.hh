#ifndef __CV_LIB_HH__
#define __CV_LIB_HH__

// selects opencv 2.1 or 2.0
#if OPENCV_VERSION == 210
  #include <opencv2/core/core_c.h>
  #include <opencv2/imgproc/imgproc_c.h>
  #include <opencv2/highgui/highgui_c.h>
#else
  #include <opencv/cv.h>
#endif

#endif
