/*****************************************************************************\
 * Redes Integradas de Telecomunicacoes
 * MIEEC/MEEC/MERSIM - FCT NOVA  2025/2026
 *
 * callbacks.h
 *
 * Header file for functions that handle main application start/stop and graphical events
 *
 * @author  Luis Bernardo
\*****************************************************************************/
#ifndef HAVE_CALLBACKS_H
#define HAVE_CALLBACKS_H


#include <gtk/gtk.h>
#include <netinet/in.h>
#include "bitmask.h"
#include "gui.h"

#define CONFIG_FILENAME		"filelist.txt"

// Auxiliary functions
#define min(x,y)		((x)>(y) ? (y) : (x))
#define max(x,y)		((x)>(y) ? (x) : (y))

// Auxiliar definitions
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif



// Maximum length of a message
#define MAX_MESSAGE_LEN	9000

/* Packet types */
// Data packet; sent by the senders
#define PKT_DATA		1
// Source Receiver Report; sent by the receivers
#define PKT_SRR			2
// Stops a transmission; sent by the senders to the receivers
#define PKT_STOP		3
// Leave receivers group; sent by the receiver to the senders
#define PKT_EXIT		4


// Parameters for file transmission
extern const int receiver_SRR_timeout;  // Waiting time to generate SRR at the receiver
extern const int OK_timeout; // Maximum waiting time for an OK at the sender

/* Global variables - main.c */
extern GUI_WindowElements *main_window;

/* Global variables - callbacks.c */
extern gboolean active;	// TRUE if server if active

extern char *path_dir;		// Directory where the received files are stored

// Temporary buffer for writing messages
extern char tmp_buf[];


/* Functions to handle the activation/deactivation of the application */
// Close all active streams
void close_all(gboolean lock_gdb);
// Handle active/stop application button
void on_togglebutton1_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data);
// Handle the closing of main window
gboolean on_window_delete_event       (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);


/* Functions to handle the sending/reception activation/termination */
// Handle the "Request" button
void on_buttonRequest_clicked           (GtkButton       *button,
                                        gpointer         user_data);

// Handle button 'Stop' receiver
void on_buttonStopR_clicked           (GtkButton       *button,
                                        gpointer         user_data);

#endif
