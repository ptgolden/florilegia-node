#include <stdio.h>
#include <string>
#include <algorithm>
#include <list>

#include <nan.h>

#include "streaming-worker.h"

#define JSON_NOEXCEPTION 1
#include "json.hpp"

#define MULTITHREADED 1

#include <goo/GooString.h>
#include <Annot.h>
#include <CharTypes.h>
#include <GlobalParams.h>
#include <SplashOutputDev.h>
#include <splash/SplashBitmap.h>
#include <TextOutputDev.h>
#include <Page.h>
#include <PDFDoc.h>
#include <PDFDocEncoding.h>
#include <PDFDocFactory.h>
#include <UnicodeMap.h>
#include <UTF.h>
#include <XRef.h>

using json = nlohmann::json;

struct annotation_t {
	int page;
	int object_id;
	const char* motivation;
	const char* stamp_label;
	std::string body_text;
	std::string highlighted_text;
};

struct rect_t {
	double xMin;
	double yMin;
	double xMax;
	double yMax;
};

std::string gooStringToStdString(UnicodeMap *u_map, GooString *rawString) {
	Unicode *u;
	char buf[8];
	std::string markup_text = "";

	if (u_map) {
		int unicodeLength = TextStringToUCS4(rawString, &u);

		for (int i = 0; i < unicodeLength; i++) {
			int n = u_map->mapUnicode(u[i], buf, sizeof(buf));
			for (int j = 0; j < n; j++) {
				markup_text += buf[j];
			}
		}
		gfree(u);
	} else {
		auto str = rawString->getCString();
		markup_text += str;
	}

	return markup_text;
}

static double resolution = 72.0;

std::list<rect_t> getRectanglesForAnnot(Annot *annot) {
	std::list<rect_t> rects;
	const Ref ref = annot->getRef();
	Object annotObj;
	Object obj1;
	PDFRectangle *rect = new PDFRectangle();

	annot->getDoc()->getXRef()->fetch(ref.num, ref.gen, &annotObj);
	annotObj.dictLookup("QuadPoints", &obj1);
	AnnotQuadrilaterals *quads = new AnnotQuadrilaterals(obj1.getArray(), rect);

	for (int i = 0; i < quads->getQuadrilateralsLength(); i++) {
		rects.push_back({ quads->getX1(i), quads->getY1(i), quads->getX2(i), quads->getY3(i) });
	}

	delete quads;
	delete rect;
	annotObj.free();
	obj1.free();

	return rects;
}

void printRectange(PDFRectangle *rect) {
	printf("(%f,%f) , (%f,%f)\n", rect->x1, rect->y1, rect->x2, rect->y2);
}

void writeStampToFile(Annot *annot, const char* objectID) {
	Gfx *gfx;
	SplashColor color;
	SplashOutputDev *splashOut;
	PDFDoc *doc;
	Page *page;
	int pageNum;

	doc = annot->getDoc();
	pageNum = annot->getPageNum();
	page = doc->getPage(pageNum);

	color[0] = color[1] = color[2] = 255;
	splashOut = new SplashOutputDev(splashModeRGB8, 4, gFalse, color);
	splashOut->startDoc(doc);

	PDFRectangle *annotCrop = annot->getRect();
	PDFRectangle *origCropBox = page->getCropBox();
	PDFRectangle *mediaBox = page->getMediaBox();

	printf("Annotation %s: ", objectID);
	printRectange(annotCrop);

	printf("Crop box: ");
	printRectange(origCropBox);

	printf("Media box: ");
	printRectange(mediaBox);

	double m = 3;

	page->displaySlice(splashOut, 72 * m, 72 * m, 0,
			gTrue, gTrue,
			annotCrop->x1 * m,
			(mediaBox->y2 - annotCrop->y2)* m,
			(annotCrop->x2 - annotCrop->x1)* m,
			(annotCrop->y2 - annotCrop->y1) * m,
			gFalse);

	SplashBitmap *bitmap = splashOut->getBitmap();

	std::string f;
	f += "/tmp/png/thing-";
	f += objectID;
	f += ".png";

	printf("Printed to %s\n\n", f.c_str());


	bitmap->writeImgFile(splashFormatPng, (char *)f.c_str(), 72, 72);

	doc->replacePageDict(pageNum, 0, mediaBox, origCropBox);

	delete splashOut;
}


