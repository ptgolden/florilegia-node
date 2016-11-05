#include <stdio.h>
#include <string>
#include <unordered_map>
#include <algorithm>

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
#include <UnicodeMap.h>
#include <UTF.h>
#include <XRef.h>

#include <raptor.h>

namespace NS {
	const std::string DC = "dc";
	const std::string OA = "oa";
	const std::string CNT = "cnt";
	const std::string RDF = "rdf";
	const std::string RDFS = "rdfs";
	const std::string BIBO = "bibo";
	const std::string DC_TYPES = "dctypes";
}

class RaptorURI {
public:
	raptor_uri *uri;

	RaptorURI(raptor_world* world, raptor_uri* base_uri, const char* term) {
		uri = raptor_new_uri_from_uri_local_name(
				world, base_uri, (const unsigned char*) term);
	}

	~RaptorURI() {
		raptor_free_uri(uri);
	}
};

class RDFState {
public:
	raptor_uri *base_uri = NULL;
	raptor_world *world = NULL;
	raptor_serializer* serializer = NULL;
	std::unordered_map<std::string, raptor_uri*> nsMap;

	RDFState() {
		world = raptor_new_world();

		raptor_world_set_log_handler(world, NULL, log_handler);
		serializer = raptor_new_serializer(world, "turtle");

		nsMap[NS::OA] = raptor_new_uri(world, (const unsigned char*)"http://www.w3.org/ns/oa#");
		raptor_serializer_set_namespace(serializer, nsMap[NS::OA], (const unsigned char*)"oa");

		nsMap[NS::DC] = raptor_new_uri(world, (const unsigned char*)"http://purl.org/dc/elements/1.1/");
		raptor_serializer_set_namespace(serializer, nsMap[NS::DC], (const unsigned char*)"dc");

		nsMap[NS::DC_TYPES] = raptor_new_uri(world, (const unsigned char*)"http://purl.org/dc/dcmitype/");
		raptor_serializer_set_namespace(serializer, nsMap[NS::DC_TYPES], (const unsigned char*)"dctypes");

		nsMap[NS::CNT] = raptor_new_uri(world, (const unsigned char*)"http://www.w3.org/2011/content#");
		raptor_serializer_set_namespace(serializer, nsMap[NS::CNT], (const unsigned char*)"cnt");

		nsMap[NS::BIBO] = raptor_new_uri(world, (const unsigned char*)"http://purl.org/ontology/bibo/");
		raptor_serializer_set_namespace(serializer, nsMap[NS::BIBO], (const unsigned char*)"bibo");

		nsMap[NS::RDF] = raptor_new_uri(world, raptor_rdf_namespace_uri);

		nsMap[NS::RDFS] = raptor_new_uri(world, (const unsigned char*)"http://www.w3.org/2000/01/rdf-schema#");
		raptor_serializer_set_namespace(serializer, nsMap[NS::RDFS], (const unsigned char*)"rdfs");
	}

	~RDFState() {
		raptor_serializer_serialize_end(serializer);

		raptor_free_uri(nsMap[NS::DC]);
		raptor_free_uri(nsMap[NS::OA]);
		raptor_free_uri(nsMap[NS::CNT]);
		raptor_free_uri(nsMap[NS::RDF]);
		raptor_free_uri(nsMap[NS::RDFS]);
		raptor_free_uri(nsMap[NS::BIBO]);
		raptor_free_uri(nsMap[NS::DC_TYPES]);
		raptor_free_uri(base_uri);

		raptor_free_serializer(serializer);
		raptor_free_world(world);
	}

	void start(raptor_uri* with_base_uri) {
		base_uri = raptor_new_uri_relative_to_base(world, with_base_uri, (const unsigned char*)"./");
		raptor_serializer_start_to_file_handle(serializer, base_uri, stdout);
	}

	void add_triple(raptor_term* subject, raptor_term* predicate, raptor_term* object) {
		raptor_statement *triple;

		triple = raptor_new_statement(world);
		triple->subject = subject;
		triple->predicate = predicate;
		triple->object = object;

		raptor_serializer_serialize_statement(serializer, triple);
		raptor_free_statement(triple);
	}

