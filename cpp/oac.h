#include <string>
#include <list>
#include <glib/poppler.h>

using std::string;
using std::list;

enum AnnotationAction {
	ANNOTATION_ACTION_UNSUPPORTED,

	ANNOTATION_ACTION_COMMENTING,
	ANNOTATION_ACTION_HIGHLIGHTING,
	ANNOTATION_ACTION_STAMPING
};

struct OacAnnot {
	AnnotationAction action;

	int page;
	string target_rect;
	string target_text;

	string body_text;
	string body_image;
	string body_creator;
	string body_modified;
	string body_subject;
	string body_color;

};

string oac_annot_to_json(OacAnnot *oac_annot);
string annotation_action_get_motivation(AnnotationAction action);

list<OacAnnot> poppler_document_get_oac_annots(PopplerDocument *doc, int page_number);
