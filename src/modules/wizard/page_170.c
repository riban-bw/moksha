/* Ask about updates checking */
#include "e.h"
#include "e_mod_main.h"

static int do_up = 1;

EAPI int
wizard_page_init(E_Wizard_Page *pg __UNUSED__)
{
   return 1;
}

EAPI int
wizard_page_shutdown(E_Wizard_Page *pg __UNUSED__)
{
   return 1;
}

EAPI int
wizard_page_show(E_Wizard_Page *pg)
{
   Evas_Object *o, *of, *ob;

   o = e_widget_list_add(pg->evas, 1, 0);
   e_wizard_title_set(_("Updates"));

   of = e_widget_framelist_add(pg->evas, _("Check for available updates"), 0);

   ob = e_widget_textblock_add(pg->evas);
   e_widget_size_min_set(ob, 200 * e_scale, 180 * e_scale);
   e_widget_textblock_markup_set
     (ob,
     _("Enlightenment can check for new<br>"
       "versions, updates, securiity and<br>"
       "bugfixes, as well as available add-ons.<br>"
       "<br>"
       "This is very useful, because it lets you<br>"
       "you know about available bug fixes and<br>"
       "security fixes when they happen. As a<br>"
       "bi-product of this, Enlightenment will<br>"
       "connect to enlightenment.org and transmit<br>"
       "some information as a result much like any<br>"
       "web browser might do. No personal information<br>"
       "such as username, password or any personal<br>"
       "files will be transmitted. If you do not like<br>"
       "this, please disable this below. It is highly<br>"
       "advised that you do not disable this as it<br>"
       "may leave you vulnerable or having to live<br>"
       "with bugs."
       )
     );
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_check_add(pg->evas, _("Enable update checking"), &(do_up));
   e_widget_framelist_object_append(of, ob);

   e_widget_list_object_append(o, of, 0, 0, 0.5);

   evas_object_show(of);

   e_wizard_page_show(o);
   return 1; /* 1 == show ui, and wait for user, 0 == just continue */
}

EAPI int
wizard_page_hide(E_Wizard_Page *pg __UNUSED__)
{
   e_config->update.check = do_up;
   e_config_save_queue();
   return 1;
}

EAPI int
wizard_page_apply(E_Wizard_Page *pg __UNUSED__)
{
   return 1;
}