	raptor_uri* make_relative_uri(const char* uri_str) {
		return raptor_new_uri_relative_to_base(world, base_uri, (const unsigned char*)uri_str);
	}

	RaptorURI make_uri(std::string ns, const char* term) {
		RaptorURI uri_wrapper(world, nsMap[ns], term);
		return uri_wrapper;
	}

private:
	static void log_handler(void *user_data, raptor_log_message* message) {
		fprintf(stderr, "%s\n", message->text);
	}
};

// Global variable (sorry, I don't know how to use C++)
RDFState rdf_state;
int annotation_ct = 1;

class OACAnnotation {
public:
	raptor_term *annotation_term;
	raptor_term *annotation_body_term;
	raptor_term *annotation_target_term;

	raptor_uri* numbered_uri(std::string label) {
		raptor_uri *uri;

		label += std::to_string(annotation_ct);
		uri = rdf_state.make_relative_uri(label.c_str());

		return uri;
	}

	OACAnnotation(raptor_term* pdf_term, int page_number, const char* motivation, bool body = false) {
		raptor_uri *annotation_uri = numbered_uri("#annotation");

		annotation_term = raptor_new_term_from_uri(rdf_state.world, annotation_uri);
			annotation_target_term = raptor_new_term_from_blank(rdf_state.world, NULL);

		if (body) {
			annotation_body_term = raptor_new_term_from_blank(rdf_state.world, NULL);
		} else {
			annotation_body_term = NULL;
		}

		rdf_state.add_triple(
			annotation_term,
			raptor_new_term_from_uri(rdf_state.world, rdf_state.make_uri(NS::RDF, "Type").uri),
			raptor_new_term_from_uri(rdf_state.world, rdf_state.make_uri(NS::OA, "Annotation").uri));

		rdf_state.add_triple(
			raptor_term_copy(annotation_term),
			raptor_new_term_from_uri(rdf_state.world, rdf_state.make_uri(NS::OA, "hasTarget").uri),
			annotation_target_term);

		rdf_state.add_triple(
			raptor_term_copy(annotation_term),
			raptor_new_term_from_uri(rdf_state.world, rdf_state.make_uri(NS::OA, "hasMotivation").uri),
			raptor_new_term_from_uri(rdf_state.world, rdf_state.make_uri(NS::OA, motivation).uri));

		rdf_state.add_triple(
			raptor_term_copy(annotation_target_term),
			raptor_new_term_from_uri(rdf_state.world, rdf_state.make_uri(NS::OA, "hasSource").uri),
			raptor_term_copy(pdf_term));

		raptor_term* fragment_selector_term = this->add_selector("FragmentSelector");
		raptor_uri* pdf_fragment_standard_uri = raptor_new_uri(
			rdf_state.world,
			(const unsigned char*)"http://tools.ietf.org/rfc/rfc3778");

		std::string pdf_fragment = "#page=" + std::to_string(page_number);
		rdf_state.add_triple(
			raptor_term_copy(fragment_selector_term),
			raptor_new_term_from_uri(rdf_state.world, rdf_state.make_uri(NS::OA, "conformsTo").uri),
			raptor_new_term_from_uri(rdf_state.world, pdf_fragment_standard_uri));

		rdf_state.add_triple(
			raptor_term_copy(fragment_selector_term),
			raptor_new_term_from_uri(rdf_state.world, rdf_state.make_uri(NS::RDF, "value").uri),
			raptor_new_term_from_literal(rdf_state.world, (const unsigned char*)pdf_fragment.c_str(), NULL, NULL));

		if (body) {
			rdf_state.add_triple(
				raptor_term_copy(annotation_term),
				raptor_new_term_from_uri(rdf_state.world, rdf_state.make_uri(NS::OA, "hasBody").uri),
				annotation_body_term);
		}

		raptor_free_uri(pdf_fragment_standard_uri);
		raptor_free_uri(annotation_uri);
	}

