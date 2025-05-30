// -*- c++ -*-
// Generated by gmmproc 2.45.3 -- DO NOT MODIFY!
#ifndef _GDKMM_GC_H
#define _GDKMM_GC_H


#include <glibmm/ustring.h>
#include <sigc++/sigc++.h>

/* $Id: gc.hg,v 1.8 2006/04/12 11:11:24 murrayc Exp $ */

/* gc.h
 *
 * Copyright (C) 1998-2002 The gtkmm Development Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <glibmm/object.h>
#include <ydkmm/region.h>
#include <ydkmm/drawable.h>
#include <ydkmm/types.h>
#include <ydkmm/rectangle.h>
#include <ydkmm/screen.h>


#ifndef DOXYGEN_SHOULD_SKIP_THIS
typedef struct _GdkGC GdkGC;
typedef struct _GdkGCClass GdkGCClass;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */


#ifndef DOXYGEN_SHOULD_SKIP_THIS
namespace Gdk
{ class GC_Class; } // namespace Gdk
#endif //DOXYGEN_SHOULD_SKIP_THIS

namespace Gdk
{

/** @addtogroup gdkmmEnums gdkmm Enums and Flags */

/** 
 *
 * @ingroup gdkmmEnums
 */
enum LineStyle
{
  LINE_SOLID,
  LINE_ON_OFF_DASH,
  LINE_DOUBLE_DASH
};

} // namespace Gdk


#ifndef DOXYGEN_SHOULD_SKIP_THIS
namespace Glib
{

template <>
class Value<Gdk::LineStyle> : public Glib::Value_Enum<Gdk::LineStyle>
{
public:
  static GType value_type() G_GNUC_CONST;
};

} // namespace Glib
#endif /* DOXYGEN_SHOULD_SKIP_THIS */


namespace Gdk
{

/** 
 *
 * @ingroup gdkmmEnums
 */
enum CapStyle
{
  CAP_NOT_LAST,
  CAP_BUTT,
  CAP_ROUND,
  CAP_PROJECTING
};

} // namespace Gdk


#ifndef DOXYGEN_SHOULD_SKIP_THIS
namespace Glib
{

template <>
class Value<Gdk::CapStyle> : public Glib::Value_Enum<Gdk::CapStyle>
{
public:
  static GType value_type() G_GNUC_CONST;
};

} // namespace Glib
#endif /* DOXYGEN_SHOULD_SKIP_THIS */


namespace Gdk
{

/** 
 *
 * @ingroup gdkmmEnums
 */
enum JoinStyle
{
  JOIN_MITER,
  JOIN_ROUND,
  JOIN_BEVEL
};

} // namespace Gdk


#ifndef DOXYGEN_SHOULD_SKIP_THIS
namespace Glib
{

template <>
class Value<Gdk::JoinStyle> : public Glib::Value_Enum<Gdk::JoinStyle>
{
public:
  static GType value_type() G_GNUC_CONST;
};

} // namespace Glib
#endif /* DOXYGEN_SHOULD_SKIP_THIS */


namespace Gdk
{

/** 
 *
 * @ingroup gdkmmEnums
 */
enum Fill
{
  SOLID,
  TILED,
  STIPPLED,
  OPAQUE_STIPPLED
};

} // namespace Gdk


#ifndef DOXYGEN_SHOULD_SKIP_THIS
namespace Glib
{

template <>
class Value<Gdk::Fill> : public Glib::Value_Enum<Gdk::Fill>
{
public:
  static GType value_type() G_GNUC_CONST;
};

} // namespace Glib
#endif /* DOXYGEN_SHOULD_SKIP_THIS */


namespace Gdk
{

/** 
 *
 * @ingroup gdkmmEnums
 */
enum Function
{
  COPY,
  INVERT,
  XOR,
  CLEAR,
  AND,
  AND_REVERSE,
  AND_INVERT,
  NOOP,
  OR,
  EQUIV,
  OR_REVERSE,
  COPY_INVERT,
  OR_INVERT,
  NAND,
  NOR,
  SET
};

} // namespace Gdk


