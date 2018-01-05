#include <iostream>
#include <list>
#include <string>

#include <napi.h>

#include <glib.h>
#include <glib/poppler.h>

#include "oac.h"

using namespace Napi;
using std::string;

class AnnotationWorker : public AsyncWorker {
	public:
	AnnotationWorker(Function &callback, string filename_) : AsyncWorker(callback) {
		filename = filename_;
	}

	~AnnotationWorker() {
	}

	void Execute() {
		GError *err = NULL;
		PopplerDocument *doc = poppler_document_new_from_file(filename.c_str(), NULL, &err);

		if (!doc) {
			// TypeError::New(env, "Error opening PDF document").ThrowAsJavaScriptException();
			this->SetError("Error opening PDF document");
			return;
		}

		int pages = poppler_document_get_n_pages(doc);
		for (int i = 0; i < pages; i++) {
			annots.splice(annots.end(), poppler_document_get_oac_annots(doc, i));
		}
	}

	void OnOK() {
		int i = 0;
		Array arr = Array::New(Env(), annots.size());

		for (OacAnnot annot : annots) {
			arr.Set(i, AnnotToObj(&annot));
			i++;
		}

		Callback().MakeCallback(
			Object::New(Env()),
			{ Env().Null(), arr }
		);
	}

	void OnError(const Error &e) {
		std::string errorString;

		errorString = e.Message();

		Callback().MakeCallback(
			Object::New(Env()),
			{ String::New(Env(), errorString), Env().Null() }
		);
	}

	Object AnnotToObj(OacAnnot *annot) {
		Object obj = Object::New(Env());

		obj.Set("page", Number::New(Env(), annot->page));
		obj.Set("motivation", String::New(Env(), annotation_action_get_motivation(annot->action)));
		obj.Set("target_rect", String::New(Env(), annot->target_rect));
		if (annot->body_text.length()) {
			obj.Set("body_text", String::New(Env(), annot->body_text));
		}
		if (annot->body_image.length()) {
			obj.Set("stamp_body", String::New(Env(), annot->body_image));
		}
		if (annot->body_subject.length()) {
			obj.Set("body_subject", String::New(Env(), annot->body_subject));
		}
		if (annot->body_creator.length()) {
			obj.Set("body_creator", String::New(Env(), annot->body_creator));
		}
		if (annot->body_modified.length()) {
			obj.Set("body_modified", String::New(Env(), annot->body_modified));
		}
		if (annot->body_color.length()) {
			obj.Set("body_color", String::New(Env(), annot->body_color));
		}
		if (annot->target_text.length()) {
			obj.Set("target_text", String::New(Env(), annot->target_text));
		}

		return obj;
	}


	std::string filename;
	std::list<OacAnnot> annots;

};

Value ExtractAnnotations(const CallbackInfo &info) {
	Env env = info.Env();

	if (info.Length() != 2) {
		TypeError::New(env, "Incorrect number of arguments").ThrowAsJavaScriptException();
		return env.Null();
	}

	if (!info[0].IsString()) {
		TypeError::New(env, "Filename argument must be a string").ThrowAsJavaScriptException();
		return env.Null();
	}

	if (!info[1].IsFunction()) {
		TypeError::New(env, "Second argument must be a function callback").ThrowAsJavaScriptException();
		return env.Null();
	}

	string filename = "file://" + (string)info[0].As<String>();
	Function cb = info[1].As<Function>();
	AnnotationWorker *worker = new AnnotationWorker(cb, filename);
	worker->Queue();

	return env.Null();
}

Object Init(Env env, Object exports) {
	exports.Set(String::New(env, "extract"), Function::New(env, ExtractAnnotations));
	return exports;
}

NODE_API_MODULE(annotation_worker, Init)
