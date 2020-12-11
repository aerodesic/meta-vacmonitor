//#define DEBUG
/*
 * vachelper
 *
 * A helper program for vacmonitor
 *
 * Usage:
 *        vachelper shutdown
 *        vachelper reboot
 *        vachelper set-date <date/time> in 'date' format
 *        vachelper set-timezone <tz>
 *        vachelper install <device to attempt to install>
 *        vachelper clear-install
 *        vachelper set-usb-network <enable> / <disable>
 *          (if 'enable' echos ip address of device after it is set.)
 *        vachelper exportfiles <device to use> <dir name> <file> ...
 *        vachelper wifi "connect" / "hotspot" / "delete" / "start" / "stop" / "list"
 *                      if "connect": "ssid", [ optionsl "password" <password> plus <modify> options ]
 *                      if "hotspot": "ssid", [ optional "password" <password> plus <modify> options ]
 *                      if "delete": <connection>
 *                      if "start": <connection>
 *                      if "list": returns list of access points
 *                      if "disconnect": <connection>
 */
#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdarg.h>
#include <time.h>
#include <libgen.h>
#include <syslog.h>

/* For IP address manipulation */
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#ifndef TRUE
#define TRUE    1
#define FALSE   0
#endif

#define UPDATE_FILE "update.zip"

#define PATH   "/bin:/sbin:/usr/bin:/usr/sbin"

#define MAX_FILES  300

#define PIPE_READ   0
#define PIPE_WRITE  1

typedef unsigned int flags_t;
#define FLAGS_CAPTURE_OUTPUT   0x0001
#define FLAGS_SHOW_COMMANDS    0x0002

static int run_command_array(flags_t flags, char* const answers[], char* const command[])
{
    int rc;
    int index;

#ifdef DEBUG
    printf("run_command_array:");
    for (index = 0; command[index] != NULL; ++index) {
        printf(" %s", command[index]);
    }
    printf("\n");
#endif

    int pipefds[2];
    if (answers != NULL) {
        if (pipe(pipefds) == -1) {
            return 1;
        }
    }

    if ((flags & FLAGS_SHOW_COMMANDS) != 0) {
        char bigbuffer[200];
	sprintf(bigbuffer, "Executing:");
        for (index = 0; command[index] != NULL; ++index) {
	    strcat(bigbuffer, " ");
	    strcat(bigbuffer, command[index]);
	}
	syslog(LOG_INFO, "%s", bigbuffer);
    }

    pid_t pid = fork();
    if (pid == 0) {
        char *env[2];

        env[0] = "PATH=" PATH;
        env[1] = NULL;

        /* Child - run the program */
        setuid(geteuid());
        setgid(getegid());

        if (answers != NULL) {
            if (dup2(pipefds[PIPE_READ], STDIN_FILENO) == 1) {
#ifdef DEBUG
               printf("Error duping answers fd\n");
#endif
               exit(1);
            }
            close(pipefds[PIPE_READ]);
            close(pipefds[PIPE_WRITE]);
        }

#ifndef DEBUG
        if ((flags & FLAGS_CAPTURE_OUTPUT) == 0) {
            /* Change stdout and stderr to null */
            int fd = open("/dev/null", O_WRONLY);
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);
        }
#endif

        execvpe(command[0], command, env);
        exit(1);
    } else if (pid != -1) {
        int status;
        if (answers != NULL) {
            // Close our copy of the read pipe
            close(pipefds[PIPE_READ]);
            // Write the NUL terminated answers to our PIPE input
            while (*answers != NULL) {
#ifdef DEBUG
                printf("Sending to answer fd: %s\n", *answers);
#endif
                write(pipefds[PIPE_WRITE], *answers, strlen(*answers));
                ++answers;
            }
        }
        /* Parent - wait for results */
        waitpid(pid, &status, 0);
        rc = WEXITSTATUS(status);
    } else {
        fprintf(stderr, "fork failed\n");
        rc = 1;
    }

    return rc;
}

