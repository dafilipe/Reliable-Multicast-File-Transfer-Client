/*****************************************************************************\
 * Redes Integradas de Telecomunicacoes
 * MIEEC/MEEC/MERSIM - FCT NOVA  2025/2026
 *
 * gui.h
 *
 * Header file of functions that handle graphical user interface interaction
 *
 * @author  Luis Bernardo
\*****************************************************************************/

#ifndef _INCL_GUI_H
#define _INCL_GUI_H

#include <gtk/gtk.h>
#include <netinet/in.h>

/* store the widgets which may need to be accessed in a typedef struct */
typedef struct
{

		GtkWidget               *window;
		GtkToggleButton			*toggleButton1;
		GtkEntry				*entryPID;
		GtkEntry				*entryOutDir;
		GtkEntry				*entryFileQuery;
		GtkEntry				*entryIPQuery;
		GtkEntry				*entryPortQuery;
		GtkTreeView				*treeProc;
		GtkListStore			*th_store;
		GtkTextView				*textView;
} GUI_WindowElements;

// Global pointer to the main window elements
extern GUI_WindowElements *main_window;
// Counter of active threads in window
extern int win_cnt;


// Initialization function
gboolean GUI_init_app (GUI_WindowElements *window);

// Logs the message str to the textview and command line
void Log (const gchar * str);



/************************************************\
|*   Functions that read and update text boxes  *|
\************************************************/

/** Gets the PID number. Return -1 if error */
int GUI_get_PID();
/** Sets the PID number entry */
void GUI_set_PID(int pid);

/** Gets the file name in the query box */
const gchar *GUI_get_QueryFile();
/** Sets the file name in the query box */
void GUI_set_QueryFile(const char *addr);

/** Gets the query IP in the query box */
const gchar *GUI_get_QueryIP();
/** Sets the query IP in the query box */
void GUI_set_QueryIP(const char *addr);

/** Gets the query port in the query box */
const int GUI_get_QueryPort();
/** Sets the query port in the query box */
void GUI_set_QueryPort(unsigned int port);

/** Gets the output directory's entry contents */
const gchar *GUI_get_OutDir();
/** Sets the output directory's entry contents */
void GUI_set_OutDir(const char *addr);

// Blocks or unblocks the configuration GtkEntry boxes in the GUI
void GUI_block_entrys(gboolean block);


/***********************************************************\
|* Functions that handle File transfer TreeView management *|
\***********************************************************/

// Search for 'tid' in file transfer list; returns iter
gboolean GUI_locate_Ftrans(unsigned tid, GtkTreeIter *iter, gboolean lock_gdk);
// Add line with thread 'tid' data to list
gboolean GUI_add_Ftrans(unsigned tid, const char *ip, int port, const char *f_name, gboolean lock_gdk);
// Update the percentage information in thread tid information
gboolean GUI_update_Ftrans_tx(unsigned tid, int trans, int total, gboolean lock_gdk);
// Get all information from the line with 'tid' in the thread list
gboolean GUI_get_Ftrans_info(unsigned tid, const char **ip, int *port,
		const char **transf, const char **filename, gboolean lock_gdk);
// Deletes the line with 'tid' in thread list
gboolean GUI_del_Ftrans(unsigned tid, gboolean lock_gdk);
// Deletes all threads from the thread table
void GUI_clear_Ftrans(gboolean lock_gdk);
// Get selected thread id
unsigned int GUI_get_selected_tid(gboolean lock_gdk);


/***************************\
|*   Auxiliary functions   *|
\***************************/

/** Creates a window with an error message and outputs it to the command line */
void GUI_error_message (const gchar *message);
/** Opens a dialog window to choose a filename to read. Return the
    newly allocated filename or NULL. */
const gchar *GUI_get_open_filename (GUI_WindowElements *win);
/** Opens a dialog window to choose a filename to save. Return the
    newly allocated filename or NULL. */
const gchar *GUI_get_save_filename (GUI_WindowElements *win);


/***********************\
|*   Event handlers    *|
\***********************/
// Event handlers in gui_g3.c

// Handles 'Clear' button - clears textMemo
void on_buttonClear_clicked (GtkButton *button, gpointer user_data);


// External event handlers in file.c and callbacks.c
void on_togglebutton1_toggled (GtkToggleButton *togglebutton, gpointer user_data);

void on_buttonQuery_clicked (GtkButton *button, gpointer user_data);
void on_buttonStopR_clicked (GtkButton *button, gpointer user_data);

#endif



