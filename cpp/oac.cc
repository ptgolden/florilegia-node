#include <stdio.h>
#include <math.h>
#include <string>
#include <list>

#include <fstream>
#include <iostream>

#include <glib.h>
#include <glib/poppler.h>
#include <cairo.h>

#include "base64.h"
#define JSON_NOEXCEPTION 1
#include "json.hpp"


using nlohmann::json;

using std::string;
using std::list;
using std::rewind;

enum AnnotationAction {
	ANNOTATION_ACTION_UNSUPPORTED,

	ANNOTATION_ACTION_COMMENTING,
	ANNOTATION_ACTION_HIGHLIGHTING,
	ANNOTATION_ACTION_STAMPING
};

struct OacAnnot {
	int page;
	AnnotationAction action;
	string body_text;
	string body_image;
	string target_text;
};

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

int i = 0;

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

	string filename;

	filename += "/tmp/png/annot-";
	filename += std::to_string(i);
	filename += ".png";

	// FIXME: Check status of (everything)

	status = cairo_surface_write_to_png(surface, filename.c_str());
	i += 1;

	if (status != CAIRO_STATUS_SUCCESS) {
		fprintf(stderr, "%s\n", cairo_status_to_string(status));
		return;
	}



	/*
	Base64::Encode(raw_png_from_cairo_surface(surface), &oac_annot->body_image);
	*/

	cairo_surface_finish(surface);
	cairo_surface_destroy(surface);

	return;
}

void
oac_annot_set_target_text(OacAnnot *oac_annot, PopplerAnnot *annot, PopplerPage *page) {

	if (oac_annot->action != ANNOTATION_ACTION_HIGHLIGHTING)
		return;

	/*
	string highlighted_text;
	char *annot_text;
	annot_text = poppler_page_get_text_for_annot(page, annot);

	highlighted_text += annot_text;

	oac_annot->target_text = highlighted_text;
	*/

	return;
}

string
oac_annot_to_json(OacAnnot *oacAnnot) {
	json out;

	out["page"] = oacAnnot->page;
	out["motivation"] = annotation_action_get_motivation(oacAnnot->action);

	if (oacAnnot->body_text.length() > 0)
		out["body_text"] = oacAnnot->body_text;

	if (oacAnnot->body_image.length() > 0)
		out["stamp_body"] = oacAnnot->body_image;

	if (oacAnnot->target_text.length() > 0)
		out["target_text"] = oacAnnot->target_text;

	return out.dump();
}

void
oac_annot_from_mapping(OacAnnot *oac_annot, PopplerAnnotMapping *m, PopplerPage *p) {
	oac_annot->page = poppler_page_get_index(p) + 1;
	oac_annot_set_annotation_action(oac_annot, m->annot);

	if (oac_annot->action == ANNOTATION_ACTION_UNSUPPORTED)
		return;

	oac_annot_set_body_text(oac_annot, m->annot);
	oac_annot_set_body_image(oac_annot, p, m->annot, &m->area);
	oac_annot_set_target_text(oac_annot, m->annot, p);

	return;
}

list<OacAnnot>
poppler_document_get_oac_annots(PopplerDocument *doc, int page_number) {
	list<OacAnnot> oac_annots;
	OacAnnot oac_annot;
	PopplerPage *page;
	GList *annots, *l;
	PopplerAnnotMapping *m;

	page = poppler_document_get_page(doc, page_number);

	annots = poppler_page_get_annot_mapping(page);

	for (l = annots; l != NULL; l = l->next) {
		m = (PopplerAnnotMapping *) l->data;
		oac_annot_from_mapping(&oac_annot, m, page);

		if (oac_annot.action != ANNOTATION_ACTION_UNSUPPORTED)
			oac_annots.push_back(oac_annot);
	}

	poppler_page_free_annot_mapping(annots);

	return oac_annots;
}
