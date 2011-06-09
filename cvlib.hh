#ifndef __CV_LIB_HH__
#define __CV_LIB_HH__

// opencv header require this
#ifdef __cplusplus
#include <cstddef>
#endif

#ifndef OPENCV_DEBIAN_PACKAGES
  #include <opencv2/core/core_c.h>
  #include <opencv2/imgproc/imgproc_c.h>
  #include <opencv2/highgui/highgui_c.h>
#else
  #include <opencv/cv.h>
  #include <opencv/highgui.h>
#endif

#endif
