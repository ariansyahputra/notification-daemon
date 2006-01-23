#include <gtk/gtk.h>
#include <libsexy/sexy-url-label.h>
#include "bgbox.h"

typedef void (*ActionInvokedCb)(GtkWindow *nw, const char *key);
typedef void (*UrlClickedCb)(GtkWindow *nw, const char *url);

typedef struct
{
	GtkWidget *top_spacer;
	GtkWidget *bottom_spacer;
	GtkWidget *iconbox;
	GtkWidget *icon;
	GtkWidget *contentbox;
	GtkWidget *summary_label;
	GtkWidget *body_label;
	GtkWidget *actions_box;
	GtkWidget *last_sep;

	guint num_actions_added;

	gboolean has_arrow;

	int point_x;
	int point_y;

	int drawn_arrow_begin_x;
	int drawn_arrow_begin_y;
	int drawn_arrow_middle_x;
	int drawn_arrow_middle_y;
	int drawn_arrow_end_x;
	int drawn_arrow_end_y;

	GdkGC *gc;
	GdkPoint *border_points;
	size_t num_border_points;
	GdkRegion *window_region;

	guchar urgency;

	UrlClickedCb url_clicked;

} WindowData;

#define WIDTH         300
#define MIN_HEIGHT    100
#define IMAGE_SIZE    48
#define IMAGE_PADDING 10
#define DEFAULT_ARROW_OFFSET  20
#define DEFAULT_ARROW_HEIGHT  14
#define DEFAULT_ARROW_WIDTH   28

static gboolean
draw_border(GtkWidget *win, GdkEventExpose *event, WindowData *windata)
{
	int w, h;

	if (windata->gc == NULL)
	{
		GdkColor color;

		windata->gc = gdk_gc_new(win->window);
		gdk_color_parse("black", &color);
		gdk_gc_set_rgb_fg_color(windata->gc, &color);
	}

	gdk_drawable_get_size(win->window, &w, &h);

	if (windata->has_arrow)
	{
		gdk_draw_polygon(win->window, windata->gc, FALSE,
						 windata->border_points, windata->num_border_points);

		/* HACK! */
#if 0
		gdk_draw_line(win->window, windata->gc,
					  windata->drawn_arrow_begin_x,
					  windata->drawn_arrow_begin_y,
					  windata->drawn_arrow_middle_x,
					  windata->drawn_arrow_middle_y);
		gdk_draw_line(win->window, windata->gc,
					  windata->drawn_arrow_begin_x + 1,
					  windata->drawn_arrow_begin_y,
					  windata->drawn_arrow_middle_x + 1,
					  windata->drawn_arrow_middle_y);

		gdk_draw_line(win->window, windata->gc,
					  windata->drawn_arrow_middle_x,
					  windata->drawn_arrow_middle_y,
					  windata->drawn_arrow_end_x,
					  windata->drawn_arrow_end_y);
		gdk_draw_line(win->window, windata->gc,
					  windata->drawn_arrow_middle_x - 1,
					  windata->drawn_arrow_middle_y,
					  windata->drawn_arrow_end_x - 1,
					  windata->drawn_arrow_end_y);
		gdk_draw_line(win->window, windata->gc, 0, h - 1, w - 1, h - 1);
#endif
		gdk_window_shape_combine_region(win->window, windata->window_region,
										0, 0);
	}
	else
	{
		gdk_draw_rectangle(win->window, windata->gc, FALSE,
						   0, 0, w - 1, h - 1);
	}

	return FALSE;
}

