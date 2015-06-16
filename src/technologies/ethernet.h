#ifndef _CONNMAN_GTK_ETHERNET_H
#define _CONNMAN_GTK_ETHERNET_H

#include <gtk/gtk.h>

#include "technology.h"

void technology_ethernet_init(struct technology *tech, GVariantDict *properties);

#endif /* _CONNMAN_GTK_ETHERNET_H */

