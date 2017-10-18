#include <stdio.h>
#include <math.h>
#include <string>
#include <list>

#include <fstream>
#include <iostream>

#include <glib.h>
#include <glib/poppler.h>
#include <cairo.h>

#include "../lib/base64.h"
#define JSON_NOEXCEPTION 1
#include "../lib/json.hpp"

using nlohmann::json;

using std::string;
using std::list;

#define return_void_if_not_markup(annot) if (!POPPLER_IS_ANNOT_MARKUP(annot)) return

enum AnnotationAction {
	ANNOTATION_ACTION_UNSUPPORTED,

	ANNOTATION_ACTION_COMMENTING,
	ANNOTATION_ACTION_HIGHLIGHTING,
	ANNOTATION_ACTION_STAMPING
};

struct OacAnnot {
	AnnotationAction action;

	int page;
	string target_rect;
	string target_text;

	string body_text;
	string body_image;
	string body_creator;
	string body_subject;
	string body_color;

};

struct png_stream_closure_t {
	string bytes;
};

cairo_status_t
write_png_stream_to_string(void *in_closure, const unsigned char *data, unsigned int length) {
	png_stream_closure_t *closure = (png_stream_closure_t *)in_closure;

	closure->bytes.append(data, data + length);

	return CAIRO_STATUS_SUCCESS;
}

string
annotation_action_get_motivation(AnnotationAction action) {
	switch(action) {
	case ANNOTATION_ACTION_COMMENTING:
		return "commenting";

	case ANNOTATION_ACTION_HIGHLIGHTING:
		return "highlighting";

	case ANNOTATION_ACTION_STAMPING:
		return "bookmarking";

	default:
		return "";
	}
}

void
oac_annot_set_annotation_action(OacAnnot *oac_annot, PopplerAnnot *annot) {
	AnnotationAction action;

	switch(poppler_annot_get_annot_type(annot)) {
	case PopplerAnnotType::POPPLER_ANNOT_TEXT: {
		action = ANNOTATION_ACTION_COMMENTING;
		break;
	}
	case PopplerAnnotType::POPPLER_ANNOT_HIGHLIGHT:
	case PopplerAnnotType::POPPLER_ANNOT_UNDERLINE: {
		action = ANNOTATION_ACTION_HIGHLIGHTING;
		break;
	}
	case PopplerAnnotType::POPPLER_ANNOT_STAMP: {
		action = ANNOTATION_ACTION_STAMPING;
		break;
	}
	default:
		action = ANNOTATION_ACTION_UNSUPPORTED;
		break;
	}

	oac_annot->action = action;
	return;
}

void
oac_annot_set_body_subject(OacAnnot *oac_annot, PopplerAnnot *annot) {
	char *subject;

	return_void_if_not_markup(annot);

	subject = poppler_annot_markup_get_subject(POPPLER_ANNOT_MARKUP(annot));

	if (subject != NULL)
		oac_annot->body_subject += subject;

	return;
}

void
oac_annot_set_body_creator(OacAnnot *oac_annot, PopplerAnnot *annot) {
	string creator_text;
	gchar *creator;

	return_void_if_not_markup(annot);

	creator = poppler_annot_markup_get_label(POPPLER_ANNOT_MARKUP(annot));

	if (creator != NULL)
		creator_text.assign(creator);

	oac_annot->body_creator = creator_text;

	return;
}

void
oac_annot_set_body_color(OacAnnot *oac_annot, PopplerAnnot *annot) {
	PopplerColor *color;
	char css_color[8];

	color = poppler_annot_get_color(annot);

	if (color == NULL)
		return;

	snprintf(css_color, sizeof(css_color),
		 "#%02x%02x%02x", color->red >> 8, color->green >> 8, color->blue >> 8);

	g_free(color);

	oac_annot->body_color += css_color;

	return;
}

void
oac_annot_set_body_text(OacAnnot *oac_annot, PopplerAnnot *annot) {
	string body_text;
	gchar *annot_contents;

	annot_contents = poppler_annot_get_contents(annot);

	if (annot_contents)
		body_text.assign(annot_contents);

	g_free(annot_contents);

	oac_annot->body_text = body_text;

	return;
}

void
oac_annot_set_body_image(OacAnnot *oac_annot, PopplerPage *page, PopplerAnnot *annot, PopplerRectangle *rect) {
	cairo_t *cairo;
	cairo_surface_t *surface;
	cairo_status_t status;
	double scale, page_height;

	if (oac_annot->action != ANNOTATION_ACTION_STAMPING)
		return;

	scale = 4.17;

	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
					     (rect->x2 - rect->x1) * scale,
					     (rect->y2 - rect->y1) * scale);

	cairo = cairo_create(surface);
	cairo_scale(cairo, scale, scale);
	poppler_page_get_size(page, NULL, &page_height);
	cairo_translate(cairo, (-rect->x1) / 2, (rect->y2 - page_height) / 2);

	poppler_page_render_annot(page, annot, cairo);

	cairo_destroy(cairo);

	cairo_surface_flush(surface);

	png_stream_closure_t closure;
	status = cairo_surface_write_to_png_stream(surface, write_png_stream_to_string, &closure);

	if (status != CAIRO_STATUS_SUCCESS) {
		fprintf(stderr, "%s\n", cairo_status_to_string(status));
		return;
	}

	Base64::Encode(closure.bytes, &oac_annot->body_image);

	cairo_surface_finish(surface);
	cairo_surface_destroy(surface);

	return;
}