GtkWindow *
create_notification(UrlClickedCb url_clicked)
{
	GtkWidget *win;
	GtkWidget *main_vbox;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkRequisition req;
	WindowData *windata;

	windata = g_new0(WindowData, 1);
	windata->url_clicked = url_clicked;

	win = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_widget_add_events(win, GDK_BUTTON_RELEASE_MASK);
	g_object_set_data(G_OBJECT(win), "windata", windata);
	gtk_widget_set_app_paintable(win, TRUE);

	g_signal_connect(G_OBJECT(win), "expose-event",
					 G_CALLBACK(draw_border), windata);

	main_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(main_vbox);
	gtk_container_add(GTK_CONTAINER(win), main_vbox);
	gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 1);

	windata->top_spacer = gtk_image_new();
	gtk_box_pack_start(GTK_BOX(main_vbox), windata->top_spacer,
					   FALSE, FALSE, 0);
	gtk_widget_set_size_request(windata->top_spacer, -1, DEFAULT_ARROW_HEIGHT);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(main_vbox), hbox, FALSE, FALSE, 0);

	windata->bottom_spacer = gtk_image_new();
	gtk_box_pack_start(GTK_BOX(main_vbox), windata->bottom_spacer,
					   FALSE, FALSE, 0);
	gtk_widget_set_size_request(windata->bottom_spacer, -1,
								DEFAULT_ARROW_HEIGHT);

	windata->iconbox = gtk_event_box_new();
	gtk_widget_show(windata->iconbox);
	gtk_box_pack_start(GTK_BOX(hbox), windata->iconbox, FALSE, TRUE, 0);

	windata->icon = gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO,
											 GTK_ICON_SIZE_DIALOG);
	gtk_widget_show(windata->icon);
	gtk_container_add(GTK_CONTAINER(windata->iconbox), windata->icon);
	gtk_misc_set_alignment(GTK_MISC(windata->icon), 0.5, 0.0);
	gtk_container_set_border_width(GTK_CONTAINER(windata->iconbox), 12);

	windata->contentbox = notifyd_bgbox_new(NOTIFYD_BASE);
	gtk_widget_show(windata->contentbox);
	gtk_box_pack_start(GTK_BOX(hbox), windata->contentbox, TRUE, TRUE, 0);

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(windata->contentbox), vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

	windata->summary_label = gtk_label_new(NULL);
	gtk_widget_show(windata->summary_label);
	gtk_box_pack_start(GTK_BOX(vbox), windata->summary_label, FALSE, FALSE, 0);
	gtk_misc_set_alignment(GTK_MISC(windata->summary_label), 0, 0);

	windata->body_label = sexy_url_label_new();
	gtk_widget_show(windata->body_label);
	gtk_box_pack_start(GTK_BOX(vbox), windata->body_label, TRUE, TRUE, 0);
	gtk_misc_set_alignment(GTK_MISC(windata->body_label), 0, 0);
	gtk_label_set_line_wrap(GTK_LABEL(windata->body_label), TRUE);
	g_signal_connect_swapped(G_OBJECT(windata->body_label), "url_activated",
							 G_CALLBACK(windata->url_clicked), win);

	windata->actions_box = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(windata->actions_box);
	gtk_box_pack_start(GTK_BOX(vbox), windata->actions_box, FALSE, TRUE, 0);

	gtk_widget_size_request(hbox, &req);
	gtk_widget_set_size_request(hbox, -1, MAX(MIN_HEIGHT, req.height));

	return GTK_WINDOW(win);
}

void
destroy_notification(GtkWindow *nw)
{
	WindowData *windata = g_object_get_data(G_OBJECT(nw), "windata");
	g_assert(windata != NULL);

	if (windata->gc != NULL)
		g_object_unref(G_OBJECT(windata->gc));

	if (windata->window_region != NULL)
		gdk_region_destroy(windata->window_region);

	gtk_widget_destroy(GTK_WIDGET(nw));
}

void
show_notification(GtkWindow *nw)
{
	gtk_widget_show(GTK_WIDGET(nw));
}

void
hide_notification(GtkWindow *nw)
{
	gtk_widget_hide(GTK_WIDGET(nw));
}

void
set_notification_hints(GtkWindow *nw, GHashTable *hints)
{
	WindowData *windata = g_object_get_data(G_OBJECT(nw), "windata");
	GValue *value;

	g_assert(windata != NULL);

	value = (GValue *)g_hash_table_lookup(hints, "urgency");

	if (value)
		windata->urgency = g_value_get_uchar(value);
}

void
set_notification_text(GtkWindow *nw, const char *summary, const char *body)
{
	char *str;
	WindowData *windata = g_object_get_data(G_OBJECT(nw), "windata");
	g_assert(windata != NULL);

	str = g_strdup_printf("<b><big>%s</big></b>", summary);
	gtk_label_set_markup(GTK_LABEL(windata->summary_label), str);
	g_free(str);

	sexy_url_label_set_markup(SEXY_URL_LABEL(windata->body_label), body);

	gtk_widget_set_size_request(
		((body != NULL && *body == '\0')
		 ? windata->body_label : windata->summary_label),
		WIDTH - (IMAGE_SIZE + IMAGE_PADDING) - 10,
		-1);
}