#ifndef DOXYGEN_SHOULD_SKIP_THIS
namespace Glib
{

template <>
class Value<Gdk::Function> : public Glib::Value_Enum<Gdk::Function>
{
public:
  static GType value_type() G_GNUC_CONST;
};

} // namespace Glib
#endif /* DOXYGEN_SHOULD_SKIP_THIS */


namespace Gdk
{

/** 
 *
 * @ingroup gdkmmEnums
 */
enum SubwindowMode
{
  CLIP_BY_CHILDREN,
  INCLUDE_INFERIORS
};

} // namespace Gdk


#ifndef DOXYGEN_SHOULD_SKIP_THIS
namespace Glib
{

template <>
class Value<Gdk::SubwindowMode> : public Glib::Value_Enum<Gdk::SubwindowMode>
{
public:
  static GType value_type() G_GNUC_CONST;
};

} // namespace Glib
#endif /* DOXYGEN_SHOULD_SKIP_THIS */


namespace Gdk
{

/** 
 *
 * @ingroup gdkmmEnums
 * @par Bitwise operators:
 * <tt>%GCValuesMask operator|(GCValuesMask, GCValuesMask)</tt><br>
 * <tt>%GCValuesMask operator&(GCValuesMask, GCValuesMask)</tt><br>
 * <tt>%GCValuesMask operator^(GCValuesMask, GCValuesMask)</tt><br>
 * <tt>%GCValuesMask operator~(GCValuesMask)</tt><br>
 * <tt>%GCValuesMask& operator|=(GCValuesMask&, GCValuesMask)</tt><br>
 * <tt>%GCValuesMask& operator&=(GCValuesMask&, GCValuesMask)</tt><br>
 * <tt>%GCValuesMask& operator^=(GCValuesMask&, GCValuesMask)</tt><br>
 */
enum GCValuesMask
{
  GC_FOREGROUND = 1 << 0,
  GC_BACKGROUND = 1 << 1,
  GC_FONT = 1 << 2,
  GC_FUNCTION = 1 << 3,
  GC_FILL = 1 << 4,
  GC_TILE = 1 << 5,
  GC_STIPPLE = 1 << 6,
  GC_CLIP_MASK = 1 << 7,
  GC_SUBWINDOW = 1 << 8,
  GC_TS_X_ORIGIN = 1 << 9,
  GC_TS_Y_ORIGIN = 1 << 10,
  GC_CLIP_X_ORIGIN = 1 << 11,
  GC_CLIP_Y_ORIGIN = 1 << 12,
  GC_EXPOSURES = 1 << 13,
  GC_LINE_WIDTH = 1 << 14,
  GC_LINE_STYLE = 1 << 15,
  GC_CAP_STYLE = 1 << 16,
  GC_JOIN_STYLE = 1 << 17
};

/** @ingroup gdkmmEnums */
inline GCValuesMask operator|(GCValuesMask lhs, GCValuesMask rhs)
  { return static_cast<GCValuesMask>(static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs)); }

/** @ingroup gdkmmEnums */
inline GCValuesMask operator&(GCValuesMask lhs, GCValuesMask rhs)
  { return static_cast<GCValuesMask>(static_cast<unsigned>(lhs) & static_cast<unsigned>(rhs)); }

/** @ingroup gdkmmEnums */
inline GCValuesMask operator^(GCValuesMask lhs, GCValuesMask rhs)
  { return static_cast<GCValuesMask>(static_cast<unsigned>(lhs) ^ static_cast<unsigned>(rhs)); }

/** @ingroup gdkmmEnums */
inline GCValuesMask operator~(GCValuesMask flags)
  { return static_cast<GCValuesMask>(~static_cast<unsigned>(flags)); }

