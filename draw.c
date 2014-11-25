
#include "batstr.h"
#include "math.h"

void _outline_rectangle( cairo_t* ctx, double startx, double starty, double endx, double endy ) {

    cairo_set_line_cap( ctx, CAIRO_LINE_CAP_SQUARE );

    cairo_set_line_width(ctx, 3.0);
    cairo_move_to(ctx, startx, starty);
    cairo_line_to(ctx, startx, endy);

    cairo_move_to(ctx, startx, endy);
    cairo_line_to(ctx, endx, endy);

    cairo_move_to(ctx, endx, endy);
    cairo_line_to(ctx, endx, starty);

    cairo_move_to(ctx, endx, starty);
    cairo_line_to(ctx, startx, starty);
    cairo_stroke(ctx);

    cairo_set_line_width(ctx, 7.0);
    cairo_move_to(ctx, startx + 3*(endx - startx) / 12.0, starty - 10 );
    cairo_line_to(ctx, startx + 9*(endx - startx) / 12.0, starty - 10 );
    cairo_stroke(ctx);
}

GdkPixbuf* draw_pixbuf( double how_full, double bound_x, double bound_y, gboolean adapter ) {
    const double padding = 10;
    const double outer_padding_x = 24;
    const double outer_padding_y = 36;


    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, bound_x + outer_padding_x, bound_y + outer_padding_y - 5.0);
    cairo_t* ctx = cairo_create( surface );

    cairo_translate( ctx, outer_padding_x / 2.0, outer_padding_y / 2.0 );

    cairo_set_source_rgba(ctx, 0,0,0, 1.0);
    cairo_rectangle( ctx, 3*bound_x/12.0, padding, 6*bound_x/12.0, bound_y-2*padding+3 );
    cairo_fill(ctx);

    double rect_height = (bound_y - padding - 3) * (how_full * 0.80);

    double x = how_full;
    double e = 2.718;
    double r = 13.111*((x+0.05)/pow(e,(4*(x+0.05))) - 0.0157);
    double g = 1.81597*(1-pow(e,(0.1 - x)));
    double b = 2.5*(1-pow(e,(0.6 - x)));

    if( x >= 0.99 && adapter ) {
        cairo_set_source_rgba(ctx,1,1,1,1);
    } else {
        cairo_set_source_rgba(ctx,r,g,b,1);
    }


    cairo_rectangle(ctx,
        bound_x/3.0,
        bound_y - rect_height - padding - 3, 
        bound_x/3.0, rect_height - 3 );

    cairo_fill(ctx);

    if( adapter ) {
        cairo_set_source_rgba(ctx, 1.0, 1.0, 0.5, 1.0);
    } else {
        cairo_set_source_rgba(ctx, 0.8, 0.8, 0.8, 1.0);
    }
    _outline_rectangle( ctx, 3*bound_x/12.0, padding, 9*bound_x/12.0, bound_y-padding+3 );


    cairo_destroy(ctx);
    GdkPixbuf* ret = gdk_pixbuf_get_from_surface( surface, 0, 0,
			            cairo_image_surface_get_width(surface),
			            cairo_image_surface_get_height(surface));
    cairo_surface_destroy(surface);

    return ret;
}
