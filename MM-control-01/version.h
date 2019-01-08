//! @file

#ifndef VERSION_H_
#define VERSION_H_

static const uint16_t fw_version = 104; //!< example: 103 means version 1.0.3
static const uint16_t fw_buildnr = 1;//${GIT_PARENT_COMMITS}; //!< number of commits preceeding current HEAD
//#define FW_HASH "1"
//! @macro FW_LOCAL_CHANGES
//! @val 0 no changes in tracked local files
//! @val 1 some local git tracked files has been changed
//#include "dirty.h"

#define FW_LOCAL_CHANGES 1

#endif //VERSION_H_
