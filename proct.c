#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 4096

static double get_uptime(void) {
  FILE *fp = fopen("/proc/uptime", "r");
  if (!fp)
    return -1;

  double uptime = 0;
  if (fscanf(fp, "%lf", &uptime) != 1) {
    fclose(fp);
    return -1;
  }

  fclose(fp);
  return uptime;
}

static long get_start_time(int pid) {
  char path[64], buf[BUF_SIZE];
  snprintf(path, sizeof(path), "/proc/%d/stat", pid);
  FILE *fp = fopen(path, "r");
  if (!fp)
    return -1;

  if (!fgets(buf, sizeof(buf), fp)) {
    fclose(fp);
    return -1;
  }

  fclose(fp);

  char *p = buf;
  int field = 1;
  while (field < 22 && (p = strchr(p, ' '))) {
    p++;
    field++;
  }

  if (!p)
    return -1;

  errno = 0;
  long val = strtol(p, NULL, 10);
  if (errno)
    return -1;

  return val;
}

static int get_proc_name(int pid, char *name, size_t len) {
  char path[64];
  snprintf(path, sizeof(path), "/proc/%d/comm", pid);
  FILE *fp = fopen(path, "r");
  if (!fp)
    return -1;

  if (!fgets(name, len, fp)) {
    fclose(fp);
    return -1;
  }

  fclose(fp);
  name[strcspn(name, "\n")] = 0;
  return 0;
}

static double get_seconds_by_name(const char *target_name, int *pid_out) {
  DIR *dir = opendir("/proc");
  if (!dir)
    return -1;

  struct dirent *entry;
  double result = -1;
  double uptime = get_uptime();
  if (uptime < 0) {
    closedir(dir);
    return -1;
  }

  while ((entry = readdir(dir))) {
    if (!isdigit((unsigned char)entry->d_name[0]))
      continue;

    int pid = atoi(entry->d_name);
    char name[256];
    if (get_proc_name(pid, name, sizeof(name)) == 0) {
      if (strcmp(name, target_name) == 0) {
        long start_time = get_start_time(pid);
        if (start_time < 0) {
          closedir(dir);
          return -1;
        }

        long clk_tck = sysconf(_SC_CLK_TCK);
        result = uptime - ((double)start_time / clk_tck);
        if (result < 0)
          result = 0;

        if (pid_out)
          *pid_out = pid;

        closedir(dir);
        return result;
      }
    }
  }

  closedir(dir);
  return -1;
}

static void print_usage(const char *prog) {
  fprintf(stderr, "Usage: %s [-h] -p <pid> | -n <name1> [name2] [name3] ...\n",
          prog);
  fprintf(stderr,
          "  -h Show output in hours and minutes instead of seconds.\n");
  fprintf(stderr, "  -p <pid> Show uptime for process with given PID\n");
  fprintf(
      stderr,
      "  -n <name> [name2] ... Show uptime for processes with given names\n");
}

static void print_time(double seconds, int human_readable) {
  if (!human_readable) {
    printf("Seconds active: %.0f\n", seconds);
  } else {
    int hours = (int)(seconds / 3600);
    int minutes = (int)((seconds - (hours * 3600)) / 60);
    int sec = (int)(seconds) % 60;
    printf("Active time: %02dh %02dm %02ds\n", hours, minutes, sec);
  }
}

int main(int argc, char **argv) {
  int human_readable = 0;
  int pid = -1;
  double seconds = -1;
  char proc_name[256] = {0};
  int argi = 1;

  if (argc < 3) {
    print_usage(argv[0]);
    return 1;
  }

  if (strcmp(argv[argi], "-h") == 0) {
    human_readable = 1;
    argi++;
    if (argc < 4) {
      print_usage(argv[0]);
      return 1;
    }
  }

  if (strcmp(argv[argi], "-p") == 0) {
    if (argi + 1 >= argc) {
      print_usage(argv[0]);
      return 1;
    }

    pid = atoi(argv[argi + 1]);
    if (pid <= 0) {
      fprintf(stderr, "Invalid PID: %s\n", argv[argi + 1]);
      return 1;
    }

    if (get_proc_name(pid, proc_name, sizeof(proc_name)) != 0) {
      fprintf(stderr, "PID %d not found.\n", pid);
      return 1;
    }

    double uptime = get_uptime();
    long start_time = get_start_time(pid);
    if (uptime < 0 || start_time < 0) {
      fprintf(stderr, "Unable to get process info for PID %d.\n", pid);
      return 1;
    }

    long clk_tck = sysconf(_SC_CLK_TCK);
    seconds = uptime - ((double)start_time / clk_tck);
    if (seconds < 0)
      seconds = 0;

    printf("Process: %s (PID: %d)\n", proc_name, pid);
    print_time(seconds, human_readable);
    return 0;

  } else if (strcmp(argv[argi], "-n") == 0) {
    if (argi + 1 >= argc) {
      print_usage(argv[0]);
      return 1;
    }

    int found_count = 0;
    int not_found_count = 0;

    for (int i = argi + 1; i < argc; i++) {
      const char *target_name = argv[i];

      seconds = get_seconds_by_name(target_name, &pid);
      if (seconds < 0) {
        fprintf(stderr, "Process with name '%s' not found.\n", target_name);
        not_found_count++;
        continue;
      }

      if (get_proc_name(pid, proc_name, sizeof(proc_name)) != 0) {
        fprintf(stderr,
                "Process with name '%s' found but failed to read comm.\n",
                target_name);
        not_found_count++;
        continue;
      }

      if (found_count > 0) {
        printf("\n");
      }

      printf("Process: %s (PID: %d)\n", proc_name, pid);
      print_time(seconds, human_readable);
      found_count++;
    }

    if (found_count == 0) {
      fprintf(stderr, "No processes found with the given names.\n");
      return 1;
    }

    return 0;

  } else {
    print_usage(argv[0]);
    return 1;
  }
}
