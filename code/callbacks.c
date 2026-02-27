/*****************************************************************************\
 * Redes Integradas de Telecomunicacoes
 * MIEEC/MEEC/MERSIM - FCT NOVA  2025/2026
 *
 * callbacks.c
 *
 * Functions that handle main application start/stop and graphical events
 *
 * @author  Luis Bernardo
\*****************************************************************************/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <assert.h>
#include <gtk/gtk.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

#include "callbacks.h"
#include "receiver_th.h"
#include "file.h"
#include "sock.h"
#include "gui.h"

// To set DEBUG globally modify the Makefile
//#define DEBUG 1

#ifdef DEBUG
#define debugstr(x)     g_print(x)
#else
#define debugstr(x)
#endif

// Parameters for file transmission
const int receiver_SRR_timeout=2000;  // Waiting time to generate SRR at the receiver (2 seg)
const int OK_timeout= 1000; // Maximum waiting time for an OK at the sender (1 seg)

gboolean active= FALSE;	// TRUE if server if active

char *path_dir;		// Directory where the received files are stored

// Temporary buffer for writing messages
char tmp_buf[MAX_MESSAGE_LEN>8000 ? MAX_MESSAGE_LEN : 8000];


/**********************************************************************\
|* Functions to handle the activation/deactivation of the application *|
 \**********************************************************************/

/** Close all active streams */
void close_all(gboolean lock_gdb) {
	stop_receivers(lock_gdb);
	GUI_block_entrys(FALSE);
}

/** Handle active/stop application button */
void on_togglebutton1_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
	if (gtk_toggle_button_get_active(togglebutton)) { // If program is starting

		/* Activate server */
		set_local_IP(); // Sets local variables at 'sock.c'

		// Get the output directory name
		const char *auxpath_dir = GUI_get_OutDir();
		if ((auxpath_dir != NULL) && (strlen(auxpath_dir) > 0)
				&& make_directory(auxpath_dir)) {
			path_dir = strdup(auxpath_dir); // copy name
			if (path_dir[strlen(path_dir) - 1] == '/')
				// remove final '/'
				path_dir[strlen(path_dir) - 1] = '\0';
		} else {
			path_dir = "";
		}

		/* */
		GUI_block_entrys(TRUE);
		GUI_set_PID((int) getpid());
		active = TRUE;
		Log("FileMulticast client is active\n");

	} else {
		/* Turn application off */
		close_all(FALSE);

		GUI_set_PID(0);
		active = FALSE;
		Log("FileMulticast client is stopped\n");
	}
}

/** Handle the closing of main window */
gboolean on_window_delete_event(GtkWidget *widget, GdkEvent *event,
		gpointer user_data) {
	gtk_main_quit();
	return FALSE;
}

/********************************************************************\
|* Functions to handle the sending/reception activation/termination *|
 \********************************************************************/

/** Handle the "Request" button */
void on_buttonRequest_clicked(GtkButton *button, gpointer user_data) {
	if (!active) {
		Log("Program is not active\n");
		return;
	}
	const gchar *name;
	const gchar *ip;
	int port;

	// Read parameters
	name = GUI_get_QueryFile();
	if ((name == NULL) || (strlen(name) == 0)) {
		Log("Empty filename requested\n");
		return;
	}
	if (strcmp(name, get_trunc_filename(name))) {
		Log("ERROR: The filename must not include the pathname\n");
		return;
	}
	// Reads IP
	ip = GUI_get_QueryIP();
	// Reads port
	port = GUI_get_QueryPort();
	if (port <= 0) {
		Log("ERROR: invalid port number\n");
		return;
	}

	// Start query
	start_file_download(name, ip, port);
}


/** Handle button 'Stop' receiver */
void on_buttonStopR_clicked(GtkButton *button, gpointer user_data) {
	unsigned int tid;

	// This call is synchronized with the graphical thread!
	tid= GUI_get_selected_tid(FALSE);
	if (tid > 0) {
#ifdef DEBUG
		g_print("File transfer with TID %d will be stopped\n", tid);
#endif
		stop_receiver_by_tid((unsigned int)tid, FALSE);
	} else {
		Log("No file transfer is selected\n");
	}
}

