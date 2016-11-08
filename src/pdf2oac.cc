#include <stdio.h>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <list>

#include <v8.h>
#include <node.h>

#include <goo/GooString.h>
#include <Annot.h>
#include <CharTypes.h>
#include <GlobalParams.h>
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
	const char* motivation;
	const char* body_type;
	const char* body_label;
	std::string body_text;
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

std::vector<rect_t> getRectanglesForAnnot(Annot *annot) {
	std::vector<rect_t> rects;
	const Ref ref = annot->getRef();
	Object annotObj;
	Object obj1;
	PDFRectangle *rect = new PDFRectangle();

	annot->getDoc()->getXRef()->fetch(ref.num, ref.gen, &annotObj);
	annotObj.dictLookup("QuadPoints", &obj1);
	AnnotQuadrilaterals *quads = new AnnotQuadrilaterals(obj1.getArray(), rect);

	for (int i = 0; i < quads->getQuadrilateralsLength(); i++) {
		rects.push_back({ quads->getX1(i), quads->getY3(i), quads->getX2(i), quads->getY1(i) });
	}

	delete quads;
	annotObj.free();
	obj1.free();

	return rects;
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
		GooString *str = textPage->getText(rect.xMin, mediaBox->y2 - rect.yMax, rect.xMax, mediaBox->y2 - rect.yMin);
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
				page_number, "commenting", "text",
				NULL,
				body_text
			};

			processed_annots.push_back(a);
			break;
		}

		case Annot::typeStamp: {
			AnnotStamp *stamp = static_cast<AnnotStamp *>(annot);

			annotation_t a = {
				page_number, "bookmarking", "image",
				stamp->getSubject()->getCString(), NULL
			};

			processed_annots.push_back(a);
			break;
		}

		case Annot::typeUnderline:
		case Annot::typeHighlight: {
			std::string c = getTextForMarkupAnnot(u_map, annot);

			annotation_t a = {
					page_number, "highlighting", "text",
					NULL, getTextForMarkupAnnot(u_map, annot)
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
	using v8::FunctionCallbackInfo;
	using v8::Isolate;
	using v8::Local;
	using v8::Array;
	using v8::Object;
	using v8::Null;
	using v8::Number;
	using v8::String;
	using v8::Value;
	using v8::Exception;

	Local<Object> annotation_as_object(Isolate* isolate, annotation_t annot) {
		Local<Object> obj = Object::New(isolate);

		obj->Set(
			String::NewFromUtf8(isolate, "page"),
			Number::New(isolate, annot.page)
		);

		obj->Set(
			String::NewFromUtf8(isolate, "motivation"),
			String::NewFromUtf8(isolate, annot.motivation)
		);

		obj->Set(
			String::NewFromUtf8(isolate, "body_type"),
			String::NewFromUtf8(isolate, annot.body_type)

		);

		if (annot.body_text.c_str()) {
			obj->Set(
				String::NewFromUtf8(isolate, "body_text"),
				String::NewFromUtf8(isolate, annot.body_text.c_str()));
		} else {
			obj->Set(
				String::NewFromUtf8(isolate, "body_text"),
				Null(isolate));
		}

		if (annot.body_label) {
			obj->Set(
				String::NewFromUtf8(isolate, "body_label"),
				String::NewFromUtf8(isolate, annot.body_label));
		} else {
			obj->Set(
				String::NewFromUtf8(isolate, "body_label"),
				Null(isolate));
		}

		return obj;
	}


	void GetAnnotations(const FunctionCallbackInfo<Value>& args) {
		PDFDoc *doc;
		UnicodeMap *uMap;
		GooString *filename;
		Isolate* isolate = args.GetIsolate();
		std::list<annotation_t> annots;

		globalParams = new GlobalParams();
		uMap = globalParams->getTextEncoding();

		if (args.Length() != 1) {
			isolate->ThrowException(Exception::TypeError(
				String::NewFromUtf8(isolate, "Function takes exactly one argument")));
			return;
		}

		if (!args[0]->IsString()) {
			isolate->ThrowException(Exception::TypeError(
				String::NewFromUtf8(isolate, "Function argument should be string of a filename")));
			return;
		}

		String::Utf8Value param(args[0]->ToString());

		filename = new GooString(std::string(*param).c_str());
		doc = PDFDocFactory().createPDFDoc(*filename, NULL, NULL);

		if (!doc->isOk()) {
			isolate->ThrowException(Exception::TypeError(
				String::NewFromUtf8(isolate, "Could not open PDF at filename.")));
		} else {
			for (int i = 1; i <= doc->getNumPages(); i++) {
				std::list<annotation_t> page_annots = process_page(uMap, doc, i);
				annots.splice(annots.end(), page_annots);
			}

			int i = 0;
			Local<Array> annots_list = Array::New(isolate);

			for (annotation_t annot : annots) {
				Local<Object> annot_obj = annotation_as_object(isolate, annot);
				annots_list->Set(i, annot_obj);
				i += 1;
			}
			args.GetReturnValue().Set(annots_list);
		}

		// Clean up
		uMap->decRefCnt();
		delete globalParams;
		delete filename;
		delete doc;
	}

	void init(Local<Object> exports) {
			  NODE_SET_METHOD(exports, "getAnnotations", GetAnnotations);
	}

	NODE_MODULE(addon, init)
}