void
set_notification_icon(GtkWindow *nw, GdkPixbuf *pixbuf)
{
	WindowData *windata = g_object_get_data(G_OBJECT(nw), "windata");
	g_assert(windata != NULL);

	gtk_image_set_from_pixbuf(GTK_IMAGE(windata->icon), pixbuf);
}

void
set_notification_arrow(GtkWindow *nw, gboolean visible, int x, int y)
{
	WindowData *windata = g_object_get_data(G_OBJECT(nw), "windata");
	g_assert(windata != NULL);

	windata->has_arrow = visible;
	windata->point_x = x;
	windata->point_y = y;

	if (!visible)
	{
		gtk_widget_hide(windata->top_spacer);
		gtk_widget_hide(windata->bottom_spacer);
	}
}

static void
action_clicked_cb(GtkWidget *w, GdkEventButton *event,
				  ActionInvokedCb action_cb)
{
	GtkWindow *nw   = g_object_get_data(G_OBJECT(w), "_nw");
	const char *key = g_object_get_data(G_OBJECT(w), "_action_key");

	action_cb(nw, key);
}

void
add_notification_action(GtkWindow *nw, const char *text, const char *key,
						ActionInvokedCb cb)
{
	/*
	 * TODO: Use SexyUrlLabel. This requires a way of disabling the
	 *       right-click menu.
	 */
	WindowData *windata = g_object_get_data(G_OBJECT(nw), "windata");
	GtkWidget *bgbox;
	GtkWidget *label;
	GdkCursor *cursor;
	char *buf;

	g_assert(windata != NULL);

	if (windata->num_actions_added > 0)
	{
		label = gtk_label_new("‧");
		gtk_widget_show(label);
		gtk_box_pack_start(GTK_BOX(windata->actions_box), label,
						    FALSE, FALSE, 0);
	}

	bgbox = notifyd_bgbox_new(NOTIFYD_BASE);
	gtk_widget_show(bgbox);
	gtk_box_pack_start(GTK_BOX(windata->actions_box), bgbox, FALSE, FALSE, 0);

	g_object_set_data(G_OBJECT(bgbox), "_nw", nw);
	g_object_set_data_full(G_OBJECT(bgbox),
						   "_action_key", g_strdup(key), g_free);
	g_signal_connect(G_OBJECT(bgbox), "button-release-event",
					 G_CALLBACK(action_clicked_cb), cb);

	cursor = gdk_cursor_new_for_display(gtk_widget_get_display(bgbox),
										GDK_HAND2);
	gdk_window_set_cursor(bgbox->window, cursor);
	gdk_cursor_unref(cursor);

	label = gtk_label_new(NULL);
	gtk_widget_show(label);
	gtk_container_add(GTK_CONTAINER(bgbox), label);
	buf = g_strdup_printf("<span color=\"blue\""
						  " underline=\"single\">%s</span>", text);
	gtk_label_set_markup(GTK_LABEL(label), buf);
	g_free(buf);

	windata->num_actions_added++;
}

#define ADD_POINT(_x, _y) \
	do { \
		windata->border_points[i].x = (_x); \
		windata->border_points[i].y = (_y); \
		i++;\
	} while (0)

