#include "avpfile.h"

int readavpfile ( const char* const filename, struct openvpn_plugin_args_func_in const* args, char**** avp_array, int* avp_array_size )
{
	int rc = 0;

	FILE* f = NULL;

	struct plugin_context* plugin = (struct plugin_context*) args->handle;

	f = fopen ( filename, "rb" );

	if  ( f == 0 )
		plugin->log ( PLOG_ERR, logprefix, "readavpfile: fopen: [%s] failed", filename );
	else
	{
		ssize_t read;
		char* line = NULL;
		size_t len = 0;

		const int m_avp_array_sadd = 5;
		const int m_avp_array_smax = 10;
		int m_avp_array_size = 0;
		int m_avp_array_fill = 0;
		char*** m_avp_array_new = 0;

		//if ( ( avp_array_tmp = malloc ( avp_delta * sizeof ( *avp_array_tmp ) ) ) != 0 )
		//	avp_array_fill = avp_delta;

		// getdelim - если line !=0, самостоятельно выделяет/перевыделяет место для line оиентируясь на len
		while ( ( read = getdelim ( &line, &len, '\n', f ) ) != -1 )
		{
			plugin->log ( PLOG_DEBUG, logprefix, "retrieved line of length %zu", read );
        	plugin->log ( PLOG_DEBUG, logprefix, "retrieved line: %s", line );

        	char a[256];
        	char b[256];

			if ( sscanf ( line, "%s \"%[^\"]s", a, b ) != 2 )
				plugin->log ( PLOG_WARN, logprefix, "sscanf failed: [%s]", filename );
			else
			{
        		plugin->log ( PLOG_DEBUG, logprefix, "avp: [%s] [%s]", a, b );

        		if ( m_avp_array_fill >= m_avp_array_size )
        		{
        			plugin->log ( PLOG_DEBUG, logprefix, "array resize" );

        			if ( m_avp_array_size >= m_avp_array_smax )
        				plugin->log ( PLOG_ERR, logprefix, "array resize failed: m_avp_array_size[%d] >= m_avp_array_smax[%d]", m_avp_array_size, m_avp_array_smax );
        			else
        			{
        				char*** m_avp_array_tmp = 0;
        				int m_avp_array_snew = m_avp_array_size + m_avp_array_sadd;

        				if ( m_avp_array_snew > m_avp_array_smax )
        					m_avp_array_snew = m_avp_array_smax;

        				if ( ( m_avp_array_tmp = malloc ( m_avp_array_snew * sizeof ( *m_avp_array_tmp ) ) ) == 0 )
        					plugin->log ( PLOG_ERR, logprefix, "array resize failed: malloc return 0" );
        				else
        				{
        					int i;
        					for ( i = 0; i < m_avp_array_snew; i ++ )
        					{
        						if ( i < m_avp_array_fill )
        							m_avp_array_tmp[i] = m_avp_array_new[i];
        						else
        							m_avp_array_tmp[i] = 0;
        					}

        					free ( m_avp_array_new );

							m_avp_array_new = m_avp_array_tmp;
        					m_avp_array_size = m_avp_array_snew;
        				}
        			}
        		}

        		if ( m_avp_array_fill < m_avp_array_size )
        		{
        			if ( ( m_avp_array_new[m_avp_array_fill] = malloc ( 2 * sizeof ( **m_avp_array_new ) ) ) == 0 )
        				plugin->log ( PLOG_ERR, logprefix, "array resize failed: malloc return 0" );
        			else
        			{
        				m_avp_array_new[m_avp_array_fill][0] = strdup ( a );
        				m_avp_array_new[m_avp_array_fill][1] = strdup ( b );

        				if ( m_avp_array_new[m_avp_array_fill][0] == 0 || m_avp_array_new[m_avp_array_fill][1] == 0 )
        				{
        					plugin->log ( PLOG_ERR, logprefix, "array resize failed: strdup return 0" );

        					free ( m_avp_array_new[m_avp_array_fill][0] );
        					free ( m_avp_array_new[m_avp_array_fill][1] );
        					free ( m_avp_array_new[m_avp_array_fill] );
        				}
        				else
        					m_avp_array_fill ++;
        			}
        		}

				*avp_array = m_avp_array_new;
				*avp_array_size = m_avp_array_size;

        		rc = 1;
        	}
		}

		free ( line );

		fclose ( f );
	}

	return rc;
}