	raptor_term* add_selector(const char* selector_type) {
		raptor_term* selector_term = raptor_new_term_from_blank(rdf_state.world, NULL);

		rdf_state.add_triple(
			raptor_term_copy(annotation_target_term),
			raptor_new_term_from_uri(rdf_state.world, rdf_state.make_uri(NS::OA, "hasSelector").uri),
			selector_term);

		rdf_state.add_triple(
			raptor_term_copy(selector_term),
			raptor_new_term_from_uri(rdf_state.world, rdf_state.make_uri(NS::RDF, "type").uri),
			raptor_new_term_from_uri(rdf_state.world, rdf_state.make_uri(NS::OA, selector_type).uri));

		return selector_term;
	}
};

void process_page(UnicodeMap *u_map, PDFDoc* doc, raptor_term* pdf_term, int page_number) {
	Page *page;
	Annots *annots;
	Annot *annot;

	page = doc->getPage(page_number);
	annots = page->getAnnots();

	for (int j = 0; j < annots->getNumAnnots(); ++j) {
		annot = annots->getAnnot(j);

		if (annot->getType() == Annot::typeText) {
			OACAnnotation oac_annotation(pdf_term, page_number, "commenting", true);
			rdf_state.add_triple(
				raptor_term_copy(oac_annotation.annotation_body_term),
				raptor_new_term_from_uri(rdf_state.world, rdf_state.make_uri(NS::RDF, "Type").uri),
				raptor_new_term_from_uri(rdf_state.world, rdf_state.make_uri(NS::DC_TYPES, "Text").uri));

			rdf_state.add_triple(
				raptor_term_copy(oac_annotation.annotation_body_term),
				raptor_new_term_from_uri(rdf_state.world, rdf_state.make_uri(NS::RDF, "Type").uri),
				raptor_new_term_from_uri(rdf_state.world, rdf_state.make_uri(NS::CNT, "ContentAsText").uri));

			rdf_state.add_triple(
				raptor_term_copy(oac_annotation.annotation_body_term),
				raptor_new_term_from_uri(rdf_state.world, rdf_state.make_uri(NS::DC, "format").uri),
				raptor_new_term_from_literal(rdf_state.world, (const unsigned char*)"text/plain", NULL, NULL));

			rdf_state.add_triple(
				raptor_term_copy(oac_annotation.annotation_body_term),
				raptor_new_term_from_uri(rdf_state.world, rdf_state.make_uri(NS::CNT, "chars").uri),
				raptor_new_term_from_literal(
						rdf_state.world,
						(const unsigned char*)annot->getContents()->getCString(),
						NULL,
						(const unsigned char*)"EN"));

			annotation_ct += 1;
		}


		if (annot->getType() == Annot::typeStamp) {
			AnnotStamp *stamp = static_cast<AnnotStamp *>(annot);
			GooString *subject = stamp->getSubject();

			OACAnnotation oac_annotation(pdf_term, page_number, "bookmarking", true);
			rdf_state.add_triple(
				raptor_term_copy(oac_annotation.annotation_body_term),
				raptor_new_term_from_uri(rdf_state.world, rdf_state.make_uri(NS::RDF, "Type").uri),
				raptor_new_term_from_uri(rdf_state.world, rdf_state.make_uri(NS::DC_TYPES, "Image").uri));

			rdf_state.add_triple(
				raptor_term_copy(oac_annotation.annotation_body_term),
				raptor_new_term_from_uri(rdf_state.world, rdf_state.make_uri(NS::RDFS, "label").uri),
				raptor_new_term_from_literal(
						rdf_state.world,
						(const unsigned char*)subject->getCString(),
						NULL,
						(const unsigned char*)"EN"));

			annotation_ct += 1;
		}


		if (annot->getType() == Annot::typeUnderline || annot->getType() == Annot::typeHighlight) {
			/* AnnotTextMarkup *underline = static_cast<AnnotTextMarkup *>(annot); */

			const Ref ref = annot->getRef();
			Object annotObj;
			Object obj1;

			doc->getXRef()->fetch(ref.num, ref.gen, &annotObj);
			annotObj.dictLookup("MarkupText", &obj1);

			Unicode *u;
			char buf[8];
			std::string markup_text = "";
			GooString *rawString = obj1.getString();

			int unicodeLength = TextStringToUCS4(rawString, &u);
			for (int i = 0; i < unicodeLength; i++) {
				int n = u_map->mapUnicode(u[i], buf, sizeof(buf));
				for (int j = 0; j < n; j++) {
					markup_text += buf[j];
				}
			}
			gfree(u);

			std::replace(markup_text.begin(), markup_text.end(), '\n', ' ');

			OACAnnotation oac_annotation(pdf_term, page_number, "highlighting");

			raptor_term* quote_selector_term = oac_annotation.add_selector("TextQuoteSelector");
			rdf_state.add_triple(
				raptor_term_copy(quote_selector_term),
				raptor_new_term_from_uri(rdf_state.world, rdf_state.make_uri(NS::OA, "exact").uri),
				raptor_new_term_from_literal(
					rdf_state.world,
					(const unsigned char*)markup_text.c_str(),
					NULL,
					(const unsigned char*)"en"));

			/*
			PDFRectangle *rect = annot->getRect();
			pdf_fragment += "&viewrect=";
			pdf_fragment += std::to_string(rect->x1) + ",";
			pdf_fragment += std::to_string(rect->y1) + ",";
			pdf_fragment += std::to_string(rect->x2) + ",";
			pdf_fragment += std::to_string(rect->y2);
			*/

			annotation_ct += 1;
			annotObj.free();
			obj1.free();
		}
	}
}