/** @ingroup gdkmmEnums */
inline GCValuesMask& operator|=(GCValuesMask& lhs, GCValuesMask rhs)
  { return (lhs = static_cast<GCValuesMask>(static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs))); }

/** @ingroup gdkmmEnums */
inline GCValuesMask& operator&=(GCValuesMask& lhs, GCValuesMask rhs)
  { return (lhs = static_cast<GCValuesMask>(static_cast<unsigned>(lhs) & static_cast<unsigned>(rhs))); }

/** @ingroup gdkmmEnums */
inline GCValuesMask& operator^=(GCValuesMask& lhs, GCValuesMask rhs)
  { return (lhs = static_cast<GCValuesMask>(static_cast<unsigned>(lhs) ^ static_cast<unsigned>(rhs))); }

} // namespace Gdk


#ifndef DOXYGEN_SHOULD_SKIP_THIS
namespace Glib
{

template <>
class Value<Gdk::GCValuesMask> : public Glib::Value_Flags<Gdk::GCValuesMask>
{
public:
  static GType value_type() G_GNUC_CONST;
};

} // namespace Glib
#endif /* DOXYGEN_SHOULD_SKIP_THIS */


namespace Gdk
{


class Drawable;

/** All drawing operations in GDK take a graphics context (GC) argument.
 * A graphics context encapsulates information about the way things are drawn, such as the foreground color or line width.
 * By using graphics contexts, the number of arguments to each drawing call is greatly reduced, and communication overhead
 * is minimized, since identical arguments do not need to be passed repeatedly.
 */

class GC : public Glib::Object
{
  
#ifndef DOXYGEN_SHOULD_SKIP_THIS

public:
  typedef GC CppObjectType;
  typedef GC_Class CppClassType;
  typedef GdkGC BaseObjectType;
  typedef GdkGCClass BaseClassType;

private:  friend class GC_Class;
  static CppClassType gc_class_;

private:
  // noncopyable
  GC(const GC&);
  GC& operator=(const GC&);

protected:
  explicit GC(const Glib::ConstructParams& construct_params);
  explicit GC(GdkGC* castitem);

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

public:
  virtual ~GC();

  /** Get the GType for this class, for use with the underlying GObject type system.
   */
  static GType get_type()      G_GNUC_CONST;

#ifndef DOXYGEN_SHOULD_SKIP_THIS


  static GType get_base_type() G_GNUC_CONST;
#endif

  ///Provides access to the underlying C GObject.
  GdkGC*       gobj()       { return reinterpret_cast<GdkGC*>(gobject_); }

  ///Provides access to the underlying C GObject.
  const GdkGC* gobj() const { return reinterpret_cast<GdkGC*>(gobject_); }

  ///Provides access to the underlying C instance. The caller is responsible for unrefing it. Use when directly setting fields in structs.
  GdkGC* gobj_copy();

private:

  
protected:
  GC();
  explicit GC(const Glib::RefPtr<Drawable>& drawable);

public:
  
  static Glib::RefPtr<GC> create();

  
  static Glib::RefPtr<GC> create(const Glib::RefPtr<Drawable>& drawable);


  // Custom wrap these?, maybe with a Gdk::GC::Value class. Need to do take care of refcounts of GdkGCValue's fields.
  // Actually, I don't see the need for these methods. murrayc.
  //_WRAP_METHOD(void get_values(GdkGCValues& values) const, gdk_gc_get_values)
  //_WRAP_METHOD(void set_values(const GdkGCValues& values, GCValuesMask values_mask), gdk_gc_set_values)
  //__IGNORE(gdk_gc_get_values, gdk_gc_set_values)

  
  /** Sets the foreground color for a graphics context.
   * Note that this function uses @a color->pixel, use 
   * set_rgb_fg_color() to specify the foreground 
   * color as red, green, blue components.
   * 
   * Deprecated: 2.22: Use gdk_cairo_set_source_color() to use a Gdk::Color
   * as the source in Cairo.
   * 
   * @param color The new foreground color.
   */
  void set_foreground(const Color& color);
  
