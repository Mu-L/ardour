// -*- c++ -*-
// Generated by gmmproc 2.45.3 -- DO NOT MODIFY!
#ifndef _GTKMM_SCROLLBAR_P_H
#define _GTKMM_SCROLLBAR_P_H


#include <ytkmm/private/range_p.h>

#include <glibmm/class.h>

namespace Gtk
{

class Scrollbar_Class : public Glib::Class
{
public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
  typedef Scrollbar CppObjectType;
  typedef GtkScrollbar BaseObjectType;
  typedef GtkScrollbarClass BaseClassType;
  typedef Gtk::Range_Class CppClassParent;
  typedef GtkRangeClass BaseClassParent;

  friend class Scrollbar;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

  const Glib::Class& init();


  static void class_init_function(void* g_class, void* class_data);

  static Glib::ObjectBase* wrap_new(GObject*);

protected:

  //Callbacks (default signal handlers):
  //These will call the *_impl member methods, which will then call the existing default signal callbacks, if any.
  //You could prevent the original default signal handlers being called by overriding the *_impl method.

  //Callbacks (virtual functions):
};


} // namespace Gtk


#include <glibmm/class.h>

namespace Gtk
{

class VScrollbar_Class : public Glib::Class
{
public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
  typedef VScrollbar CppObjectType;
  typedef GtkVScrollbar BaseObjectType;
  typedef GtkVScrollbarClass BaseClassType;
  typedef Gtk::Scrollbar_Class CppClassParent;
  typedef GtkScrollbarClass BaseClassParent;

  friend class VScrollbar;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

  const Glib::Class& init();


  static void class_init_function(void* g_class, void* class_data);

  static Glib::ObjectBase* wrap_new(GObject*);

protected:

  //Callbacks (default signal handlers):
  //These will call the *_impl member methods, which will then call the existing default signal callbacks, if any.
  //You could prevent the original default signal handlers being called by overriding the *_impl method.

  //Callbacks (virtual functions):
};


} // namespace Gtk


#include <glibmm/class.h>

namespace Gtk
{

class HScrollbar_Class : public Glib::Class
{
public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
  typedef HScrollbar CppObjectType;
  typedef GtkHScrollbar BaseObjectType;
  typedef GtkHScrollbarClass BaseClassType;
  typedef Gtk::Scrollbar_Class CppClassParent;
  typedef GtkScrollbarClass BaseClassParent;

  friend class HScrollbar;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

  const Glib::Class& init();


  static void class_init_function(void* g_class, void* class_data);

  static Glib::ObjectBase* wrap_new(GObject*);

protected:

  //Callbacks (default signal handlers):
  //These will call the *_impl member methods, which will then call the existing default signal callbacks, if any.
  //You could prevent the original default signal handlers being called by overriding the *_impl method.

  //Callbacks (virtual functions):
};


} // namespace Gtk


#endif /* _GTKMM_SCROLLBAR_P_H */