int main(int argc, char **argv) {
	PDFDoc *doc;
	GooString *filename;
	unsigned char *pdf_uri_string;
	raptor_uri *pdf_uri;
	raptor_term *pdf_term;
	UnicodeMap *uMap;

	globalParams = new GlobalParams();
	uMap = globalParams->getTextEncoding();

	pdf_uri_string = raptor_uri_filename_to_uri_string(filename->getCString());
	pdf_uri = raptor_new_uri(rdf_state.world, pdf_uri_string);
	rdf_state.start(pdf_uri);

	pdf_term = raptor_new_term_from_uri(rdf_state.world, pdf_uri);

	rdf_state.add_triple(
		pdf_term,
		raptor_new_term_from_uri(rdf_state.world, rdf_state.make_uri(NS::RDF, "Type").uri),
		raptor_new_term_from_uri(rdf_state.world, rdf_state.make_uri(NS::BIBO, "Document").uri));

	for (int i = 1; i < doc->getNumPages(); ++i) {
		process_page(uMap, doc, pdf_term, i);
	}

	// Clean up
	uMap->decRefCnt();
	delete globalParams;
	delete filename;
	delete doc;

	raptor_free_memory(pdf_uri_string);
	raptor_free_uri(pdf_uri);
}

namespace binding {
	using v8::FunctionCallbackInfo;
	using v8::Isolate;
	using v8::Local;
	using v8::Object;
	using v8::String;
	using v8::Value;
	using v8::Exception;

	void GetAnnotations(const FunctionCallbackInfo<Value>& args) {
		Isolate* isolate = args.GetIsolate();

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

		PDFDoc *doc;
		GooString *filename;
		// unsigned char *pdf_uri_string;

		String::Utf8Value param(args[0]->ToString());

		filename = new GooString(std::string(*param).c_str());
		doc = PDFDocFactory().createPDFDoc(*filename, NULL, NULL);

		if (!doc->isOk()) {
			isolate->ThrowException(Exception::TypeError(
				String::NewFromUtf8(isolate, "Could not open PDF at filename.")));
			delete filename;
			delete doc;
			return;
		}

		// Process the document here!

		args.GetReturnValue().Set(String::NewFromUtf8(isolate, "world"));
	}

	void init(Local<Object> exports) {
			  NODE_SET_METHOD(exports, "getAnnotations", GetAnnotations);
	}

	NODE_MODULE(addon, init)
}