  /** Sets the background color for a graphics context.
   * Note that this function uses @a color->pixel, use 
   * set_rgb_bg_color() to specify the background 
   * color as red, green, blue components.
   * 
   * Deprecated: 2.22: Use gdk_cairo_set_source_color() to use a Gdk::Color
   * as the source in Cairo. Note that if you want to draw a background and a
   * foreground in Cairo, you need to call drawing functions (like cairo_fill())
   * twice.
   * 
   * @param color The new background color.
   */
  void set_background(const Color& color);
  
  /** Determines how the current pixel values and the
   * pixel values being drawn are combined to produce
   * the final pixel values.
   * 
   * Deprecated: 2.22: Use cairo_set_operator() with Cairo.
   * 
   * @param function The Gdk::Function to use.
   */
  void set_function(Function function);
  
  /** Set the fill mode for a graphics context.
   * 
   * Deprecated: 2.22: You can achieve tiling in Cairo by using
   * cairo_pattern_set_extend() on the source. For stippling, see the
   * deprecation comments on set_stipple().
   * 
   * @param fill The new fill mode.
   */
  void set_fill(Fill fill);
  
  /** Set a tile pixmap for a graphics context.
   * This will only be used if the fill mode
   * is Gdk::TILED.
   * 
   * Deprecated: 2.22: The following code snippet sets a tiling Gdk::Pixmap
   * as the source in Cairo:
   * 
   * [C example ellipted]
   * 
   * @param tile The new tile pixmap.
   */
  void set_tile(const Glib::RefPtr<Pixmap>& tile);
  
  /** Set the stipple bitmap for a graphics context. The
   * stipple will only be used if the fill mode is
   * Gdk::STIPPLED or Gdk::OPAQUE_STIPPLED.
   * 
   * Deprecated: 2.22: Stippling has no direct replacement in Cairo. If you
   * want to achieve an identical look, you can use the stipple bitmap as a
   * mask. Most likely, this involves rendering the source to an intermediate
   * surface using cairo_push_group() first, so that you can then use
   * cairo_mask() to achieve the stippled look.
   * 
   * @param stipple The new stipple bitmap.
   */
  void set_stipple(const Glib::RefPtr<Pixmap>& stipple);
  
  /** Set the origin when using tiles or stipples with
   * the GC. The tile or stipple will be aligned such
   * that the upper left corner of the tile or stipple
   * will coincide with this point.
   * 
   * Deprecated: 2.22: You can set the origin for tiles and stipples in Cairo
   * by changing the source's matrix using cairo_pattern_set_matrix(). Or you
   * can specify it with gdk_cairo_set_source_pixmap() as shown in the example
   * for set_tile().
   * 
   * @param x The x-coordinate of the origin.
   * @param y The y-coordinate of the origin.
   */
  void set_ts_origin(int x, int y);
  
  /** Sets the origin of the clip mask. The coordinates are
   * interpreted relative to the upper-left corner of
   * the destination drawable of the current operation.
   * 
   * Deprecated: 2.22: Use cairo_translate() before applying the clip path in
   * Cairo.
   * 
   * @param x The x-coordinate of the origin.
   * @param y The y-coordinate of the origin.
   */
  void set_clip_origin(int x, int y);
  
  /** Sets the clip mask for a graphics context from a bitmap.
   * The clip mask is interpreted relative to the clip
   * origin. (See set_clip_origin()).
   * 
   * Deprecated: 2.22: Use cairo_mask() instead.
   * 
   * @param mask A bitmap.
   */
  void set_clip_mask(const Glib::RefPtr<Bitmap>& mask);

//Note: The rectangle parameter was made const in GTK+ 2.16, but we can't change that in our C++ API. TODO: Change it when we can do an ABI break.
 

#ifndef GDKMM_DISABLE_DEPRECATED