void
oac_annot_set_target_text(OacAnnot *oac_annot, PopplerAnnot *annot, PopplerPage *page) {
	std::string text;
	char *rect_text;
	GArray *quads;
	PopplerQuadrilateral quad;
	PopplerRectangle rect;
	double page_height, y1, y2;

	if (oac_annot->action != ANNOTATION_ACTION_HIGHLIGHTING)
		return;

	poppler_page_get_size(page, NULL, &page_height);
	quads = poppler_annot_text_markup_get_quadrilaterals(POPPLER_ANNOT_TEXT_MARKUP(annot));

	for (unsigned int i = 0; i < quads->len; i++) {
		quad = g_array_index(quads, PopplerQuadrilateral, i);

		y1 = page_height - quad.p1.y;
		y2 = page_height - quad.p3.y;

		// Make the rectangle to through the middle of the rectangle so
		// that it doesn't need to be exactly right. Using the whole
		// rectangle kept overlapping with lines above/below
		rect.x1 = quad.p1.x;
		rect.y1 = (y2 + y1) / 2 - 1;
		rect.x2 = quad.p2.x;
		rect.y2 = (y2 + y1) / 2 + 1;

		rect_text = poppler_page_get_text_for_area(page, &rect);

		if (text.length() > 0)
			text += " ";

		text += rect_text;
	}

	oac_annot->target_text = text;

	g_array_free(quads, TRUE);

	return;
}

string
oac_annot_to_json(OacAnnot *oacAnnot) {
	json out;

	out["page"] = oacAnnot->page;
	out["motivation"] = annotation_action_get_motivation(oacAnnot->action);
	out["target_rect"] = oacAnnot->target_rect;

	if (oacAnnot->body_text.length() > 0)
		out["body_text"] = oacAnnot->body_text;

	if (oacAnnot->body_image.length() > 0)
		out["stamp_body"] = oacAnnot->body_image;

	if (oacAnnot->body_subject.length() > 0)
		out["body_subject"] = oacAnnot->body_subject;

	if (oacAnnot->body_creator.length() > 0)
		out["body_creator"] = oacAnnot->body_creator;

	if (oacAnnot->body_color.length() > 0)
		out["body_color"] = oacAnnot->body_color;

	if (oacAnnot->target_text.length() > 0)
		out["target_text"] = oacAnnot->target_text;

	return out.dump();
}

string
format_coord(double p) {
	return std::to_string((int)(p));
}

void
oac_annot_from_mapping(OacAnnot *oac_annot, PopplerAnnotMapping *m, PopplerPage *p) {
	oac_annot_set_annotation_action(oac_annot, m->annot);

	if (oac_annot->action == ANNOTATION_ACTION_UNSUPPORTED)
		return;

	oac_annot->page = poppler_page_get_index(p) + 1;
	oac_annot->target_rect = format_coord(m->area.x1) + ","
			       + format_coord(m->area.y1) + ","
			       + format_coord(m->area.x2) + ","
			       + format_coord(m->area.y2);

	oac_annot_set_body_text(oac_annot, m->annot);
	oac_annot_set_body_image(oac_annot, p, m->annot, &m->area);
	oac_annot_set_body_subject(oac_annot, m->annot);
	oac_annot_set_body_color(oac_annot, m->annot);
	oac_annot_set_body_creator(oac_annot, m->annot);
	oac_annot_set_target_text(oac_annot, m->annot, p);

	return;
}

list<OacAnnot>
poppler_document_get_oac_annots(PopplerDocument *doc, int page_number) {
	list<OacAnnot> oac_annots;
	PopplerPage *page;
	GList *annots, *l;
	PopplerAnnotMapping *m;

	page = poppler_document_get_page(doc, page_number);

	annots = poppler_page_get_annot_mapping(page);

	for (l = annots; l != NULL; l = l->next) {
		OacAnnot oac_annot;
		m = (PopplerAnnotMapping *) l->data;
		oac_annot_from_mapping(&oac_annot, m, page);

		if (oac_annot.action != ANNOTATION_ACTION_UNSUPPORTED)
			oac_annots.push_back(oac_annot);
	}

	poppler_page_free_annot_mapping(annots);

	return oac_annots;
}
