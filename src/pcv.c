/*
 * Picviz - Parallel coordinates ploter
 * Copyright (C) 2008 Sebastien Tricaud <toady@gscore.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * $Id: pcv.c 683 2009-07-20 22:02:37Z toady $
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>

#include <picviz.h>

#define PCV_VERSION "0.6 ($Id: pcv.c 683 2009-07-20 22:02:37Z toady $)"

/* char debug = 0; */
char *plugin_arg = NULL;
char plugin_out_str[1024];
char plugin_ren_in = 0;
char plugin_ren_str[1024];
char *listen_sock = NULL;

/*
 * Copy arg vector into a new buffer, concatenating arguments with spaces.
 * stolen from tcpdump
 */
char *
concat_argv(char **argv)
{
	char **p;
	u_int len = 0;
	char *buf;
	char *src, *dst;

	p = argv;
	if (*p == 0)
		return 0;

	picviz_debug(PICVIZ_DEBUG_NOTICE, PICVIZ_AREA_FILTER, "Applying filter '%s'\n", *p);

	while (*p)
		len += strlen(*p++) + 1;

	buf = (char *)malloc(len);
	if (buf == NULL) {
		fprintf(stderr, "concat_argv: malloc");
		return NULL;
	}

	p = argv;
	dst = buf;
	while ((src = *p++) != NULL) {
		while ((*dst++ = *src++) != '\0')
			;
		dst[-1] = ' ';
	}
	dst[-1] = '\0';

	return buf;
}

void picviz_handlesig()
{
	fprintf(stderr, "[+] Terminating\n");
	if (listen_sock) {
		unlink(listen_sock);
	}
	unlink(engine.pid_file);
	exit(0);
}

void image_write_callback(PicvizImage *image)
{
	fprintf(stderr, ".");
	/* if (debug) { */
	/* 	picviz_image_debug_printall(image); */
	/* } */

	picviz_render_image(image); /* Our very costly function ;) */
	picviz_plugin_load(PICVIZ_PLUGIN_OUTPUT, plugin_out_str, image, plugin_arg);
}

void help(char *prg)
{
	fprintf(stderr, "Usage: %s -Tplugin [-Rplugin] [-Warg] [-ad] [-A argument] file.pcv ['filter']\nread the manpage pcv(1) for details\n",
			prg);
	exit(EXIT_FAILURE);
}

/* Write the pid number to the file 'file' */
void write_pid(char *file)
{
	FILE *fp;
	pid_t pid;

	pid = getpid();
	fp = fopen(file,"w");
	if (!fp) {
		if (errno) {
			fprintf(stderr, "[+] Using .picviz.pid to store PID\n");
			engine.pid_file = ".picviz.pid";
			fp = fopen(engine.pid_file,"w");
		} else {
			fprintf(stderr, "[E] Cannot write pid file '%s': %s\n", file, strerror(errno));
			fprintf(stderr, "[+] Terminating\n");
			exit(1);
		}
	}
	fprintf(fp,"%d",pid);
	fclose(fp);
}

