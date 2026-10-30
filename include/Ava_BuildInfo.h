#ifndef AVA_BUILD_INFO_H
#define AVA_BUILD_INFO_H

// Local/manual builds stay at the last known production baseline.
// This prevents an experimental local image (build 0) from treating
// the currently published production firmware as a newer OTA update.
#define AVA_BUILD_NUMBER 150
#define AVA_BUILD_VERSION "0.0.0"
#define AVA_BUILD_COMMIT ""

#endif
