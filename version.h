//! @file

#ifndef VERSION_H_
#define VERSION_H_

static const uint16_t fw_version = 106; //!< example: 103 means version 1.0.3
static const uint16_t fw_buildnr = 374; //!< number of commits preceeding current HEAD
#define FW_HASH "13bc6224bf30c95a6305cf5924a0a93bd942c4f1"

//! @macro FW_LOCAL_CHANGES
//! @val 0 no changes in tracked local files
//! @val 1 some local git tracked files has been changed
#include "dirty.h"

#endif //VERSION_H_