static void
create_border_with_arrow(GtkWidget *nw, WindowData *windata,
						 int arrow_tip_x, int arrow_tip_y)
{
	GtkRequisition req;
	GtkArrowType arrow_type;
	GdkRectangle border_rect;
	GdkScreen *screen;
	int screen_width;
	int screen_height;
	int arrow_side1_width = DEFAULT_ARROW_WIDTH / 2;
	int arrow_side2_width = DEFAULT_ARROW_WIDTH / 2;
	int arrow_offset = DEFAULT_ARROW_OFFSET;
	int i = 0;

	gtk_widget_realize(nw);
	gtk_widget_size_request(nw, &req);

	border_rect.width = req.width;
	border_rect.height = req.height;

	screen        = gdk_drawable_get_screen(GDK_DRAWABLE(nw->window));
	screen_width  = gdk_screen_get_width(screen);
	screen_height = gdk_screen_get_height(screen);

	if (windata->border_points != NULL)
		g_free(windata->border_points);

	windata->num_border_points = 5;

	if (arrow_tip_y + req.height + DEFAULT_ARROW_HEIGHT > screen_height)
		arrow_type = GTK_ARROW_DOWN;
	else
		arrow_type = GTK_ARROW_UP;

	/* Handle the offset and such */
	switch (arrow_type)
	{
		case GTK_ARROW_UP:
		case GTK_ARROW_DOWN:
			if (arrow_tip_x < arrow_side1_width)
			{
				arrow_side1_width = 0;
				arrow_offset = 0;
			}
			else if (arrow_tip_x > screen_width - arrow_side2_width)
			{
				arrow_side2_width = 0;
				arrow_offset = req.width - arrow_side1_width;
			}
			else
			{
				if (arrow_tip_x - arrow_side2_width + req.width >= screen_width)
				{
					arrow_offset =
						req.width - arrow_side1_width - arrow_side2_width -
						(screen_width - MAX(arrow_tip_x + arrow_side1_width,
											screen_width -
											DEFAULT_ARROW_OFFSET));
				}
				else
				{
					arrow_offset = MIN(arrow_tip_x - arrow_side1_width,
									   DEFAULT_ARROW_OFFSET);
				}

				if (arrow_offset == 0 ||
					arrow_offset == req.width - arrow_side1_width)
					windata->num_border_points++;
				else
					windata->num_border_points += 2;
			}

			/*
			 * Why risk this for official builds? If it's somehow off the
			 * screen, it won't horribly impact the user. Definitely less
			 * than an assertion would...
			 */
#if 0
			g_assert(arrow_offset + arrow_side1_width >= 0);
			g_assert(arrow_offset + arrow_side1_width + arrow_side2_width <=
					 req.width);
#endif

			windata->border_points = g_new0(GdkPoint,
											windata->num_border_points);

			border_rect.x = arrow_tip_x - arrow_side1_width - arrow_offset;
			border_rect.y = arrow_tip_y + (arrow_type == GTK_ARROW_UP
										   ? DEFAULT_ARROW_HEIGHT
										   : -DEFAULT_ARROW_HEIGHT);

			windata->drawn_arrow_begin_x = arrow_offset;
			windata->drawn_arrow_middle_x = arrow_offset + arrow_side1_width;
			windata->drawn_arrow_end_x = arrow_offset + arrow_side1_width +
										 arrow_side2_width;

			if (arrow_type == GTK_ARROW_UP)
			{
				gtk_widget_show(windata->top_spacer);
				windata->drawn_arrow_begin_y = DEFAULT_ARROW_HEIGHT;
				windata->drawn_arrow_middle_y = 0;
				windata->drawn_arrow_end_y = DEFAULT_ARROW_HEIGHT;

				if (arrow_side1_width == 0)
				{
					ADD_POINT(0, 0);
				}
				else
				{
					ADD_POINT(0, DEFAULT_ARROW_HEIGHT);

					if (arrow_offset > 0)
						ADD_POINT(arrow_offset, DEFAULT_ARROW_HEIGHT);

					ADD_POINT(arrow_offset + arrow_side1_width, 0);
				}

				if (arrow_side2_width > 0)
				{
					ADD_POINT(windata->drawn_arrow_end_x,
							  windata->drawn_arrow_end_y);
					ADD_POINT(req.width, DEFAULT_ARROW_HEIGHT);
				}

				ADD_POINT(req.width, req.height + DEFAULT_ARROW_HEIGHT);
				ADD_POINT(0, req.height + DEFAULT_ARROW_HEIGHT);
			}
			else
			{
				gtk_widget_show(windata->bottom_spacer);
				windata->drawn_arrow_begin_y = req.height;
				windata->drawn_arrow_middle_y = req.height +
				                                DEFAULT_ARROW_HEIGHT;
				windata->drawn_arrow_end_y = req.height;

				ADD_POINT(0, 0);
				ADD_POINT(req.width, 0);

				if (arrow_side2_width == 0)
				{
					ADD_POINT(req.width, req.height + DEFAULT_ARROW_HEIGHT);
				}
				else
				{
					ADD_POINT(req.width, req.height);

					if (arrow_offset < req.width - arrow_side1_width)
					{
						ADD_POINT(arrow_offset + arrow_side1_width +
								  arrow_side2_width, req.height);
					}

					ADD_POINT(arrow_offset + arrow_side1_width,
							  req.height + DEFAULT_ARROW_HEIGHT);
				}

				if (arrow_side1_width > 0)
				{
					ADD_POINT(windata->drawn_arrow_begin_x,
							  windata->drawn_arrow_begin_y);
					ADD_POINT(0, req.height);
				}
			}

			g_assert(i == windata->num_border_points);
			g_assert(arrow_tip_x - arrow_offset - arrow_side1_width >= 0);
			gtk_window_move(GTK_WINDOW(nw),
							arrow_tip_x - arrow_offset - arrow_side1_width,
							(arrow_type == GTK_ARROW_UP
							 ? arrow_tip_y
							 : arrow_tip_y - req.height -
							   DEFAULT_ARROW_HEIGHT));

			break;

		case GTK_ARROW_LEFT:
		case GTK_ARROW_RIGHT:
			if (arrow_tip_y < arrow_side1_width)
			{
				arrow_side1_width = 0;
				arrow_offset = arrow_tip_y;
			}
			else if (arrow_tip_y > screen_height - arrow_side2_width)
			{
				arrow_side2_width = 0;
				arrow_offset = arrow_tip_y - arrow_side1_width;
			}

			border_rect.x = arrow_tip_x + (arrow_type == GTK_ARROW_LEFT
										   ? DEFAULT_ARROW_HEIGHT
										   : -DEFAULT_ARROW_HEIGHT);
			border_rect.y = arrow_tip_y + arrow_offset;
			break;
	}

	windata->window_region =
		gdk_region_polygon(windata->border_points, windata->num_border_points,
						   GDK_EVEN_ODD_RULE);

	draw_border(nw, NULL, windata);
}

