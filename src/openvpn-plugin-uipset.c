#include "openvpn-plugin-uipset.h"

static const char* get_env ( const char* name, const char* envp[] )
{
	if ( envp )
	{
		int i;
		const int namelen = strlen ( name );

		for ( i = 0; envp[i]; ++ i )
		{
			if ( ! strncmp ( envp[i], name, namelen ) )
			{
				const char* cp = envp[i] + namelen;

				if ( *cp == '=' )
					return cp + 1;
			}
		}
	}

	return NULL;
}

OPENVPN_EXPORT int openvpn_plugin_open_v3 (
	__attribute__((unused)) const int version,
	struct openvpn_plugin_args_open_in const* args,
	struct openvpn_plugin_args_open_return* rv )
{
	int rc = OPENVPN_PLUGIN_FUNC_ERROR;

	if ( args->argv[1] == 0 )
		args->callbacks->plugin_log ( PLOG_ERR, logprefix, "uipset socket path does not set" );
	else if ( args->argv[2] == 0 )
		args->callbacks->plugin_log ( PLOG_ERR, logprefix, "IPset chain does not set" );
	else if ( args->argv[3] == 0 )
		args->callbacks->plugin_log ( PLOG_ERR, logprefix, "client_avp_dir does not set" );
	else
	{
		// создаём и заполняем нулями
		struct plugin_context* plugin = calloc ( 1, sizeof ( *plugin ) );

		plugin->log = args->callbacks->plugin_log;

		strncpy ( plugin->uipset_socket_path, args->argv[1], sizeof ( plugin->uipset_socket_path ) );

		strncpy ( plugin->ipset_common_set, args->argv[2], sizeof ( plugin->ipset_common_set ) );

		strncpy ( plugin->client_avp_dir, args->argv[3], sizeof ( plugin->client_avp_dir ) );

		rv->type_mask = OPENVPN_PLUGIN_MASK ( OPENVPN_PLUGIN_CLIENT_CONNECT ) | OPENVPN_PLUGIN_MASK ( OPENVPN_PLUGIN_CLIENT_DISCONNECT );

		rv->handle = (void*) plugin;

		rc = OPENVPN_PLUGIN_FUNC_SUCCESS;
	}

	return rc;
}

OPENVPN_EXPORT void openvpn_plugin_close_v1 ( openvpn_plugin_handle_t handle )
{
	struct plugin_context* plugin = (struct plugin_context*) handle;

	free ( plugin );
}

