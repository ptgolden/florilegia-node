#include <string>
#include <list>
#include <poppler.h>

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

string oac_annot_to_json(OacAnnot *oac_annot);

list<OacAnnot> poppler_document_get_oac_annots(PopplerDocument *doc, int page_number);