std::string getTextForMarkupAnnot(UnicodeMap *u_map, Annot *annot) {
	TextOutputDev *textOut;
	TextPage *textPage;
	PDFDoc *doc;
	PDFRectangle *mediaBox;
	Page *page;
	int pageNum;
	std::string text = "";

	doc = annot->getDoc();
	pageNum = annot->getPageNum();
	page = doc->getPage(pageNum);

	mediaBox = page->getMediaBox();

	// Set up the text to be rendered
	textOut = new TextOutputDev(NULL, gFalse, 0, gFalse, gFalse);

	doc->displayPage(textOut, pageNum, resolution, resolution, 0, gTrue, gFalse, gFalse);
	textPage = textOut->takeText();

	auto rects = getRectanglesForAnnot(annot);;
	for (auto rect : rects) {
		GooString *str = textPage->getText(rect.xMin, mediaBox->y2 - rect.yMin, rect.xMax, mediaBox->y2 - rect.yMax);
		if (text.length() > 0) {
			text += " ";
		}
		text += gooStringToStdString(NULL, str);
		delete str;
	}


	textPage->decRefCnt();
	delete textOut;
	return text;
}

std::string annotationToJSON(annotation_t *annot) {
	json out;

	out["page"] = annot->page;
	out["object_id"] = annot->object_id;
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

	return out.dump();
}


std::list<annotation_t> process_page(UnicodeMap *u_map, PDFDoc* doc, int page_number, std::string imgDirectory) {
	Page *page = doc->getPage(page_number);
	Annots *annots = page->getAnnots();
	std::list<annotation_t> processed_annots;
	Annot *annot;

	for (int j = 0; j < annots->getNumAnnots(); ++j) {
		annot = annots->getAnnot(j);

		switch (annot->getType()) {
		case Annot::typeText: {
			std::string body_text = gooStringToStdString(u_map, annot->getContents());

			annotation_t a = {
				page_number, annot->getId(), "commenting",
				NULL,
				body_text, ""
			};

			processed_annots.push_back(a);
			break;
		}

		case Annot::typeStamp: {
			AnnotStamp *stamp = static_cast<AnnotStamp *>(annot);


			annotation_t a = {
				page_number, annot->getId(), "bookmarking",
				stamp->getSubject()->getCString(), "", ""
			};

			writeStampToFile(annot, std::to_string(stamp->getId()).c_str());

			processed_annots.push_back(a);
			break;
		}

		case Annot::typeUnderline:
		case Annot::typeHighlight: {
			std::string c = getTextForMarkupAnnot(u_map, annot);

			annotation_t a = {
					page_number, annot->getId(), "highlighting",
					NULL, "", getTextForMarkupAnnot(u_map, annot)
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
}

class AnnotationWorker : public StreamingWorker {
	public:
	AnnotationWorker(Callback *data, Callback *complete, Callback *error_callback, v8::Local<v8::Object> &options)
		: StreamingWorker(data, complete, error_callback) {

		// TODO: handle cases where options weren't passed
		v8::Local<v8::Value> pdfFilename_ = options->Get(Nan::New<v8::String>("pdfFilename").ToLocalChecked());
		v8::Local<v8::Value> imageDirectory_ = options->Get(Nan::New<v8::String>("imageDirectory").ToLocalChecked());

		v8::String::Utf8Value s(pdfFilename_);
		pdfFilename = *s;

		v8::String::Utf8Value t(imageDirectory_);
		imageDirectory = *t;

		if (!globalParams) {
			globalParams = new GlobalParams();
		}
	}

	~AnnotationWorker() {
	}

	void Execute (const AsyncProgressWorker::ExecutionProgress &progress) {
		PDFDoc *doc;
		UnicodeMap *uMap;
		GooString *filename;

		uMap = globalParams->getTextEncoding();

		filename = new GooString(pdfFilename.c_str());
		doc = PDFDocFactory().createPDFDoc(*filename, NULL, NULL);

		if (!doc->isOk()) {
			Nan::ThrowError("Could not open PDF at given filename.");
		}

		for (int i = 1; i <= doc->getNumPages(); i++) {
			if (this->closed()) break;

			std::list<annotation_t> page_annots = process_page(uMap, doc, i, imageDirectory);

			/*
			for (annotation_t annot : page_annots) {
				if (this->closed()) break;
				std::string annotJSON = annotationToJSON(&annot);
				Message tosend("annotation", annotJSON);
				writeToNode(progress, tosend);
			}
			*/
		}

		close();

		delete filename;
		delete doc;
		uMap->decRefCnt();
		// FIXME: I don't know how to deal with globalParams in a multithreaded environment
		// delete globalParams;
	}

	private:
	std::string pdfFilename;
	std::string imageDirectory;

};

StreamingWorker * create_worker(Callback *data
    , Callback *complete
    , Callback *error_callback,
    v8::Local<v8::Object> & options) {

	return new AnnotationWorker(data, complete, error_callback, options);
}

NODE_MODULE(annotation_worker, StreamWorkerWrapper::Init)
