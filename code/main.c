/*****************************************************************************\
 * Redes Integradas de Telecomunicacoes
 * MIEEC/MEEC/MERSIM - FCT NOVA  2025/2026
 *
 * main.c
 *
 * Main function
 *
 * @author  Luis Bernardo
\*****************************************************************************/

#include <gtk/gtk.h>
#include <arpa/inet.h>
#include <assert.h>
#include <time.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include "gui.h"
#include "sock.h"
#include "callbacks.h"
#include "file.h"

/* Public variables */
GUI_WindowElements *main_window;	// Pointer to all elements of main window


int main (int argc, char *argv[]) {
    /* allocate the memory needed by our TutorialTextEditor struct */
    main_window = g_slice_new (GUI_WindowElements);

    /* init glib threads */
    gdk_threads_init ();

    /* initialize GTK+ libraries */
    gtk_init (&argc, &argv);

    if (GUI_init_app (main_window) == FALSE) return 1; /* error loading UI */
	gtk_widget_show (main_window->window);

    // Defines the output directory, where the files will be written
    char *homedir= getenv("HOME");
    if (homedir == NULL)
      homedir= getenv("PWD");
    if (homedir == NULL)
      homedir= "/tmp";
    path_dir= g_strdup_printf("%s/out%d", homedir, getpid());
    if (!make_directory(path_dir)) {
      Log("Failed creation of output directory: '");
      Log(path_dir);
      Log("'.\n");
      path_dir= "/tmp";
    }
    Log("Received files will be created at: '");
    Log(path_dir);
    Log("'\n");
    // Writes the output directory in the graphical window
    GUI_set_OutDir(path_dir);

	/* get GTK thread lock */
    gdk_threads_enter ();
	// Infinite loop handled by GTK+3.0
	gtk_main();
	/* release GTK thread lock */
	gdk_threads_leave ();

    /* free memory we allocated for TutorialTextEditor struct */
    g_slice_free (GUI_WindowElements, main_window);

	return 0;
}

