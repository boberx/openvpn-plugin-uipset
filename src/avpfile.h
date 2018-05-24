#ifndef OPENVPN_PLUGIN_UIPSET_AVPFILE_H
#define OPENVPN_PLUGIN_UIPSET_AVPFILE_H

#include <stdio.h>
#include <stdlib.h>

#include "common.h"

/* возвращае 0, если ошибка
*/
int readavpfile ( const char* const, struct openvpn_plugin_args_func_in const*, char****, int* avp_array_size );

#endif
