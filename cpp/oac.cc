#include <stdio.h>
#include <math.h>
#include <string>
#include <list>

#include <fstream>
#include <iostream>

#include <glib.h>
#include <glib/poppler.h>
#include <goo/gmem.h>
#include <goo/ImgWriter.h>
#include <goo/PNGWriter.h>
#include <cairo.h>

#include "base64.h"
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

uint32_t
apply_alpha(uint32_t level, uint32_t alpha) {
	return (level * 255 + alpha / 2) / alpha;
}

string
raw_png_from_cairo_surface(cairo_surface_t *surface) {
	string raw_png;
	char *raw_png_c;
	int width, height, stride;
	unsigned char *data;
	FILE *fp;
	long fsize;
	ImgWriter *writer;

	width = cairo_image_surface_get_width(surface);
	height = cairo_image_surface_get_height(surface);
	stride = cairo_image_surface_get_stride(surface);
	cairo_surface_flush(surface);
	data = cairo_image_surface_get_data(surface);


	writer = new PNGWriter(PNGWriter::RGBA);
	fp = tmpfile();
	if (!writer->init(fp, width, height, 72.0, 72.0)) {
		fprintf(stderr, "Error writing file");
		exit(-1);
	}

	unsigned char *row = (unsigned char *)gmallocn(width, 4);
	for (int y = 0; y < height; y++) {
		uint32_t *pixel = (uint32_t *)(data + y * stride);
		unsigned char *rowp = row;
		for (int x = 0; x < width; x++, pixel++) {
			uint8_t r, g, b, a;

			a = ((*pixel & 0xff000000) >> 24);

			if (a == 0) {
				r = g = b = 0;
			} else {
				r = apply_alpha(((*pixel & 0xff0000) >> 16), a);
				g = apply_alpha(((*pixel & 0x00ff00) >> 8), a);
				b = apply_alpha(((*pixel & 0x0000ff) >> 0), a);
			}

			*rowp++ = r;
			*rowp++ = g;
			*rowp++ = b;
			*rowp++ = a;
		}

		writer->writeRow(&row);
	}
	gfree(row);
	writer->close();
	fflush(fp);
	fseek(fp, 0, SEEK_END);
	fsize = ftell(fp);
	std::rewind(fp);
	raw_png_c = (char *)malloc(fsize + 1);

	if (fread(raw_png_c, fsize, 1, fp) != 1) {
		fprintf(stderr, "Error reading stamp PNG to buffer\n");
		exit(-1);
	}
	fclose(fp);

	raw_png.assign(raw_png_c, fsize);
	free(raw_png_c);

	return raw_png;
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
oac_annot_set_body_image(OacAnnot *oac_annot, PopplerAnnot *annot, PopplerRectangle *rect, PopplerPage *page) {
	string png_base64;
	string png_raw;
	double annot_width, annot_height, page_width, page_height;
	cairo_t *cairo;
	cairo_surface_t *surface;

	if (oac_annot->action != ANNOTATION_ACTION_STAMPING)
		return;

	poppler_page_get_size(page, &page_width, &page_height);

	annot_width = rect->x2 - rect->x1;
	annot_height = rect->y2 - rect->y1;
	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, annot_width, annot_height);

	cairo = cairo_create(surface);

	cairo_translate(cairo, -(rect->x1 / 2), (rect->y2 - page_height) / 2);
	poppler_page_render_annot(page, annot, cairo);

	cairo_save(cairo);

	cairo_destroy(cairo);
	cairo_surface_flush(surface);

	png_raw = raw_png_from_cairo_surface(surface);
	Base64::Encode(png_raw, &png_base64);
	oac_annot->body_image = png_base64;

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
	oac_annot_set_body_image(oac_annot, m->annot, &(m->area), p);
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
