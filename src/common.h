#ifndef OPENVPN_PLUGIN_UIPSET_COMMON_H
#define OPENVPN_PLUGIN_UIPSET_COMMON_H

#include <string.h>
#include <openvpn/openvpn-plugin.h>

extern const char* logprefix;

struct plugin_context {
	plugin_log_t log;

	/*! путь до сокета uipset */
	char uipset_socket_path[256];

	/*! IPset common set */
	char ipset_common_set[256];

	/*! путь до папки, содержащей доп. значения из радиуса */
	char client_avp_dir[256];
};

#endif
