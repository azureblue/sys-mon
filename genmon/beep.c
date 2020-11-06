#include <stdlib.h>
#include <unistd.h>

int main() {
  setuid(0);
  system("modprobe pcspkr");
  system("echo -e \"\\a\" > /dev/console");
  system("sleep 0.2 && rmmod pcspkr");
  return 0;
}