static void clear_text_screen(void)
{
    // Clear screen 2 which is the screen on which Wayland is launched
    FILE *fp = fopen("/dev/tty2", "w");
    if (fp != NULL) {
        fprintf(fp, "\033[H\033[2J");
        fclose(fp);
    }
}

/*
 * Implement run_command(flags, answers, cmd, arg, arg, arg, ..., NULL)
 *
 * If FLAGS_CAPTURE_OUTPUT is set, stdout and stderr kept intact.
 * If answers is NULL, no answers are given.
 */
static int run_command(flags_t flags, char* const answers[], char *cmd, ...)
{
    va_list ap;

    /* First count parameters until NULL */
    va_start(ap, cmd);
    int  count = 1;

    while (va_arg(ap, char*) != NULL) {
        ++count;
    }

    /* Allocate storage for the commands + a NULL */
    char *commands[count + 1];
    char **p = &commands[0];

    /* Restart to arg */
    va_start(ap, cmd);

    /* Add first value */
    *p++ = cmd;

    /* Copy parameters plus the NULL at end */
    for (int i = 0; i <= count; i++) {
        *p++ = va_arg(ap, char*);
    }

    va_end(ap);

    return run_command_array(flags, answers, commands);
}

static int check_file_manifest(flags_t flags, const char* update_file)
{
    int rc = 0;

#ifdef DEBUG
    printf("check_file_manifest: %s\n", update_file);
#endif

    /* Create a temp directory and unpack zip file */
    char template[] = "/tmp/MANIXXXXXX";
    char *tempdir = mkdtemp(template);

#ifdef DEBUG
    printf("tempdir '%s'\n", tempdir);
#endif

    if (tempdir == NULL) {
        printf("Unable to create temp dir\n");
        rc = 21;
    } else if (run_command(flags, NULL, "unzip", update_file, "-q", "-d", tempdir, NULL) == 0) {
        chdir(tempdir);
        FILE *fp;
        char *buffer = NULL;
        size_t nbuffer = 0;

        /* Check if a required_version file is present and if so see if it matches the version required */
        fp = fopen("update-only-from-version", "r");
        if (fp != NULL) {
            /* Contains one or more version names allowed */
            getline(&buffer, &nbuffer, fp);
            fclose(fp);
            char *p = strchr(buffer, '\n');
            if (p != NULL) {
                *p = '\0';
            }

            char *wanted_version = strdup(buffer);
#ifdef DEBUG
            printf("update-only-from-version '%s'\n", wanted_version);
#endif

            FILE* fp2 = fopen("/etc/vacversion", "r");
            if (fp2 != NULL) {
                getline(&buffer, &nbuffer, fp2);
                fclose(fp2);
                p = strchr(buffer, '\n');
                if (p != NULL) {
                    *p = '\0';
                }
#ifdef DEBUG
                printf("Running version '%s'\n", buffer);
#endif

                /* Make a copy since strtok messes with original */
                char *wanted_version_copy = strdup(wanted_version);

                char *ptr = strtok(wanted_version, ",");
                int found_version = FALSE;

                /* See if current version matches the running version. */
                while (!found_version && ptr != NULL) {
                    found_version = strcmp(ptr, buffer) == 0;
                    ptr = strtok(NULL, ",");
                }

                if (!found_version) {
                    printf("Cannot install:\n\n"
                           "You are running %s\n"
                           "but this update requires %s\n", buffer, wanted_version_copy);
                    rc = 9;
                }

                free(wanted_version_copy);
            }
            free(wanted_version);
        }

        /* Default to OK if no required_version file present */
        if (rc == 0) {
            /* Read the readme into the log */
            fp = fopen("readme.txt", "r");
            if (fp != NULL) {
               ssize_t nread;

               do {
                   nread = getline(&buffer, &nbuffer, fp);
                   if (nread > 0) {
                       printf("%s", buffer);
                   }
               } while (nread > 0);
               fclose(fp);
            }
        }

        free(buffer);

        run_command(flags, NULL, "rm", "-rf", tempdir, NULL);

    } else {
        rc = 6;
        printf("Improper manifest\n");
    }

    return rc;
}

