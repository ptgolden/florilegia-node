#include <stdio.h>
#include <string>
#include <algorithm>
#include <list>

#include <nan.h>
#include "streaming-worker.h"

#define JSON_NOEXCEPTION 1
#include "json.hpp"
#include "base64.h"
#define MULTITHREADED 1

#include <GlobalParams.h>
#include <poppler.h>
#include <glib.h>

struct annotation_t {
	int page;
	// int object_id;
	const char* motivation;
	const char* stamp_label;
	std::string body_text;
	std::string highlighted_text;
	std::string body_png;
};

#ifdef NOPE

static double resolution = 72.0;

std::string getStampPNG(Annot *annot) {
	PDFRectangle *origCropBox;
	PDFRectangle *origMediaBox;
	PDFRectangle *annotRect;
	PDFRectangle *tmpAnnotRect;
	double x1, x2, y1, y2;

	doc = annot->getDoc();
	pageNum = annot->getPageNum();
	page = doc->getPage(annot->getPageNum());

	origCropBox = page->getCropBox();
	origMediaBox = page->getMediaBox();


	annot->getRect(&x1, &y1, &x2, &y2);
	double annotWidth = x2 - x1;
	double annotHeight = y2 - y1;
	double edge = page->getCropWidth();

	annot->setRect(edge, y1, edge + annotWidth, y2);
	tmpAnnotRect = annot->getRect();

	doc->replacePageDict(pageNum, 0, tmpAnnotRect, tmpAnnotRect);
	doc->displayPage

	gfx = page->createGfx(splashOut, resolution, resolution, 0,
			gTrue, gFalse,
			-1, -1, -1, -1,
			gFalse, NULL, NULL);

	// gfx->drawAnnot

	annot->draw(gfx, gFalse);

	return out;
}


#endif

std::string annotationToJSON(annotation_t *annot) {
	using json = nlohmann::json;

	json out;

	out["page"] = annot->page;
	// out["object_id"] = annot->object_id;
	out["motivation"] = annot->motivation;

	if (annot->body_text.length() > 0) {
		out["body_text"] = annot->body_text;
	}

	if (annot->highlighted_text.length() > 0) {
		out["highlighted_text"] = annot->highlighted_text;
	}

	if (annot->stamp_label) {
		out["stamp_label"] = annot->stamp_label;
	}

	if (annot->body_png.length() > 0) {
		out["stamp_body"] = annot->body_png;
	}

	return out.dump();
}

