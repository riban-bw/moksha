#include "e.h"
#include "e_mod_main.h"

/* TODO List:
 *
 * should support proepr resize and move handles in the edje.
 * 
 */

/* module private routines */
static Battery *_battery_init(E_Module *m);
static void     _battery_shutdown(Battery *e);
static E_Menu  *_battery_config_menu_new(Battery *e);
static void     _battery_config_menu_del(Battery *e, E_Menu *m);
static void     _battery_face_init(Battery_Face *ef);
static void     _battery_face_free(Battery_Face *ef);
static void     _battery_face_reconfigure(Battery_Face *ef);
static void     _battery_cb_face_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void     _battery_cb_face_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void     _battery_cb_face_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static int      _battery_cb_event_container_resize(void *data, int type, void *event);

/* public module routines. all modules must have these */
void *
init(E_Module *m)
{
   Battery *e;
   
   /* check module api version */
   if (m->api->version < E_MODULE_API_VERSION)
     {
	e_error_dialog_show("Module API Error",
			    "Error initializing Module: IBar\n"
			    "It requires a minimum module API version of: %i.\n"
			    "The module API advertized by Enlightenment is: %i.\n"
			    "Aborting module.",
			    E_MODULE_API_VERSION,
			    m->api->version);
	return NULL;
     }
   /* actually init battery */
   e = _battery_init(m);
   m->config_menu = _battery_config_menu_new(e);
   return e;
}

int
shutdown(E_Module *m)
{
   Battery *e;
   
   e = m->data;
   if (e)
     {
	if (m->config_menu)
	  {
	     _battery_config_menu_del(e, m->config_menu);
	     m->config_menu = NULL;
	  }
	_battery_shutdown(e);
     }
   return 1;
}

int
save(E_Module *m)
{
   Battery *e;

   e = m->data;
   e_config_domain_save("module.battery", e->conf_edd, e->conf);
   return 1;
}

int
info(E_Module *m)
{
   char buf[4096];
   
   m->label = strdup("Battery");
   snprintf(buf, sizeof(buf), "%s/module_icon.png", e_module_dir_get(m));
   m->icon_file = strdup(buf);
   return 1;
}

int
about(E_Module *m)
{
   e_error_dialog_show("Enlightenment Battery Module",
		       "A simple module to give E17 a battery meter.");
   return 1;
}

/* module private routines */
static Battery *
_battery_init(E_Module *m)
{
   Battery *e;
   Evas_List *managers, *l, *l2;
   
   e = calloc(1, sizeof(Battery));
   if (!e) return NULL;
   
   e->conf_edd = E_CONFIG_DD_NEW("Battery_Config", Config);
#undef T
#undef D
#define T Config
#define D e->conf_edd   
   E_CONFIG_VAL(D, T, width, INT);
   E_CONFIG_VAL(D, T, x, DOUBLE);
   E_CONFIG_VAL(D, T, y, DOUBLE);

   e->conf = e_config_domain_load("module.battery", e->conf_edd);
   if (!e->conf)
     {
	e->conf = E_NEW(Config, 1);
	e->conf->width = 64;
	e->conf->x = 1.0;
	e->conf->y = 1.0;
     }
   E_CONFIG_LIMIT(e->conf->width, 2, 256);
   E_CONFIG_LIMIT(e->conf->x, 0.0, 1.0);
   E_CONFIG_LIMIT(e->conf->y, 0.0, 1.0);
   
   managers = e_manager_list();
   for (l = managers; l; l = l->next)
     {
	E_Manager *man;
	
	man = l->data;
	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     E_Container *con;
	     Battery_Face *ef;
	     
	     con = l2->data;
	     ef = calloc(1, sizeof(Battery_Face));
	     if (ef)
	       {
		  ef->bat = e;
		  ef->con = con;
		  ef->evas = con->bg_evas;
		  _battery_face_init(ef);
		  e->face = ef;
	       }
	  }
     }
   return e;
}

static void
_battery_shutdown(Battery *e)
{
   free(e->conf);
   E_CONFIG_DD_FREE(e->conf_edd);
   
   _battery_face_free(e->face);
   free(e);
}

static E_Menu *
_battery_config_menu_new(Battery *e)
{
   E_Menu *mn;
   E_Menu_Item *mi;

   /* FIXME: hook callbacks to each menu item */
   mn = e_menu_new();
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "(Unused)");
/*   e_menu_item_callback_set(mi, _battery_cb_time_set, e);*/
   e->config_menu = mn;
   
   return mn;
}

static void
_battery_config_menu_del(Battery *e, E_Menu *m)
{
   e_object_del(E_OBJECT(m));
}