OPENVPN_EXPORT int openvpn_plugin_func_v3 (
	__attribute__((unused)) const int version,
	struct openvpn_plugin_args_func_in const* args,
	__attribute__((unused)) struct openvpn_plugin_args_func_return* rv )
{
	int rc = OPENVPN_PLUGIN_FUNC_ERROR;

	struct plugin_context* plugin = (struct plugin_context*) args->handle;

	int i;
	for ( i = 0; args->envp[i]; ++ i )
		plugin->log ( PLOG_DEBUG, logprefix, "openvpn_plugin_func_v3: envp[%d]: %s\n", i, args->envp[i] );

	// используем username, а если его нет, то common_name
	const char* usr_str = get_env ( "username", args->envp );

	if ( usr_str == NULL )
		usr_str = get_env ( "common_name", args->envp );

	const char* rip_str = get_env ( "ifconfig_pool_remote_ip", args->envp );

	struct uipset_client uclient;

	if ( rip_str == NULL )
		plugin->log ( PLOG_ERR, logprefix, "envp[ifconfig_pool_remote_ip] is not set" );
	else if ( usr_str == NULL )
		plugin->log ( PLOG_ERR, logprefix, "envp[username] or envp[common_name] is not set" );
	else if ( uipset_start ( &uclient, plugin->uipset_socket_path ) != 1 )
		plugin->log ( PLOG_ERR, logprefix, "openvpn_plugin_func_v3: uipset_start failed" );
	else
	{
		char set_str[256] = "block";
		uipset_msg_response r;

		char avpfile_path[2048];

		int snprintf_r = snprintf ( avpfile_path, sizeof ( avpfile_path ) / sizeof ( *avpfile_path ), "%s/%s.avp", plugin->client_avp_dir, usr_str );

		if ( snprintf_r <= 0 || snprintf_r >= (int)( sizeof ( avpfile_path ) / sizeof ( *avpfile_path ) ) )
			plugin->log ( PLOG_ERR, logprefix, "set path to the avpfile failed: snprintf_r: %d", snprintf_r );
		else
		{
			//reading client specific options from:
			plugin->log ( PLOG_NOTE, logprefix, "read acl from [%s]",  avpfile_path );

			char*** avp_array = 0;
			int avp_array_size = 0;

			if ( readavpfile ( avpfile_path, args, &avp_array, &avp_array_size ) != 1 )
				plugin->log ( PLOG_WARN, logprefix, "readavpfile failed [%s]",  avpfile_path );

			int i;
			for ( i = 0; i < avp_array_size; i ++ )
			{
				if ( avp_array[i] != 0 )
				{
					plugin->log ( PLOG_DEBUG, logprefix, "avp_array[%d]: {%s,%s}", i, avp_array[i][0], avp_array[i][1] );

					if ( strcmp ( avp_array[i][0], "Cisco-AVPair" ) == 0 )
					{
						if ( sscanf ( avp_array[i][1], "lcp:interface-config=ip acce %s out", set_str ) != 1 )
							plugin->log ( PLOG_WARN, logprefix, "sscanf failed" );
						else
							plugin->log ( PLOG_NOTE, logprefix, "access control list: [%s]", set_str );
					}

					free ( avp_array[i][0] );
					free ( avp_array[i][1] );
					free ( avp_array[i] );
				}
			}

			free ( avp_array );

			// на любой запрос очищаем запись с ип из всех списков
			if ( uipset_request ( &uclient, UIPSET_CMD_LST, plugin->ipset_common_set, "", &r ) != 1)
				plugin->log ( PLOG_ERR, logprefix, "openvpn_plugin_func_v3: uipset_request failed" );
			else
			{
				size_t rmsglen = strlen ( r.msg );

				plugin->log ( PLOG_DEBUG, logprefix, "uipset_request: rmsglen: [%d] list: [%s]", rmsglen, r.msg );

				const char* tmpstr = r.msg;
				size_t i;

				for ( i = 0; i < rmsglen; i++ )
				{
					if ( r.msg[i] == '\n' )
					{
						r.msg[i] = 0;

						plugin->log ( PLOG_DEBUG, logprefix, "tmpstr: [%s]", tmpstr );
						
						uipset_msg_response rd;

						if ( uipset_request ( &uclient, UIPSET_CMD_DEL, tmpstr, rip_str, &rd ) != 1 )
							plugin->log ( PLOG_ERR, logprefix, "openvpn_plugin_func_v3: uipset_request failed" );
						else
							plugin->log ( PLOG_NOTE, logprefix, "address [%s] removed from the set: [%s]", rip_str, tmpstr );

						tmpstr = &r.msg[i + 1];
					}
				}

				switch ( args->type )
				{
					default:
						plugin->log ( PLOG_ERR, logprefix, "openvpn_plugin_func_v3: unknown type" );
						break;
					case OPENVPN_PLUGIN_CLIENT_CONNECT:
						// проверяем, что набор есть в общем наборе plugin->ipset_common_set
						if ( uipset_request ( &uclient, UIPSET_CMD_TST, plugin->ipset_common_set, set_str, &r ) != 1 )
							plugin->log ( PLOG_ERR, logprefix, "openvpn_plugin_func_v3: uipset_request failed: %s", r.msg );
						else if ( r.ret != 1 )
						{
							plugin->log ( PLOG_DEBUG, logprefix, "openvpn_plugin_func_v3: uipset_request ret: [%d]", r.ret );
							plugin->log ( PLOG_WARN, logprefix, "openvpn_plugin_func_v3: there is no set [%s] in [%s] set", set_str, plugin->ipset_common_set );
							
							rc = OPENVPN_PLUGIN_FUNC_DEFERRED;
						}
						else if ( uipset_request ( &uclient, UIPSET_CMD_ADD, set_str, rip_str, &r ) != 1 )
							plugin->log ( PLOG_ERR, logprefix, "openvpn_plugin_func_v3: uipset_request failed" );
						else
						{
							plugin->log ( PLOG_NOTE, logprefix, "address [%s] added to the set: [%s]", rip_str, set_str );

							rc = OPENVPN_PLUGIN_FUNC_SUCCESS;
						}
						break;
					case OPENVPN_PLUGIN_CLIENT_DISCONNECT:
						// нужно! на тот случай, если набор в сете был изменён, а avp файл - нет
						if ( uipset_request ( &uclient, UIPSET_CMD_DEL, set_str, rip_str, &r ) != 1 )
							plugin->log ( PLOG_ERR, logprefix, "openvpn_plugin_func_v3: uipset_request failed" );
						else
						{
							plugin->log ( PLOG_NOTE, logprefix, "address [%s] removed from the set: [%s]", rip_str, set_str );

							rc = OPENVPN_PLUGIN_FUNC_SUCCESS;
						}
						break;	
				}
			}
		}

		uipset_stop ( &uclient );
	}

	return rc;
}
