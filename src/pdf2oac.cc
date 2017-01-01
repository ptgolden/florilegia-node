#include <stdio.h>
#include <string>
#include <algorithm>
#include <list>

#include <nan.h>

#include <goo/GooString.h>
#include <Annot.h>
#include <CharTypes.h>
#include <GlobalParams.h>
#include <utils/ImageOutputDev.h>
#include <Page.h>
#include <PDFDoc.h>
#include <PDFDocEncoding.h>
#include <PDFDocFactory.h>
#include <TextOutputDev.h>
#include <UnicodeMap.h>
#include <UTF.h>
#include <XRef.h>

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

	int unicodeLength = TextStringToUCS4(rawString, &u);
	for (int i = 0; i < unicodeLength; i++) {
		int n = u_map->mapUnicode(u[i], buf, sizeof(buf));
		for (int j = 0; j < n; j++) {
			markup_text += buf[j];
		}
	}
	gfree(u);

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
	annotObj.free();
	obj1.free();

	return rects;
}

void writeStampToFile(Annot *annot, const char* objectID) {
	Gfx *gfx;
	ImageOutputDev *imageOut;
	PDFDoc *doc;
	Page *page;
	int pageNum;

	std::string imgRoot = "TEST/";
	imgRoot.append(objectID);

	char *pf = new char[imgRoot.length() + 1];
	std::strcpy(pf, imgRoot.c_str());

	imageOut = new ImageOutputDev(pf, NULL, gFalse);
	imageOut->enablePNG(gTrue);

	doc = annot->getDoc();
	pageNum = annot->getPageNum();
	page = doc->getPage(pageNum);

	gfx = page->createGfx(imageOut, resolution, resolution, 0,
			gTrue, gFalse,
			-1, -1, -1, -1,
			gFalse, NULL, NULL);

	annot->draw(gfx, gFalse);

	delete pf;
	delete imageOut;
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
		text += gooStringToStdString(u_map, str);
	}


	textPage->decRefCnt();
	delete textOut;
	return text;
}


std::list<annotation_t> process_page(UnicodeMap *u_map, PDFDoc* doc, int page_number) {
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

namespace binding {
	using namespace Nan;
	using namespace v8;

	v8::Local<v8::Object> annotation_as_object(annotation_t *annot) {
		Nan::HandleScope scope;
		v8::Local<v8::Object> obj = Nan::New<v8::Object>();

		obj->Set(
			Nan::New("page").ToLocalChecked(),
			Nan::New(annot->page));

		obj->Set(
			Nan::New("object_id").ToLocalChecked(),
			Nan::New(annot->object_id));

		obj->Set(
			Nan::New("motivation").ToLocalChecked(),
			Nan::New(annot->motivation).ToLocalChecked());

		if (annot->body_text.length() > 0) {
			obj->Set(
				Nan::New("body_text").ToLocalChecked(),
				Nan::New(annot->body_text.c_str()).ToLocalChecked());
		}

		if (annot->highlighted_text.length() > 0) {
			obj->Set(
				Nan::New("highlighted_text").ToLocalChecked(),
				Nan::New(annot->highlighted_text.c_str()).ToLocalChecked());
		}

		if (annot->stamp_label) {
			obj->Set(
				Nan::New("stamp_label").ToLocalChecked(),
				Nan::New(annot->stamp_label).ToLocalChecked());
		}

		return obj;
	}


	NAN_METHOD(GetAnnotations) {
		PDFDoc *doc;
		UnicodeMap *uMap;
		GooString *filename;
		std::list<annotation_t> annots;

		if (info.Length() != 1) {
			Nan::ThrowTypeError("getAnnotations takes exactly one argument");
			return;
		}

		if (!info[0]->IsString()) {
			Nan::ThrowTypeError("Argument should be a path to a PDF file");
			return;
		}

		globalParams = new GlobalParams();
		uMap = globalParams->getTextEncoding();

		std::string fnameString(*v8::String::Utf8Value(info[0]));

		filename = new GooString(fnameString.c_str());
		doc = PDFDocFactory().createPDFDoc(*filename, NULL, NULL);

		if (!doc->isOk()) {
			Nan::ThrowError("Could not open PDF at given filename.");
		} else {
			for (int i = 1; i <= doc->getNumPages(); i++) {
				std::list<annotation_t> page_annots = process_page(uMap, doc, i);

				annots.splice(annots.end(), page_annots);
			}

			int i = 0;
			v8::Local<v8::Array> annots_list = New<v8::Array>(annots.size());

			for (annotation_t annot : annots) {
				v8::Local<v8::Object> annot_obj = annotation_as_object(&annot);
				annots_list->Set(i, annot_obj);
				i += 1;
			}

			info.GetReturnValue().Set(annots_list);
		}

		uMap->decRefCnt();
		delete globalParams;
		delete filename;
		delete doc;
	}

	void init(v8::Local<v8::Object> exports) {
		exports->Set(
			Nan::New("getAnnotations").ToLocalChecked(),
			Nan::New<v8::FunctionTemplate>(GetAnnotations)->GetFunction());
	}

	NODE_MODULE(addon, init)
}
