#include <stdio.h>
#include <string>
#include <list>

#include <glib.h>
#include <glib/poppler.h>
#include <goo/gmem.h>
#include <goo/ImgWriter.h>
#include <goo/PNGWriter.h>
#include <cairo.h>

#define JSON_NOEXCEPTION 1
#include "json.hpp"

using nlohmann::json;

using std::string;
using std::list;

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

void
oac_annot_set_body_image(OacAnnot *oac_annot, PopplerAnnot *annot, PopplerPage *page) {
	cairo_t *cairo;
	cairo_surface_t *surface;

	if (oac_annot->action != ANNOTATION_ACTION_STAMPING)
		return;

	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1000, 1000);

	cairo = cairo_create(surface);
	cairo_save(cairo);

	poppler_page_render_annot(page, annot, cairo);

	cairo_destroy(cairo);

	cairo_surface_flush(surface);

	auto status = cairo_surface_write_to_png(surface, "/tmp/png/test.png");
	printf("%s\n", cairo_status_to_string(status));

	cairo_surface_finish(surface);
	cairo_surface_destroy(surface);

	return;
}

void
oac_annot_set_target_text(OacAnnot *oac_annot, PopplerAnnot *annot, PopplerPage *page) {
	string highlighted_text;
	char *annot_text;

	if (oac_annot->action != ANNOTATION_ACTION_HIGHLIGHTING)
		return;

	annot_text = poppler_page_get_text_for_annot(page, annot);

	highlighted_text += annot_text;

	oac_annot->target_text = highlighted_text;
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
	oac_annot_set_body_image(oac_annot, m->annot, p);
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