  /** Sets the clip mask for a graphics context from a
   * rectangle. The clip mask is interpreted relative to the clip
   * origin. (See set_clip_origin()).
   * 
   * Deprecated: 2.22: Use cairo_rectangle() and cairo_clip() in Cairo.
   * 
   * @deprecated Use set_clip_rectangle(const Rectangle& rectangle) instead.
   * 
   * @param rectangle The rectangle to clip to.
   */
  void set_clip_rectangle(Rectangle& rectangle);
#endif // GDKMM_DISABLE_DEPRECATED


  /** Sets the clip mask for a graphics context from a
   * rectangle. The clip mask is interpreted relative to the clip
   * origin. (See set_clip_origin()).
   * 
   * Deprecated: 2.22: Use cairo_rectangle() and cairo_clip() in Cairo.
   * 
   * @param rectangle The rectangle to clip to.
   */
  void set_clip_rectangle(const Rectangle& rectangle);

  
  /** Sets the clip mask for a graphics context from a region structure.
   * The clip mask is interpreted relative to the clip origin. (See
   * set_clip_origin()).
   * 
   * Deprecated: 2.22: Use gdk_cairo_region() and cairo_clip() in Cairo.
   * 
   * @param region The Gdk::Region.
   */
  void set_clip_region(const Region& region);
  
  /** Sets how drawing with this GC on a window will affect child
   * windows of that window. 
   * 
   * Deprecated: 2.22: There is no replacement. If you need to control
   * subwindows, you must use drawing operations of the underlying window
   * system manually. Cairo will always use Gdk::INCLUDE_INFERIORS on sources
   * and masks and Gdk::CLIP_BY_CHILDREN on targets.
   * 
   * @param mode The subwindow mode.
   */
  void set_subwindow(SubwindowMode mode);
  
  /** Sets whether copying non-visible portions of a drawable
   * using this graphics context generate exposure events
   * for the corresponding regions of the destination
   * drawable. (See gdk_draw_drawable()).
   * 
   * Deprecated: 2.22: There is no replacement. If you need to control
   * exposures, you must use drawing operations of the underlying window
   * system or use Gdk::Window::invalidate_rect(). Cairo will never
   * generate exposures.
   * 
   * @param exposures If <tt>true</tt>, exposure events will be generated.
   */
  void set_exposures(bool exposures);
  
  /** Sets various attributes of how lines are drawn. See
   * the corresponding members of Gdk::GCValues for full
   * explanations of the arguments.
   * 
   * Deprecated: 2.22: Use the Cairo functions cairo_set_line_width(),
   * cairo_set_line_join(), cairo_set_line_cap() and cairo_set_dash()
   * to affect the stroking behavior in Cairo. Keep in mind that the default
   * attributes of a <tt>cairo_t</tt> are different from the default attributes of
   * a Gdk::GC.
   * 
   * @param line_width The width of lines.
   * @param line_style The dash-style for lines.
   * @param cap_style The manner in which the ends of lines are drawn.
   * @param join_style The in which lines are joined together.
   */
  void set_line_attributes(int line_width, LineStyle line_style, CapStyle cap_style, JoinStyle join_style);
  
  /** Sets the way dashed-lines are drawn. Lines will be
   * drawn with alternating on and off segments of the
   * lengths specified in @a dash_list. The manner in
   * which the on and off segments are drawn is determined
   * by the @a line_style value of the GC. (This can
   * be changed with set_line_attributes().)
   * 
   * The @a dash_offset defines the phase of the pattern, 
   * specifying how many pixels into the dash-list the pattern 
   * should actually begin.
   * 
   * Deprecated: 2.22: Use cairo_set_dash() to set the dash in Cairo.
   * 
   * @param dash_offset The phase of the dash pattern.
   * @param dash_list An array of dash lengths.
   * @param n The number of elements in @a dash_list.
   */
  void set_dashes(int dash_offset, gint8* dash_list, int n);
  
