#include <signal.h>
#include <stddef.h>
#include <sys/wait.h>

void SIGCHLDCatcher(int)
{
  wait3(NULL,WNOHANG,NULL);
}

__attribute__((constructor)) void _os_signal_main() {
    signal(SIGCHLD,SIGCHLDCatcher);
}