/*
 * Report insufficient arguments.
 */
int do_insufficient_args(flags_t flags, int argc, char **argv)
{
    printf("Insufficient args; need at least one\n");
    return 1;
}

/*
 * Report bad command.
 */
int do_unknown_command(flags_t flags, int argc, char **argv)
{
    printf("Unknown option '%s'\n", argv[1]);
    return 1;
}

/*
 * Perform a shutdown immediately.
 */
int do_shutdown(flags_t flags, int argc, char **argv)
{
    clear_text_screen();
    return run_command(flags, NULL, "/sbin/shutdown", "--halt", "now", NULL);
}

/*
 * Perform  reboot immediately.
 */
int do_reboot(flags_t flags, int argc, char **argv)
{
    clear_text_screen();
    return run_command(flags, NULL, "/sbin/reboot", "-f", NULL);
}

/*
 * Set date and time from arguments.
 */
int do_setdate(flags_t flags, int argc, char **argv)
{
    int rc;

    if (argc < 3) {
        return do_insufficient_args(flags, argc, argv);
    } else {
        rc = run_command(flags, NULL, "date", argv[2], NULL);

        if (rc == 0) {
            rc = run_command(flags, NULL, "/sbin/hwclock", "--systohc", "--utc", NULL);
        }
    }
    return rc;
}

/*
 * Set timezone from arguments.
 */
int do_settimezone(flags_t flags, int argc, char **argv)
{
    int rc;

    if (argc < 3) {
        rc = do_insufficient_args(flags, argc, argv);
    } else {
        rc = run_command(flags, NULL, "timedatectl", "set-timezone", argv[2], NULL);
    }
    return rc;
}

/*
 * Install software.  arg is the drive to scan for updates
 */
int do_install(flags_t flags, int argc, char **argv)
{
    int rc;

    if (argc < 3) {
        rc = do_insufficient_args(flags, argc, argv);
    } else {
// printf("Creating temp dir\n");

        char template[] = "/tmp/VMXXXXXX";

        /* Try mounting the device on a tmp directory */
        char *tempdir = mkdtemp(template);

        if (tempdir == NULL) {
            printf("Unable to create temp dir\n");
            rc = 3;
        }

        if (rc == 0) {
//printf("Mounting '%s' on '%s'\n", argv[2], tempdir);
            /* Attempt to mount the specifed device onto this directory */
            rc = mount(argv[2], tempdir, "vfat", 0, NULL);
        }

        int mounted = FALSE;

        if (rc == 0) {
            mounted = TRUE;

//printf("Opening %s\n", tempdir);
            /* Look for UPDATE_FILE in this folder */

            char *filename;

            asprintf(&filename, "%s/%s", tempdir, UPDATE_FILE);

//printf("USERHOMEDIR is %s\n", getenv("USERHOMEDIR"));

            rc = check_file_manifest(flags, filename);
            if (rc == 0) {
                chdir(getenv("USERHOMEDIR"));
                mkdir("software_updates", 0600);
                FILE *in_fp = fopen(filename, "r");
                if (in_fp != NULL) {
                    FILE *out_fp = fopen("software_updates/" UPDATE_FILE, "w");
                    if (out_fp != NULL) {
                        char buf[512];
                        int len;
                        while ((len = fread(buf, 1, sizeof(buf), in_fp)) != 0) {
                            fwrite(buf, 1, len, out_fp);
                        }
                        fclose(out_fp);
                    } else {
                        printf("Unable to create " UPDATE_FILE "\n");
                        rc = 4;
                    }
                    fclose(in_fp);
                } else {
                    printf("No updates found.\n");
                    rc = 5;
                }
            }

            free(filename);
        } else {
            printf("No update thumbdrive found.\n");
            rc = 7;
        }

        if (tempdir != NULL) {
            if (mounted) {
                umount(tempdir);
            }
            rmdir(tempdir);
        }
    }
    return rc;
}