  /** Offset attributes such as the clip and tile-stipple origins
   * of the GC so that drawing at x - x_offset, y - y_offset with
   * the offset GC  has the same effect as drawing at x, y with the original
   * GC.
   * 
   * Deprecated: 2.22: There is no direct replacement, as this is just a
   * convenience function for gdk_gc_set_ts_origin and set_clip_origin().
   * 
   * @param x_offset Amount by which to offset the GC in the X direction.
   * @param y_offset Amount by which to offset the GC in the Y direction.
   */
  void offset(int x_offset, int y_offset);
  
  /** Sets the colormap for the GC to the given colormap. The depth
   * of the colormap's visual must match the depth of the drawable
   * for which the GC was created.
   * 
   * Deprecated: 2.22: There is no replacement. Cairo handles colormaps
   * automatically, so there is no need to care about them.
   * 
   * @param colormap A Gdk::Colormap.
   */
  void set_colormap(const Glib::RefPtr<Colormap>& colormap);
  
  /** Retrieves the colormap for a given GC, if it exists.
   * A GC will have a colormap if the drawable for which it was created
   * has a colormap, or if a colormap was set explicitely with
   * gdk_gc_set_colormap.
   * 
   * Deprecated: 2.22: There is no replacement. Cairo handles colormaps
   * automatically, so there is no need to care about them.
   * 
   * @return The colormap of @a gc, or <tt>0</tt> if @a gc doesn't have one.
   */
  Glib::RefPtr<Colormap> get_colormap();
  
  /** Set the foreground color of a GC using an unallocated color. The
   * pixel value for the color will be determined using GdkRGB. If the
   * colormap for the GC has not previously been initialized for GdkRGB,
   * then for pseudo-color colormaps (colormaps with a small modifiable
   * number of colors), a colorcube will be allocated in the colormap.
   * 
   * Calling this function for a GC without a colormap is an error.
   * 
   * Deprecated: 2.22: Use gdk_cairo_set_source_color() instead.
   * 
   * @param color An unallocated Gdk::Color.
   */
  void set_rgb_fg_color(const Color& color);
  
  /** Set the background color of a GC using an unallocated color. The
   * pixel value for the color will be determined using GdkRGB. If the
   * colormap for the GC has not previously been initialized for GdkRGB,
   * then for pseudo-color colormaps (colormaps with a small modifiable
   * number of colors), a colorcube will be allocated in the colormap.
   * 
   * Calling this function for a GC without a colormap is an error.
   * 
   * Deprecated: 2.22: Use gdk_cairo_set_source_color() instead.
   * 
   * @param color An unallocated Gdk::Color.
   */
  void set_rgb_bg_color(const Color& color);

  
  /** Gets the Gdk::Screen for which @a gc was created
   * 
   * @return The Gdk::Screen for @a gc.
   */
  Glib::RefPtr<Screen> get_screen();
  
  /** Gets the Gdk::Screen for which @a gc was created
   * 
   * @return The Gdk::Screen for @a gc.
   */
  Glib::RefPtr<const Screen> get_screen() const;


public:

public:
  //C++ methods used to invoke GTK+ virtual functions:

protected:
  //GTK+ Virtual Functions (override these to change behaviour):

  //Default Signal Handlers::


};

} /* namespace Gdk */


namespace Glib
{
  /** A Glib::wrap() method for this object.
   * 
   * @param object The C instance.
   * @param take_copy False if the result should take ownership of the C instance. True if it should take a new copy or ref.
   * @result A C++ instance that wraps this C instance.
   *
   * @relates Gdk::GC
   */
  Glib::RefPtr<Gdk::GC> wrap(GdkGC* object, bool take_copy = false);
}


#endif /* _GDKMM_GC_H */

