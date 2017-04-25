#include <stdio.h>
#include <string>
#include <list>
#include <glib.h>
#include <glib/poppler.h>

// In node_modules
#include "nan.h"
#include "streaming-worker.h"

// In lib
#include "base64.h"

#include "oac.h"

class AnnotationWorker : public StreamingWorker {
	public:
	AnnotationWorker(Callback *data, Callback *complete, Callback *error_callback, v8::Local<v8::Object> &options)
		: StreamingWorker(data, complete, error_callback) {

		// TODO: handle cases where options weren't passed
		v8::Local<v8::Value> pdfFilename_ = options->Get(Nan::New<v8::String>("pdfFilename").ToLocalChecked());

		v8::String::Utf8Value s(pdfFilename_);
		pdfFilename = *s;
	}

	~AnnotationWorker() {
	}

	void Execute (const AsyncProgressWorker::ExecutionProgress &progress) {
		PopplerDocument *doc;
		std::string fileURI = "file://" + pdfFilename;
		GError *err = NULL;
		int numPages;

		doc = poppler_document_new_from_file(fileURI.c_str(), NULL, &err);

		if (!doc) {
			// FIXME: err->msg;
			Nan::ThrowError("Error opening file");
		}

		numPages = poppler_document_get_n_pages(doc);

		for (int i = 0; i < numPages; i++) {
			std::list<OacAnnot> page_annots = poppler_document_get_oac_annots(doc, i);

			if (this->closed())
				break;

			for (OacAnnot annot : page_annots) {
				Message tosend("annotation", oac_annot_to_json(&annot));
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