std::list<annotation_t> process_page(PopplerDocument *doc, int page_number) {
	std::list<annotation_t> processed_annots;

	PopplerPage *p = poppler_document_get_page(doc, page_number);
	GList *l, *annots;

	annots = poppler_page_get_annot_mapping(p);

	for (l = annots; l; l = g_list_next(l)) {
		PopplerAnnotMapping *m;
		m = (PopplerAnnotMapping *) l->data;

		switch(poppler_annot_get_annot_type(m->annot)) {
		case PopplerAnnotType::POPPLER_ANNOT_TEXT: {
			gchar *content;

			content = poppler_annot_get_contents(m->annot);

			annotation_t a = {
					page_number,
					"commenting",
					NULL,
					content,
					"",
					""
			};

			processed_annots.push_back(a);

			g_free(content);
			break;
		}

		case PopplerAnnotType::POPPLER_ANNOT_HIGHLIGHT:
		case PopplerAnnotType::POPPLER_ANNOT_UNDERLINE: {
			PopplerRectangle cropBox;
			PopplerQuadrilateral quad;
			std::string s;
			auto markup = POPPLER_ANNOT_TEXT_MARKUP(m->annot);
			auto quads = poppler_annot_text_markup_get_quadrilaterals(markup);

			poppler_page_get_crop_box(p, &cropBox);

			for (gint i = 0; i < quads->len; i++) {
				PopplerRectangle rect;
				quad = g_array_index(quads, PopplerQuadrilateral, i);

				double xMin, yMin, xMax, yMax;

				xMin = quad.p1.x;
				yMin = quad.p1.y;
				xMax = quad.p2.x;
				yMax = quad.p3.y;

				rect.x1 = xMin;
				rect.y1 = (cropBox.y1 + cropBox.y2) - yMin;
				rect.x2 = xMax;
				rect.y2 = (cropBox.y1 + cropBox.y2) - yMax;

				// auto b = poppler_page_get_selected_text(p, POPPLER_SELECTION_WORD, &rect);
				auto b = poppler_page_get_text_for_area(p, &rect);

				if (s.length()) {
					s += "\n";
				}

				s += b;
			}

			annotation_t a = {
					page_number,
					"highlighting",
					NULL,
					"",
					s,
					""
			};

			processed_annots.push_back(a);
			g_array_free(quads, TRUE);
			break;
		}
		default:
			break;
		}
	}

	poppler_page_free_annot_mapping(annots);

	return processed_annots;

	/*
	Page *page = doc->getPage(page_number);
	Annots *annots = page->getAnnots();
	std::list<annotation_t> processed_annots;
	Annot *annot;

	for (int j = 0; j < annots->getNumAnnots(); ++j) {
		annot = annots->getAnnot(j);

		switch (annot->getType()) {

		case Annot::typeStamp: {
			AnnotStamp *stamp = static_cast<AnnotStamp *>(annot);

			std::string pngRaw = getStampPNG(annot);
			std::string pngEncoded;
			Base64::Encode(pngRaw, &pngEncoded);

			printf("\n\n%s\n\n", pngEncoded.c_str());

			annotation_t a = {
				page_number, annot->getId(), "bookmarking",
				stamp->getSubject()->getCString(), "", "", pngEncoded
			};

			processed_annots.push_back(a);
			break;
		}

		case Annot::typeUnderline:
		case Annot::typeHighlight: {
			std::string c = getTextForMarkupAnnot(u_map, annot);

			annotation_t a = {
					page_number, annot->getId(), "highlighting",
					NULL, "", getTextForMarkupAnnot(u_map, annot), ""
			};
			processed_annots.push_back(a);
			break;
		}

		default: {
			break;
		}
		}
	}

	return processed_annots;
	*/
}

class AnnotationWorker : public StreamingWorker {
	public:
	AnnotationWorker(Callback *data, Callback *complete, Callback *error_callback, v8::Local<v8::Object> &options)
		: StreamingWorker(data, complete, error_callback) {

		// TODO: handle cases where options weren't passed
		v8::Local<v8::Value> pdfFilename_ = options->Get(Nan::New<v8::String>("pdfFilename").ToLocalChecked());

		v8::String::Utf8Value s(pdfFilename_);
		pdfFilename = *s;

		if (!globalParams) {
			globalParams = new GlobalParams();
		}
	}

	~AnnotationWorker() {
	}

	void Execute (const AsyncProgressWorker::ExecutionProgress &progress) {
		PopplerDocument *doc;
		std::string fileURI = "file://" + pdfFilename;
		GError *err = NULL;
		int n;

		doc = poppler_document_new_from_file(fileURI.c_str(), NULL, &err);

		if (!doc) {
			// FIXME: err->msg;
			Nan::ThrowError("Error opening file");
		}

		n = poppler_document_get_n_pages(doc);

		for (int i = 0; i < n; i++) {
			if (this->closed()) break;
			std::list<annotation_t> page_annots = process_page(doc, i);

			for (annotation_t annot : page_annots) {
				if (this->closed()) break;
				std::string annotJSON = annotationToJSON(&annot);
				Message tosend("annotation", annotJSON);
				writeToNode(progress, tosend);
			}
		}

		close();
	}

	private:
	std::string pdfFilename;
};

StreamingWorker * create_worker(Callback *data
    , Callback *complete
    , Callback *error_callback,
    v8::Local<v8::Object> & options) {

	return new AnnotationWorker(data, complete, error_callback, options);
}

NODE_MODULE(annotation_worker, StreamWorkerWrapper::Init)