/*
 * Clear any pendint software update.
 */
int do_clear_install(flags_t flags, int argc, char **argv)
{
    chdir(getenv("USERHOMEDIR"));

    return run_command(flags, NULL, "rm", "-rf", "software_updates", NULL);
}

/*
 * Enable usb network device access (experimental)
 */
int do_set_usb_network(flags_t flags, int argc, char **argv)
{
    int rc;

    if (strcmp(argv[2], "enable") == 0) {
        rc = run_command(flags, NULL, "ifup", "--ignore-errors", "eth1", NULL);
        if (rc == 0) {
            struct ifreq ifr;

            ifr.ifr_addr.sa_family = AF_INET;
            strncpy(ifr.ifr_name, "eth1", IFNAMSIZ-1);

            /* Try to get the IP address for a few seconds */
            int count = 0;

            int fd = socket(AF_INET, SOCK_DGRAM, 0);

            do {
                rc = ioctl(fd, SIOCGIFADDR, &ifr);

                if (rc < 0) {
                    sleep(1);
               }
            } while (rc < 0 && --count != 0);

            close(fd);

            if (rc == 0) {
               /* Display IP address */
               printf("%s\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
            }
        }
    } else if (strcmp(argv[2], "disable") == 0) {
        rc = run_command(flags, NULL, "ifdown", "--ignore-errors", "eth1", NULL);
    } else {
        printf("set-usb-network: must be 'enable' or 'disable'.\n");
        rc = 8;
    }

    return rc;
}

/*
 * Save a copy of a file to the selected drive.  Encrypt if possible.
 *
 * vachelper exportfiles <device> <output_file> <file>... <file>
 *
 * Zips up the list of files specified and then encrypts using private key as <output_file>.zip
 */
int do_export_files(flags_t flags, int argc, char **argv)
{
    int rc = 0;

    if (argc < 5) {
        printf("Insufficient args; need 4\n");
        rc = 2;

    } else {
        char *device = argv[2];
        char *output_file = argv[3];
        int first_file = 4;

// printf("Creating temp dir\n");

        /* Try mounting the device on a tmp directory */
        char mountdir_template[] = "/tmp/RHMXXXXXX";
        char *mountdir = mkdtemp(mountdir_template);

        if (mountdir == NULL) {
            printf("Unable to create mount dir\n");
            rc = 3;
        }

        char workdir_template[] = "/tmp/RHWXXXXXX";
        char *workdir = mkdtemp(workdir_template);
        if (workdir == NULL) {
            printf("Unable to create work dir\n");
            rc = 4;
        }

        int mounted = FALSE;

        if (rc == 0) {
            rc = chmod(mountdir, 0777);
        }

        if (rc == 0) {
            /* Attempt to mount the specifed device onto this directory */
            rc = mount(device, mountdir, "vfat", 0, NULL);
// printf("Mounting '%s' on '%s' returned %d\n", device, mountdir, rc);
        }

        if (rc == 0) {
            mounted = TRUE;

            /* Create output zip file name */
            char *zip_file;
            asprintf(&zip_file, "%s/%s.zip", workdir, output_file);

            char *zip_command[argc - first_file + 1 + 4];
            int zip_cmd_index = 0;

            zip_command[zip_cmd_index++] = "zip";
            zip_command[zip_cmd_index++] = "-j";
            zip_command[zip_cmd_index++] = zip_file;

            /* Zip the input files into an output_file.zip in the work directory */
            int file;
            for (file = first_file; file < argc; ++file) {
                if (access(argv[file], F_OK) == 0) {
                    zip_command[zip_cmd_index++] = argv[file];
                }
            }

            zip_command[zip_cmd_index++] = NULL;

            /* Do the zip */
            rc = run_command_array(flags, NULL, zip_command);

            if (rc == 0) {
                /* Get year month and day */
                time_t timeval;
                struct tm tm;
                time(&timeval);
                localtime_r(&timeval, &tm);

                /* Create an output directory with name specifed and date and encrypt the selected files into that directory.
                 * Files created in this directory will be only the basename of the filename given.
                 */
                char *gpg_file;
                asprintf(&gpg_file, "%s/%s-%04d-%02d-%02d-%02d-%02d-%02d.zip.gpg", workdir, output_file, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

                rc = run_command(flags, NULL, "gpg", "--yes", "--always-trust", "--recipient", "Package Signing Key", "-o", gpg_file, "-e", zip_file, NULL);

                if (rc == 0) {
                    /* Copy file to mountdir */
                    rc = run_command(flags, NULL, "cp", gpg_file, mountdir, NULL);
#ifdef DEBUG
                    if (rc != 0) {
                        printf("Unable to copy file to USB\n");
                    }
                } else {
                    printf("gpg '%s' returned %d\n", gpg_file, rc);
#endif
                }

                free(gpg_file);

#ifdef DEBUG
            } else {
                printf("Unable to zip contents\n");
#endif
            }

            free(zip_file);

        } else {
            printf("No thumbdrive found.\n");
            rc = 7;
        }

        if (mountdir != NULL) {
            if (mounted) {
                umount(mountdir);
            }
#ifndef DEBUG
            rmdir(mountdir);
#endif
        }

#ifndef DEBUG
        if (workdir != NULL) {
            // Remove work dir and contents
            run_command(flags, NULL, "rm", "-rf", workdir, NULL);
        }
#endif
    }

    return rc;
}

/*
 * "wifi" "connect" <connection name> <ssid> <extra>
 *                   'password' <password>
 *                   'autoconnect' TRUE/FALSE
 * "wifi" "hotspot" <connection name> <ssid> <extra>
 *                   'password' <password>
 *                   'autoconnect' TRUE/FALSE
 *                   'network' <network description>
 * "wifi" "start" <connection name>
 * "wifi" "stop" <connection name>
 * "wifi" "delete" <connection name>
 * "wifi" "list" [ "ap" | "connections" ]
 * "wifi" "disconnect" <connection name>
 */
#define WIFI_MAX_ARGS    20
int do_wifi(flags_t flags, int argc, char **argv)
{
    int rc = 0;
    char *args[WIFI_MAX_ARGS+1];
    int argn = 0;

    if (strcmp(argv[2], "connect") == 0) {
        /* Connect to an access point */
        if (argc >= 5) {

            char* con_name = argv[3];

            args[argn++] = "/usr/bin/nmcli";
            args[argn++] = "dev";
            args[argn++] = "wifi";
            args[argn++] = "connect";
            args[argn++] = "con-name";
            args[argn++] = con_name;
            args[argn++] = "ssid";
            args[argn++] = argv[4];

	    char *modify_directives[WIFI_MAX_ARGS+1];
	    int modify_count = 0;

	    modify_directives[modify_count++] = "/usr/bin/nmcli";
	    modify_directives[modify_count++] = "con";
	    modify_directives[modify_count++] = "modify";
	    modify_directives[modify_count++] = con_name;

	    int modify_count_init = modify_count;

            int arg = 5;

            while (rc == 0 && arg < argc && argn < WIFI_MAX_ARGS && modify_count < WIFI_MAX_ARGS) {
                if (strcmp(argv[arg], "password") == 0) {
                    /* Put password into initial nmcli command */
                    args[argn++] = argv[arg++];
                    args[argn++] = argv[arg++];
	        } else {
                    /* Otherwise take args specific to a modify directive */
                    modify_directives[modify_count++] = argv[arg++];
                    modify_directives[modify_count++] = argv[arg++];
                }
            }
	    /* Terminate args with a NULL */
	    modify_directives[modify_count] = NULL;

            if (rc == 0) {
                args[argn] = NULL;
                rc = run_command_array(flags, NULL, args);
            }

            if (rc == 0) {
                sleep(2);

                (void) run_command(flags, NULL, "/usr/bin/nmcli", "con", "down", con_name, NULL);

                if (modify_count > modify_count_init) {
                    sleep(2);
		    run_command_array(flags, NULL, modify_directives);
                }

		sleep(2);

                (void) run_command(flags, NULL, "/bin/systemctl", "daemon-reload", NULL);

		sleep(2);

                (void) run_command(flags, NULL, "/bin/systemctl", "restart", "NetworkManager", NULL);
	    }

            /* Loop trying to start connection but only for a few times */
            if (rc == 0) {
                int count = 0;
                do {
                    count++;
                    rc = run_command(flags, NULL, "/usr/bin/nmcli", "con", "up", con_name, NULL);
                    if (rc != 0) {
                        sleep(2);
                    }
                } while (rc != 0 && count < 5);
            }
        }

    } else if (strcmp(argv[2], "hotspot") == 0) {
        /* Connect to an access point */
        if (argc >= 5) {

            char* con_name = argv[3];

            args[argn++] = "/usr/bin/nmcli";
            args[argn++] = "dev";
            args[argn++] = "wifi";
            args[argn++] = "hotspot";
            args[argn++] = "con-name";
            args[argn++] = con_name;
            args[argn++] = "ssid";
            args[argn++] = argv[4];

	    char *modify_directives[WIFI_MAX_ARGS+1];
	    int modify_count = 0;

	    modify_directives[modify_count++] = "/usr/bin/nmcli";
	    modify_directives[modify_count++] = "con";
	    modify_directives[modify_count++] = "modify";
	    modify_directives[modify_count++] = con_name;

	    int modify_count_init = modify_count;

            int arg = 5;
            while (rc == 0 && arg < argc && argn < WIFI_MAX_ARGS && modify_count < WIFI_MAX_ARGS) {
                if (strcmp(argv[arg], "password") == 0) {
                    /* Put password into initial nmcli command */
                    args[argn++] = argv[arg++];
                    args[argn++] = argv[arg++];
	        } else {
                    /* Otherwise take args specific to a modify directive */
                    modify_directives[modify_count++] = argv[arg++];
                    modify_directives[modify_count++] = argv[arg++];
                }
            }
	    /* Terminate args with a NULL */
	    modify_directives[modify_count] = NULL;

            if (rc == 0) {
                args[argn] = NULL;
                rc = run_command_array(flags, NULL, args);
            }

            if (rc == 0) {
                sleep(2);

                (void) run_command(flags, NULL, "/usr/bin/nmcli", "con", "down", con_name, NULL);

                if (modify_count > modify_count_init) {
		    sleep(2);
		    run_command_array(flags, NULL, modify_directives);
                }

		sleep(2);

                (void) run_command(flags, NULL, "/bin/systemctl", "daemon-reload", NULL);

		sleep(2);

                (void) run_command(flags, NULL, "/bin/systemctl", "restart", "NetworkManager", NULL);
            }

            /* Loop trying to start connection but only for a few times */
            if (rc == 0) {
                int count = 0;
                do {
                    count++;
                    rc = run_command(flags, NULL, "/usr/bin/nmcli", "con", "up", con_name, NULL);
                    if (rc != 0) {
                        sleep(2);
                    }
                } while (rc != 0 && count < 5);
            }
        } else {
            rc = 1;
        }

    } else if (strcmp(argv[2], "start") == 0) {
        /* Start a connection if it exists */
        if (argc > 3) {
            rc = run_command(flags, NULL, "/usr/bin/nmcli", "con", "up", argv[3], NULL);
        } else {
            rc = 2;
        }

    } else if (strcmp(argv[2], "stop") == 0) {
        /* Start a connection if it exists */
        if (argc > 3) {
            rc = run_command(flags, NULL, "/usr/bin/nmcli", "con", "down", argv[3], NULL);
        } else {
            rc = 2;
        }

    } else if (strcmp(argv[2], "delete") == 0) {
        /* Start a connection if it exists */
        if (argc > 3) {
            rc = run_command(flags, NULL, "/usr/bin/nmcli", "con", "del", argv[3], NULL);
        } else {
            rc = 2;
        }

    } else if (strcmp(argv[2], "list") == 0) {
        if (argc <= 3 || strcmp(argv[3], "ap") == 0) {
            /* Create list of access points */
            (void) run_command(flags, NULL, "/usr/bin/nmcli", "dev", "wifi", "rescan", NULL);
            //printf("rescan returned %d\n", rc);
            rc = run_command(flags, NULL, "/usr/bin/nmcli",  "-t", "-f", "in-use,ssid,mode,rate,signal,security",  "dev", "wifi", "list", NULL);
            // printf("list returned %d\n", rc);
        } else if (argc > 3 && strcmp(argv[3], "connections") == 0) {
            rc = run_command(flags, NULL, "/usr/bin/nmcli", "-t", "-f", "common", "con", NULL);
        } else {
            fprintf(stderr, "Bad wifi list parameter\n");
            rc = 9;
        }
    } else if (strcmp(argv[2], "disconnect") == 0) {
        /* Disconnect */
        if (argc > 3) {
            rc = run_command(flags, NULL, "/usr/bin/nmcli", "disconnect", argv[3], NULL);
        } else {
            rc = 2;
        }

    } else {
        rc = 1;
    }

    return rc;
}

int main(int argc, char** argv)
{
    int rc = 0;
    unsigned int flags = 0;

    openlog("Logs", LOG_PID, LOG_USER);

    /* Look through for options first */
    int arg = 1;

    while (arg < argc) {
        if (strcmp(argv[arg], "--capture-output") == 0) {
            flags |= FLAGS_CAPTURE_OUTPUT;
            memcpy(argv + arg, argv + arg + 1, sizeof(argv[arg]) * (argc - arg));
            argc--;
        } else if (strcmp(argv[arg], "--show-commands") == 0) {
            flags |= FLAGS_SHOW_COMMANDS;
            memcpy(argv + arg, argv + arg + 1, sizeof(argv[arg]) * (argc - arg));
            argc--;
        } else {
            arg++;
        }
    }

#if 0
    printf("Args remaining:\n");
    for (arg = 0; arg < argc; ++arg) {
        printf("  %s\n", argv[arg]);
    }
#endif


    if (argc < 2)                                             rc = do_insufficient_args(flags, argc, argv);
    else if (strcmp(argv[1], "shutdown") == 0)                rc = do_shutdown(flags, argc, argv);
    else if (strcmp(argv[1], "reboot") == 0)                  rc = do_reboot(flags, argc, argv);
    else if (strcmp(argv[1], "set-date") == 0)                rc = do_setdate(flags, argc, argv);
    else if (strcmp(argv[1], "set-timezone") == 0)            rc = do_settimezone(flags, argc, argv);
    else if (strcmp(argv[1], "install") == 0)                 rc = do_install(flags, argc, argv);
    else if (strcmp(argv[1], "clear-install") == 0)           rc = do_clear_install(flags, argc, argv);
    else if (strcmp(argv[1], "set-usb-network") == 0)         rc = do_set_usb_network(flags, argc, argv);
    else if (strcmp(argv[1], "exportfiles") == 0)             rc = do_export_files(flags, argc, argv);
    else if (strcmp(argv[1], "wifi") == 0)                    rc = do_wifi(flags, argc, argv);
    else                                                      rc = do_unknown_command(flags, argc, argv);

    closelog();

    return rc;
}

