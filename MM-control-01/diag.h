//diag.h - Diagnostic functions
#ifndef _DIAG_H
#define _DIAG_H

#include <inttypes.h>
#include <stdio.h>


#if defined(__cplusplus)
extern "C" {
#endif //defined(__cplusplus)


#ifdef _DIAG

extern int8_t cmd_uart_bridge(FILE* inout);
extern int8_t cmd_diag_uart1(FILE* inout);
extern int8_t cmd_diag_tmc(FILE* inout, uint8_t axis);

#endif //_DIAG


#if defined(__cplusplus)
}
#endif //defined(__cplusplus)
#endif //_DIAG_H

