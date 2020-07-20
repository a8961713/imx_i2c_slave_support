#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdlib.h>

#define SIG_TEST 44
sem_t writeSem;

/* register_pid()
 * Function to register the process id with the i2c-slave notifier.
 * The proc_id is exposed on the i2c device path.
 * Writing the pid to this sysfs register the process to be notified
 */
void register_pid(int pid, char *bus, int addr) {
  int fd;
  int num;
  char str[100];

  num = sprintf(str, "/sys/bus/i2c/devices/%s/%c-00%x/proc_id", bus, bus[4], addr);
  fd = open(str, O_WRONLY);
  
  num = sprintf(str, "%d", pid);
  write (fd, str, num);
  close(fd);
}

/* load_eeprom_from_file()
 * Load the contents of a file (filename) into the simulated eeprom
 * Since the eeprom is simulated by the slave-eeprom backend driver
 * it starts with zeros at probe time. To keep the record a backup
 * file can be stored on the filesystem and initialize the eeprom
 * with this backup-file
 */
#define EEPROM_SIZE 0x100
void load_eeprom_from_file(char* filename, char *bus, int addr) {
  FILE *fd_device;
  FILE *fd_file;
  int i;
  char ch;
  char str[100];

  sprintf(str, "/sys/bus/i2c/devices/%s/%c-00%x/slave-eeprom", bus, bus[4], addr);
  fd_device = fopen(str, "w");
  fd_file = fopen(filename, "r");
 
  printf("Loading eeprom values\n");
  for (i = 0; i < EEPROM_SIZE; i ++) {
    ch = fgetc(fd_file);
    fputc(ch, fd_device);
  }

  fclose(fd_file);
  fclose(fd_device);
}

/* backup_eeprom_on_file()
 * This function updates the contents of the backup-file, so it 
 * mirror the slave-eeprom content and this can be restored at any
 * time if needed or after restarting the system.
 */

void backup_eeprom_on_file(char* filename, char *bus, int addr) {
  FILE *fd_device;
  FILE *fd_file;
  int i;
  char ch;
  char str[100];

  sprintf(str, "/sys/bus/i2c/devices/%s/%c-00%x/slave-eeprom", bus, bus[4], addr);
  fd_device = fopen(str, "r");
  fd_file = fopen(filename, "w");

  printf("Backup eeprom content on file...\n");
  for (i = 0; i < EEPROM_SIZE; i ++) {
    ch = fgetc(fd_device);
    fputc(ch, fd_file);
  }

  fclose(fd_file);
  fclose(fd_device);
}

/* signalFunction()
 * Callback function called when kernel driver signal the STOP event
 * Since the STOP event is called multiple times, the very first time
 * the callback sleeps to give a chance the I2C TX/RX event finishes
 * and let the rest of the signals (STOP events) arrive all at once.
 * Posting the semaphores after the TX/RX has finished allow to only
 * unlock and attend  once the backup_eeprom_on_file routine
 * instead of multiple times during the middle of the TX/RX event.
 */
void signalFunction(int n, siginfo_t *info, void *unused) {
  int count;
  sem_getvalue(&writeSem, &count);

  /* Sleep 1 sec when receiving the very first signal     
   * to allow the TX/RX finishes. After that we can post  
   * semaphore to backup the eeprom content.
   */

  if (!count)
    sleep(1); /* Wait for the I2C TX/RX to finish */
  sem_post(&writeSem);
     
}

void print_usage(int argc, char *argv[])
{
  printf("Usage: \n\n");
  printf("  %s i2c-<bus> <address> <romfile> & \n\n", argv[0]);
  printf("Example for a device at addr=0x64 on the 12c-1 bus with storage file named romFile: \n\n");
  printf("  %s i2c-1 0x64 romFile & \n\n", argv[0]);
}

void main(int argc, char *argv[]){
  int pid;
  int count;
  char *bus;
  char* filename;
  int addr;
  struct sigaction sig;

  /* Minimal error-checking added */
  if (argc < 4) {
    print_usage(argc, argv);
    return;
 }

  bus = argv[1];
  addr = strtol(argv[2], NULL, 0);
  filename = argv[3];

  sem_init(&writeSem, 0, 0);

  /* Registering this process with the i2c-slave backend by pid */
  pid = getpid();
  register_pid(pid, bus, addr);

  /* Register the callback that attends the notification */
  sig.sa_sigaction = signalFunction;
  sig.sa_flags = SA_SIGINFO;
  sigaction(SIG_TEST, &sig, NULL);

  /* Load eeprom register from the backup-file */
  load_eeprom_from_file(filename, bus, addr);

  printf("Starting the listener loop...\n");
  while(1)
  {
    /* Pending for notifications to arrive*/
    sem_wait(&writeSem);
    sem_getvalue(&writeSem, &count);
    /* Only until no more posts are pending backup the file */
    if (!count) {
      backup_eeprom_on_file(filename, bus, addr);
    }
  }

}