int main(int argc, char **argv)
{
	struct pcimage_t *image;
	char *filename;
	char *filter = NULL;
	char plugin_in = 0;
	char draw_heatline = 0;
	int opt;
	unsigned int height = 0;
	unsigned int axis_default_space = 0;
	unsigned int axis_initial_cursor = 0;
	char *template = NULL; /* Template file for daemon mode */

	picviz_engine_init();

	signal(SIGTERM, picviz_handlesig);
	signal(SIGINT, picviz_handlesig);
	signal(SIGQUIT, picviz_handlesig);

	while ((opt = getopt(argc, argv, "L:lrT:R:W:dao:A:s:t:vVp:mq")) != -1) {
		switch (opt) {
			case 'A': /* Gives the argument to the plugin */
				plugin_arg = optarg;
				break;
			case 'a': /* Displays text */
				engine.display_raw_data = 1;
				break;
			case 'd': /* Starts in debug mode*/
				if (engine.debug < 255) {
					engine.debug += 1;
				}
				break;
			case 'L': /* Draw the text every N lines */
				engine.draw_text = atoi(optarg);
				break;
			case 'l': /* We don't learn the string algo */
				engine.learn = 0;
				break;
		        case 'm':
			        engine.display_minmax = 1;
				/* If we display min/max only, this is because we want text */
				engine.display_raw_data = 1;
			        break;
			case 'o': /* Output to a file instead of stdout */
				engine.output_file = optarg;
				break;
			case 'p':
				engine.pid_file = optarg;
				break;
			case 'q':
				engine.quiet = 1;
				break;
			case 'R': /* Rendering plugin (heatline, ...) */
				sprintf(plugin_ren_str, "libpicvizren%s.so", optarg);
				if ( ! strcmp("healine", optarg) ) {
					draw_heatline = 1;
				}
				plugin_ren_in = 1;
				break;
			case 'r': /* Inscreases the image size */
				height += 200;
				axis_default_space += 100;
				axis_initial_cursor += 50;
				break;
			case 's': /* Listen to the specified socket */
				fprintf(stderr,"[+] Starting Picviz daemon\n");
				fprintf(stderr,"[+] Listen to %s\n", optarg);
				listen_sock = optarg;
				break;
			case 'T': /* Output plugin (pngcairo, svg, ...) */
				sprintf(plugin_out_str, "libpicvizout%s.so", optarg);
				plugin_in = 1;
				break;
			case 't': /* Template file to use when run in daemon mode */
				template = optarg;
				break;
			case 'v':
			case 'V':
				picviz_debug(PICVIZ_DEBUG_NOTICE, PICVIZ_AREA_CORE, "pcv version %s\n", PCV_VERSION);
				exit(0);
				break;
			case 'W': /* Argument for the engine */
				if ( ! strcmp("pcre", optarg)) {
					engine.use_pcre = 1;
					break;
				}
				if ( ! strcmp("string_algo_basic", optarg)) {
					engine.string_algo = 0;
					break;
				}
				fprintf(stderr,"ERROR: unrecognized -W%s.\n", optarg);
				fprintf(stderr,"ERROR: unknown engine parameter!\n");
				help(argv[0]);
				break;
			default: /* '?' */
				fprintf(stderr,"ERROR: arguments parsing!\n");
				help(argv[0]);
		}
	}

	picviz_init(&argc, argv);

	if (!plugin_in) {
		fprintf(stderr,"ERROR: plugin required!\n");
		help(argv[0]);
	}

	if (listen_sock) {

		filter = concat_argv(&argv[optind]);
		engine.real_time = 1;
		if (!template) {
			fprintf(stderr, "No template given. Add '-t template.pcv'.\n");
			exit(1);
		}

		write_pid(engine.pid_file);

		/* We create the image template with its associated filter */
		image = (PicvizImage *)pcv_parse(template, filter);
		if (!image) {
			fprintf(stderr, "Cannot create image. Exiting!\n");
			exit(1);
		}
		picviz_render_image(image); /* We render our template */
		picviz_plugin_load(PICVIZ_PLUGIN_OUTPUT, plugin_out_str, image, plugin_arg);

		picviz_fifo_data_read(image, listen_sock, image_write_callback);

	} else {
		filter = concat_argv(&argv[optind+1]);
		if (optind >= argc) {
			fprintf(stderr, "File to parse missing\n");
			exit(EXIT_FAILURE);
		}

		filename = argv[optind];

		engine.axis_default_space += axis_default_space;
		engine.initial_axis_x_cursor += axis_initial_cursor;
		engine.image_height += height;
		image = (struct pcimage_t *)pcv_parse(filename, filter);
		if (!image) {
			fprintf(stderr, "Cannot parse image %s\n", argv[optind]);
			exit(EXIT_FAILURE);
		}

		/* if (debug) { */
		/* 	picviz_image_debug_printall(image); */
		/* 	goto out; */
		/* } */

		if (plugin_ren_in) {
			picviz_plugin_load(PICVIZ_PLUGIN_RENDER, plugin_ren_str, image, plugin_arg);
		}
		picviz_plugin_load(PICVIZ_PLUGIN_OUTPUT, plugin_out_str, image, plugin_arg);

	}


out:
	picviz_image_destroy(image);

	exit(EXIT_SUCCESS);
}