static void
generate_arrow(GtkWidget *nw, WindowData *windata, int *arrow_x, int *arrow_y)
{
	GtkRequisition req;
	int new_height;
	int arrow_left_width  = DEFAULT_ARROW_WIDTH / 2;
	int arrow_right_width = DEFAULT_ARROW_WIDTH / 2;
	int arrow_offset = DEFAULT_ARROW_OFFSET;

	gtk_widget_realize(nw);
	gtk_widget_size_request(nw, &req);

	new_height = req.height + DEFAULT_ARROW_HEIGHT;

	printf("point_x = %d\n", windata->point_x);
	if (windata->point_x < DEFAULT_ARROW_WIDTH / 2)
	{
		arrow_left_width = 0;
		arrow_offset = 0;
	}

	windata->drawn_arrow_begin_x  = arrow_offset;
	windata->drawn_arrow_begin_y  = DEFAULT_ARROW_HEIGHT;
	windata->drawn_arrow_middle_x = arrow_offset + arrow_left_width;
	windata->drawn_arrow_middle_y = 0;
	windata->drawn_arrow_end_x    = windata->drawn_arrow_middle_x +
	                                arrow_right_width;
	windata->drawn_arrow_end_y    = DEFAULT_ARROW_HEIGHT;

	windata->border_points[0].x = 0;
	windata->border_points[0].y = DEFAULT_ARROW_HEIGHT;

	windata->border_points[1].x = windata->drawn_arrow_begin_x;
	windata->border_points[1].y = windata->drawn_arrow_begin_y;

	windata->border_points[2].x = windata->drawn_arrow_middle_x;
	windata->border_points[2].y = windata->drawn_arrow_middle_y;

	windata->border_points[3].x = windata->drawn_arrow_end_x;
	windata->border_points[3].y = windata->drawn_arrow_end_y;

	windata->border_points[4].x = req.width;
	windata->border_points[4].y = DEFAULT_ARROW_HEIGHT;

	windata->border_points[5].x = req.width;
	windata->border_points[5].y = new_height;

	windata->border_points[6].x = 0;
	windata->border_points[6].y = new_height;

	windata->window_region =
		gdk_region_polygon(windata->border_points,
						   G_N_ELEMENTS(windata->border_points),
						   GDK_EVEN_ODD_RULE);

	windata->border_points[4].x--;
	windata->border_points[5].x--;
	windata->border_points[5].y--;
	windata->border_points[6].y--;

	*arrow_x = windata->drawn_arrow_middle_x;
	*arrow_y = windata->drawn_arrow_middle_y;

	draw_border(nw, NULL, windata);
}

void
move_notification(GtkWindow *nw, int x, int y)
{
	WindowData *windata = g_object_get_data(G_OBJECT(nw), "windata");
	g_assert(windata != NULL);

	if (windata->has_arrow)
	{
		create_border_with_arrow(GTK_WIDGET(nw), windata, 500, 1100);
		//generate_arrow(GTK_WIDGET(nw), windata, &arrow_x, &arrow_y);
		return;
	}
	else
	{
		gtk_window_move(GTK_WINDOW(nw), x, y);
	}
}
