#include <gtk/gtk.h>

#include "ethernet.h"
#include "technology.h"

void technology_ethernet_init(struct technology *tech, GVariantDict *properties) {
	gtk_image_set_from_icon_name(GTK_IMAGE(tech->list_item->icon),
			"network-wired-symbolic", GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_image_set_from_icon_name(GTK_IMAGE(tech->settings->icon),
			"network-wired", GTK_ICON_SIZE_DIALOG);
}