static void
_battery_face_init(Battery_Face *ef)
{
   Evas_Coord ww, hh, bw, bh;
   Evas_Object *o;
   
   ef->ev_handler_container_resize =
     ecore_event_handler_add(E_EVENT_CONTAINER_RESIZE,
			     _battery_cb_event_container_resize,
			     ef);
   evas_output_viewport_get(ef->evas, NULL, NULL, &ww, &hh);
   ef->fx = ef->bat->conf->x * (ww - ef->bat->conf->width);
   ef->fy = ef->bat->conf->y * (hh - ef->bat->conf->width);
      
   evas_event_freeze(ef->evas);
   o = edje_object_add(ef->evas);
   ef->bat_object = o;

   edje_object_file_set(o,
			/* FIXME: "default.eet" needs to come from conf */
			e_path_find(path_themes, "default.eet"),
			"modules/battery/main");
   evas_object_show(o);
   
   o = evas_object_rectangle_add(ef->evas);
   ef->event_object = o;
   evas_object_layer_set(o, 2);
   evas_object_repeat_events_set(o, 1);
   evas_object_color_set(o, 0, 0, 0, 0);

   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _battery_cb_face_down, ef);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP, _battery_cb_face_up, ef);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE, _battery_cb_face_move, ef);
   evas_object_show(o);
   
   edje_object_size_min_calc(ef->bat_object, &bw, &bh);
   ef->minsize = bh;
   ef->minsize = bw;

   _battery_face_reconfigure(ef);
   
   evas_event_thaw(ef->evas);
}

static void
_battery_face_free(Battery_Face *ef)
{
   ecore_event_handler_del(ef->ev_handler_container_resize);
   evas_object_del(ef->bat_object);
   evas_object_del(ef->event_object);
   free(ef);
}

static void
_battery_face_reconfigure(Battery_Face *ef)
{
   Evas_Coord minw, minh, maxw, maxh, ww, hh;

   edje_object_size_min_calc(ef->bat_object, &minw, &maxh);
   edje_object_size_max_get(ef->bat_object, &maxw, &minh);
   evas_output_viewport_get(ef->evas, NULL, NULL, &ww, &hh);
   ef->fx = ef->bat->conf->x * (ww - ef->bat->conf->width);
   ef->fy = ef->bat->conf->y * (hh - ef->bat->conf->width);
   ef->fw = ef->bat->conf->width;
   ef->minsize = minw;
   ef->maxsize = maxw;

   evas_object_move(ef->bat_object, ef->fx, ef->fy);
   evas_object_resize(ef->bat_object, ef->bat->conf->width, ef->bat->conf->width);
   evas_object_move(ef->event_object, ef->fx, ef->fy);
   evas_object_resize(ef->event_object, ef->bat->conf->width, ef->bat->conf->width);
}

static void
_battery_cb_face_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   Battery_Face *ef;
   
   ev = event_info;
   ef = data;
   if (ev->button == 3)
     {
	e_menu_activate_mouse(ef->bat->config_menu, ef->con,
			      ev->output.x, ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN);
	e_util_container_fake_mouse_up_all_later(ef->con);
     }
   else if (ev->button == 2)
     {
	ef->resize = 1;
     }
   else if (ev->button == 1)
     {
	ef->move = 1;
     }
   evas_pointer_canvas_xy_get(e, &ef->xx, &ef->yy);
}

static void
_battery_cb_face_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   Battery_Face *ef;
   Evas_Coord ww, hh;
   
   ev = event_info;
   ef = data;
   ef->move = 0;
   ef->resize = 0;
   evas_output_viewport_get(ef->evas, NULL, NULL, &ww, &hh);
   ef->bat->conf->width = ef->fw;
   ef->bat->conf->x = (double)ef->fx / (double)(ww - ef->bat->conf->width);
   ef->bat->conf->y = (double)ef->fy / (double)(hh - ef->bat->conf->width);
   e_config_save_queue();
}

static void
_battery_cb_face_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{ 
   Evas_Event_Mouse_Move *ev;
   Battery_Face *ef;
   Evas_Coord cx, cy, sw, sh;
   
   evas_pointer_canvas_xy_get(e, &cx, &cy);
   evas_output_viewport_get(e, NULL, NULL, &sw, &sh);

   ev = event_info;
   ef = data;
   if (ef->move)
     {
	ef->fx += cx - ef->xx;
	ef->fy += cy - ef->yy;
	if (ef->fx < 0) ef->fx = 0;
	if (ef->fy < 0) ef->fy = 0;
	if (ef->fx + ef->fw > sw) ef->fx = sw - ef->fw;
	if (ef->fy + ef->fw > sh) ef->fy = sh - ef->fw;
	evas_object_move(ef->bat_object, ef->fx, ef->fy);
	evas_object_move(ef->event_object, ef->fx, ef->fy);
     }
   else if (ef->resize)
     {
	Evas_Coord d;

	d = cx - ef->xx;
	ef->fw += d;
	if (ef->fw < ef->minsize) ef->fw = ef->minsize;
	if (ef->fw > ef->maxsize) ef->fw = ef->maxsize;
	if (ef->fx + ef->fw > sw) ef->fw = sw - ef->fx;
	if (ef->fy + ef->fw > sh) ef->fw = sh - ef->fy;
	evas_object_resize(ef->bat_object, ef->fw, ef->fw);
	evas_object_resize(ef->event_object, ef->fw, ef->fw);
   }
   ef->xx = ev->cur.canvas.x;
   ef->yy = ev->cur.canvas.y;
}  

static int
_battery_cb_event_container_resize(void *data, int type, void *event)
{
   Battery_Face *ef;
   
   ef = data;
   _battery_face_reconfigure(ef);
   return 1;
}
