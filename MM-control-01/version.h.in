//! @file

#ifndef VERSION_H_
#define VERSION_H_

static const uint16_t fw_version = 106; //!< example: 103 means version 1.0.3
static const uint16_t fw_buildnr = ${GIT_PARENT_COMMITS}; //!< number of commits preceeding current HEAD
#define FW_HASH "${GIT_COMMIT_HASH}"

//! @macro FW_LOCAL_CHANGES
//! @val 0 no changes in tracked local files
//! @val 1 some local git tracked files has been changed
#include "dirty.h"

#endif //VERSION_H_