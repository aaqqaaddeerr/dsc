
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

typedef void (IPC) (const struct ip *);
IPC ip_message_handle;

void ip_message_report(void);
int ip_message_add_array(const char *name, const char *fn, const char *fi,
    const char *sn, const char *si, const char *f, int, int);
